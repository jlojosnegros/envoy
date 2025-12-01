# Sub-agent: Extension Review

## Purpose
Verify that extension changes comply with Envoy's extension policy.

## Activates When
There are changes in:
- `source/extensions/`
- `contrib/`
- `api/envoy/extensions/`
- `api/contrib/`

## Requires Docker: NO (static analysis)

## Verifications

### 1. New Extension - Sponsor (INFO)
If it's a new extension, verify sponsor:

**Detect new extension:**
```bash
git diff --name-only --diff-filter=A <base>...HEAD | grep -E 'source/extensions/.*/[^/]+\.(cc|h)$'
```

**If new:**
```
INFO: New extension detected
According to EXTENSION_POLICY.md, new extensions require:
- Sponsor: An existing maintainer to sponsor the extension
- Reviewers: At least 2 reviewers for the code
Please make sure you have sponsor and reviewers before the PR.
```

### 2. CODEOWNERS Updated - WARNING
If there's a new extension, CODEOWNERS must be updated:

```bash
git diff <base>...HEAD -- CODEOWNERS | grep -E 'extensions|contrib'
```

**If new extension without CODEOWNERS entry:**
```
WARNING: New extension without CODEOWNERS entry
Location: source/extensions/filters/http/new_filter/
Suggestion: Add entry in CODEOWNERS:
/source/extensions/filters/http/new_filter/ @reviewer1 @reviewer2
```

### 3. Security Posture Tag - WARNING
Verify that envoy_cc_extension has security_posture:

```bash
git diff <base>...HEAD -- 'source/extensions/**/BUILD' | grep -E 'envoy_cc_extension|security_posture'
```

**Valid tags:**
- `robust_to_untrusted_downstream`
- `robust_to_untrusted_downstream_and_upstream`
- `requires_trusted_downstream_and_upstream`
- `unknown`
- `data_plane_agnostic`

### 4. Status Tag - INFO
Verify it has status tag:

```bash
git diff <base>...HEAD -- 'source/extensions/**/BUILD' | grep -E 'status\s*='
```

**Valid tags:**
- `stable`
- `alpha`
- `wip`

### 5. Metadata in extensions_metadata.yaml - WARNING
For contrib extensions, verify metadata entry:

```bash
git diff <base>...HEAD -- 'contrib/extensions_metadata.yaml'
```

### 6. Contrib vs Core Extension - INFO
If it's contrib, warn about differences:

```
INFO: This is a contrib extension
- Requires end-user sponsor
- NOT included in default Docker image
- Does NOT have Envoy security team coverage
- Should use v3alpha for API
```

### 7. Platform Specific Features - WARNING
If the extension uses platform-specific features:

```bash
git diff <base>...HEAD | grep -E '#ifdef|#if defined|__linux__|__APPLE__|_WIN32'
```

**If platform-specific code detected:**
```
WARNING: Platform-specific code detected
According to EXTENSION_POLICY.md:
- Avoid #ifdef <OSNAME>
- Prefer feature guards in build system
- Add to *_SKIP_TARGETS in bazel/repositories.bzl if some platform not supported
```

## BUILD File Verification

For new extensions, verify correct BUILD structure:

```python
envoy_cc_extension(
    name = "config",
    # ... deps ...
    security_posture = "robust_to_untrusted_downstream",
    status = "alpha",  # or stable, wip
)
```

## Output Format

```json
{
  "agent": "extension-review",
  "requires_docker": false,
  "extension_type": "core|contrib|new",
  "extension_paths": ["source/extensions/filters/http/new_filter/"],
  "findings": [
    {
      "type": "WARNING",
      "check": "codeowners_missing",
      "message": "New extension without CODEOWNERS entry",
      "location": "source/extensions/filters/http/new_filter/",
      "suggestion": "Add reviewers in CODEOWNERS for this extension"
    },
    {
      "type": "WARNING",
      "check": "security_posture_missing",
      "message": "Missing security_posture in envoy_cc_extension",
      "location": "source/extensions/filters/http/new_filter/BUILD",
      "suggestion": "Add appropriate security_posture based on threat model"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "extension_info": {
    "is_new": true|false,
    "is_contrib": true|false,
    "has_sponsor": "unknown",
    "security_posture": "robust_to_untrusted_downstream|...|missing",
    "status": "stable|alpha|wip|missing"
  }
}
```

## Execution

1. Identify extension changes:
```bash
git diff --name-only <base>...HEAD | grep -E '(source/extensions|contrib)'
```

2. Determine if it's new extension or modification

3. If new:
   - Inform about sponsor requirement
   - Verify CODEOWNERS

4. Verify BUILD file:
   - security_posture present
   - status present

5. Verify if it's contrib:
   - extensions_metadata.yaml updated
   - Inform about differences

6. Search for platform-specific code

7. Generate report

## References

- EXTENSION_POLICY.md: Complete policy
- CODEOWNERS: Reviewer assignment
- extensions_metadata.yaml (contrib): Contrib metadata
