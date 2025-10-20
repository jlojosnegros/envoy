#!/usr/bin/env python3
"""
Envoy Coverage Tracker

Manages historical coverage data for branches to enable coverage regression detection.

Usage:
    # Save coverage for a branch
    ./envoy-coverage-tracker.py --branch main --save

    # Check if cache is stale
    ./envoy-coverage-tracker.py --branch main --check-stale

    # Compare current branch with baseline
    ./envoy-coverage-tracker.py --compare-with main

    # Get coverage summary
    ./envoy-coverage-tracker.py --branch main --summary

    # Clean old caches
    ./envoy-coverage-tracker.py --clean-cache --older-than 30

Author: AI Assistant for Envoy Development
"""

import os
import sys
import json
import argparse
import subprocess
from pathlib import Path
from datetime import datetime, timedelta, timezone
from typing import Dict, Optional, Tuple
import re


class CoverageTracker:
    """Manages coverage data caching and comparison."""

    def __init__(self, repo_path: Path):
        self.repo_path = Path(repo_path).resolve()
        self.cache_dir = self.repo_path / ".claude" / "coverage-cache"
        self.coverage_dir = self.repo_path / "generated" / "coverage"

        # Create cache dir if needed
        self.cache_dir.mkdir(parents=True, exist_ok=True)

    def _run_git_command(self, args: list) -> Tuple[str, int]:
        """Run git command and return (output, returncode)."""
        try:
            result = subprocess.run(
                ["git"] + args,
                cwd=self.repo_path,
                capture_output=True,
                text=True,
                timeout=30
            )
            return result.stdout.strip(), result.returncode
        except Exception as e:
            print(f"Error running git command: {e}", file=sys.stderr)
            return "", 1

    def get_branch_commit(self, branch: str) -> Optional[str]:
        """Get current commit SHA for a branch."""
        stdout, code = self._run_git_command(["rev-parse", branch])
        return stdout if code == 0 else None

    def get_current_branch(self) -> Optional[str]:
        """Get current branch name."""
        stdout, code = self._run_git_command(["branch", "--show-current"])
        return stdout if code == 0 else None

    def get_branch_timestamp(self, branch: str) -> Optional[str]:
        """Get timestamp of last commit on branch."""
        stdout, code = self._run_git_command([
            "log", "-1", "--format=%cI", branch
        ])
        return stdout if code == 0 else None

    def parse_coverage_summary(self, coverage_txt_path: Path) -> Optional[Dict]:
        """
        Parse coverage summary from generated/coverage/coverage.txt

        Expected format from llvm-cov:
        TOTAL  150000  148500  99.00%
        """
        if not coverage_txt_path.exists():
            return None

        try:
            content = coverage_txt_path.read_text()

            # Look for TOTAL line
            for line in content.split('\n'):
                if 'TOTAL' in line or 'total' in line.lower():
                    parts = line.split()
                    if len(parts) >= 4:
                        try:
                            total = int(parts[1])
                            covered = int(parts[2])
                            percent = float(parts[3].rstrip('%'))

                            return {
                                "lines_total": total,
                                "lines_covered": covered,
                                "coverage_percent": percent
                            }
                        except ValueError:
                            continue

            # Fallback: try to extract from any percentage line
            percent_match = re.search(r'(\d+\.?\d*)%', content)
            if percent_match:
                return {
                    "lines_total": 0,
                    "lines_covered": 0,
                    "coverage_percent": float(percent_match.group(1))
                }

            return None
        except Exception as e:
            print(f"Error parsing coverage summary: {e}", file=sys.stderr)
            return None

    def save_coverage(self, branch: str) -> bool:
        """
        Save current coverage data for a branch.
        Reads from generated/coverage/ and saves to cache.
        """
        # Get branch info
        commit_sha = self.get_branch_commit(branch)
        if not commit_sha:
            print(f"Error: Could not get commit SHA for branch '{branch}'", file=sys.stderr)
            return False

        # Check if coverage data exists
        coverage_txt = self.coverage_dir / "coverage.txt"
        if not coverage_txt.exists():
            print(f"Error: Coverage data not found at {coverage_txt}", file=sys.stderr)
            print("Run: test/run_envoy_bazel_coverage.sh", file=sys.stderr)
            return False

        # Parse coverage
        summary = self.parse_coverage_summary(coverage_txt)
        if not summary:
            print(f"Error: Could not parse coverage summary from {coverage_txt}", file=sys.stderr)
            return False

        # Create coverage data
        coverage_data = {
            "branch": branch,
            "commit_sha": commit_sha,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "branch_last_commit": self.get_branch_timestamp(branch),
            "coverage_summary": summary,
            "metadata": {
                "generated_by": "envoy-coverage-tracker.py",
                "repo_path": str(self.repo_path),
                "coverage_source": str(coverage_txt)
            }
        }

        # Save to cache
        cache_file = self.cache_dir / f"{branch}.json"
        try:
            cache_file.write_text(json.dumps(coverage_data, indent=2))
            print(f"✅ Coverage saved for branch '{branch}'")
            print(f"   Commit: {commit_sha[:8]}")
            print(f"   Coverage: {summary['coverage_percent']:.2f}%")
            print(f"   Cache: {cache_file}")
            return True
        except Exception as e:
            print(f"Error saving coverage: {e}", file=sys.stderr)
            return False

    def load_coverage(self, branch: str) -> Optional[Dict]:
        """Load cached coverage data for a branch."""
        cache_file = self.cache_dir / f"{branch}.json"
        if not cache_file.exists():
            return None

        try:
            return json.loads(cache_file.read_text())
        except Exception as e:
            print(f"Error loading coverage cache: {e}", file=sys.stderr)
            return None

    def is_cache_stale(self, branch: str, max_age_days: int = 7) -> Tuple[bool, str]:
        """
        Check if cached coverage is stale.
        Returns (is_stale, reason)
        """
        coverage_data = self.load_coverage(branch)
        if not coverage_data:
            return True, f"No cache found for branch '{branch}'"

        # Check if branch commit changed
        current_commit = self.get_branch_commit(branch)
        cached_commit = coverage_data.get("commit_sha")

        if current_commit != cached_commit:
            return True, f"Branch '{branch}' has new commits ({cached_commit[:8]} → {current_commit[:8]})"

        # Check age
        try:
            cached_time = datetime.fromisoformat(coverage_data["timestamp"])
            age = datetime.now(timezone.utc) - cached_time

            if age.days > max_age_days:
                return True, f"Cache is {age.days} days old (max {max_age_days} days)"
        except Exception:
            pass

        return False, "Cache is fresh"

    def check_stale(self, branch: str, max_age_days: int = 7) -> bool:
        """Check and report if cache is stale."""
        is_stale, reason = self.is_cache_stale(branch, max_age_days)

        if is_stale:
            print(f"⚠️  Cache is STALE: {reason}")
            return False
        else:
            print(f"✅ Cache is FRESH: {reason}")
            coverage_data = self.load_coverage(branch)
            if coverage_data:
                summary = coverage_data["coverage_summary"]
                print(f"   Coverage: {summary['coverage_percent']:.2f}%")
                print(f"   Commit: {coverage_data['commit_sha'][:8]}")
                print(f"   Date: {coverage_data['timestamp'][:19]}")
            return True

    def compare_coverage(self, base_branch: str, current_data: Optional[Dict] = None) -> Optional[Dict]:
        """
        Compare current coverage with base branch.

        Args:
            base_branch: Branch to compare against (e.g., 'main')
            current_data: Optional current coverage data (if None, reads from generated/)

        Returns:
            Dict with comparison results or None if base not cached
        """
        # Load base coverage
        base_data = self.load_coverage(base_branch)
        if not base_data:
            return {
                "error": f"No cached coverage for base branch '{base_branch}'",
                "suggestion": f"Run: ./scripts/envoy-coverage-tracker.py --branch {base_branch} --save"
            }

        # Check if base is stale
        is_stale, stale_reason = self.is_cache_stale(base_branch)

        # Get current coverage
        if current_data is None:
            coverage_txt = self.coverage_dir / "coverage.txt"
            if not coverage_txt.exists():
                return {
                    "error": "No current coverage data found",
                    "suggestion": "Run: test/run_envoy_bazel_coverage.sh"
                }
            current_summary = self.parse_coverage_summary(coverage_txt)
            if not current_summary:
                return {"error": "Could not parse current coverage"}
            current_branch = self.get_current_branch()
        else:
            current_summary = current_data["coverage_summary"]
            current_branch = current_data.get("branch", "unknown")

        base_summary = base_data["coverage_summary"]

        # Calculate diff
        diff_percent = current_summary["coverage_percent"] - base_summary["coverage_percent"]
        diff_lines = current_summary.get("lines_covered", 0) - base_summary.get("lines_covered", 0)

        has_regression = diff_percent < -0.1  # More than 0.1% drop is regression

        return {
            "base_branch": base_branch,
            "base_coverage": base_summary["coverage_percent"],
            "base_commit": base_data["commit_sha"][:8],
            "base_is_stale": is_stale,
            "base_stale_reason": stale_reason if is_stale else None,
            "current_branch": current_branch,
            "current_coverage": current_summary["coverage_percent"],
            "diff_percent": diff_percent,
            "diff_lines": diff_lines,
            "has_regression": has_regression,
            "assessment": self._assess_coverage_change(diff_percent, is_stale)
        }

    def _assess_coverage_change(self, diff_percent: float, base_stale: bool) -> str:
        """Assess coverage change and return human-readable assessment."""
        if base_stale:
            warning = " (⚠️  base cache is stale)"
        else:
            warning = ""

        if diff_percent < -0.5:
            return f"❌ Significant regression{warning}"
        elif diff_percent < -0.1:
            return f"⚠️  Minor regression{warning}"
        elif diff_percent < 0.1:
            return f"✅ No change{warning}"
        elif diff_percent < 0.5:
            return f"✅ Minor improvement{warning}"
        else:
            return f"🎉 Significant improvement{warning}"

    def get_summary(self, branch: str) -> bool:
        """Print coverage summary for a branch."""
        data = self.load_coverage(branch)
        if not data:
            print(f"No cached coverage for branch '{branch}'", file=sys.stderr)
            return False

        summary = data["coverage_summary"]

        print(f"📊 Coverage Summary for '{branch}':")
        print(f"   Coverage: {summary['coverage_percent']:.2f}%")
        if summary.get('lines_total'):
            print(f"   Lines: {summary['lines_covered']:,} / {summary['lines_total']:,}")
        print(f"   Commit: {data['commit_sha'][:8]}")
        print(f"   Cached: {data['timestamp'][:19]}")

        # Check if stale
        is_stale, reason = self.is_cache_stale(branch)
        if is_stale:
            print(f"   Status: ⚠️  STALE ({reason})")
        else:
            print(f"   Status: ✅ FRESH")

        return True

    def clean_cache(self, older_than_days: int) -> int:
        """Remove cache files older than specified days. Returns count removed."""
        cutoff = datetime.now(timezone.utc) - timedelta(days=older_than_days)
        removed = 0

        for cache_file in self.cache_dir.glob("*.json"):
            try:
                data = json.loads(cache_file.read_text())
                cached_time = datetime.fromisoformat(data["timestamp"])

                if cached_time < cutoff:
                    cache_file.unlink()
                    print(f"Removed: {cache_file.name} (age: {(datetime.now(timezone.utc) - cached_time).days} days)")
                    removed += 1
            except Exception as e:
                print(f"Error processing {cache_file}: {e}", file=sys.stderr)

        return removed


def main():
    parser = argparse.ArgumentParser(
        description="Envoy Coverage Tracker - Manage historical coverage data",
        epilog="See .claude/coverage-cache/README.md for detailed usage"
    )

    parser.add_argument('--repo', default='.', help='Path to Envoy repository')
    parser.add_argument('--branch', help='Branch name (e.g., main, develop)')

    # Actions
    action_group = parser.add_mutually_exclusive_group(required=True)
    action_group.add_argument('--save', action='store_true',
                            help='Save current coverage for --branch')
    action_group.add_argument('--check-stale', action='store_true',
                            help='Check if cached coverage is stale')
    action_group.add_argument('--compare-with',
                            help='Compare current coverage with specified branch')
    action_group.add_argument('--summary', action='store_true',
                            help='Show coverage summary for --branch')
    action_group.add_argument('--clean-cache', action='store_true',
                            help='Remove old cached coverage data')

    parser.add_argument('--older-than', type=int, default=30,
                       help='Days threshold for --clean-cache (default: 30)')
    parser.add_argument('--max-age', type=int, default=7,
                       help='Max cache age in days for staleness check (default: 7)')

    args = parser.parse_args()

    tracker = CoverageTracker(args.repo)

    try:
        if args.save:
            if not args.branch:
                parser.error("--save requires --branch")
            success = tracker.save_coverage(args.branch)
            sys.exit(0 if success else 1)

        elif args.check_stale:
            if not args.branch:
                parser.error("--check-stale requires --branch")
            is_fresh = tracker.check_stale(args.branch, args.max_age)
            sys.exit(0 if is_fresh else 1)

        elif args.compare_with:
            result = tracker.compare_coverage(args.compare_with)
            if not result:
                sys.exit(1)

            if "error" in result:
                print(f"❌ {result['error']}", file=sys.stderr)
                if "suggestion" in result:
                    print(f"   {result['suggestion']}", file=sys.stderr)
                sys.exit(1)

            # Print comparison
            print(f"📊 Coverage Comparison:")
            print(f"   Base ({result['base_branch']}): {result['base_coverage']:.2f}% @ {result['base_commit']}")
            print(f"   Current ({result['current_branch']}): {result['current_coverage']:.2f}%")
            print(f"   Difference: {result['diff_percent']:+.2f}%")
            print(f"   Assessment: {result['assessment']}")

            if result['base_is_stale']:
                print(f"\n⚠️  Warning: {result['base_stale_reason']}")
                print(f"   Consider re-running: ./scripts/envoy-coverage-tracker.py --branch {result['base_branch']} --save")

            sys.exit(0 if not result['has_regression'] else 1)

        elif args.summary:
            if not args.branch:
                parser.error("--summary requires --branch")
            success = tracker.get_summary(args.branch)
            sys.exit(0 if success else 1)

        elif args.clean_cache:
            removed = tracker.clean_cache(args.older_than)
            print(f"Cleaned {removed} old cache file(s)")
            sys.exit(0)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(2)


if __name__ == '__main__':
    main()
