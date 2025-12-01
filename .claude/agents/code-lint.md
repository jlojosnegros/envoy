# Sub-agent: Code Lint Check (clang-tidy)

## Purpose
Run static analysis with clang-tidy and verify inclusive language.

## ACTION:
- **Inclusive Language**: ALWAYS EXECUTE (< 1 second, no Docker)
- **Full clang-tidy**: Only with --full-lint flag (takes hours)

## Requires Docker: YES (for full clang-tidy)

## Main CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh clang-tidy' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-clang-tidy.log
```

**WARNING:** This command takes a LONG time (can be hours). Recommended only for final reviews.

## Verifications

### 1. clang-tidy Errors - ERROR
Errors detected by clang-tidy according to `.clang-tidy`.

**Patterns in output:**
- `error:` - clang-tidy error
- `warning:` - Warning (may be ERROR depending on configuration)

### 2. clang-tidy Warnings - WARNING
Warnings that don't block but should be reviewed.

### 3. Inclusive Language - ERROR (No Docker)
**This verification does NOT require Docker and MUST always be executed.**

Search for prohibited terms in modified files:

```bash
git diff --name-only <base>...HEAD | grep -v '^\.claude/' | xargs grep -n -E -i '\b(whitelist|blacklist|master|slave)\b' 2>/dev/null
```

**NOTE**: `<base>` is the base branch determined in step 2 of the main agent (default: main).

**Prohibited terms and replacements:**
| Prohibited | Replacement |
|------------|-------------|
| whitelist | allowlist |
| blacklist | denylist, blocklist |
| master | primary, main |
| slave | secondary, replica |

**Exceptions:**
- References to external git branches (e.g.: `upstream/master`)
- Textual quotes from external documentation
- External API names we don't control
- Files in `.claude/` (review agent documentation)

**If found:**
```
ERROR: Use of non-inclusive language
Location: source/common/foo.cc:123
Text: "whitelist"
Suggestion: Replace with "allowlist"
```

## Quick Verification Without Docker

For quick verification without full Docker:

### Inclusive Language (ALWAYS execute):
```bash
# Search in modified files (excluding .claude/)
# <base> is the base branch determined in envoy-review.md (default: main)
for file in $(git diff --name-only <base>...HEAD | grep -v '^\.claude/'); do
  grep -n -E -i '\b(whitelist|blacklist|master|slave)\b' "$file" 2>/dev/null
done
```

### Basic code verification (optional):
- Search for known problematic patterns
- `memcpy` without bounds checking
- `printf` with unsafe format
- Obvious uninitialized variables

## Output Format

```json
{
  "agent": "code-lint",
  "requires_docker": true,
  "docker_executed": true|false,
  "inclusive_language_checked": true,
  "findings": [
    {
      "type": "ERROR",
      "check": "inclusive_language",
      "message": "Use of prohibited term 'whitelist'",
      "location": "source/common/foo.cc:123",
      "suggestion": "Replace 'whitelist' with 'allowlist'"
    },
    {
      "type": "ERROR",
      "check": "clang-tidy",
      "message": "Uninitialized variable",
      "location": "source/common/bar.cc:456",
      "suggestion": "Initialize variable before use"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  }
}
```

## Execution

### Phase 1: Inclusive Language (No Docker - ALWAYS)

1. Get modified files (using base branch from envoy-review.md):
```bash
git diff --name-only <base>...HEAD | grep -v '^\.claude/'
```

2. Search for prohibited terms in each file

3. Report found ones as ERROR

### Phase 2: clang-tidy (With Docker - OPTIONAL)

**Because clang-tidy takes a long time, ask the user:**
```
clang-tidy is a slow process (may take hours).
Do you want to run it now? (y/n)
Alternative: It will run automatically in CI when creating the PR.
```

If user confirms:

1. Verify ENVOY_DOCKER_BUILD_DIR

2. Create logs directory

3. Show command and execute

4. Parse output and report

## Notes

- Inclusive language verification is fast and MUST always be executed
- Full clang-tidy is very slow, consider as optional
- clang-tidy errors in `.clang-tidy` are enforced, warnings are suggestions
