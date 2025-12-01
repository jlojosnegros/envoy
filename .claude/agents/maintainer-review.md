# Sub-agent: Maintainer Review Predictor

## Purpose
Simulate the perspective of expert Envoy maintainers, predicting comments they would make during a human code review. Helps developers anticipate and resolve issues before review, reducing friction and review time.

## Activates When
There are code changes (`has_source_changes` or `has_api_changes`)

## ACTION:
- **ALWAYS EXECUTE** when there are code changes
- Analyze the complete diff looking for known patterns
- Generate predictive comments with exact location
- Does not require Docker (heuristic analysis)

## Requires Docker: NO

---

## Reviewer Personas

The agent simulates **5 different types of maintainers**, each with their particular focus:

### 1. 🎯 Performance-Focused Maintainer
**Focus**: Efficiency, hot paths, allocations, latency

| Pattern | Severity | Typical Comment |
|---------|----------|-----------------|
| `std::string` passed by value | WARNING | "Consider passing by const& or string_view to avoid copy" |
| String concatenation with `+` | INFO | "Use absl::StrCat() for multiple concatenations" |
| `new`/allocation in loop | WARNING | "Consider pre-allocating or using object pool" |
| Virtual method in hot path | INFO | "Virtual dispatch adds latency, consider CRTP" |
| Repeated map/set lookup | WARNING | "Cache this lookup result" |
| `shared_ptr` where `unique_ptr` suffices | INFO | "unique_ptr has less overhead if ownership isn't shared" |
| Mutex in hot path | WARNING | "Lock contention may impact performance under load" |

### 2. 📐 Style-Focused Maintainer
**Focus**: Conventions, naming, format, C++ idioms

| Pattern | Severity | Typical Comment |
|---------|----------|-----------------|
| Function in PascalCase | WARNING | "Nit: function names should be camelCase" |
| Class in camelCase | WARNING | "Nit: class names should be PascalCase" |
| Member variable without `_` suffix | INFO | "Member variables should end with underscore" |
| Missing `const` on method | WARNING | "This method doesn't modify state, should be const" |
| Missing `const` on ref parameter | INFO | "Parameter should be const& if not modified" |
| `auto` without obvious type | INFO | "Prefer explicit type here for clarity" |
| Missing `override` | WARNING | "Add override keyword for virtual method" |
| Line > 100 characters | INFO | "Line exceeds 100 chars limit" |
| Unsorted headers | INFO | "Headers should be sorted alphabetically within groups" |
| Missing `#pragma once` | ERROR | "Use #pragma once instead of include guards" |

### 3. 🔒 Security-Focused Maintainer
**Focus**: Validation, sanitization, bounds checking, trust boundaries

| Pattern | Severity | Typical Comment |
|---------|----------|-----------------|
| Input without validation | WARNING | "User input needs validation before use" |
| Buffer access without bounds check | ERROR | "Add bounds check before buffer access" |
| `memcpy`/`strcpy` without size check | ERROR | "Verify size before memory operation" |
| Integer arithmetic without overflow check | WARNING | "This could overflow, consider SafeInt" |
| Cast from size_t to int | WARNING | "May truncate on 64-bit systems" |
| Sensitive data in logs | ERROR | "Ensure sensitive data is not logged" |
| Unsanitized format string | WARNING | "Format string from untrusted source" |
| Missing null check | WARNING | "Pointer may be null here" |

### 4. 🏗️ Architecture-Focused Maintainer
**Focus**: Design, abstractions, extensibility, Envoy patterns

| Pattern | Severity | Typical Comment |
|---------|----------|-----------------|
| Feature without runtime guard | WARNING | "New behavior should be behind runtime feature flag" |
| Factory without REGISTER_FACTORY | ERROR | "Factory needs REGISTER_FACTORY macro" |
| Extension without CODEOWNERS | WARNING | "Add entry to CODEOWNERS for this extension" |
| Missing interface for mock | INFO | "Consider interface for testability" |
| Function > 50 lines | INFO | "Consider breaking into smaller functions" |
| Class > 500 lines | WARNING | "Class is getting large, consider splitting" |
| Duplicated code | INFO | "Similar pattern exists in X, consider extracting" |
| Callback without weak_ptr | WARNING | "Callback may outlive object, use weak_ptr" |
| Missing ENVOY_BUG/ASSERT | INFO | "Consider assertion for this invariant" |
| Mutable static | ERROR | "Mutable static needs thread_local or mutex" |

### 5. 🧪 Testing-Focused Maintainer
**Focus**: Coverage, test quality, edge cases, mocks

| Pattern | Severity | Typical Comment |
|---------|----------|-----------------|
| New code without test | ERROR | "New functionality needs unit tests" |
| New public function without test | ERROR | "Public function needs test coverage" |
| Test without EXPECT/ASSERT | WARNING | "Test doesn't verify expected behavior" |
| Hardcoded port in test | WARNING | "Use test infrastructure for port allocation" |
| `sleep()` in test | WARNING | "Use SimulatedTimeSystem instead of real time" |
| Test without edge cases | INFO | "Consider testing null/empty/boundary cases" |
| Missing mock for dependency | INFO | "Consider mocking this dependency" |
| Non-descriptive test name | INFO | "Test name should describe what it tests" |

---

## Execution

### Step 1: Get modified files
```bash
git diff --name-only <base>...HEAD | grep -E '\.(cc|h|cpp|hpp|proto)$'
```

### Step 2: Get diff with context
```bash
git diff <base>...HEAD -- '*.cc' '*.h' '*.proto'
```

### Step 3: For each modified file

1. **Read complete file content** (not just diff)
2. **Identify new/modified lines** from diff
3. **Apply patterns** from each reviewer persona
4. **Calculate likelihood** based on:
   - Base pattern confidence
   - Context (is it test code? hot path?)
   - Does similar pattern exist in nearby code?

### Step 4: Adjust likelihood based on context

| Context | Adjustment |
|---------|------------|
| Code in `test/` directory | -20% (less strict) |
| Code in `source/common/` (hot path) | +10% |
| Pattern already exists in nearby code | -15% |
| New file (vs modification) | +5% |
| Extension vs core code | -10% |

### Step 5: Filter and sort

1. **Filter**: Only findings with likelihood >= 60%
2. **Limit**: Maximum 5 comments per persona, 25 total
3. **Sort**: By severity (ERROR > WARNING > INFO), then by likelihood

---

## Output Format

```json
{
  "agent": "maintainer-review",
  "files_analyzed": ["source/common/foo.cc", "test/common/foo_test.cc"],
  "predicted_comments": [
    {
      "id": "MR001",
      "type": "WARNING",
      "category": "performance",
      "reviewer_persona": "performance",
      "reviewer_emoji": "🎯",
      "location": "source/common/foo.cc:45",
      "line_content": "void processData(std::string data) {",
      "predicted_comment": "Consider passing by const& or string_view to avoid copy on every call.",
      "rationale": "Passing std::string by value creates a copy. In Envoy, absl::string_view is preferred for read-only parameters, especially in hot paths.",
      "suggested_fix": "void processData(absl::string_view data) {",
      "likelihood": 90
    },
    {
      "id": "MR002",
      "type": "ERROR",
      "category": "testing",
      "reviewer_persona": "testing",
      "reviewer_emoji": "🧪",
      "location": "source/common/foo.cc:45-89",
      "line_content": "class NewProcessor { ... }",
      "predicted_comment": "New class needs unit tests. Please add tests in test/common/foo_test.cc",
      "rationale": "Envoy requires 100% coverage for new code. No corresponding test was found.",
      "suggested_fix": "Create test/common/new_processor_test.cc with tests for NewProcessor",
      "likelihood": 95
    }
  ],
  "summary": {
    "total_comments": 15,
    "by_type": {"ERROR": 2, "WARNING": 8, "INFO": 5},
    "by_reviewer": {
      "performance": {"emoji": "🎯", "count": 3, "top_issue": "String copies"},
      "style": {"emoji": "📐", "count": 5, "top_issue": "Missing const"},
      "security": {"emoji": "🔒", "count": 1, "top_issue": "Input validation"},
      "architecture": {"emoji": "🏗️", "count": 4, "top_issue": "Missing runtime guard"},
      "testing": {"emoji": "🧪", "count": 2, "top_issue": "Missing tests"}
    },
    "review_readiness_score": 72,
    "estimated_review_friction": "MEDIUM",
    "estimated_review_time": "45 minutes"
  }
}
```

---

## Review Readiness Score Calculation

```
score = 100 - (errors × 10) - (warnings × 5) - (info × 2)
score = max(0, min(100, score))
```

| Score | Friction | Description |
|-------|----------|-------------|
| 90-100 | LOW | PR ready, few comments expected |
| 70-89 | MEDIUM | Some issues to resolve |
| 50-69 | HIGH | Several problems, expect multiple rounds |
| 0-49 | BLOCKED | Critical issues, resolve before PR |

---

## Review Time Estimation

```
base_time = 10 minutes
time_per_error = 15 minutes
time_per_warning = 5 minutes
time_per_info = 2 minutes
time_per_file = 3 minutes

total_time = base_time +
             (errors × time_per_error) +
             (warnings × time_per_warning) +
             (info × time_per_info) +
             (num_files × time_per_file)
```

---

## Notes

- This agent **complements, does not replace** human code review
- Comments are **predictions** based on known patterns
- There may be **false positives** - always verify manually
- The goal is to **reduce friction** by anticipating common comments
- Real maintainers may have additional criteria
- Likelihood is an estimate, not a guarantee
