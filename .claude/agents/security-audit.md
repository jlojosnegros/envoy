# Sub-agent: Security Audit

## Purpose
Audit code and dependency security, detecting:
- Dependencies with deprecated versions
- Dependencies with open CVEs
- Code with known security vulnerabilities

## ACTION:
- **Phase 1 (No Docker)**: ALWAYS EXECUTE - Query external CVE APIs
- **Phase 2 (With Docker)**: EXECUTE if dependency changes - `//tools/dependency:validate`

## Requires Docker: Only Phase 2

## Confidence Threshold
**Only report findings with confidence ≥ 70%**

---

## Phase 1: Dependency Analysis (No Docker)

### Step 1: Identify project dependencies

```bash
# Get list of Envoy dependencies
cat bazel/repository_locations.bzl | grep -E '(name|version|sha256)' | head -100
```

### Step 2: Identify modified dependencies

```bash
# See if there are changes in dependency files
git diff --name-only <base>...HEAD | grep -E '(repository_locations|repositories)\.bzl'
```

### Step 3: Query CVE APIs

For each dependency (especially modified ones), query:

#### 3.1 OSV (Open Source Vulnerabilities) - Preferred
```
URL: https://api.osv.dev/v1/query
Method: POST
Body: {"package": {"name": "<name>", "ecosystem": "<ecosystem>"}, "version": "<version>"}
```

#### 3.2 GitHub Advisory Database
```
URL: https://api.github.com/advisories
Query: ?ecosystem=<ecosystem>&package=<name>
```

#### 3.3 NVD (National Vulnerability Database)
```
URL: https://services.nvd.nist.gov/rest/json/cves/2.0
Query: ?keywordSearch=<name>
```

### Step 4: Fallback to local tools

If external APIs are not available:
```bash
# Use Envoy tools
cat bazel/repository_locations.bzl | grep -A5 "cve_"
```

---

## Phase 2: Docker Verification

### Requires Docker: YES

### Activates When
- There are changes in `bazel/repository_locations.bzl`
- There are changes in `bazel/repositories.bzl`
- There are changes in BUILD files that add dependencies

### CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/${TIMESTAMP}-security-deps.log
```

### Output Parsing

Look for in output:
- `CVE-` followed by number
- `vulnerability`
- `deprecated`
- `outdated`
- `FAIL` or `ERROR`

---

## Finding Categories

### CVE (Common Vulnerabilities and Exposures)

| CVSS Severity | Report Type | Description |
|---------------|-------------|-------------|
| 9.0 - 10.0 | ERROR (Critical) | Critical vulnerability, immediate action |
| 7.0 - 8.9 | ERROR (High) | High vulnerability, priority action |
| 4.0 - 6.9 | WARNING (Medium) | Medium vulnerability, plan fix |
| 0.1 - 3.9 | INFO (Low) | Low vulnerability, evaluate |

### Deprecated Dependencies

| Situation | Type | Confidence |
|-----------|------|------------|
| EOL (End of Life) version | ERROR | 95% |
| Version without active support | WARNING | 85% |
| New major version available | INFO | 75% |

### Insecure Code

| Pattern | Type | Confidence |
|---------|------|------------|
| Use of deprecated crypto (MD5, SHA1 for security) | ERROR | 90% |
| Hardcoded secrets/credentials | ERROR | 95% |
| Insecure random (rand() for crypto) | ERROR | 90% |
| HTTP instead of HTTPS for resources | WARNING | 80% |

---

## Main Envoy Dependencies to Monitor

| Dependency | Ecosystem | Criticality |
|------------|-----------|-------------|
| boringssl | C++ | CRITICAL |
| nghttp2 | C++ | HIGH |
| libevent | C++ | HIGH |
| protobuf | C++ | HIGH |
| abseil-cpp | C++ | MEDIUM |
| grpc | C++ | HIGH |
| yaml-cpp | C++ | MEDIUM |
| zlib | C++ | MEDIUM |
| curl | C++ | HIGH |

---

## Output Format

```json
{
  "agent": "security-audit",
  "phase1_executed": true,
  "phase2_executed": true|false,
  "api_sources_used": ["osv", "github", "nvd"],
  "dependencies_checked": 45,
  "findings": [
    {
      "id": "SA001",
      "type": "ERROR|WARNING|INFO",
      "severity": "critical|high|medium|low",
      "category": "cve|deprecated|insecure_code|outdated",
      "location": "boringssl:1.0.0 | source/common/crypto.cc:45",
      "confidence": 95,
      "description": "CVE-2024-1234: Buffer overflow in BoringSSL < 1.1.0",
      "source": "https://nvd.nist.gov/vuln/detail/CVE-2024-1234",
      "cvss_score": 8.5,
      "affected_versions": "< 1.1.0",
      "current_version": "1.0.0",
      "fixed_version": "1.1.0",
      "suggestion": "Update boringssl to version 1.1.0 or higher",
      "exploitability": "Requires network access",
      "patch_available": true
    }
  ],
  "summary": {
    "critical": 0,
    "high": 1,
    "medium": 2,
    "low": 3,
    "total_cves": 6,
    "deprecated_deps": 1,
    "outdated_deps": 4
  }
}
```

---

## Execution

### Phase 1 (No Docker - Always):

1. Read `bazel/repository_locations.bzl` to get dependencies:
```bash
cat bazel/repository_locations.bzl
```

2. For each critical dependency, query OSV:
```
Query: https://api.osv.dev/v1/query with package and version
```

3. If OSV unavailable, use GitHub Advisory API

4. If GitHub unavailable, use NVD

5. If no API available, mark as "APIs unavailable" and continue with Phase 2

6. Filter findings with confidence < 70%

### Phase 2 (With Docker - If dependency changes):

1. Check if there are dependency changes:
```bash
git diff --name-only <base>...HEAD | grep -E 'repository_locations|repositories'
```

2. If there are changes, execute:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh 'bazel run //tools/dependency:validate'
```

3. Parse output for CVEs and warnings

4. Combine with Phase 1 results

5. Generate consolidated report

---

## Insecure Code Detection

### Patterns to search in diff:

```bash
# Insecure crypto
git diff <base>...HEAD | grep -E '(MD5|SHA1|DES|RC4)' | grep -v test

# Hardcoded secrets
git diff <base>...HEAD | grep -iE '(password|secret|api_key|token)\s*=\s*["\047]'

# Insecure random
git diff <base>...HEAD | grep -E '\brand\(\)|\bsrand\(' | grep -v test

# Insecure HTTP
git diff <base>...HEAD | grep -E 'http://' | grep -v '(localhost|127\.0\.0\.1|example\.com)'
```

---

## Finding Examples

### Example 1: CVE in Dependency
```json
{
  "id": "SA001",
  "type": "ERROR",
  "severity": "high",
  "category": "cve",
  "location": "nghttp2:1.43.0",
  "confidence": 98,
  "description": "CVE-2023-44487: HTTP/2 Rapid Reset Attack",
  "source": "https://nvd.nist.gov/vuln/detail/CVE-2023-44487",
  "cvss_score": 7.5,
  "affected_versions": "< 1.57.0",
  "current_version": "1.43.0",
  "fixed_version": "1.57.0",
  "suggestion": "Update nghttp2 to version 1.57.0 in bazel/repository_locations.bzl"
}
```

### Example 2: Deprecated Dependency
```json
{
  "id": "SA002",
  "type": "WARNING",
  "severity": "medium",
  "category": "deprecated",
  "location": "old-library:2.0.0",
  "confidence": 90,
  "description": "Dependency 'old-library' has been EOL since 2023-01",
  "source": "https://github.com/old-library/old-library/releases",
  "suggestion": "Migrate to 'new-library' or update to supported branch"
}
```

### Example 3: Insecure Code
```json
{
  "id": "SA003",
  "type": "ERROR",
  "severity": "high",
  "category": "insecure_code",
  "location": "source/common/auth.cc:78",
  "confidence": 85,
  "description": "Use of MD5 for credential hashing",
  "code_snippet": "std::string hash = MD5::hash(password);",
  "suggestion": "Use SHA-256 or bcrypt for credential hashing"
}
```

---

## CVE APIs - Details

### OSV API (Preferred)
```bash
curl -X POST https://api.osv.dev/v1/query \
  -H "Content-Type: application/json" \
  -d '{"package":{"name":"nghttp2","ecosystem":"C++"},"version":"1.43.0"}'
```

**Response contains:**
- `vulns[]`: List of vulnerabilities
- `vulns[].id`: CVE ID
- `vulns[].summary`: Description
- `vulns[].severity[]`: CVSS severity
- `vulns[].affected[]`: Affected versions

### GitHub Advisory API
```bash
curl -H "Accept: application/vnd.github+json" \
  "https://api.github.com/advisories?affects=nghttp2"
```

### Fallback: Manual search
If APIs don't work, search in:
- https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=<dependency>
- https://security.snyk.io/package/

---

## Notes

- External APIs may have rate limits
- Cache CVE results to avoid repeated queries
- Critical/high severity CVEs should always be reported
- Verify that specific version is affected, not just the package
- This agent complements, does not replace, professional security audits
- Update critical dependency list as Envoy evolves
