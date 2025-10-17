#!/usr/bin/env python3
"""
Envoy Code Review Helper

This script provides automated analysis for Envoy code reviews.
It's designed to be used standalone or invoked by the Claude Code review agent.

Usage:
    ./envoy-review-helper.py [options]

Options:
    --repo PATH         Path to Envoy repository (default: current directory)
    --base BRANCH       Base branch for comparison (default: main)
    --format FORMAT     Output format: json, markdown, text (default: json)
    --coverage          Run coverage analysis (requires bazel)
    --verbose           Verbose output

Author: AI Assistant for Envoy Development
"""

import os
import sys
import subprocess
import json
import argparse
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass, asdict


@dataclass
class FileAnalysis:
    """Analysis result for a single file."""
    path: str
    file_type: str  # source, test, build, api, docs, other
    lines_added: int
    lines_removed: int
    has_corresponding_test: bool
    test_path: Optional[str]
    issues: List[str]
    warnings: List[str]


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

        # Check if file exists
        test_path = self.repo_path / test_file
        exists = test_path.exists()

        return exists, test_file if exists else test_file

    def check_release_notes_updated(self) -> bool:
        """Check if changelogs/current.yaml was modified."""
        changed_files = self.get_changed_files()
        return 'changelogs/current.yaml' in changed_files

    def verify_code_has_tests(self, filepath: str) -> Tuple[bool, List[str]]:
        """
        CRITICAL CHECK: Verify that code in source file actually has test coverage.
        Returns (has_coverage, list_of_untested_functions)
        """
        if not filepath.startswith('source/') or not filepath.endswith('.cc'):
            return True, []  # Not applicable

        # Find test file
        test_file = filepath.replace('source/', 'test/', 1).replace('.cc', '_test.cc')
        test_path = self.repo_path / test_file

        if not test_path.exists():
            return False, ["Test file does not exist"]

        # Get the diff to see what changed in source
        stdout, stderr, code = self._run_git_command([
            "diff", f"{self.base_branch}...HEAD", "--", filepath
        ])

        if code != 0:
            return True, []  # Can't verify

        # Look for new/modified function definitions or case statements
        import re
        untested = []

        # Check for new enum cases (like NE operator)
        for line in stdout.split('\n'):
            if line.startswith('+') and 'case ' in line and '::' in line:
                # Extract enum value
                match = re.search(r'case\s+\w+::\w+::(\w+):', line)
                if match:
                    enum_val = match.group(1)
                    # Check if test file mentions this enum
                    try:
                        test_content = test_path.read_text(encoding='utf-8', errors='ignore')
                        if enum_val not in test_content:
                            untested.append(f"Enum case {enum_val} not found in tests")
                    except:
                        pass

        has_coverage = len(untested) == 0
        return has_coverage, untested

    def analyze_file(self, filepath: str) -> FileAnalysis:
        """Analyze a single file for issues."""
        file_type = self.categorize_file(filepath)
        lines_added, lines_removed = self.get_file_diff_stats(filepath)

        issues = []
        warnings = []
        has_test = False
        test_path = None

        # Check for corresponding test if it's a source file
        if file_type == 'source' and filepath.endswith(('.cc', '.h')):
            has_test, test_path = self.find_corresponding_test(filepath)

            if not has_test:
                issues.append(
                    f"Missing test file: expected {test_path}"
                )
            else:
                # CRITICAL: Verify code actually has test coverage
                has_coverage, untested = self.verify_code_has_tests(filepath)
                if not has_coverage:
                    for problem in untested:
                        issues.append(
                            f"CRITICAL: Code without test coverage - {problem}"
                        )

        # Check file-specific patterns
        if file_type == 'source':
            file_path = self.repo_path / filepath
            if file_path.exists():
                try:
                    content = file_path.read_text(encoding='utf-8', errors='ignore')

                    # Check for common anti-patterns
                    if 'shared_ptr' in content and 'unique_ptr' not in content:
                        warnings.append(
                            "Uses shared_ptr - consider unique_ptr for clearer ownership"
                        )

                    if 'time(' in content or 'time_t' in content:
                        warnings.append(
                            "Direct time() call detected - should use TimeSystem"
                        )

                    # Check for thread safety annotations
                    if 'mutex' in content.lower() and 'ABSL_GUARDED_BY' not in content:
                        warnings.append(
                            "Mutex usage detected without ABSL_GUARDED_BY annotation"
                        )

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
            warnings=warnings
        )

    def check_deleted_tests(self) -> List[str]:
        """Check if any tests were deleted."""
        deleted_tests = []

        # Get diff to see deleted lines
        stdout, stderr, code = self._run_git_command([
            "diff", f"{self.base_branch}...HEAD"
        ])

        if code != 0:
            return deleted_tests

        # Look for deleted TEST_F or TEST( lines
        in_deleted_test = False
        test_name = None

        for line in stdout.split('\n'):
            # Check for deleted test definition
            if line.startswith('-TEST_F(') or line.startswith('-TEST('):
                # Extract test name
                import re
                match = re.search(r'-TEST(?:_F)?\(([^,]+),\s*([^)]+)\)', line)
                if match:
                    test_name = f"{match.group(1)}.{match.group(2)}"
                    in_deleted_test = True
            # Check if this is in a test file
            elif line.startswith('---') and '_test.cc' in line:
                in_deleted_test = True

        # Also check for net deletions in test files
        stdout, stderr, code = self._run_git_command([
            "diff", "--numstat", f"{self.base_branch}...HEAD"
        ])

        if code == 0:
            for line in stdout.split('\n'):
                parts = line.split()
                if len(parts) >= 3 and '_test.cc' in parts[2]:
                    try:
                        added = int(parts[0]) if parts[0] != '-' else 0
                        removed = int(parts[1]) if parts[1] != '-' else 0
                        if removed > added:  # Net deletion
                            deleted_tests.append(
                                f"Test file {parts[2]} has net deletion: "
                                f"-{removed} +{added} lines"
                            )
                    except ValueError:
                        pass

        return deleted_tests

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
        suggestions = []
        missing_tests = []

        total_added = 0
        total_removed = 0

        # Check for deleted tests - this is a WARNING (not critical by itself)
        deleted_tests = self.check_deleted_tests()
        if deleted_tests:
            for deleted_test in deleted_tests:
                all_warnings.append(
                    f"Test deleted: {deleted_test}. "
                    "Verify this was intentional and coverage is still 100%."
                )

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

            # Track missing tests
            if analysis.issues and not analysis.has_corresponding_test:
                missing_tests.append({
                    'source': filepath,
                    'expected_test': analysis.test_path or 'unknown'
                })

        # Check release notes
        release_notes_updated = self.check_release_notes_updated()

        # Add critical issue if release notes missing for source changes
        if source_files and not release_notes_updated and not test_files:
            critical_issues.append(
                "Source files modified but changelogs/current.yaml not updated. "
                "Add release note if this change affects users."
            )

        # Add suggestions
        if source_files and not test_files:
            suggestions.append(
                "Only source files modified without test updates. "
                "Verify existing tests cover new code paths."
            )

        if api_files:
            suggestions.append(
                "API files (.proto) modified. Ensure inline documentation is complete "
                "and follows api/STYLE.md guidelines."
            )

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
            missing_tests=missing_tests,
            critical_issues=critical_issues,
            warnings=all_warnings,
            suggestions=suggestions,
            file_analyses=file_analyses
        )


def format_markdown_report(report: ReviewReport) -> str:
    """Format report as markdown."""
    md = ["# 🔍 Envoy Code Review Analysis\n"]

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

    # File details
    md.append("## 📂 File Details\n")
    md.append("| File | Type | +/- | Test | Issues |")
    md.append("|------|------|-----|------|--------|")

    for fa in report.file_analyses:
        test_status = "✅" if fa.has_corresponding_test or fa.file_type != 'source' else "❌"
        issues_count = len(fa.issues) + len(fa.warnings)
        issues_str = f"{issues_count} issues" if issues_count > 0 else "✅"

        md.append(
            f"| `{fa.path}` | {fa.file_type} | "
            f"+{fa.lines_added}/-{fa.lines_removed} | {test_status} | {issues_str} |"
        )

    return '\n'.join(md)


def format_text_report(report: ReviewReport) -> str:
    """Format report as plain text."""
    lines = ["=" * 80]
    lines.append("ENVOY CODE REVIEW ANALYSIS")
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

    if report.missing_tests:
        lines.append("MISSING TESTS:")
        for mt in report.missing_tests:
            lines.append(f"  Source: {mt['source']}")
            lines.append(f"  Expected: {mt['expected_test']}")
        lines.append("")

    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Envoy Code Review Helper - Automated analysis for code reviews"
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
