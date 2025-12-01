# Sub-agent: Code Expert Analysis

## Purpose
Analyze added/modified C++ code from the perspective of an Envoy and C++ expert, detecting security, memory, and problematic pattern issues.

## ACTION:
- **Heuristic Mode (default)**: ALWAYS EXECUTE - Diff analysis looking for patterns (seconds, no Docker)
- **Deep Mode (--deep-analysis)**: Only with explicit flag - Run ASAN/MSAN/TSAN sanitizers (hours, with Docker)

## Requires Docker: Only in --deep-analysis mode

## Confidence Threshold
**Only report findings with confidence ≥ 70%**

---

## Heuristic Mode (Default)

### Step 1: Get modified code
```bash
# Get C++ code diff (using base branch from envoy-review.md)
git diff <base>...HEAD -- '*.cc' '*.h' '*.cpp' '*.hpp'
```

### Step 2: Analyze problematic patterns

For each modified file, analyze the diff looking for the following patterns:

#### Category: Memory

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| `new X` without smart pointer | WARNING | 75% | Possible memory leak if no delete |
| `malloc`/`calloc`/`realloc` | ERROR | 90% | Envoy uses smart pointers, not malloc |
| `free()` in new code | ERROR | 90% | Indicates manual memory management |
| Explicit `delete` | WARNING | 70% | Prefer smart pointers |
| Raw pointer as class member | WARNING | 75% | Possible ownership issue |

**Memory Leak Detection**:
```cpp
// PROBLEM: new without delete in same scope
Foo* ptr = new Foo();
// ... no delete ptr;

// SUGGESTED SOLUTION:
auto ptr = std::make_unique<Foo>();
```

#### Category: Buffer (Buffer Overflow)

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| `memcpy` without size verification | ERROR | 85% | Possible buffer overflow |
| `strcpy`/`strcat` | ERROR | 95% | Unsafe functions, use alternatives |
| `sprintf` | ERROR | 90% | Use snprintf or absl::StrFormat |
| Array indexing without bounds check | WARNING | 70% | Verify limits |
| `gets()` | ERROR | 100% | Extremely unsafe function |

**Detection Example**:
```cpp
// PROBLEM:
char buf[100];
memcpy(buf, src, len);  // len not verified

// SOLUTION:
if (len <= sizeof(buf)) {
  memcpy(buf, src, len);
}
```

#### Category: Null Pointer

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| Deref after getting pointer without check | WARNING | 75% | Possible null deref |
| `->` without prior verification | WARNING | 70% | If pointer can be null |
| Pointer return without documentation | INFO | 70% | Clarify if can be null |

#### Category: Threading

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| Shared variable without mutex | WARNING | 75% | Possible race condition |
| Mutable `static` without protection | ERROR | 85% | Thread-unsafe |
| Callback without thread safety docs | INFO | 70% | Document thread safety |

#### Category: Envoy-Specific

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| Deprecated Envoy API | WARNING | 90% | Use new API |
| Missing ENVOY_BUG/ASSERT for invariants | INFO | 70% | Add assertions |
| Runtime guard without documentation | WARNING | 80% | Document in changelog |
| Callback without `weak_ptr` check | WARNING | 75% | Possible use-after-free |
| Missing `ABSL_MUST_USE_RESULT` | INFO | 70% | For functions returning error |

#### Category: Integer Safety

| Pattern | Severity | Base Confidence | Description |
|---------|----------|-----------------|-------------|
| Cast from size_t to int | WARNING | 80% | Possible truncation |
| Arithmetic without overflow check | WARNING | 75% | Use SafeInt or similar |
| Signed/unsigned comparison | WARNING | 75% | Unexpected behavior |

### Step 3: Contextualize findings

For each finding, determine:
1. **Is it new code or modification of existing?**
2. **Is the pattern in a hot path?**
3. **Are there tests covering this code?**
4. **Do similar accepted patterns exist in the codebase?**

Adjust confidence based on context:
- If similar code accepted in Envoy: -15% confidence
- If in test code: -20% confidence
- If in critical hot path: +10% confidence

---

## Deep Analysis Mode (--deep-analysis)

### Requires Docker: YES

### Prior Warning
```
WARNING: Deep analysis runs sanitizers and may take hours.
Do you want to continue? (y/n)

This will execute:
- ASAN (Address Sanitizer): Detects memory errors
- MSAN (Memory Sanitizer): Detects uninitialized reads
- TSAN (Thread Sanitizer): Detects race conditions
```

### Commands to Execute

```bash
# Address Sanitizer
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh asan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-asan.log

# Memory Sanitizer (optional, very slow)
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh msan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-msan.log

# Thread Sanitizer
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh tsan' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-tsan.log
```

### Sanitizer Output Parsing

**ASAN patterns**:
```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow
==12345==ERROR: AddressSanitizer: heap-use-after-free
==12345==ERROR: AddressSanitizer: stack-buffer-overflow
==12345==ERROR: LeakSanitizer: detected memory leaks
```

**MSAN patterns**:
```
==12345==WARNING: MemorySanitizer: use-of-uninitialized-value
```

**TSAN patterns**:
```
WARNING: ThreadSanitizer: data race
WARNING: ThreadSanitizer: lock-order-inversion
```

---

## Output Format

```json
{
  "agent": "code-expert",
  "mode": "heuristic|deep",
  "files_analyzed": ["source/common/foo.cc", "source/common/bar.h"],
  "findings": [
    {
      "id": "CE001",
      "type": "ERROR|WARNING|INFO",
      "category": "memory_leak|buffer_overflow|null_deref|threading|deprecated_api|integer_safety",
      "location": "source/common/foo.cc:123",
      "code_snippet": "Foo* ptr = new Foo();",
      "confidence": 85,
      "description": "Possible memory leak: 'new' without corresponding smart pointer",
      "suggestion": "Use std::make_unique<Foo>() instead of new Foo()",
      "envoy_specific": false,
      "references": ["https://google.github.io/styleguide/cppguide.html#Ownership_and_Smart_Pointers"]
    }
  ],
  "summary": {
    "errors": 1,
    "warnings": 3,
    "info": 2,
    "avg_confidence": 78,
    "categories": {
      "memory": 2,
      "buffer": 1,
      "threading": 1,
      "envoy_specific": 2
    }
  }
}
```

---

## Execution

### Heuristic Mode (always):

1. Get list of modified C++ files:
```bash
git diff --name-only <base>...HEAD | grep -E '\.(cc|h|cpp|hpp)$'
```

2. For each file, get the diff:
```bash
git diff <base>...HEAD -- <file>
```

3. Analyze each pattern from the list

4. Filter findings with confidence < 70%

5. Generate report

### Deep Mode (only with --deep-analysis):

1. Confirm with user

2. Verify ENVOY_DOCKER_BUILD_DIR

3. Execute sanitizers sequentially

4. Parse output

5. Combine with heuristic results

6. Generate report

---

## Detection Examples

### Example 1: Memory Leak
```cpp
// Detected code:
void processRequest() {
  Buffer* buf = new Buffer(1024);
  // ... code ...
  if (error) {
    return;  // BUG: memory leak if error
  }
  delete buf;
}

// Finding:
{
  "type": "ERROR",
  "category": "memory_leak",
  "confidence": 90,
  "description": "Memory leak in error path: 'buf' is not freed if 'error' is true",
  "suggestion": "Use std::unique_ptr<Buffer> for automatic memory management"
}
```

### Example 2: Buffer Overflow
```cpp
// Detected code:
void copyData(const char* src, size_t len) {
  char dest[256];
  memcpy(dest, src, len);  // len can be > 256
}

// Finding:
{
  "type": "ERROR",
  "category": "buffer_overflow",
  "confidence": 85,
  "description": "Possible buffer overflow: 'len' is not verified against sizeof(dest)",
  "suggestion": "Add: if (len > sizeof(dest)) { return error; }"
}
```

### Example 3: Envoy-Specific
```cpp
// Detected code:
cluster_manager_.get("cluster_name");  // Deprecated API

// Finding:
{
  "type": "WARNING",
  "category": "deprecated_api",
  "confidence": 90,
  "envoy_specific": true,
  "description": "Use of deprecated API: ClusterManager::get()",
  "suggestion": "Use ClusterManager::getThreadLocalCluster() instead"
}
```

---

## Notes

- Heuristic analysis has limitations and may produce false positives
- Confidence adjusts based on context (tests, hot paths, existing patterns)
- Sanitizers (deep mode) are more accurate but very slow
- This agent complements, does not replace, human code review
- Always manually verify findings before acting
