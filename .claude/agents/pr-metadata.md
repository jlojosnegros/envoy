# Sub-agent: PR Metadata Review

## Purpose
Verify that commits and PR metadata meet Envoy requirements.

## ACTION: ALWAYS EXECUTE (instant git commands, no Docker)

## Expected Input
- List of commits to analyze (from git log)
- Modified files (to calculate LOC)

## Verifications

### 1. Commit Title Format (ERROR)
The title must follow the format: `subsystem: description`
- All lowercase (except known acronyms like HTTP, TLS, etc.)
- Subsystem followed by colon and space
- Brief and descriptive description

**Valid examples:**
- `docs: fix grammar error`
- `http conn man: add new feature`
- `router: add x-envoy-overloaded header`
- `tls: add support for specifying TLS session ticket keys`

**Regex pattern:** `^[a-z][a-z0-9 /_-]*: .+$`

### 2. DCO Sign-off (ERROR)
Each commit MUST have the line:
```
Signed-off-by: Name <email@example.com>
```

**Command to verify:**
```bash
git log --format='%B' HEAD~N..HEAD | grep -c "Signed-off-by:"
```

If missing, provide instructions:
```
To add DCO to existing commits:
git commit --amend -s
# or for multiple commits:
git rebase -i HEAD~N
# and add -s to each commit
```

### 3. Commit Message (ERROR if empty)
The commit message must:
- Explain WHAT the change does and WHY
- Not be empty
- Have correct English grammar and punctuation

### 4. Risk Level (WARNING)
For PRs, verify that user has considered:
- Low: Small bug fix or small optional feature
- Medium: New features not enabled, small-medium additions to existing components
- High: Complicated changes like flow control, rewrites of critical components

### 5. Testing Documentation (WARNING)
Verify there is information about what testing was performed:
- Unit tests
- Integration tests
- Manual testing

### 6. Co-authored-by (INFO)
If there are multiple authors, verify format:
```
Co-authored-by: name <name@example.com>
```

### 7. Issue Reference (INFO for changes >100 LOC)
For larger changes (>100 lines), there should be:
- Reference to a GitHub issue
- Or link to design document

**LOC calculation:**
```bash
git diff --stat <base>...HEAD | tail -1
```

### 8. Fixes Format (INFO)
If closing an issue, verify format:
```
Fixes #XXX
```

## Output Format

```json
{
  "agent": "pr-metadata",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "check_name",
      "message": "Problem description",
      "location": "commit SHA or line",
      "suggestion": "How to fix it"
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

1. Get list of commits (from base branch):
```bash
git log --oneline <base>...HEAD
```

2. For each commit, analyze:
```bash
git log -1 --format='%s' <SHA>  # title
git log -1 --format='%B' <SHA>  # complete message
git log -1 --format='%an <%ae>' <SHA>  # author
```

3. Calculate changed lines:
```bash
git diff --shortstat <base>...HEAD
```

4. Generate report with findings
