# Sub-agent: API Review

## Purpose
Verify that API changes comply with the Envoy review checklist.

## Activates When
There are changes in the `api/` directory

## ACTION:
- **Phase 1 (Static analysis)**: ALWAYS EXECUTE (instant git/grep commands)
- **Phase 2 (api_compat)**: ALWAYS EXECUTE if there are changes in api/ (takes 5-15 min with Docker)

## Requires Docker: YES (for api_compat)

## Verifications

### Phase 1: Static Analysis (Without Docker)

#### 1.1 Default Values Safe - ERROR
Verify that default values don't cause behavior changes:

```bash
git diff <base>...HEAD -- 'api/**/*.proto' | grep -E 'default|= [0-9]|= true|= false'
```

**Questions to consider:**
- Will default values cause changes for existing users?
- Is a runtime guard needed?

#### 1.2 Validation Rules - WARNING
Verify presence of protoc-gen-validate rules:

```bash
git diff <base>...HEAD -- 'api/**/*.proto' | grep -E '\[(validate\.|rules)'
```

**Fields to verify:**
- Numeric fields have bounds
- Required fields are marked
- Repeated fields have min/max

#### 1.3 Deprecation Documentation - ERROR
If there are deprecated fields, verify documentation:

```bash
git diff <base>...HEAD -- 'api/**/*.proto' | grep -i 'deprecated'
```

**If there's deprecated:**
- Alternative must be documented
- Must be in release notes
- No deprecated without ready alternative

#### 1.4 Style Compliance - WARNING
Verify compliance with api/STYLE.md:

- Field names in snake_case
- Message names in PascalCase
- Documentation comments present
- Correct usage of WKT (Well-Known Types)

#### 1.5 Extension Point Usage - INFO
Verify if TypedExtensionConfig should be used:

```bash
git diff <base>...HEAD -- 'api/**/*.proto' | grep -E 'oneof|Any|typed_config'
```

**Consider:**
- Is this new extensible functionality?
- Should it be a plugin instead of core code?

### Phase 2: Breaking Changes (With Docker)

#### CI Command
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh api_compat' 2>&1 | tee ${ENVOY_DOCKER_BUILD_DIR}/review-agent-logs/YYYYMMDDHHMM-api-compat.log
```

**This command detects:**
- Renumbered fields
- Changed types
- Renamed fields
- Breaking changes in wire format

## Checklist from api/review_checklist.md

### Feature Enablement
- [ ] Do default values cause behavior changes?
- [ ] Can users disable the change?
- [ ] Does it need WKT for values that might change?

### Validation Rules
- [ ] Does it have validate rules?
- [ ] Is required field marked as required?
- [ ] Do numeric fields have bounds?

### Deprecations
- [ ] Is alternative available in known clients?
- [ ] Does documentation point to replacement?

### Extensibility
- [ ] Should it be an extension point?
- [ ] Should enum be oneof with empty messages?

### Consistency
- [ ] Does it reuse existing types where possible?
- [ ] Are names consistent with existing API?

### Failure Modes
- [ ] Is failure mode documented?
- [ ] Is behavior consistent across clients?

### Documentation
- [ ] Are proto comments clear?
- [ ] Are there examples where useful?

## Output Format

```json
{
  "agent": "api-review",
  "requires_docker": true,
  "docker_executed": true|false,
  "api_files_changed": ["api/envoy/config/foo.proto"],
  "findings": [
    {
      "type": "ERROR",
      "check": "breaking_change",
      "message": "Renamed field causes breaking change",
      "location": "api/envoy/config/foo.proto:45",
      "suggestion": "Don't rename existing fields, add new field and deprecate old one"
    },
    {
      "type": "WARNING",
      "check": "validation_missing",
      "message": "Numeric field without bounds validation",
      "location": "api/envoy/config/foo.proto:67",
      "suggestion": "Add [(validate.rules).uint32 = {gte: 0, lte: 100}]"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "checklist_status": {
    "feature_enablement": "PASS|WARN|FAIL",
    "validation_rules": "PASS|WARN|FAIL",
    "deprecations": "PASS|WARN|FAIL|N/A",
    "extensibility": "PASS|WARN|FAIL",
    "consistency": "PASS|WARN|FAIL"
  }
}
```

## Execution

### Phase 1 (Without Docker - ALWAYS):

1. Identify modified proto files:
```bash
git diff --name-only <base>...HEAD | grep '\.proto$'
```

2. Analyze diff for each static verification

3. Review checklist

### Phase 2 (With Docker):

1. Verify ENVOY_DOCKER_BUILD_DIR

2. Execute api_compat:
```bash
ENVOY_DOCKER_BUILD_DIR=<dir> ./ci/run_envoy_docker.sh './ci/do_ci.sh api_compat'
```

3. Parse output for breaking changes

4. Combine with Phase 1 results

## Notes

- api_compat compares against base commit to detect breaking changes
- Backwards compatibility rules are strict in Envoy
- Proto changes can affect multiple xDS clients
