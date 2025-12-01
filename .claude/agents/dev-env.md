# Sub-agent: Development Environment Check

## Purpose
Verify that the development environment is correctly configured according to Envoy instructions.

## ACTION: ALWAYS EXECUTE (instant ls/cat commands, no Docker)

## Verifications

### 1. Git Hooks Installed (ERROR)
Verify that git hooks installed by `./support/bootstrap` exist:

**Files to verify:**
- `.git/hooks/pre-commit` - Must exist and be executable
- `.git/hooks/pre-push` - Must exist and be executable
- `.git/hooks/commit-msg` - Must exist and be executable

**Command:**
```bash
ls -la .git/hooks/pre-commit .git/hooks/pre-push .git/hooks/commit-msg 2>/dev/null
```

**If they don't exist:**
```
ERROR: Git hooks not installed.
Suggestion: Run ./support/bootstrap from project root
```

### 2. Bootstrap Executed (WARNING)
Verify indicators that bootstrap was executed:
- Hooks exist
- They are symlinks or copies from `support/hooks/`

**Verify content:**
```bash
head -5 .git/hooks/pre-commit
```
Should contain reference to Envoy scripts.

### 3. .env File (INFO)
If `.env` exists, check if it contains `NO_VERIFY=1`:

```bash
grep -q "NO_VERIFY" .env 2>/dev/null && echo "WARNING"
```

**If NO_VERIFY is active:**
```
INFO: NO_VERIFY is configured in .env
This disables pre-commit/pre-push checks.
Make sure to run checks manually before PR.
```

### 4. Development Tools (INFO)
Optional common tool verifications:
- clang-format available
- bazel available

**Note:** These are not errors because Docker commands include the tools.

## Output Format

```json
{
  "agent": "dev-env",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "git_hooks",
      "message": "Git hooks not installed",
      "location": ".git/hooks/",
      "suggestion": "Run: ./support/bootstrap"
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

1. Verify hooks existence:
```bash
for hook in pre-commit pre-push commit-msg; do
  if [ -f ".git/hooks/$hook" ]; then
    echo "$hook: OK"
  else
    echo "$hook: MISSING"
  fi
done
```

2. Verify .env:
```bash
if [ -f ".env" ]; then
  cat .env
fi
```

3. Generate report
