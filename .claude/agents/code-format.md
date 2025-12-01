# Sub-agent: Code Format Check

## Purpose
Verify that code meets Envoy formatting standards using clang-format and other tools.

## ACTION: ALWAYS EXECUTE (takes 2-5 minutes)
This command is fast. DO NOT use heuristics - execute the command directly.

## Requires Docker: YES

## Main CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-format.log
```

## Verifications

### 1. C++ Format (clang-format) - ERROR
Verifies that all C++ code is formatted according to `.clang-format`.

**Affected files:** `*.cc`, `*.h`, `*.cpp`, `*.hpp`

**If there are errors:**
```
ERROR: C++ code not formatted correctly
Affected files: [list]
Suggestion: Run automatic fix:
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'
```

### 2. Proto Format - ERROR
Verifies protobuf file format.

**Affected files:** `*.proto` in `api/`

**Specific command:**
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh check_proto_format'
```

**Fix:**
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh fix_proto_format'
```

### 3. Python Format - WARNING
Verifies Python script format.

**Affected files:** `*.py`

### 4. Bash Format - WARNING
Verifies Bash script format.

**Affected files:** `*.sh`

### 5. BUILD Files Format - WARNING
Verifies Bazel file format.

**Affected files:** `BUILD`, `*.bzl`, `BUILD.bazel`

## Output Parsing

The `format` command produces output indicating files with problems.

**Patterns to look for:**
- `ERROR:` - Format errors
- `would reformat` - Files that need reformatting
- `--- a/` and `+++ b/` - Diff of needed changes

## Pre-check Without Docker

Before running Docker, a quick check can be done:

```bash
# Check if there are pending local format changes
git diff --name-only | grep -E '\.(cc|h|cpp|hpp)$'
```

## Output Format

```json
{
  "agent": "code-format",
  "requires_docker": true,
  "docker_executed": true|false,
  "findings": [
    {
      "type": "ERROR",
      "check": "clang_format",
      "message": "File not formatted correctly",
      "location": "source/common/foo.cc",
      "suggestion": "Run: ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "fix_command": "ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'"
}
```

## Execution

1. **Verify ENVOY_DOCKER_BUILD_DIR** - If not defined, don't execute and report

2. **Create logs directory:**
```bash
mkdir -p ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs
```

3. **Generate timestamp:**
```bash
TIMESTAMP=$(date +%Y%m%d%H%M)
```

4. **Show command to user before executing:**
```
Running format verification...
Command: ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'
Log: ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-format.log
```

5. **Execute command**

6. **Parse output and generate report**

## Notes

- The `format` command may take several minutes
- If there are errors, exit code will be != 0
- Complete log will be available for debugging
