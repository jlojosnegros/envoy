# Getting Started with Envoy Code Reviewer Agent

Quick start guide for using the Envoy Code Reviewer agent in your development workflow.

## 🚀 Quick Start (2 minutes)

### 1. Verify Installation

Check that all files are in place:

```bash
# From repository root
ls -la .claude/agents/code-reviewer.md       # ✅ Agent prompt
ls -la .claude/commands/envoy-review.md            # ✅ Slash command
ls -la scripts/envoy-review-helper.py        # ✅ Helper script
ls -la tests/ai-review-scenarios/            # ✅ Test scenarios
```

### 2. Test the Helper Script

```bash
# Make sure it's executable
chmod +x scripts/envoy-review-helper.py

# Run a quick test (on current branch vs main)
./scripts/envoy-review-helper.py --format markdown
```

Expected output:
```markdown
# 🔍 Envoy Code Review Analysis

## 📊 Summary
- **Files changed:** X
...
```

### 3. Use in Claude Code

Open Claude Code and type:

```
/envoy-review
```

The agent will analyze your changes and provide a detailed report!

---

## 📋 First Real Review

Let's do a real code review with the agent:

### Step 1: Make Some Changes

```bash
# Create a test branch
git checkout -b test-agent-review

# Make a simple change (e.g., add a comment)
echo "// Test comment" >> source/common/http/conn_manager_impl.cc

# Commit it
git add source/common/http/conn_manager_impl.cc
git commit -m "test: agent review"
```

### Step 2: Run the Review

**Option A: In Claude Code**
```
/envoy-review
```

**Option B: Command Line**
```bash
./scripts/envoy-review-helper.py --format markdown
```

### Step 3: Understand the Output

The agent will report:

```markdown
## ❌ Critical Issues
- Missing release note (if user-facing change)
- Missing test coverage (if code change)

## ⚠️ Warnings
- Style issues
- Pattern violations

## ✅ Passing Checks
- What's correct

## 📝 Action Items
- Specific fixes needed
```

### Step 4: Fix Issues

Address each critical issue:

```bash
# Example: Add missing release note
vim changelogs/current.yaml

# Re-run review
/envoy-review

# Iterate until all checks pass
```

---

## 🔄 Typical Workflow

### Daily Development Flow

```bash
# 1. Start new feature
git checkout -b feature/my-awesome-feature

# 2. Develop incrementally
vim source/extensions/filters/http/my_filter/filter.cc
git commit -m "wip: initial implementation"

# 3. Review early (catch issues early!)
/envoy-review

# 4. Fix issues
# (address feedback)

# 5. Continue development
vim source/extensions/filters/http/my_filter/filter.cc
git commit -m "feat: complete implementation"

# 6. Final review before PR
/envoy-review

# 7. Create PR when clean
git push origin feature/my-awesome-feature
gh pr create
```

### Pre-PR Checklist

Before creating a PR, ensure:

```bash
# ✅ All changes committed
git status

# ✅ Review passes
/envoy-review
# → No critical issues

# ✅ Tests pass locally
bazel test //test/...

# ✅ Format is clean
bazel run //tools/code_format:check_format -- check

# ✅ Ready to push
git push
```

---

## 🧪 Testing the Agent

### Run Automated Tests

```bash
# Run all test scenarios
./tests/ai-review-scenarios/run-all-scenarios.sh
```

This validates that the agent correctly detects:
- Missing test coverage
- Missing release notes
- Style violations

### Manual Testing Scenarios

**Scenario 1: Missing Test**
```bash
git checkout -b test-missing-test
touch source/common/http/new_file.cc
git add source/common/http/new_file.cc
git commit -m "test: file without test"
/envoy-review
# → Should detect missing test
```

**Scenario 2: Missing Release Note**
```bash
git checkout -b test-missing-note
echo "// feature" >> source/common/http/conn_manager_impl.cc
git commit -am "feat: new feature"
/envoy-review
# → Should detect missing release note
```

**Scenario 3: Everything Perfect**
```bash
git checkout -b test-perfect
# Make change with test + release note
/envoy-review
# → Should pass all checks ✅
```

---

## 💡 Tips & Tricks

### Tip 1: Review Early and Often

```bash
# Don't wait until code is "done"
# Review during development to catch issues early

git commit -m "wip: partial implementation"
/envoy-review  # ← Get early feedback
```

### Tip 2: Use Helper Script for Quick Checks

```bash
# Quick check without opening Claude Code
./scripts/envoy-review-helper.py --format text

# Save report for later
./scripts/envoy-review-helper.py --format markdown > review.md
```

### Tip 3: Focus on Critical Issues First

```markdown
Agent output priorities:
1. ❌ Critical Issues → Must fix before merge
2. ⚠️  Warnings → Should fix (improves quality)
3. 💡 Suggestions → Nice to have (optional)
```

### Tip 4: Learn Envoy Standards

```bash
# Use agent to learn Envoy policies
/envoy-review

# Read the "why" behind each issue
# Apply learnings to future code
# Become a better Envoy contributor!
```

### Tip 5: Iterate Quickly

```bash
# Fast iteration loop:
/envoy-review  # Find issue
# Fix it
git commit --amend --no-edit
/envoy-review  # Verify fix
# Repeat
```

---

## 🔧 Customization

### Change Base Branch

Edit `.claude/commands/envoy-review.md`:

```markdown
# Default: compares to 'main'
# Change to compare to different branch:

Please analyze changes compared to `develop` branch...
```

### Adjust Strictness

Edit `.claude/agents/code-reviewer.md`:

```markdown
# Make warnings into critical issues:
**Critical:** Shared pointer usage (was: Warning)

# Or relax requirements for certain files:
**Skip coverage check for:** test utilities, mocks
```

### Add Project-Specific Checks

Add to `.claude/agents/code-reviewer.md`:

```markdown
### X. Custom Project Check

**Check for:**
- Your specific pattern

**How to verify:**
```bash
# Your verification command
```

**Report as:**
- Critical/Warning/Suggestion
```

---

## 🐛 Troubleshooting

### Issue: "/envoy-review command not found"

**Solution:**
```bash
# Verify file exists
ls -la .claude/commands/envoy-review.md

# Check file permissions
chmod 644 .claude/commands/envoy-review.md
```

### Issue: "No changes detected"

**Solution:**
```bash
# Changes must be committed
git add .
git commit -m "your message"

# Then run review
/envoy-review
```

### Issue: "Helper script fails"

**Solution:**
```bash
# Check Python version (need 3.7+)
python3 --version

# Make executable
chmod +x scripts/envoy-review-helper.py

# Test directly
python3 scripts/envoy-review-helper.py --verbose
```

### Issue: "Agent gives wrong advice"

**Solution:**
- Agent isn't perfect - use human judgment
- Check against official docs (CONTRIBUTING.md, STYLE.md)
- Ask maintainers if unsure
- File issue to improve agent prompt

---

## 📊 Success Metrics

Track your improvement:

### Before Agent
```
Average issues found in human review: 5-10
Time to first review: 1-2 days
Iterations to merge: 3-5
```

### After Agent
```
Average issues found in human review: 0-2
Time to first review: < 1 hour (automated)
Iterations to merge: 1-2
```

### Quality Improvements
- ✅ Zero submissions with missing tests
- ✅ Zero submissions with missing release notes
- ✅ Faster review cycles
- ✅ Higher quality first submissions

---

## 🎓 Learning Path

### Week 1: Getting Comfortable
- Run `/envoy-review` on every commit
- Understand each issue type
- Learn to fix common issues

### Week 2: Anticipating Issues
- Recognize patterns before agent does
- Write tests before implementation
- Add release notes proactively

### Week 3: Mastery
- Clean reviews first try
- Help others use agent
- Suggest agent improvements

---

## 📚 Additional Resources

### Documentation
- [Full README](./.claude/agents/README.md) - Complete documentation
- [Usage Examples](./.claude/agents/USAGE_EXAMPLES.md) - Detailed examples
- [CLAUDE.md](../../CLAUDE.md) - Envoy development guide

### Envoy Resources
- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Contribution guidelines
- [STYLE.md](../../STYLE.md) - Code style guide
- [bazel/README.md](../../bazel/README.md) - Build system

### Getting Help
- Check documentation first
- Run test scenarios to understand behavior
- Ask in Envoy Slack #development
- File issues for bugs or improvements

---

## ✅ Checklist: Ready to Use

Verify you're ready:

- [ ] All agent files installed
- [ ] Helper script runs successfully
- [ ] `/envoy-review` command works in Claude Code
- [ ] Test scenarios pass
- [ ] Understand output format
- [ ] Know how to fix common issues

**Congratulations!** You're ready to use the Envoy Code Reviewer agent! 🎉

Start with: `/envoy-review` on your next commit!

---

**Next:** Check out [Usage Examples](./USAGE_EXAMPLES.md) for detailed scenarios and workflows.
