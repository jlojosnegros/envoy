# Sub-agent: Report Generator

## Purpose
Consolidate all sub-agent reports into a unified final report.

## Input
Results from all executed sub-agents in JSON format.

## Final Report Format

```markdown
# Envoy PR Pre-Review Report

**Generated**: YYYY-MM-DD HH:MM:SS
**Branch**: [current branch name]
**Base commit**: [base commit SHA]
**Head commit**: [current commit SHA]
**Commits analyzed**: [number of commits]

---

## Executive Summary

| Category | Errors | Warnings | Info |
|----------|:------:|:--------:|:----:|
| PR Metadata | X | Y | Z |
| Dev Environment | X | Y | Z |
| Code Format | X | Y | Z |
| Code Lint | X | Y | Z |
| Code Expert | X | Y | Z |
| Security Audit | X | Y | Z |
| Unit Tests | X | Y | Z |
| Test Coverage | X | Y | Z |
| Docs/Changelog | X | Y | Z |
| API Review | X | Y | Z |
| Dependencies | X | Y | Z |
| Extensions | X | Y | Z |
| Maintainer Review | X | Y | Z |
| **TOTAL** | **X** | **Y** | **Z** |

### Overall Status

[EMOJI] **[STATUS]**

Where:
- 🔴 **BLOCKED** - There are critical errors that must be fixed
- 🟡 **NEEDS_WORK** - There are warnings that should be reviewed
- 🟢 **READY** - No errors or significant warnings

**Review Readiness Score**: [score]/100

---

## 👥 Predicted Reviewer Comments

Based on previous Envoy review patterns, these are the comments
you would likely receive from different types of maintainers:

### 🎯 Performance-Focused Reviewer ([N] comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| [location] | [predicted comment] | [suggested fix] |

### 📐 Style-Focused Reviewer ([N] comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| [location] | [predicted comment] | [suggested fix] |

### 🔒 Security-Focused Reviewer ([N] comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| [location] | [predicted comment] | [suggested fix] |

### 🏗️ Architecture-Focused Reviewer ([N] comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| [location] | [predicted comment] | [suggested fix] |

### 🧪 Testing-Focused Reviewer ([N] comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| [location] | [predicted comment] | [suggested fix] |

**Estimated review time**: ~[X] minutes

---

## Analyzed Files

```
[List of modified files grouped by type]

Source files (N):
  - source/common/foo.cc
  - source/common/bar.cc

Test files (N):
  - test/common/foo_test.cc

API files (N):
  - api/envoy/config/foo.proto

Documentation (N):
  - docs/root/intro/foo.rst
  - changelogs/current.yaml
```

---

## Detailed Findings

### 🔴 Errors (Must be fixed)

These issues BLOCK PR merge:

#### [E001] [Category] Error title
- **Location**: `file:line`
- **Description**: Detailed problem description
- **Suggestion**: How to fix it

```
[Relevant code or diff if applicable]
```

---

#### [E002] ...

---

### 🟡 Warnings (Should be fixed)

These issues don't block but should be reviewed:

#### [W001] [Category] Warning title
- **Location**: `file:line`
- **Description**: Problem description
- **Suggestion**: How to fix it

---

### 🔵 Info (Optional improvements)

Improvement suggestions that are not mandatory:

#### [I001] [Category] Title
- **Location**: `file:line`
- **Description**: Description
- **Suggestion**: Suggestion

---

## Fix Commands

### Automatic Fixes

```bash
# C++ code formatting
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'

# Proto formatting
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh fix_proto_format'

# Add DCO sign-off to commits
git commit --amend -s
# For multiple commits:
git rebase -i HEAD~N  # and add -s to each one

# Run specific tests locally
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel test //test/path/to:test'

# Verify dependencies
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate'
```

### Release Notes Template

If release notes are missing, add in `changelogs/current.yaml`:
```yaml
# For new feature:
new_features:
- area: <subsystem>
  change: |
    Added <feature description>.

# For bug fix:
bug_fixes:
- area: <subsystem>
  change: |
    Fixed <bug description>.

# For behavior change:
behavior_changes:
- area: <subsystem>
  change: |
    Changed <change description>. This can be reverted by setting
    runtime guard ``envoy.reloadable_features.<flag>`` to false.
```

### Required Manual Fixes

1. [ ] Fix [E001]: [brief description]
2. [ ] Fix [E002]: [brief description]
3. [ ] Review [W001]: [brief description]

---

## Checks Not Executed

[If any sub-agent wasn't executed, list it here with reason]

| Check | Reason | How to execute |
|-------|--------|----------------|
| clang-tidy | Requires --full-lint | `/envoy-review --full-lint` |
| coverage (full) | Requires --coverage-full | `/envoy-review --coverage-full` |
| deep-analysis | Requires --deep-analysis | `/envoy-review --deep-analysis` |
| unit-tests | Skipped with --skip-tests | `/envoy-review` (default) |
| security-deps | No dependency changes | Automatic if changes in bazel/ |

---

## Next Steps

1. [ ] Fix all errors listed above
2. [ ] Review and fix applicable warnings
3. [ ] Run tests locally: `bazel test //test/...`
4. [ ] Verify CI passes
5. [ ] Create/update PR

---

## Additional Information

### Unit Tests
| Metric | Value |
|--------|-------|
| Tests executed | X |
| Passed | X |
| Failed | X |
| Timeout | X |
| Duration | Xm Xs |

### Code Expert Analysis
| Metric | Value |
|--------|-------|
| Files analyzed | X |
| Average confidence | X% |
| Categories detected | memory, buffer, threading |

### Security Audit
| Metric | Value |
|--------|-------|
| Dependencies verified | X |
| CVEs found | X |
| Maximum severity | critical/high/medium/low |

### Estimated Coverage (Semi Mode)
- **Confidence**: X%
- **Files without apparent test**: [list]

### Maintainer Review
| Metric | Value |
|--------|-------|
| Predicted comments | X |
| Review Readiness Score | X/100 |
| Estimated review time | X minutes |
| By reviewer: Performance | X |
| By reviewer: Style | X |
| By reviewer: Security | X |
| By reviewer: Architecture | X |
| By reviewer: Testing | X |

### Execution Logs
Detailed logs are in:
```
${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/
├── YYYYMMDDHHMM-format.log
├── YYYYMMDDHHMM-unit-tests.log
├── YYYYMMDDHHMM-security-deps.log
└── ...
```

---

*Report generated by Envoy PR Pre-Review Agent*
*For more information: /envoy-review --help*
```

## Generation Logic

### 1. Determine Overall Status

```python
def determine_status(findings):
    total_errors = sum(f['errors'] for f in findings)
    total_warnings = sum(f['warnings'] for f in findings)

    if total_errors > 0:
        return "BLOCKED", "🔴"
    elif total_warnings > 0:
        return "NEEDS_WORK", "🟡"
    else:
        return "READY", "🟢"
```

### 2. Assign IDs to Findings

```
Errors: E001, E002, E003, ...
Warnings: W001, W002, W003, ...
Info: I001, I002, I003, ...
```

### 3. Group by Category

Group findings by generating agent:
- pr-metadata → "PR Metadata"
- dev-env → "Dev Environment"
- code-format → "Code Format"
- code-lint → "Code Lint"
- code-expert → "Code Expert"
- security-audit → "Security Audit"
- unit-tests → "Unit Tests"
- test-coverage → "Test Coverage"
- docs-changelog → "Docs/Changelog"
- api-review → "API Review"
- deps-check → "Dependencies"
- extension-review → "Extensions"
- maintainer-review → "Maintainer Review"

### 4. Generate Fix Commands

Include specific commands based on errors found:
- If there are format errors → format command
- If there are proto format errors → fix_proto_format command
- If release notes are missing → entry template

## Report Saving

If --save-report is active:

```bash
REPORT_FILE="${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-report.md"
```

Also show a short summary in console:

```
════════════════════════════════════════════════════════════════
                    ENVOY PR PRE-REVIEW SUMMARY
════════════════════════════════════════════════════════════════

Status: 🟡 NEEDS_WORK

Errors:   2  (must fix before PR)
Warnings: 5  (should review)
Info:     3  (optional improvements)

Top Issues:
  [E001] DCO sign-off missing in commit abc1234
  [E002] Release notes not updated for user-facing change
  [W001] clang-format: 3 files need formatting

Full report: ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-report.md

To fix formatting automatically:
  ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh format'

════════════════════════════════════════════════════════════════
```

## Notes

- Report is always shown in console (summary)
- Complete report is saved to file if --save-report
- Use colors/emojis for better terminal readability
- IDs allow easy reference in discussions
