# Sub-agent: Test Coverage Analysis

## Purpose
Verify that new code has adequate test coverage.

## Operation Modes

### SEMI Mode (default) - No Docker
Heuristic analysis to estimate coverage without running full build.

### FULL Mode (--coverage-full) - With Docker
Runs real coverage build for precise verification.

---

## SEMI MODE (Heuristic)

### Objective
Estimate with high probability whether new code is covered by tests.

### Verifications

#### 1. Test File Matching
For each modified `.cc` file, search for corresponding test file:

```bash
# For source/common/foo/bar.cc search:
# - test/common/foo/bar_test.cc
# - test/common/foo_test.cc

FILE="source/common/foo/bar.cc"
TEST_DIR=$(echo $FILE | sed 's/^source/test/')
TEST_FILE=$(echo $TEST_DIR | sed 's/\.cc$/_test.cc/')
```

**If test file doesn't exist:**
```
WARNING: No test file found for source/common/foo/bar.cc
Expected: test/common/foo/bar_test.cc
```

#### 2. New Functions Analysis

Identify new functions/methods in the diff:

```bash
git diff <base>...HEAD -- '*.cc' '*.h' | grep -E '^\+.*\b(void|bool|int|string|Status)\s+\w+\s*\('
```

For each new function, verify if test exists that references it:

```bash
grep -r "functionName" test/
```

#### 3. Added Tests Verification

If there are new files in `source/`, there should be new files in `test/`:

```bash
git diff --name-only --diff-filter=A <base>...HEAD | grep '^source/'
git diff --name-only --diff-filter=A <base>...HEAD | grep '^test/'
```

#### 4. Code Branch Analysis

Search for new `if`/`else`/`switch` and verify coverage:
- Are there tests for the positive case?
- Are there tests for the negative case?
- Are there tests for edge cases?

### Confidence Calculation

```
Base Confidence: 50%
+ 15% if corresponding test file exists
+ 15% if test file was modified/added
+ 10% if references to new functions found in tests
+ 10% if new_tests/new_code ratio > 0.5
= Final Confidence (max 100%)
```

### SEMI Mode Output Format

```json
{
  "agent": "test-coverage",
  "mode": "semi",
  "confidence_percentage": 75,
  "findings": [
    {
      "type": "WARNING",
      "check": "missing_test_file",
      "message": "No test file found",
      "location": "source/common/foo.cc",
      "suggestion": "Create test/common/foo_test.cc"
    },
    {
      "type": "INFO",
      "check": "uncovered_function",
      "message": "Potentially untested function",
      "location": "source/common/foo.cc:123 - processRequest()",
      "suggestion": "Add test for processRequest()"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "coverage_estimate": {
    "new_source_files": 3,
    "matching_test_files": 2,
    "new_functions": 5,
    "tested_functions": 3
  }
}
```

---

## FULL MODE (Coverage Build)

### Requires Docker: YES

### Prior Warning
```
WARNING: Coverage build is a VERY slow process (may take hours).
Are you sure you want to run it? (y/n)

Alternative: You can use the /coverage command on your GitHub PR
to get a coverage report without running locally.
```

### CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh coverage' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-coverage.log
```

### Report Analysis

After build, analyze coverage report for modified files:

1. Locate generated coverage report
2. Filter by files in diff
3. Verify coverage % in new code
4. Identify uncovered lines

### Envoy Criteria
- 100% coverage expected in new code
- Exceptions must be justified in PR

### FULL Mode Output Format

```json
{
  "agent": "test-coverage",
  "mode": "full",
  "docker_executed": true,
  "findings": [
    {
      "type": "ERROR",
      "check": "coverage_below_100",
      "message": "New code coverage: 85%",
      "location": "source/common/foo.cc",
      "suggestion": "Add tests for lines: 45, 67, 89-92",
      "uncovered_lines": [45, 67, 89, 90, 91, 92]
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "coverage_data": {
    "new_lines": 150,
    "covered_lines": 127,
    "percentage": 84.67
  }
}
```

---

## Execution

### For SEMI Mode:

1. Get list of modified files in `source/`:
```bash
git diff --name-only <base>...HEAD | grep '^source/.*\.cc$'
```

2. For each file, search for corresponding test

3. Analyze new functions in diff

4. Calculate confidence

5. Generate report

### For FULL Mode:

1. Show warning and confirm with user

2. Verify ENVOY_DOCKER_BUILD_DIR

3. Execute coverage build

4. Analyze report

5. Generate report with uncovered lines

## Notes

- SEMI mode is sufficient for most PRs
- FULL mode should only be used when there are doubts or for critical PRs
- GitHub CI will run coverage automatically
