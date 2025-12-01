# Sub-agent: Dependencies Check

## Purpose
Verify that dependency changes comply with Envoy's dependency policy.

## Activates When
There are changes in dependency files:
- `BUILD`
- `*.bzl`
- `bazel/repositories.bzl`
- `bazel/repository_locations.bzl`
- Files in `bazel/`

## Requires Docker: YES

## Main CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh deps' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-deps.log
```

## Verifications

### 1. New Dependency Documented - ERROR
If a new dependency is added, it must be documented:

```bash
git diff <base>...HEAD -- 'bazel/repository_locations.bzl' | grep -E '^\+'
```

**Verify:**
- Justification for the dependency
- Compatible license
- Active project maintenance

### 2. Dependency Policy - ERROR
According to DEPENDENCY_POLICY.md:

- Dependencies must have compatible license (Apache 2.0, MIT, BSD)
- No dependencies with restrictive licenses (GPL, LGPL without exception)
- Dependencies must be actively maintained

### 3. CVE/Vulnerabilities - WARNING
The deps command checks for known vulnerabilities:

**Look for in output:**
- `CVE-` followed by number
- `vulnerability`
- `security`

### 4. Version Pinning - WARNING
Dependencies must be pinned to specific versions:

```bash
git diff <base>...HEAD | grep -E 'version|sha256|commit'
```

### 5. Transitive Dependencies - INFO
Warn about new transitive dependencies being added.

## Quick Verification Without Docker

Before running Docker, verify basic changes:

```bash
# See which dependency files changed
git diff --name-only <base>...HEAD | grep -E '(BUILD|\.bzl|bazel/)'

# See new dependencies added
git diff <base>...HEAD -- 'bazel/repository_locations.bzl' | grep -E '^\+.*name.*='
```

## Output Format

```json
{
  "agent": "deps-check",
  "requires_docker": true,
  "docker_executed": true|false,
  "dependency_files_changed": ["bazel/repository_locations.bzl"],
  "findings": [
    {
      "type": "ERROR",
      "check": "new_dependency",
      "message": "New dependency without documented justification",
      "location": "bazel/repository_locations.bzl:123",
      "suggestion": "Document justification for 'new_lib' in PR description"
    },
    {
      "type": "WARNING",
      "check": "cve_detected",
      "message": "Dependency with known CVE",
      "location": "dependency_name:version",
      "suggestion": "Update to version without vulnerability"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "new_dependencies": [
    {
      "name": "new_lib",
      "version": "1.2.3",
      "license": "Apache-2.0"
    }
  ]
}
```

## Execution

1. Check if there are changes in dependency files:
```bash
git diff --name-only <base>...HEAD | grep -E '(BUILD|\.bzl|bazel/)'
```

2. If no changes, skip this agent

3. If there are changes:
   - Do quick analysis without Docker
   - Execute deps command with Docker
   - Parse output

4. Generate report

## Dependency Policy (Summary)

According to DEPENDENCY_POLICY.md:

### Allowed Licenses
- Apache 2.0
- MIT
- BSD (2-clause, 3-clause)
- ISC
- Zlib

### NOT Allowed Licenses
- GPL (any version)
- LGPL (without linking exception)
- AGPL
- Proprietary

### Quality Criteria
- Actively maintained project
- Reasonable community
- No known vulnerabilities without patch
- Reproducible builds

## Notes

- The deps command may take several minutes
- Verifies both direct and transitive dependencies
- CVE warnings should be investigated before merge
