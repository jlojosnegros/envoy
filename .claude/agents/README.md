# Envoy Code Reviewer Agent

An AI-powered code review agent specifically designed for the Envoy proxy project. This agent provides automated, comprehensive code reviews following Envoy's strict development standards and policies.

## 🎯 Overview

The Envoy Code Reviewer agent analyzes pull requests and code changes to ensure compliance with:

- **100% test coverage requirement**
- **Release note policies**
- **Code style standards** (STYLE.md)
- **Breaking change & deprecation policies**
- **Build system requirements**
- **Documentation standards**
- **Envoy-specific patterns** (thread safety, time handling, smart pointers, etc.)

## 🚀 Quick Start

### Using the Slash Command

The easiest way to invoke the agent:

```
# Compare against main branch (default)
/envoy-review

# Compare against a specific branch
/envoy-review develop
/envoy-review upstream/main
```

This will analyze all changes in your current branch compared to the specified base branch (default: `main`) and provide a comprehensive review report.

### Direct Invocation

You can also invoke the agent directly:

```
User: "Please review my code changes for the new rate limit filter"
```

The agent will automatically detect this as a code review request and proceed with analysis.

### Using the Helper Script

For command-line usage outside Claude Code:

```bash
# Basic usage
./scripts/envoy-review-helper.py

# Specify repository and base branch
./scripts/envoy-review-helper.py --repo /path/to/envoy --base main

# Get markdown report
./scripts/envoy-review-helper.py --format markdown

# Verbose output
./scripts/envoy-review-helper.py --verbose
```

## 📋 What Gets Checked

### Critical Checks (Must Pass)

1. **Test Coverage** ✅
   - Every source file has corresponding `*_test.cc`
   - All code paths tested (including errors)
   - Integration tests for user-facing changes

2. **Release Notes** ✅
   - `changelogs/current.yaml` updated for user-facing changes
   - Proper categorization (new_features, bug_fixes, deprecated, etc.)

3. **Build System** ✅
   - BUILD files updated correctly
   - Extensions registered in `extensions_build_config.bzl`
   - Dependencies properly declared

4. **Breaking Changes** ✅
   - Deprecation policy followed
   - Runtime guards for behavioral changes
   - Migration path documented

### Style & Pattern Checks

- ✅ Naming conventions (camelCase functions, _postfix members)
- ✅ Smart pointer usage (prefer `unique_ptr` over `shared_ptr`)
- ✅ Error handling patterns (ASSERT vs RELEASE_ASSERT vs ENVOY_BUG)
- ✅ Thread safety annotations (ABSL_GUARDED_BY)
- ✅ Time handling (use TimeSystem, not direct time())
- ✅ clang-format compliance

### Documentation Checks

- ✅ API documentation in .proto files
- ✅ User documentation for features
- ✅ Code comments for complex logic
- ✅ Migration guides for breaking changes

## 📊 Example Output

```markdown
# 📋 Envoy Code Review Report

## 📊 Summary
- **Files changed:** 5
- **Coverage:** 94% ❌ (need 100%)
- **Build:** ✅ PASS
- **Tests:** ✅ PASS

---

## ❌ Critical Issues

### 1. Missing Test Coverage
**File:** `source/extensions/filters/http/my_filter/filter.cc:67-89`
**Issue:** Error handling path not tested
**Fix:** Add test case for invalid config handling

### 2. Missing Release Note
**File:** `changelogs/current.yaml`
**Issue:** New feature requires release note
**Fix:** Add entry under new_features:

```yaml
- area: http
  change: |
    Added my_filter for custom header manipulation.
```

---

## ⚠️  Warnings

### 1. Shared Pointer Usage
**File:** `source/extensions/filters/http/my_filter/filter.h:23`
**Suggestion:** Consider unique_ptr for clearer ownership

---

## 💡 Suggestions

1. Add integration test for end-to-end flow
2. Consider runtime guard for safe rollback

---

## 📝 Action Items

- [ ] Add test for error path
- [ ] Update changelogs/current.yaml
- [ ] Run: `bazel test //test/extensions/filters/http/my_filter:filter_test`
```

## 🔧 Configuration

### Agent Settings

The agent is configured in `.claude/agents/code-reviewer.md`. You can customize:

- **Strictness levels** - Adjust which checks are critical vs warnings
- **Excluded paths** - Skip certain directories from analysis
- **Custom patterns** - Add project-specific checks

### Slash Command Customization

Edit `.claude/commands/envoy-review.md` to customize the slash command behavior:

- Change the base branch (default: `main`)
- Add specific focus areas
- Customize output verbosity

## 🧪 Testing

### Run Test Scenarios

Validate the agent with predefined test scenarios:

```bash
./tests/ai-review-scenarios/run-all-scenarios.sh
```

This runs automated tests for:
- Missing test coverage detection
- Missing release notes detection
- Style violation detection

### Manual Testing

1. Create a test branch:
```bash
git checkout -b test-code-review
```

2. Make some changes (e.g., add a file without test)

3. Run the review:
```bash
/envoy-review
```

4. Verify expected issues are detected

## 📖 Best Practices

### For Developers

**Before committing:**
1. Run local tests: `bazel test //path/to:test`
2. Check coverage: `bazel coverage //path/to:target`
3. Verify format: `bazel run //tools/code_format:check_format -- check`

**Before creating PR:**
1. Run `/envoy-review` to catch issues early
2. Address all critical issues
3. Consider warnings and suggestions
4. Ensure release notes are updated

**During review:**
- Use agent feedback to improve code quality
- Ask agent for clarification on Envoy policies
- Iterate based on recommendations

### For Reviewers

- Use agent as **first pass** review
- Focus human review on:
  - Architectural decisions
  - Business logic correctness
  - Performance implications
  - Security concerns
- Agent handles mechanical checks (coverage, style, docs)

## 🔍 Troubleshooting

### Agent doesn't detect changes

**Cause:** Not in a git repository or no changes committed
**Solution:** Commit your changes first, then run `/envoy-review`

### False positives

**Cause:** Agent might not understand special cases
**Solution:** Add comment explaining why the pattern is correct

### Helper script fails

**Cause:** Python dependencies missing or wrong Python version
**Solution:** Ensure Python 3.7+ is installed

### No output from /envoy-review

**Cause:** Slash command not properly configured
**Solution:** Verify `.claude/commands/envoy-review.md` exists

## 🛠️ Development

### Architecture

```
User Input (/envoy-review)
    ↓
Slash Command (.claude/commands/envoy-review.md)
    ↓
Agent Prompt (.claude/agents/code-reviewer.md)
    ↓
Claude Code Execution
    ↓
    ├── Git commands (identify changes)
    ├── File reads (analyze code)
    ├── Grep searches (find patterns)
    ├── Helper script (structured analysis)
    └── Bazel commands (verify build/tests)
    ↓
Formatted Report
```

### Extending the Agent

To add new checks:

1. **Add to agent prompt** (`.claude/agents/code-reviewer.md`):
```markdown
### X. New Check Name

**Check for:**
- Specific pattern or requirement

**How to verify:**
```bash
# Command to run
```

**Expected output:**
- What to report if issue found
```

2. **Update helper script** (`scripts/envoy-review-helper.py`):
```python
def check_new_pattern(self, filepath: str) -> List[str]:
    """Check for new pattern."""
    issues = []
    # Implementation
    return issues
```

3. **Add test scenario** (`tests/ai-review-scenarios/`):
```bash
# Create new scenario directory
mkdir tests/ai-review-scenarios/04-new-check/
# Document expected behavior
```

### Contributing Improvements

To contribute improvements to the agent:

1. Test your changes with real PRs
2. Run test scenarios to ensure no regressions
3. Update documentation
4. Submit PR with:
   - Description of improvement
   - Test results
   - Example of new detection capability

## 📚 Resources

### Envoy Documentation
- [CONTRIBUTING.md](../../CONTRIBUTING.md) - Contribution guidelines
- [STYLE.md](../../STYLE.md) - Code style guide
- [CLAUDE.md](../../CLAUDE.md) - Development guide
- [bazel/README.md](../../bazel/README.md) - Build system

### Agent Development
- [Claude Code Docs](https://docs.claude.com/claude-code) - Official documentation
- [Agent Guide](https://docs.claude.com/claude-code/agents) - How agents work

## 📊 Metrics & Impact

Track the agent's effectiveness:

- **Time saved** - Compare manual review time vs agent analysis
- **Issues caught** - Number of issues detected before human review
- **False positive rate** - Incorrect flags (target: <5%)
- **Coverage improvements** - % increase in test coverage

## 🤝 Support

### Getting Help

- **Documentation issues**: Check this README and Envoy docs
- **Agent bugs**: File issue with example PR and expected vs actual output
- **Feature requests**: Suggest improvements with use case

### Known Limitations

1. **No semantic understanding** - Agent checks patterns, not logic correctness
2. **Context limited** - May not understand project-specific context
3. **No performance analysis** - Doesn't catch performance regressions
4. **No security auditing** - Basic security checks only

## 🔮 Future Enhancements

Planned improvements:

- [ ] Integration with GitHub Actions for automatic PR comments
- [ ] Performance regression detection
- [ ] Security vulnerability scanning
- [ ] Suggested fix generation (auto-fix common issues)
- [ ] Learning from past reviews (pattern database)
- [ ] Coverage gap visualization
- [ ] Benchmark comparison

## 📄 License

This agent is part of the Envoy project and follows the same license (Apache 2.0).

---

**Version:** 1.0.0
**Last Updated:** 2025-01-17
**Maintainer:** AI Development Team
