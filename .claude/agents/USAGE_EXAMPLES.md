# Envoy Code Reviewer - Usage Examples

This document provides practical examples of using the Envoy Code Reviewer agent in different scenarios.

## Example 1: New HTTP Filter Development

### Scenario
You're developing a new HTTP filter for custom header manipulation.

### Files Changed
```
source/extensions/filters/http/custom_headers/
├── filter.h
├── filter.cc
├── config.h
├── config.cc
└── BUILD

test/extensions/filters/http/custom_headers/
├── filter_test.cc
├── config_test.cc
├── integration_test.cc
└── BUILD

api/envoy/extensions/filters/http/custom_headers/v3/
└── custom_headers.proto

changelogs/current.yaml (modified)
```

### Running the Review

```bash
# Option 1: Using slash command in Claude Code
/envoy-review

# Option 2: Using helper script
./scripts/envoy-review-helper.py --format markdown
```

### Expected Agent Output

```markdown
# 📋 Envoy Code Review Report

## 📊 Summary
- **Files changed:** 11 (4 source, 3 test, 2 build, 1 API, 1 docs)
- **Lines:** +456 / -0
- **Coverage:** 98% ⚠️  (need 100%)
- **Build:** ✅ PASS
- **Tests:** ✅ PASS
- **Release notes:** ✅ Updated

---

## ❌ Critical Issues

### 1. Incomplete Test Coverage
**File:** `source/extensions/filters/http/custom_headers/filter.cc:89-102`
**Issue:** Error handling when header value exceeds max length is not tested
**Coverage:** 98% (missing lines: 95-98)

**Missing test case:**
```cpp
TEST_F(CustomHeadersFilterTest, HeaderValueExceedsMaxLength) {
  setupFilterWithMaxLength(100);
  Http::TestRequestHeaderMapImpl headers{
    {":method", "GET"},
    {"custom-header", std::string(101, 'x')}  // Exceeds max
  };

  EXPECT_EQ(Http::FilterHeadersStatus::StopIteration,
            filter_->decodeHeaders(headers, false));
  EXPECT_EQ(1, stats_.value_too_long_.value());
}
```

**Action:** Add this test to `test/extensions/filters/http/custom_headers/filter_test.cc`

---

## ✅ Passing Checks

- ✅ All files have corresponding tests
- ✅ Build files properly configured
- ✅ Extension registered in `extensions_build_config.bzl`
- ✅ API documentation complete
- ✅ Release note added
- ✅ Code style compliant
- ✅ No shared_ptr usage
- ✅ Thread safety annotations present

---

## 💡 Suggestions

1. **Integration Test**: Consider adding integration test for multiple filters interaction
2. **Performance**: Add benchmark for header manipulation overhead
3. **Documentation**: Add example configuration in docs/root/configuration/http/http_filters/

---

## 📝 Action Items

**Before merging:**

1. [ ] Add test case for header value length validation
2. [ ] Run coverage to verify 100%:
   ```bash
   bazel coverage //test/extensions/filters/http/custom_headers:filter_test
   ```
3. [ ] Verify integration test passes:
   ```bash
   bazel test //test/extensions/filters/http/custom_headers:integration_test
   ```

**Estimated time to fix:** 15-20 minutes
```

### Developer Actions

```bash
# 1. Add the missing test
vim test/extensions/filters/http/custom_headers/filter_test.cc
# (add the test case suggested above)

# 2. Verify coverage
bazel coverage //test/extensions/filters/http/custom_headers:filter_test

# 3. Re-run review
/envoy-review

# Output: ✅ All checks passed!
```

---

## Example 2: Bug Fix in Connection Pool

### Scenario
Fixing a crash in TCP connection pool during connection teardown.

### Files Changed
```
source/common/conn_pool/conn_pool_base.cc (modified)
test/common/conn_pool/conn_pool_base_test.cc (modified)
changelogs/current.yaml (NOT modified - forgot!)
```

### Agent Output

```markdown
# 📋 Envoy Code Review Report

## ❌ Critical Issues

### 1. Missing Release Note
**Issue:** Bug fix in connection pool but `changelogs/current.yaml` not updated

**User Impact:** Users experiencing this crash need to know it's fixed

**Required entry:**
```yaml
bug_fixes:
- area: connection pool
  change: |
    Fixed crash in TCP connection pool during connection teardown when large
    requests trigger flow control. This resolves connections being improperly
    released during shutdown sequence.
```

**Action:** Edit `changelogs/current.yaml` and add under `bug_fixes:` section

---

## ✅ Passing Checks

- ✅ Test coverage: 100%
- ✅ Build passes
- ✅ Tests pass
- ✅ Code style compliant
- ✅ Regression test added

---

## 💡 Suggestions

1. Consider adding integration test to verify fix end-to-end
2. Check if related connection pool code has similar pattern
```

### Developer Actions

```bash
# 1. Add release note
vim changelogs/current.yaml

# Add under bug_fixes:
# - area: connection pool
#   change: |
#     Fixed crash during connection teardown...

# 2. Re-run review
/envoy-review

# Output: ✅ All checks passed!

# 3. Create PR
git push origin fix-connection-pool-crash
gh pr create --title "fix: connection pool crash on teardown" \
  --body "$(cat changelog-entry.txt)"
```

---

## Example 3: API Breaking Change

### Scenario
Updating HTTP codec API to add stream metadata parameter.

### Files Changed
```
include/envoy/http/codec.h (public API changed)
source/common/http/codec_impl.cc
test/common/http/codec_impl_test.cc
```

### Agent Output

```markdown
# 📋 Envoy Code Review Report

## ❌ Critical Issues

### 1. Breaking API Change Without Deprecation
**File:** `include/envoy/http/codec.h:45`
**Issue:** Changed `encodeHeaders()` signature - breaking change for extensions

**Current change:**
```cpp
// Old (removed):
virtual void encodeHeaders(HeaderMap& headers, bool end_stream);

// New:
virtual void encodeHeaders(HeaderMap& headers, bool end_stream,
                          const StreamInfo& stream_info);
```

**Required approach (deprecation policy):**

1. **Keep old signature with deprecation:**
```cpp
// Old signature (deprecated)
[[deprecated("Use encodeHeaders with StreamInfo parameter. Will be removed in version X.Y")]]
virtual void encodeHeaders(HeaderMap& headers, bool end_stream) {
  // Default implementation calls new signature
  encodeHeaders(headers, end_stream, StreamInfo::default());
}

// New signature
virtual void encodeHeaders(HeaderMap& headers, bool end_stream,
                          const StreamInfo& stream_info) = 0;
```

2. **Add runtime guard:**
```cpp
if (Runtime::runtimeFeatureEnabled("envoy.reloadable_features.encode_headers_with_stream_info")) {
  // Use new signature
} else {
  // Use old signature
}
```

Register in `source/common/runtime/runtime_features.cc`

3. **Update release notes:**
```yaml
deprecated:
- area: http
  change: |
    Deprecated `encodeHeaders(HeaderMap&, bool)` in favor of
    `encodeHeaders(HeaderMap&, bool, const StreamInfo&)`. The old signature
    will be removed in version X.Y. Extensions should migrate to the new
    signature. Runtime guard `envoy.reloadable_features.encode_headers_with_stream_info`
    controls the behavior.
```

4. **Add migration guide:**
Document in `docs/root/version_history/current.rst` how extensions should update.

---

## ⚠️  Additional Requirements

### Cross-Company Review
This API change affects extensions. Per CONTRIBUTING.md, you need:
- [ ] Approval from at least one maintainer from different organization
- [ ] Update CODEOWNERS if adding new functionality

### API Review Checklist
- [ ] Follow `api/STYLE.md` for proto changes
- [ ] Add field documentation
- [ ] Consider backward compatibility
- [ ] Document migration path

---

## 📝 Action Items

**Must complete before merge:**

1. [ ] Implement deprecation approach (both signatures)
2. [ ] Add runtime guard
3. [ ] Update all release note sections (deprecated, new_features)
4. [ ] Write migration guide
5. [ ] Get cross-company review approval
6. [ ] Update tests to cover both code paths (old & new)

**Estimated effort:** 2-3 hours
```

---

## Example 4: Perfect Submission ✅

### Scenario
Adding timeout configuration option to existing filter.

### Files Changed
```
api/envoy/extensions/filters/http/my_filter/v3/my_filter.proto (added timeout field)
source/extensions/filters/http/my_filter/filter.cc (implementation)
source/extensions/filters/http/my_filter/config.cc (config parsing)
test/extensions/filters/http/my_filter/filter_test.cc (new tests)
test/extensions/filters/http/my_filter/integration_test.cc (e2e test)
docs/root/configuration/http/http_filters/my_filter.rst (documentation)
changelogs/current.yaml (release note)
```

### Agent Output

```markdown
# 📋 Envoy Code Review Report

## 📊 Summary
- **Files changed:** 7 (2 source, 2 test, 1 API, 1 docs, 1 changelog)
- **Lines:** +234 / -12
- **Coverage:** 100% ✅
- **Build:** ✅ PASS
- **Tests:** ✅ PASS
- **Release notes:** ✅ Updated

---

## ✅ All Checks Passed! 🎉

This PR meets all Envoy quality standards:

- ✅ **100% test coverage** - All code paths tested
- ✅ **Release note** - Properly categorized under `new_features`
- ✅ **API documentation** - Proto field has clear description
- ✅ **User documentation** - Configuration example added
- ✅ **Integration test** - End-to-end behavior verified
- ✅ **Code style** - Fully compliant with STYLE.md
- ✅ **Build system** - No changes needed (existing extension)
- ✅ **Error handling** - Proper validation and logging
- ✅ **Thread safety** - No shared mutable state
- ✅ **Time handling** - Uses TimeSystem correctly

---

## 💡 Minor Suggestions (Optional)

1. **Performance**: Consider adding benchmark for timeout handling overhead
   - Add to `test/extensions/filters/http/my_filter/benchmark_test.cc`

2. **Future enhancement**: Timeout could be dynamic based on headers
   - Could be follow-up feature

---

## 📝 Ready for Review!

This PR is ready for maintainer review. No action items required.

**Suggested reviewers:**
- @envoyproxy/http-maintainers (area experts)
- @envoyproxy/senior-maintainers (final approval)

**Testing checklist:**
- ✅ Unit tests: `bazel test //test/extensions/filters/http/my_filter:filter_test`
- ✅ Integration: `bazel test //test/extensions/filters/http/my_filter:integration_test`
- ✅ Coverage: `bazel coverage //source/extensions/filters/http/my_filter/...`

Great work! 🚀
```

---

## Example 5: Using Helper Script in CI

### Scenario
Automating code review in GitHub Actions.

### GitHub Actions Workflow

```yaml
# .github/workflows/ai-code-review.yml
name: AI Code Review

on:
  pull_request:
    types: [opened, synchronize]

jobs:
  ai-review:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout code
        uses: actions/checkout@v3
        with:
          fetch-depth: 0  # Need full history for git diff

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.9'

      - name: Run Envoy Code Reviewer
        id: review
        run: |
          ./scripts/envoy-review-helper.py \
            --repo . \
            --base ${{ github.base_ref }} \
            --format markdown > review-report.md

      - name: Post Review as Comment
        uses: actions/github-script@v6
        with:
          script: |
            const fs = require('fs');
            const report = fs.readFileSync('review-report.md', 'utf8');

            const comment = `## 🤖 AI Code Review

            ${report}

            ---
            *This review was automatically generated by the Envoy Code Reviewer agent.*
            *Human reviewers should focus on architecture, logic, and performance.*
            `;

            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: comment
            });

      - name: Fail if Critical Issues
        run: |
          # Exit with non-zero if critical issues found
          if grep -q "❌ Critical Issues" review-report.md; then
            echo "Critical issues found. Please address before merging."
            exit 1
          fi
```

### Example PR Comment Output

The action will post a comment on the PR:

```markdown
## 🤖 AI Code Review

# 📋 Envoy Code Review Report

## 📊 Summary
- **Files changed:** 3
- **Coverage:** 95% ❌ (need 100%)

## ❌ Critical Issues

### 1. Missing Test Coverage
**File:** `source/common/http/new_feature.cc:67-89`
...

---
*This review was automatically generated by the Envoy Code Reviewer agent.*
*Human reviewers should focus on architecture, logic, and performance.*
```

---

## Tips for Best Results

### 1. Commit Changes Before Review
```bash
# Agent analyzes committed changes
git add .
git commit -m "feat: add new feature"

# Then run review
/envoy-review
```

### 2. Review Early and Often
```bash
# Don't wait until PR is ready - review during development
git commit -m "wip: partial implementation"
/envoy-review  # Get early feedback
```

### 3. Iterative Improvements
```bash
# Address issues incrementally
/envoy-review  # Find issues
# Fix issue #1
git commit --amend
/envoy-review  # Verify fix, find remaining issues
# Fix issue #2
...
```

### 4. Focus Human Review Time
```markdown
# Let agent handle mechanical checks:
- Coverage ✅
- Style ✅
- Documentation ✅
- Build system ✅

# Focus human reviewers on:
- Architecture decisions
- Algorithm correctness
- Performance implications
- Security concerns
```

### 5. Learn Envoy Standards
```bash
# Use agent to learn Envoy policies
/envoy-review  # Get feedback
# Read suggested documentation
# Understand rationale behind policies
# Apply in future PRs
```

---

## Common Patterns

### Pattern 1: Fix Coverage Gap

```bash
# 1. Agent identifies untested code
/envoy-review  # → "Line 45-67 not tested"

# 2. Add test
vim test/path/to/file_test.cc
# Add test case

# 3. Verify
bazel test //test/path/to:file_test
bazel coverage //test/path/to:file_test

# 4. Confirm
/envoy-review  # → "100% coverage ✅"
```

### Pattern 2: Add Missing Release Note

```bash
# 1. Agent detects missing note
/envoy-review  # → "Missing release note"

# 2. Add note
vim changelogs/current.yaml
# Add appropriate entry

# 3. Confirm
/envoy-review  # → "Release notes updated ✅"
```

### Pattern 3: Fix Style Issue

```bash
# 1. Agent detects style violation
/envoy-review  # → "shared_ptr should be unique_ptr"

# 2. Fix
vim source/path/to/file.cc
# Change shared_ptr → unique_ptr

# 3. Verify
bazel run //tools/code_format:check_format -- check

# 4. Confirm
/envoy-review  # → "No style issues ✅"
```

---

**Remember:** The agent is a tool to accelerate development and ensure quality. It complements, but doesn't replace, human code review and judgment.
