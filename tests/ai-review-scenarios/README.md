# Test Scenarios for Envoy Code Reviewer Agent

This directory contains test scenarios to validate the Envoy Code Reviewer agent.

## Scenarios

### Scenario 1: Missing Test Coverage
**Purpose:** Verify agent detects when source files lack corresponding tests

**Setup:**
- New source file added: `source/extensions/filters/http/test_filter/filter.cc`
- No corresponding test file

**Expected Detection:**
- ❌ Critical issue: Missing test file
- Suggest creating `test/extensions/filters/http/test_filter/filter_test.cc`

### Scenario 2: Missing Release Notes
**Purpose:** Verify agent detects user-facing changes without release notes

**Setup:**
- Modified source file with new feature
- `changelogs/current.yaml` not updated

**Expected Detection:**
- ❌ Critical issue: Missing release note
- Provide example release note format

### Scenario 3: Style Violations
**Purpose:** Verify agent catches common style issues

**Setup:**
- Code using `shared_ptr` instead of `unique_ptr`
- Direct `time()` calls instead of `TimeSystem`
- Missing thread annotations

**Expected Detection:**
- ⚠️  Warning: Prefer unique_ptr over shared_ptr
- ⚠️  Warning: Direct time() call detected
- ⚠️  Warning: Missing ABSL_GUARDED_BY annotation

### Scenario 4: Breaking Change Without Deprecation
**Purpose:** Verify agent detects API breaking changes

**Setup:**
- Modified public function signature
- No deprecation period

**Expected Detection:**
- ❌ Critical issue: Breaking change requires deprecation
- Suggest runtime guard and old signature preservation

### Scenario 5: Complete Valid Change
**Purpose:** Verify agent gives positive feedback for correct changes

**Setup:**
- Source file with corresponding test
- 100% coverage
- Release notes updated
- Style compliant

**Expected Detection:**
- ✅ All checks passed
- Minimal or no issues

## How to Test

### Manual Testing

1. Create a test branch:
```bash
git checkout -b test-code-review
```

2. Simulate a scenario (e.g., add file without test):
```bash
# Add a source file without test
touch source/common/http/test_file.cc
git add source/common/http/test_file.cc
git commit -m "test: add file without test"
```

3. Run the review helper:
```bash
./scripts/envoy-review-helper.py --format markdown
```

4. Or invoke the Claude Code agent:
```
/review
```

### Automated Testing

Run all scenarios:
```bash
./tests/ai-review-scenarios/run-all-scenarios.sh
```

## Creating New Scenarios

To add a new scenario:

1. Create directory: `tests/ai-review-scenarios/XX-scenario-name/`
2. Add description: `README.md`
3. Create test files as needed
4. Document expected detection in README

## Validation Criteria

A scenario passes if:
- ✅ All expected critical issues are detected
- ✅ All expected warnings are raised
- ✅ Suggestions are relevant and actionable
- ✅ No false positives
- ✅ Clear, specific fixes provided
