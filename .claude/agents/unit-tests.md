# Sub-agent: Unit Tests Runner

## Purpose
Execute unit test suites impacted by new/modified code and generate a detailed results report, including explanations for failing tests.

## ACTION:
- **Default**: EXECUTE if there are changes in source/ (requires Docker)
- **Skip**: With flag `--skip-tests`

## Requires Docker: YES

## Timeout: 30 minutes maximum

---

## Execution Flow

### Step 1: Identify modified files

```bash
# Get modified source files
git diff --name-only <base>...HEAD | grep '^source/.*\.(cc|h)$'
```

### Step 2: Determine impacted tests (Hybrid Approach)

#### 2.1 Corresponding directory (Fast)

For each file `source/path/to/file.cc`:
1. Search for direct test: `test/path/to/file_test.cc`
2. Search for tests in directory: `test/path/to/...`

```bash
# Example: if source/common/http/codec.cc was modified
# Search:
#   - test/common/http/codec_test.cc
#   - test/common/http/*_test.cc
```

#### 2.2 Bazel query (Complete)

Find all tests that depend on modified files:

```bash
# For each modified file, execute inside Docker:
bazel query "rdeps(//test/..., //source/path/to/file.cc)" --output=label 2>/dev/null
```

**Combine results from 2.1 and 2.2, remove duplicates.**

### Step 3: Execute tests

```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh \
  'bazel test --test_timeout=1800 --test_output=errors <test_list>' \
  2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-unit-tests.log
```

**bazel test options:**
- `--test_timeout=1800` - 30 minute timeout
- `--test_output=errors` - Only show output from failed tests
- `--test_summary=detailed` - Detailed summary

### Step 4: Parse results

#### Patterns to look for in output:

**Passed tests:**
```
//test/common/http:utility_test                                          PASSED in X.Xs
```

**Failed tests:**
```
//test/common/http:codec_test                                            FAILED in X.Xs
```

**Timed out tests:**
```
//test/common/http:slow_test                                             TIMEOUT in X.Xs
```

**Final summary:**
```
Executed N out of M tests: X tests pass, Y fail
```

#### Extract failed test logs:

```bash
# Logs are in bazel-testlogs/<test_path>/test.log
cat bazel-testlogs/test/common/http/codec_test/test.log
```

### Step 5: Analyze failures and generate explanations

For each failed test:

1. **Read test log** to obtain:
   - Exact error message
   - Stack trace
   - Failed assertion

2. **Read diff of modified code**:
   ```bash
   git diff <base>...HEAD -- <modified_file>
   ```

3. **Read the failing test code**:
   ```bash
   cat test/path/to/failing_test.cc
   ```

4. **Generate explanation** correlating:
   - What changed in the code
   - What the test expected
   - Why the change may have caused the failure

5. **Suggest solution** based on:
   - The specific error
   - The change made
   - The test expectations

---

## Output Format

```json
{
  "agent": "unit-tests",
  "execution_time_seconds": 450,
  "timeout_seconds": 1800,
  "tests_discovered": 25,
  "tests_executed": 25,
  "results": {
    "passed": 22,
    "failed": 2,
    "timeout": 1,
    "skipped": 0
  },
  "failures": [
    {
      "test_name": "//test/common/http:codec_test",
      "test_case": "Http2CodecImplTest.BasicRequest",
      "status": "FAILED",
      "error_message": "Value of: response.status()\n  Actual: 500\nExpected: 200",
      "assertion": "EXPECT_EQ(response.status(), 200)",
      "file": "test/common/http/codec_test.cc",
      "line": 234,
      "stack_trace": "test/common/http/codec_test.cc:234\nsource/common/http/codec.cc:89",
      "log_file": "bazel-testlogs/test/common/http/codec_test/test.log",
      "related_changes": [
        "source/common/http/codec.cc:45-67"
      ],
      "possible_explanation": "The change in codec.cc line 52 modified ':status' header handling. The new logic returns 500 when header is empty, but test expects default value 200.",
      "suggestion": "Review condition on line 52. Test expects backward-compatible behavior. Consider: if (status.empty()) { status = '200'; }"
    }
  ],
  "passed_tests": [
    "//test/common/http:utility_test",
    "//test/common/http:header_map_test",
    "//test/common/http:parser_test"
  ],
  "summary": {
    "status": "FAILED",
    "pass_rate": 88.0,
    "duration": "7m 30s",
    "message": "2 tests failed, 1 timeout. See details above."
  }
}
```

---

## Final Report Integration

### Report section:

```markdown
## Unit Tests

**Executed**: 25 tests
**Duration**: 7m 30s
**Result**: ❌ 2 FAILED, 1 TIMEOUT

### Failed Tests

#### [UT001] //test/common/http:codec_test - Http2CodecImplTest.BasicRequest

- **Error**: `Expected: 200, Actual: 500`
- **Location**: test/common/http/codec_test.cc:234
- **Related change**: source/common/http/codec.cc:45-67
- **Explanation**: The change in codec.cc line 52 modified ':status' header handling. The new logic returns 500 when header is empty, but test expects default value 200.
- **Suggestion**: Review condition on line 52. Consider maintaining backward-compatible behavior.

### Passed Tests (22)

<details>
<summary>See complete list</summary>

- //test/common/http:utility_test
- //test/common/http:header_map_test
- //test/common/http:parser_test
...
</details>
```

---

## Docker Commands

### Test execution:
```bash
bash -c 'TIMESTAMP=$(date +%Y%m%d%H%M) && ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh "bazel test --test_timeout=1800 --test_output=errors <tests>" 2>&1 | tee <dir>/review-agent-logs/${TIMESTAMP}-unit-tests.log'
```

### Bazel query for impacted tests:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel query "rdeps(//test/..., //source/path/to/file.cc)" --output=label'
```

---

## Special Cases Handling

### No impacted tests
If no related tests are found:
```json
{
  "agent": "unit-tests",
  "tests_discovered": 0,
  "message": "No tests related to modified files found",
  "suggestion": "Consider adding tests for new code in test/path/to/"
}
```

### Global timeout
If 30 minute timeout is exceeded:
```json
{
  "status": "TIMEOUT",
  "message": "Test execution exceeded 30 minute limit",
  "tests_completed": 15,
  "tests_pending": 10
}
```

### Compilation error
If tests don't compile:
```json
{
  "status": "BUILD_FAILED",
  "error": "Compilation error in //test/common/http:codec_test",
  "build_log": "...",
  "suggestion": "Fix compilation errors before running tests"
}
```

---

## Notes

- Tests run inside Envoy's Docker container
- 30 minute timeout is for total execution, not per individual test
- bazel test caches results, only re-runs affected tests
- Failure explanations are generated by agent analyzing code, not infallible
- If many tests impacted (>50), consider running only most relevant ones
- Complete logs are always saved in `review-agent-logs/`
