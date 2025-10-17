#!/bin/bash
#
# Run all test scenarios for Envoy Code Reviewer
#
# This script creates test branches for each scenario and runs the review helper
# to validate the agent's detection capabilities.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
HELPER="$REPO_ROOT/scripts/envoy-review-helper.py"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "Envoy Code Reviewer Test Scenarios"
echo "========================================="
echo ""

# Save current branch
ORIGINAL_BRANCH=$(git -C "$REPO_ROOT" rev-parse --abbrev-ref HEAD)
echo "Current branch: $ORIGINAL_BRANCH"
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    git -C "$REPO_ROOT" checkout "$ORIGINAL_BRANCH" 2>/dev/null || true
    git -C "$REPO_ROOT" branch -D test-scenario-1 2>/dev/null || true
    git -C "$REPO_ROOT" branch -D test-scenario-2 2>/dev/null || true
    git -C "$REPO_ROOT" branch -D test-scenario-3 2>/dev/null || true
}

trap cleanup EXIT

# Test Scenario 1: Missing Test Coverage
echo "========================================="
echo "Scenario 1: Missing Test Coverage"
echo "========================================="

git -C "$REPO_ROOT" checkout -b test-scenario-1 2>/dev/null || git -C "$REPO_ROOT" checkout test-scenario-1

# Create a source file without test
cat > "$REPO_ROOT/source/common/http/test_missing_coverage.cc" << 'EOF'
#include "source/common/http/test_missing_coverage.h"

namespace Envoy {
namespace Http {

void TestFunction::doSomething() {
  // This function has no test coverage
  if (condition_) {
    performAction();
  }
}

} // namespace Http
} // namespace Envoy
EOF

git -C "$REPO_ROOT" add source/common/http/test_missing_coverage.cc
git -C "$REPO_ROOT" commit -m "test: add file without test coverage" --no-verify 2>/dev/null || true

echo "Running review..."
if python3 "$HELPER" --repo "$REPO_ROOT" --base "$ORIGINAL_BRANCH" --format markdown > /tmp/scenario1-report.md; then
    echo -e "${GREEN}✓ Analysis completed${NC}"
else
    echo -e "${YELLOW}⚠ Analysis completed with warnings${NC}"
fi

# Check if missing test was detected
if grep -q "Missing test file" /tmp/scenario1-report.md; then
    echo -e "${GREEN}✓ PASS: Missing test detected${NC}"
else
    echo -e "${RED}✗ FAIL: Missing test NOT detected${NC}"
fi

echo ""
echo "Report saved to: /tmp/scenario1-report.md"
echo ""

# Cleanup scenario 1
rm -f "$REPO_ROOT/source/common/http/test_missing_coverage.cc"
git -C "$REPO_ROOT" checkout "$ORIGINAL_BRANCH"

# Test Scenario 2: Missing Release Notes
echo "========================================="
echo "Scenario 2: Missing Release Notes"
echo "========================================="

git -C "$REPO_ROOT" checkout -b test-scenario-2 2>/dev/null || git -C "$REPO_ROOT" checkout test-scenario-2

# Modify an existing file (simulate feature addition)
echo "// New feature added" >> "$REPO_ROOT/source/common/http/conn_manager_impl.cc"
git -C "$REPO_ROOT" add source/common/http/conn_manager_impl.cc
git -C "$REPO_ROOT" commit -m "feat: add new feature" --no-verify 2>/dev/null || true

echo "Running review..."
if python3 "$HELPER" --repo "$REPO_ROOT" --base "$ORIGINAL_BRANCH" --format markdown > /tmp/scenario2-report.md; then
    echo -e "${GREEN}✓ Analysis completed${NC}"
else
    echo -e "${YELLOW}⚠ Analysis completed with warnings${NC}"
fi

# Check if missing release notes detected
if grep -q "changelogs/current.yaml" /tmp/scenario2-report.md; then
    echo -e "${GREEN}✓ PASS: Missing release notes detected${NC}"
else
    echo -e "${RED}✗ FAIL: Missing release notes NOT detected${NC}"
fi

echo ""
echo "Report saved to: /tmp/scenario2-report.md"
echo ""

# Cleanup scenario 2
git -C "$REPO_ROOT" checkout source/common/http/conn_manager_impl.cc
git -C "$REPO_ROOT" checkout "$ORIGINAL_BRANCH"

# Test Scenario 3: Style Violations
echo "========================================="
echo "Scenario 3: Style Violations (Simulated)"
echo "========================================="

git -C "$REPO_ROOT" checkout -b test-scenario-3 2>/dev/null || git -C "$REPO_ROOT" checkout test-scenario-3

# Create file with style issues
cat > "$REPO_ROOT/source/common/http/test_style.cc" << 'EOF'
#include <memory>
#include <time.h>

namespace Envoy {
namespace Http {

class TestClass {
public:
  void doSomething() {
    // Bad: using shared_ptr where unique_ptr would work
    std::shared_ptr<Foo> foo = std::make_shared<Foo>();

    // Bad: direct time() call
    time_t now = time(nullptr);

    // Bad: mutex without ABSL_GUARDED_BY
    std::mutex mutex_;
    int data_;  // should be ABSL_GUARDED_BY(mutex_)
  }
};

} // namespace Http
} // namespace Envoy
EOF

git -C "$REPO_ROOT" add source/common/http/test_style.cc
git -C "$REPO_ROOT" commit -m "test: file with style violations" --no-verify 2>/dev/null || true

echo "Running review..."
if python3 "$HELPER" --repo "$REPO_ROOT" --base "$ORIGINAL_BRANCH" --format markdown > /tmp/scenario3-report.md; then
    echo -e "${GREEN}✓ Analysis completed${NC}"
else
    echo -e "${YELLOW}⚠ Analysis completed with warnings${NC}"
fi

# Check if style issues detected
ISSUES_FOUND=0

if grep -q "shared_ptr" /tmp/scenario3-report.md; then
    echo -e "${GREEN}✓ PASS: shared_ptr usage detected${NC}"
    ((ISSUES_FOUND++))
fi

if grep -q "time()" /tmp/scenario3-report.md || grep -q "TimeSystem" /tmp/scenario3-report.md; then
    echo -e "${GREEN}✓ PASS: Direct time() call detected${NC}"
    ((ISSUES_FOUND++))
fi

if grep -q "ABSL_GUARDED_BY" /tmp/scenario3-report.md; then
    echo -e "${GREEN}✓ PASS: Missing thread annotation detected${NC}"
    ((ISSUES_FOUND++))
fi

if [ $ISSUES_FOUND -ge 2 ]; then
    echo -e "${GREEN}✓ PASS: Style checks working (found $ISSUES_FOUND/3 issues)${NC}"
else
    echo -e "${YELLOW}⚠ PARTIAL: Only found $ISSUES_FOUND/3 style issues${NC}"
fi

echo ""
echo "Report saved to: /tmp/scenario3-report.md"
echo ""

# Cleanup scenario 3
rm -f "$REPO_ROOT/source/common/http/test_style.cc"
git -C "$REPO_ROOT" checkout "$ORIGINAL_BRANCH"

# Summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo ""
echo "All scenario reports saved to /tmp/"
echo "  - /tmp/scenario1-report.md (Missing test coverage)"
echo "  - /tmp/scenario2-report.md (Missing release notes)"
echo "  - /tmp/scenario3-report.md (Style violations)"
echo ""
echo -e "${GREEN}Testing completed!${NC}"
echo ""
echo "To view a report:"
echo "  cat /tmp/scenario1-report.md"
echo ""
