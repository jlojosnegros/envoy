# Envoy PR Pre-Review Agent

An AI-powered code review agent for [Envoy Proxy](https://github.com/envoyproxy/envoy) that helps developers identify potential issues before submitting Pull Requests. The agent simulates expert maintainer perspectives and runs comprehensive checks to ensure code meets all project requirements.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Sub-Agents](#sub-agents)
  - [Without Docker](#without-docker-sub-agents)
  - [With Docker](#with-docker-sub-agents)
- [Example Execution](#example-execution)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

The Envoy PR Pre-Review Agent is a Claude Code slash command that performs comprehensive pre-submission code review for Envoy contributions. It helps developers:

- **Catch issues early** - Identify problems before CI runs
- **Reduce review friction** - Predict and fix common reviewer comments
- **Ensure compliance** - Verify DCO, formatting, and documentation requirements
- **Save time** - Get instant feedback instead of waiting for CI pipelines

The agent operates in two modes:
1. **Without Docker** - Fast heuristic checks (seconds to minutes)
2. **With Docker** - Full CI-equivalent checks using Envoy's official tooling

---

## Features

| Feature | Description |
|---------|-------------|
| **14 Specialized Sub-Agents** | Each focused on a specific aspect of code quality |
| **Maintainer Review Prediction** | Simulates 5 different reviewer personas |
| **Review Readiness Score** | 0-100 score indicating PR readiness |
| **Actionable Reports** | Every issue includes location and suggested fix |
| **Parallel Execution** | Non-Docker checks run concurrently |
| **Incremental Analysis** | Only checks files changed since base branch |

---

## Installation

### Prerequisites

- [Claude Code](https://claude.ai/claude-code) CLI installed
- Git repository with Envoy source code
- (Optional) Docker for full CI checks

### Setup

1. **Clone this repository** or copy the `.claude` directory to your Envoy repository:

```bash
# Option 1: Clone and copy
git clone https://github.com/your-org/envoy-review-agent.git
cp -r envoy-review-agent/.claude /path/to/your/envoy-repo/

# Option 2: Add as submodule
cd /path/to/your/envoy-repo
git submodule add https://github.com/your-org/envoy-review-agent.git .claude-agent
ln -s .claude-agent/.claude .claude
```

2. **Verify installation**:

```bash
cd /path/to/your/envoy-repo
claude
# Then type: /envoy-review --help
```

### Directory Structure

```
your-envoy-repo/
├── .claude/
│   ├── commands/
│   │   └── envoy-review.md      # Main slash command
│   └── agents/
│       ├── api-review.md        # API compatibility checks
│       ├── code-expert.md       # C++ expert analysis
│       ├── code-format.md       # Formatting verification
│       ├── code-lint.md         # Static analysis
│       ├── deps-check.md        # Dependency validation
│       ├── dev-env.md           # Development environment
│       ├── docs-changelog.md    # Documentation checks
│       ├── extension-review.md  # Extension policy
│       ├── maintainer-review.md # Reviewer simulation
│       ├── pr-metadata.md       # Commit/PR metadata
│       ├── report-generator.md  # Final report
│       ├── security-audit.md    # CVE detection
│       ├── test-coverage.md     # Test coverage analysis
│       └── unit-tests.md        # Test execution
└── ... (Envoy source code)
```

---

## Usage

### Basic Commands

```bash
# Show help
/envoy-review --help

# Run all checks without Docker (fast)
/envoy-review --skip-docker

# Run all checks with Docker
/envoy-review --build-dir=/path/to/envoy-build

# Run specific checks only
/envoy-review --only=pr-metadata,inclusive-language,maintainer-review

# Compare against specific branch
/envoy-review --base=upstream/main --skip-docker
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--help` | Show help message and exit | - |
| `--base=<branch>` | Base branch for comparison | `main` |
| `--build-dir=<path>` | Docker build directory (ENVOY_DOCKER_BUILD_DIR) | Required for Docker checks |
| `--skip-docker` | Skip all Docker-based checks | `false` |
| `--skip-tests` | Skip unit test execution | `false` |
| `--full-lint` | Run complete clang-tidy analysis | `false` |
| `--deep-analysis` | Run ASAN/MSAN/TSAN sanitizers | `false` |
| `--coverage-full` | Run full coverage build | `false` |
| `--only=<checks>` | Run only specific checks (comma-separated) | All applicable |
| `--fix` | Apply automatic fixes where possible | `false` |
| `--save-report` | Save report to file | `false` |

---

## Sub-Agents

### Without Docker Sub-Agents

These checks run instantly without Docker and are always executed:

#### 1. PR Metadata (`pr-metadata`)

**Purpose**: Verify commit messages and PR metadata meet Envoy requirements.

| Aspect | Details |
|--------|---------|
| **Activates** | Always |
| **Duration** | < 1 second |
| **Docker** | No |

**Checks**:
- DCO (Developer Certificate of Origin) sign-off
- Commit title format: `subsystem: description`
- Commit message quality
- Co-authored-by format
- Issue references for large changes

**Output Example**:
```json
{
  "type": "ERROR",
  "check": "dco_signoff",
  "message": "Missing DCO sign-off",
  "location": "commit abc1234",
  "suggestion": "Run: git commit --amend -s"
}
```

---

#### 2. Development Environment (`dev-env`)

**Purpose**: Verify development environment is correctly configured.

| Aspect | Details |
|--------|---------|
| **Activates** | Always |
| **Duration** | < 1 second |
| **Docker** | No |

**Checks**:
- Git hooks installed (pre-commit, pre-push, commit-msg)
- Bootstrap script executed
- NO_VERIFY not enabled

---

#### 3. Inclusive Language (`code-lint` - Phase 1)

**Purpose**: Search for prohibited non-inclusive terms.

| Aspect | Details |
|--------|---------|
| **Activates** | Always |
| **Duration** | < 1 second |
| **Docker** | No |

**Prohibited Terms**:
| Prohibited | Replacement |
|------------|-------------|
| whitelist | allowlist |
| blacklist | denylist, blocklist |
| master | primary, main |
| slave | secondary, replica |

---

#### 4. Documentation & Changelog (`docs-changelog`)

**Purpose**: Verify release notes and documentation for user-facing changes.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/` or `api/` changes |
| **Duration** | < 1 second |
| **Docker** | No |

**Checks**:
- Release notes in `changelogs/current.yaml` for user-facing changes
- Correct YAML format
- Runtime guard documentation
- Breaking change documentation

---

#### 5. Extension Review (`extension-review`)

**Purpose**: Verify extension changes comply with Envoy's extension policy.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/extensions/` or `contrib/` changes |
| **Duration** | < 1 second |
| **Docker** | No |

**Checks**:
- CODEOWNERS entry for new extensions
- `security_posture` tag in BUILD file
- `status` tag (stable/alpha/wip)
- Sponsor requirement for new extensions
- Platform-specific code warnings

---

#### 6. Test Coverage (`test-coverage` - Semi Mode)

**Purpose**: Heuristically estimate test coverage for new code.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/` changes |
| **Duration** | 1-5 seconds |
| **Docker** | No (semi mode) |

**Checks**:
- Matching test file exists for new source files
- New public functions have test references
- Test-to-code ratio analysis

**Output**:
- Confidence percentage (0-100%)
- List of potentially untested files/functions

---

#### 7. Code Expert (`code-expert` - Heuristic Mode)

**Purpose**: Expert C++ analysis for memory, security, and pattern issues.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/` changes |
| **Duration** | 5-30 seconds |
| **Docker** | No (heuristic mode) |

**Categories Detected**:
| Category | Examples |
|----------|----------|
| Memory | `new` without smart pointer, `malloc`/`free` |
| Buffer | `memcpy` without size check, `strcpy`/`sprintf` |
| Null Pointer | Dereferencing without null check |
| Threading | Shared state without mutex, mutable statics |
| Envoy-Specific | Deprecated APIs, missing runtime guards |
| Integer Safety | size_t to int casts, overflow risks |

**Confidence Threshold**: Only reports findings with ≥70% confidence.

---

#### 8. Security Audit (`security-audit` - Phase 1)

**Purpose**: Detect CVEs in dependencies and insecure code patterns.

| Aspect | Details |
|--------|---------|
| **Activates** | Always |
| **Duration** | 5-30 seconds (API queries) |
| **Docker** | No (Phase 1) |

**Data Sources**:
- OSV (Open Source Vulnerabilities) API
- GitHub Advisory Database
- NVD (National Vulnerability Database)

**Checks**:
- Known CVEs in dependencies
- Deprecated cryptographic functions (MD5, SHA1 for security)
- Hardcoded secrets/credentials
- Insecure random number generation

---

#### 9. Maintainer Review Predictor (`maintainer-review`)

**Purpose**: Simulate expert maintainer perspectives and predict review comments.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/` or `api/` changes |
| **Duration** | 10-60 seconds |
| **Docker** | No |

**Reviewer Personas**:

| Persona | Focus | Example Comments |
|---------|-------|------------------|
| 🎯 Performance | Allocations, hot paths, latency | "Consider string_view to avoid copy" |
| 📐 Style | Naming, const, formatting | "Function names should be camelCase" |
| 🔒 Security | Validation, bounds checking | "Add bounds check before buffer access" |
| 🏗️ Architecture | Patterns, extensibility | "New behavior needs runtime guard" |
| 🧪 Testing | Coverage, edge cases | "New functionality needs unit tests" |

**Output Metrics**:
- **Review Readiness Score**: 0-100
- **Estimated Review Time**: Based on predicted comments
- **Top Issues**: Prioritized list of likely comments

---

### With Docker Sub-Agents

These checks require Docker and use Envoy's official CI tooling:

#### 10. Code Format (`code-format`)

**Purpose**: Verify code formatting with clang-format.

| Aspect | Details |
|--------|---------|
| **Activates** | When code changes |
| **Duration** | 2-5 minutes |
| **Docker** | Yes |
| **CI Command** | `./ci/do_ci.sh format` |

**Checks**:
- C++ formatting (`.clang-format`)
- Proto formatting
- Python formatting
- Bash script formatting
- BUILD file formatting

---

#### 11. API Compatibility (`api-review`)

**Purpose**: Detect breaking changes in API definitions.

| Aspect | Details |
|--------|---------|
| **Activates** | When `api/` changes |
| **Duration** | 5-15 minutes |
| **Docker** | Yes |
| **CI Command** | `./ci/do_ci.sh api_compat` |

**Checks**:
- Field renumbering
- Type changes
- Wire format compatibility
- Validation rules
- Deprecation documentation

---

#### 12. Dependencies (`deps-check`)

**Purpose**: Validate dependency changes comply with policy.

| Aspect | Details |
|--------|---------|
| **Activates** | When `bazel/` or BUILD changes |
| **Duration** | 5-15 minutes |
| **Docker** | Yes |
| **CI Command** | `./ci/do_ci.sh deps` |

**Checks**:
- License compatibility (Apache 2.0, MIT, BSD)
- CVE/vulnerability scanning
- Version pinning
- Transitive dependency analysis

---

#### 13. Unit Tests (`unit-tests`)

**Purpose**: Execute impacted unit tests and analyze failures.

| Aspect | Details |
|--------|---------|
| **Activates** | When `source/` changes (unless `--skip-tests`) |
| **Duration** | 5-30 minutes |
| **Docker** | Yes |
| **Timeout** | 30 minutes |

**Features**:
- Automatic test discovery via bazel query
- Failure analysis with explanations
- Suggested fixes for failing tests
- Pass rate calculation

---

#### 14. Code Lint (`code-lint` - Full Mode)

**Purpose**: Complete static analysis with clang-tidy.

| Aspect | Details |
|--------|---------|
| **Activates** | Only with `--full-lint` flag |
| **Duration** | ~30 minutes |
| **Docker** | Yes |
| **CI Command** | `./ci/do_ci.sh clang-tidy` |

---

### Special Flags

#### Deep Analysis (`--deep-analysis`)

Runs memory and thread sanitizers:

| Sanitizer | Purpose | Duration |
|-----------|---------|----------|
| ASAN | Address Sanitizer - memory errors | Hours |
| MSAN | Memory Sanitizer - uninitialized reads | Hours |
| TSAN | Thread Sanitizer - race conditions | Hours |

#### Full Coverage (`--coverage-full`)

Runs complete coverage build (~1+ hour).

---

## Example Execution

### Running the Agent

```bash
$ claude
> /envoy-review --skip-docker --base=main
```

### Example Output

```
[1/9] ⏳ Analyzing PR metadata...
[1/9] ✅ PR metadata: 0 errors, 1 warning

[2/9] ⏳ Checking development environment...
[2/9] ✅ Dev environment: OK

[3/9] ⏳ Searching for prohibited terms...
[3/9] ✅ Inclusive language: OK

[4/9] ⏳ Checking documentation and changelog...
[4/9] ⚠️ Docs/Changelog: 1 warning

[5/9] ⏳ Analyzing extension policy...
[5/9] ❌ N/A (no extension changes)

[6/9] ⏳ Estimating test coverage...
[6/9] ✅ Test coverage: 85% confidence

[7/9] ⏳ Running C++ expert analysis...
[7/9] ⚠️ Code expert: 2 warnings, 1 info

[8/9] ⏳ Querying CVE databases...
[8/9] ✅ Security audit: OK

[9/9] ⏳ Predicting reviewer comments...
[9/9] 📊 Maintainer review: 12 predicted comments

📊 Generating final report...

════════════════════════════════════════════════════════════════
                 ENVOY PR PRE-REVIEW REPORT
════════════════════════════════════════════════════════════════

Branch: feature/add-new-filter
Base: main
Commits: 3

## Executive Summary

| Category          | Errors | Warnings | Info |
|-------------------|:------:|:--------:|:----:|
| PR Metadata       |   0    |    1     |  0   |
| Docs/Changelog    |   0    |    1     |  0   |
| Code Expert       |   0    |    2     |  1   |
| Maintainer Review |   0    |    8     |  4   |
| **TOTAL**         | **0**  |  **12**  |**5** |

**Status**: 🟡 NEEDS_WORK
**Review Readiness Score**: 72/100
**Estimated Review Time**: ~35 minutes

## 👥 Predicted Reviewer Comments

### 🎯 Performance-Focused (3 comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| filter.cc:45 | String copy in request path | Use absl::string_view |
| filter.cc:89 | Allocation per request | Consider object pool |

### 📐 Style-Focused (4 comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| filter.h:23 | Missing const on method | Add const qualifier |
| filter.cc:67 | Line exceeds 100 chars | Wrap line |

### 🧪 Testing-Focused (2 comments)
| File:Line | Comment | Suggestion |
|-----------|---------|------------|
| filter.cc:45-120 | New class needs tests | Create filter_test.cc |

## Detailed Findings

### 🟡 Warnings

#### [W001] PR Metadata - Commit message format
- **Location**: commit abc1234
- **Description**: Commit title should be lowercase
- **Suggestion**: Use `filter: add new capability` instead of `Filter: Add new capability`

#### [W002] Docs/Changelog - Missing release notes
- **Location**: changelogs/current.yaml
- **Description**: User-facing change needs release notes
- **Suggestion**: Add entry in `new_features` section

## Next Steps

1. [ ] Fix commit message format
2. [ ] Add release notes entry
3. [ ] Consider performance suggestions
4. [ ] Add unit tests for new code
5. [ ] Run with Docker: `/envoy-review --build-dir=/path/to/build`

════════════════════════════════════════════════════════════════
```

---

## Configuration

### Environment Variables

| Variable | Description |
|----------|-------------|
| `ENVOY_DOCKER_BUILD_DIR` | Directory for Docker builds (required for Docker checks) |

### Customizing Checks

Edit the sub-agent files in `.claude/agents/` to:
- Adjust confidence thresholds
- Add custom patterns
- Modify output formats

---

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "Docker command not found" | Docker not installed | Install Docker or use `--skip-docker` |
| "Permission denied" | User not in docker group | Run `sudo usermod -aG docker $USER` |
| "Cannot connect to Docker daemon" | Docker not running | Start Docker service |
| "buf.build: connection refused" | API unavailable | Retry later or use `--skip-docker` |
| "No targets found" | Incorrect test paths | Verify test file locations |

### Logs

Docker command logs are saved to:
```
${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/
├── YYYYMMDDHHMM-format.log
├── YYYYMMDDHHMM-api_compat.log
├── YYYYMMDDHHMM-unit-tests.log
└── ...
```

---

## Contributing

Contributions are welcome! Please ensure your changes:
1. Follow Envoy's contribution guidelines
2. Include appropriate tests
3. Update documentation as needed

---

## License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- [Envoy Proxy](https://www.envoyproxy.io/) - The cloud-native edge and service proxy
- [Claude Code](https://claude.ai/claude-code) - AI-powered development assistant
- Envoy maintainers and contributors for their review guidelines and CI tooling
