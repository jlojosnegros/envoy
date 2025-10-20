#!/usr/bin/env python3
"""
Envoy Code Review Helper

This script provides automated heuristic analysis for Envoy code reviews.
It's designed for fast CI/CD usage and complements the interactive Claude Code agent.

For comprehensive analysis with bazel coverage, use: /envoy-review --rigorous-coverage

Usage:
    ./envoy-review-helper.py [options]

Options:
    --repo PATH         Path to Envoy repository (default: current directory)
    --base BRANCH       Base branch for comparison (default: main)
    --format FORMAT     Output format: json, markdown, text (default: json)
    --verbose           Verbose output

Author: AI Assistant for Envoy Development
"""

import os
import sys
import subprocess
import json
import argparse
import re
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Set
from dataclasses import dataclass, asdict, field


@dataclass
class FileAnalysis:
    """Analysis result for a single file."""
    path: str
    file_type: str  # source, test, build, api, docs, other
    lines_added: int
    lines_removed: int
    has_corresponding_test: bool
    test_path: Optional[str]
    issues: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    suggestions: List[str] = field(default_factory=list)


@dataclass
class ReviewReport:
    """Complete review report."""
    files_changed: int
    source_files: List[str]
    test_files: List[str]
    build_files: List[str]
    api_files: List[str]
    docs_files: List[str]
    lines_added: int
    lines_removed: int
    release_notes_updated: bool
    release_notes_appropriate: bool
    missing_tests: List[Dict[str, str]]
    critical_issues: List[str]
    warnings: List[str]
    suggestions: List[str]
    file_analyses: List[FileAnalysis]


class EnvoyReviewer:
    """Main reviewer class for analyzing Envoy code changes."""

    def __init__(self, repo_path: Path, base_branch: str = "main"):
        self.repo_path = Path(repo_path).resolve()
        self.base_branch = base_branch

        if not self._is_git_repo():
            raise ValueError(f"{repo_path} is not a git repository")

    def _is_git_repo(self) -> bool:
        """Check if path is a git repository."""
        git_dir = self.repo_path / ".git"
        return git_dir.exists()

    def _run_git_command(self, args: List[str]) -> Tuple[str, str, int]:
        """Run a git command and return stdout, stderr, returncode."""
        cmd = ["git"] + args
        try:
            result = subprocess.run(
                cmd,
                cwd=self.repo_path,
                capture_output=True,
                text=True,
                timeout=30
            )
            return result.stdout, result.stderr, result.returncode
        except subprocess.TimeoutExpired:
            return "", "Command timed out", 1
        except Exception as e:
            return "", str(e), 1

    def get_changed_files(self) -> List[str]:
        """Get list of files changed compared to base branch."""
        stdout, stderr, code = self._run_git_command([
            "diff", "--name-only", f"{self.base_branch}...HEAD"
        ])

        if code != 0:
            print(f"Warning: git diff failed: {stderr}", file=sys.stderr)
            return []

        return [f.strip() for f in stdout.split('\n') if f.strip()]

    def get_file_diff_stats(self, filepath: str) -> Tuple[int, int]:
        """Get lines added and removed for a file."""
        stdout, stderr, code = self._run_git_command([
            "diff", "--numstat", f"{self.base_branch}...HEAD", "--", filepath
        ])

        if code != 0 or not stdout.strip():
            return 0, 0

        parts = stdout.strip().split()
        if len(parts) >= 2:
            try:
                added = int(parts[0]) if parts[0] != '-' else 0
                removed = int(parts[1]) if parts[1] != '-' else 0
                return added, removed
            except ValueError:
                return 0, 0

        return 0, 0

    def get_file_diff(self, filepath: str) -> str:
        """Get the diff content for a file."""
        stdout, stderr, code = self._run_git_command([
            "diff", f"{self.base_branch}...HEAD", "--", filepath
        ])
        return stdout if code == 0 else ""

    def categorize_file(self, filepath: str) -> str:
        """Categorize file by type."""
        path = Path(filepath)

        if filepath.startswith('source/'):
            if '_test' in filepath:
                return 'test'
            return 'source'
        elif filepath.startswith('test/'):
            return 'test'
        elif filepath.startswith('api/'):
            return 'api'
        elif filepath.startswith('docs/'):
            return 'docs'
        elif 'BUILD' in filepath or filepath.endswith('.bzl'):
            return 'build'
        elif filepath.endswith(('.md', '.rst')):
            return 'docs'
        else:
            return 'other'

    def find_corresponding_test(self, source_file: str) -> Tuple[bool, Optional[str]]:
        """
        Find the test file corresponding to a source file.

        Convention: source/path/to/foo.cc -> test/path/to/foo_test.cc
        """
        if not source_file.startswith('source/'):
            return False, None

        # Convert source path to test path
        test_file = source_file.replace('source/', 'test/', 1)

        # Handle different extensions
        if test_file.endswith('.cc'):
            test_file = test_file.replace('.cc', '_test.cc')
        elif test_file.endswith('.h'):
            # For header files, check if there's a corresponding _test.cc
            test_file = test_file.replace('.h', '_test.cc')
        else:
            return False, None

        # Check if file exists in CURRENT working tree
        test_path = self.repo_path / test_file
        exists = test_path.exists()

        return exists, test_file if exists else test_file

    def extract_added_functions(self, diff_content: str) -> List[Tuple[str, int]]:
        """
        Extract function names and line numbers from added lines in diff.
        Returns list of (function_name, line_number) tuples.
        """
        functions = []

        # Pattern for C++ function definitions
        # Matches: ReturnType ClassName::functionName(...) {
        function_pattern = re.compile(
            r'^\+\s*(?:[\w:]+\s+)?'  # Return type (optional)
            r'([\w:]+)\s*\('  # Function name and opening paren
        )

        # Pattern for class methods
        method_pattern = re.compile(
            r'^\+\s*(?:virtual\s+|static\s+|inline\s+)*'
            r'(?:[\w:]+\s+)?'  # Return type
            r'([\w]+)::([\w]+)\s*\('  # Class::method
        )

        line_num = 0
        for line in diff_content.split('\n'):
            if line.startswith('@@'):
                # Extract line number from hunk header
                match = re.search(r'\+(\d+)', line)
                if match:
                    line_num = int(match.group(1))
            elif line.startswith('+') and not line.startswith('+++'):
                # Check for method definition
                method_match = method_pattern.search(line)
                if method_match:
                    class_name = method_match.group(1)
                    method_name = method_match.group(2)
                    functions.append((f"{class_name}::{method_name}", line_num))
                else:
                    # Check for function definition
                    func_match = function_pattern.search(line)
                    if func_match and '{' in line:
                        functions.append((func_match.group(1), line_num))
                line_num += 1

        return functions

    def extract_added_enum_cases(self, diff_content: str) -> List[Tuple[str, int]]:
        """
        Extract enum case values from added lines.
        Returns list of (enum_value, line_number) tuples.
        """
        enum_cases = []

        # Pattern for enum case statements: case EnumClass::Value:
        case_pattern = re.compile(r'^\+\s*case\s+[\w:]+::([\w]+):')

        line_num = 0
        for line in diff_content.split('\n'):
            if line.startswith('@@'):
                match = re.search(r'\+(\d+)', line)
                if match:
                    line_num = int(match.group(1))
            elif line.startswith('+'):
                match = case_pattern.search(line)
                if match:
                    enum_cases.append((match.group(1), line_num))
                line_num += 1

        return enum_cases

    def check_test_coverage_heuristic(self, source_file: str, test_file_path: Path) -> Tuple[bool, List[str]]:
        """
        Heuristic check for test coverage.

        This is NOT 100% accurate - for rigorous coverage use:
        /envoy-review --rigorous-coverage

        Returns (likely_has_coverage, list_of_potential_gaps)
        """
        if not test_file_path.exists():
            return False, ["Test file does not exist"]

        potential_gaps = []

        try:
            # Get the diff to see what changed
            diff_content = self.get_file_diff(source_file)
            if not diff_content:
                return True, []  # No changes to verify

            # Read CURRENT test file content
            test_content = test_file_path.read_text(encoding='utf-8', errors='ignore')

            # Check for new enum cases
            enum_cases = self.extract_added_enum_cases(diff_content)
            for enum_val, line_num in enum_cases:
                # Look for references to this enum in tests
                # Be lenient - check for value name anywhere in test
                if enum_val not in test_content:
                    potential_gaps.append(
                        f"{source_file}:{line_num} - Enum case '{enum_val}' not found in tests"
                    )

            # Check for new functions
            functions = self.extract_added_functions(diff_content)
            for func_name, line_num in functions:
                # Extract just the function name (remove namespace/class)
                simple_name = func_name.split('::')[-1]

                # Skip constructors, destructors, and getters/setters (likely tested indirectly)
                if simple_name.startswith('~') or simple_name in ['get', 'set']:
                    continue

                # Look for test that might cover this function
                # Check for the function name in test names or test content
                test_pattern = f"(?i)test.*{re.escape(simple_name)}|{re.escape(simple_name)}.*test"
                if not re.search(test_pattern, test_content):
                    # Be conservative - only warn, don't mark as critical
                    # (function might be tested indirectly)
                    potential_gaps.append(
                        f"{source_file}:{line_num} - Function '{simple_name}' may not have explicit test (heuristic check)"
                    )

        except Exception as e:
            return True, [f"Could not verify coverage: {e}"]

        has_coverage = len(potential_gaps) == 0
        return has_coverage, potential_gaps

    def check_envoy_patterns(self, filepath: str, content: str, diff_content: str) -> Tuple[List[str], List[str], List[str]]:
        """
        Check for Envoy-specific patterns and anti-patterns.
        Returns (issues, warnings, suggestions)
        """
        issues = []
        warnings = []
        suggestions = []

        # Only check new/modified lines (starts with '+')
        added_lines = [line[1:] for line in diff_content.split('\n') if line.startswith('+') and not line.startswith('+++')]
        added_content = '\n'.join(added_lines)

        if not added_content:
            return issues, warnings, suggestions

        # Check for direct time() calls
        if re.search(r'\btime\s*\(', added_content) or 'time_t' in added_content:
            warnings.append(
                f"{filepath} - Direct time() usage detected. "
                "Use TimeSystem instead: time_source_.systemTime() or monotonicTime(). "
                "See STYLE.md for details."
            )

        # Check for shared_ptr usage (prefer unique_ptr)
        shared_ptr_matches = list(re.finditer(r'shared_ptr<(\w+)>', added_content))
        unique_ptr_count = added_content.count('unique_ptr')

        if shared_ptr_matches and unique_ptr_count == 0:
            warnings.append(
                f"{filepath} - Uses shared_ptr without unique_ptr. "
                "Consider unique_ptr for clearer ownership semantics unless shared ownership is required. "
                "See STYLE.md for smart pointer guidelines."
            )

        # Check for mutex without ABSL_GUARDED_BY
        if re.search(r'\bmutex\b', added_content, re.IGNORECASE):
            if 'ABSL_GUARDED_BY' not in content:
                warnings.append(
                    f"{filepath} - Mutex usage detected without ABSL_GUARDED_BY annotation. "
                    "Add thread annotations for shared state. See STYLE.md for thread safety."
                )

        # Check for ASSERT vs RELEASE_ASSERT usage
        if 'ASSERT(' in added_content and 'RELEASE_ASSERT' not in added_content:
            suggestions.append(
                f"{filepath} - Uses ASSERT(). Verify this is appropriate: "
                "ASSERT() is debug-only. Use RELEASE_ASSERT() if check must run in production."
            )

        # Check for breaking changes without runtime guard
        breaking_keywords = ['deprecated', 'remove', 'delete', 'breaking']
        if any(keyword in added_content.lower() for keyword in breaking_keywords):
            if 'Runtime::runtimeFeatureEnabled' not in content:
                suggestions.append(
                    f"{filepath} - Potential breaking change detected. "
                    "Ensure runtime guard is used if changing default behavior. "
                    "See CONTRIBUTING.md for breaking change policy."
                )

        # Check for proper error handling
        if re.search(r'\.\w+\(.*\)(?!\s*;|\s*\))', added_content):
            # Look for unchecked return values (heuristic)
            # This is very basic - just a suggestion
            pass  # Too many false positives to warn

        return issues, warnings, suggestions

    def check_build_files_updated(self, source_files: List[str]) -> List[str]:
        """
        Check if BUILD files were updated when source files changed.
        Returns list of warnings.
        """
        warnings = []
        changed_files = set(self.get_changed_files())

        for source_file in source_files:
            if not source_file.startswith('source/'):
                continue

            # Check if BUILD file in same directory was modified
            source_dir = str(Path(source_file).parent)
            build_file = f"{source_dir}/BUILD"

            # If adding a new source file, BUILD should be updated
            # Check if file is new (not in base branch)
            stdout, stderr, code = self._run_git_command([
                "cat-file", "-e", f"{self.base_branch}:{source_file}"
            ])

            is_new_file = (code != 0)

            if is_new_file and build_file not in changed_files:
                warnings.append(
                    f"New source file {source_file} but {build_file} not updated. "
                    "Verify BUILD file includes new file."
                )

        return warnings

    def check_release_notes_updated(self) -> bool:
        """Check if changelogs/current.yaml was modified."""
        changed_files = self.get_changed_files()
        return 'changelogs/current.yaml' in changed_files

    def check_release_notes_appropriate(self, source_files: List[str], test_files: List[str], api_files: List[str]) -> Tuple[bool, List[str]]:
        """
        Check if release notes are appropriate for the changes.
        Returns (is_appropriate, list_of_suggestions)
        """
        suggestions = []

        release_notes_updated = self.check_release_notes_updated()

        # Source changes likely need release notes
        if source_files and not release_notes_updated:
            # Exception: Only test files modified
            if test_files and len(source_files) == len(test_files):
                # Might be just test fixes
                suggestions.append(
                    "Only source files with tests modified. "
                    "If this fixes a user-visible bug, add to changelogs/current.yaml under 'bug_fixes'."
                )
            else:
                suggestions.append(
                    "Source files modified but changelogs/current.yaml not updated. "
                    "If this affects users, add entry under appropriate category: "
                    "new_features, bug_fixes, deprecated, breaking_changes, etc."
                )

        # API changes definitely need release notes
        if api_files and not release_notes_updated:
            suggestions.append(
                "API files (.proto) modified but changelogs/current.yaml not updated. "
                "API changes are always user-facing and require release notes."
            )

        is_appropriate = len(suggestions) == 0 or release_notes_updated
        return is_appropriate, suggestions

    def analyze_file(self, filepath: str) -> FileAnalysis:
        """Analyze a single file for issues."""
        file_type = self.categorize_file(filepath)
        lines_added, lines_removed = self.get_file_diff_stats(filepath)

        issues = []
        warnings = []
        suggestions = []
        has_test = False
        test_path = None

        # Check for corresponding test if it's a source file
        if file_type == 'source' and filepath.endswith(('.cc', '.h')):
            has_test, test_path = self.find_corresponding_test(filepath)

            if not has_test:
                issues.append(
                    f"Missing test file: expected {test_path}. "
                    "Envoy requires 100% test coverage. See CONTRIBUTING.md."
                )
            else:
                # Heuristic coverage check
                test_file_obj = self.repo_path / test_path
                has_coverage, potential_gaps = self.check_test_coverage_heuristic(filepath, test_file_obj)

                if not has_coverage:
                    for gap in potential_gaps:
                        # Treat gaps as warnings (not critical) since this is heuristic
                        warnings.append(
                            f"Potential coverage gap: {gap}. "
                            "Verify with 'bazel test' or run '/envoy-review --rigorous-coverage' for 100% accurate check."
                        )

        # Check Envoy-specific patterns in source files
        if file_type == 'source':
            file_path = self.repo_path / filepath
            if file_path.exists():
                try:
                    content = file_path.read_text(encoding='utf-8', errors='ignore')
                    diff_content = self.get_file_diff(filepath)

                    pattern_issues, pattern_warnings, pattern_suggestions = \
                        self.check_envoy_patterns(filepath, content, diff_content)

                    issues.extend(pattern_issues)
                    warnings.extend(pattern_warnings)
                    suggestions.extend(pattern_suggestions)

                except Exception as e:
                    warnings.append(f"Could not analyze file content: {e}")

        return FileAnalysis(
            path=filepath,
            file_type=file_type,
            lines_added=lines_added,
            lines_removed=lines_removed,
            has_corresponding_test=has_test,
            test_path=test_path,
            issues=issues,
            warnings=warnings,
            suggestions=suggestions
        )

    def generate_report(self) -> ReviewReport:
        """Generate complete review report."""
        changed_files = self.get_changed_files()

        # Categorize files
        source_files = []
        test_files = []
        build_files = []
        api_files = []
        docs_files = []

        file_analyses = []
        critical_issues = []
        all_warnings = []
        all_suggestions = []
        missing_tests = []

        total_added = 0
        total_removed = 0

        # Analyze each file
        for filepath in changed_files:
            analysis = self.analyze_file(filepath)
            file_analyses.append(analysis)

            # Accumulate stats
            total_added += analysis.lines_added
            total_removed += analysis.lines_removed

            # Categorize
            if analysis.file_type == 'source':
                source_files.append(filepath)
            elif analysis.file_type == 'test':
                test_files.append(filepath)
            elif analysis.file_type == 'build':
                build_files.append(filepath)
            elif analysis.file_type == 'api':
                api_files.append(filepath)
            elif analysis.file_type == 'docs':
                docs_files.append(filepath)

            # Collect issues
            critical_issues.extend(analysis.issues)
            all_warnings.extend(analysis.warnings)
            all_suggestions.extend(analysis.suggestions)

            # Track missing tests
            if not analysis.has_corresponding_test and analysis.file_type == 'source':
                missing_tests.append({
                    'source': filepath,
                    'expected_test': analysis.test_path or 'unknown'
                })

        # Check release notes
        release_notes_updated = self.check_release_notes_updated()
        release_notes_appropriate, rn_suggestions = self.check_release_notes_appropriate(
            source_files, test_files, api_files
        )
        all_suggestions.extend(rn_suggestions)

        # Check BUILD files
        build_warnings = self.check_build_files_updated(source_files)
        all_warnings.extend(build_warnings)

        # Add helpful suggestions based on file mix
        if source_files and not test_files and not missing_tests:
            all_suggestions.append(
                "Source files modified without new tests. "
                "Verify existing tests cover new code paths with '/envoy-review' or 'bazel test'."
            )

        if api_files:
            all_suggestions.append(
                "API files (.proto) modified. Ensure: "
                "(1) Inline documentation is complete, "
                "(2) Follows api/STYLE.md guidelines, "
                "(3) Has release notes entry."
            )

        # Deduplicate lists
        critical_issues = list(dict.fromkeys(critical_issues))
        all_warnings = list(dict.fromkeys(all_warnings))
        all_suggestions = list(dict.fromkeys(all_suggestions))

        return ReviewReport(
            files_changed=len(changed_files),
            source_files=source_files,
            test_files=test_files,
            build_files=build_files,
            api_files=api_files,
            docs_files=docs_files,
            lines_added=total_added,
            lines_removed=total_removed,
            release_notes_updated=release_notes_updated,
            release_notes_appropriate=release_notes_appropriate,
            missing_tests=missing_tests,
            critical_issues=critical_issues,
            warnings=all_warnings,
            suggestions=all_suggestions,
            file_analyses=file_analyses
        )


def format_markdown_report(report: ReviewReport) -> str:
    """Format report as markdown."""
    md = ["# 🔍 Envoy Code Review Analysis (Heuristic)\n"]

    # Add disclaimer about heuristic nature
    md.append("> **Note:** This is a fast heuristic check (~90% accuracy).")
    md.append("> For 100% accurate coverage verification, use: `/envoy-review --rigorous-coverage`\n")

    # Summary
    md.append("## 📊 Summary\n")
    md.append(f"- **Files changed:** {report.files_changed}")
    md.append(f"  - Source: {len(report.source_files)}")
    md.append(f"  - Tests: {len(report.test_files)}")
    md.append(f"  - Build: {len(report.build_files)}")
    md.append(f"  - API: {len(report.api_files)}")
    md.append(f"  - Docs: {len(report.docs_files)}")
    md.append(f"- **Lines:** +{report.lines_added} / -{report.lines_removed}")
    md.append(f"- **Release notes:** {'✅ Updated' if report.release_notes_updated else '❌ Not updated'}\n")

    # Critical issues
    if report.critical_issues:
        md.append("## ❌ Critical Issues\n")
        for i, issue in enumerate(report.critical_issues, 1):
            md.append(f"{i}. {issue}")
        md.append("")

    # Warnings
    if report.warnings:
        md.append("## ⚠️  Warnings\n")
        for i, warning in enumerate(report.warnings, 1):
            md.append(f"{i}. {warning}")
        md.append("")

    # Suggestions
    if report.suggestions:
        md.append("## 💡 Suggestions\n")
        for i, suggestion in enumerate(report.suggestions, 1):
            md.append(f"{i}. {suggestion}")
        md.append("")

    # Missing tests detail
    if report.missing_tests:
        md.append("## 🧪 Missing Tests\n")
        for mt in report.missing_tests:
            md.append(f"- **Source:** `{mt['source']}`")
            md.append(f"  **Expected:** `{mt['expected_test']}`\n")

    # Success message
    if not report.critical_issues and not report.warnings:
        md.append("## ✅ All Checks Passed\n")
        md.append("No critical issues or warnings detected!")
        if report.suggestions:
            md.append("\nConsider the suggestions above to further improve code quality.")
        md.append("")

    # Next steps
    md.append("## 📝 Next Steps\n")
    if report.critical_issues:
        md.append("1. Fix critical issues before merging")
        md.append("2. Run tests: `bazel test //test/...`")
        md.append("3. Verify coverage: `test/run_envoy_bazel_coverage.sh` or `/envoy-review --rigorous-coverage`")
    elif report.warnings:
        md.append("1. Review warnings and address if appropriate")
        md.append("2. Run tests: `bazel test //test/...`")
        md.append("3. Consider suggestions for code quality improvements")
    else:
        md.append("1. Run full test suite: `bazel test //test/...`")
        md.append("2. Request human review for architecture and logic")
        md.append("3. Consider rigorous review: `/envoy-review --rigorous-coverage`")
    md.append("")

    # Reference
    md.append("## 📚 References\n")
    md.append("- Test coverage policy: `CONTRIBUTING.md`")
    md.append("- Code style guide: `STYLE.md`")
    md.append("- Build system: `bazel/README.md`")
    md.append("- Development guide: `CLAUDE.md`")

    return '\n'.join(md)


def format_text_report(report: ReviewReport) -> str:
    """Format report as plain text."""
    lines = ["=" * 80]
    lines.append("ENVOY CODE REVIEW ANALYSIS (HEURISTIC)")
    lines.append("=" * 80)
    lines.append("")

    lines.append(f"Files changed: {report.files_changed}")
    lines.append(f"  - Source: {len(report.source_files)}, Tests: {len(report.test_files)}")
    lines.append(f"Lines: +{report.lines_added} / -{report.lines_removed}")
    lines.append(f"Release notes: {'YES' if report.release_notes_updated else 'NO'}")
    lines.append("")

    if report.critical_issues:
        lines.append("CRITICAL ISSUES:")
        for issue in report.critical_issues:
            lines.append(f"  - {issue}")
        lines.append("")

    if report.warnings:
        lines.append("WARNINGS:")
        for warning in report.warnings:
            lines.append(f"  - {warning}")
        lines.append("")

    if report.missing_tests:
        lines.append("MISSING TESTS:")
        for mt in report.missing_tests:
            lines.append(f"  Source: {mt['source']}")
            lines.append(f"  Expected: {mt['expected_test']}")
        lines.append("")

    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Envoy Code Review Helper - Fast heuristic analysis for CI/CD. "
                    "For comprehensive analysis, use: /envoy-review --rigorous-coverage",
        epilog="See CLAUDE.md for comparison of this script vs the interactive agent."
    )
    parser.add_argument(
        '--repo',
        default='.',
        help='Path to Envoy repository (default: current directory)'
    )
    parser.add_argument(
        '--base',
        default='main',
        help='Base branch for comparison (default: main)'
    )
    parser.add_argument(
        '--format',
        choices=['json', 'markdown', 'text'],
        default='json',
        help='Output format (default: json)'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    try:
        reviewer = EnvoyReviewer(args.repo, args.base)

        if args.verbose:
            print(f"Analyzing repository: {reviewer.repo_path}", file=sys.stderr)
            print(f"Base branch: {args.base}", file=sys.stderr)
            print(f"Note: This is heuristic analysis (~90% accuracy)", file=sys.stderr)
            print(f"For 100% coverage: /envoy-review --rigorous-coverage\n", file=sys.stderr)

        report = reviewer.generate_report()

        # Output in requested format
        if args.format == 'json':
            # Convert dataclasses to dict
            report_dict = asdict(report)
            print(json.dumps(report_dict, indent=2))

        elif args.format == 'markdown':
            print(format_markdown_report(report))

        elif args.format == 'text':
            print(format_text_report(report))

        # Return non-zero if critical issues found
        sys.exit(1 if report.critical_issues else 0)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(2)


if __name__ == '__main__':
    main()
