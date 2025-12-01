# Sub-agent: Documentation and Changelog Review

## Purpose
Verify that documentation and release notes changes meet Envoy requirements.

## ACTION: ALWAYS EXECUTE if there are changes in source/ or api/ (instant git commands, no Docker)

## Expected Input
- List of modified files
- Detected change type (user-facing, API, extension, etc.)

## Verifications

### 1. Release Notes Updated (conditional ERROR)

**When it's ERROR:**
- User-facing change (modifies visible behavior)
- API change (files in `api/envoy/`)
- New feature or extension
- Important bug fix
- Deprecation

**Verify:**
```bash
git diff --name-only <base>...HEAD | grep -q "changelogs/current.yaml"
```

**If missing and should exist:**
```
ERROR: Release notes not updated for user-facing change
Location: changelogs/current.yaml
Suggestion: Add entry in appropriate section:
- behavior_changes: For incompatible behavior changes
- minor_behavior_changes: For minor behavior changes
- bug_fixes: For bug fixes
- removed_config_or_runtime: For removed configuration
- new_features: For new functionality
- deprecated: For deprecations
```

### 2. Release Notes Format (WARNING)

If there are changes in `changelogs/current.yaml`, verify:

**Correct structure:**
```yaml
- area: subsystem
  change: |
    Description of change with appropriate :ref:`links`.
    May mention runtime guard if applicable.
```

**Verifications:**
- `area:` present and valid
- `change:` present and not empty
- Links in RST format `:ref:\`text <reference>\``

### 3. Documentation for Behavior Changes (WARNING)

**If there are changes in `source/` that affect behavior:**
- Verify if there are corresponding changes in `docs/`
- Especially for new features or API changes

```bash
# Verify if there are changes in docs
git diff --name-only <base>...HEAD | grep -q "^docs/"
```

### 4. Breaking Changes Documented (ERROR)

**If there are deprecations:**
- Verify entry in `deprecated` section of changelog
- Verify that alternative is documented

```bash
# Search in diff for deprecated
git diff <base>...HEAD | grep -i "deprecated"
```

### 5. Runtime Guard Documented (WARNING)

**If there's a new runtime guard:**
- Must be documented in release notes
- Must explain how to revert behavior

**Search in diff:**
```bash
git diff <base>...HEAD | grep -E "reloadable_features\.|envoy\.reloadable_features"
```

### 6. Grammar and Punctuation (INFO)

**In modified documentation files:**
- Verify correct English usage
- One space after period
- No obvious typos

**Note:** This is a heuristic verification, not exhaustive.

## User-Facing Change Detection

A change is user-facing if:
1. Modifies files in `api/envoy/` (API change)
2. Adds/modifies extensions in `source/extensions/`
3. Contains "runtime guard" or "reloadable_feature"
4. Modifies network/HTTP/routing behavior
5. Adds new statistics or metrics
6. Modifies configuration

## Output Format

```json
{
  "agent": "docs-changelog",
  "findings": [
    {
      "type": "ERROR|WARNING|INFO",
      "check": "release_notes",
      "message": "Release notes not updated for user-facing change",
      "location": "changelogs/current.yaml",
      "suggestion": "Add entry describing: [change description]"
    }
  ],
  "summary": {
    "errors": N,
    "warnings": N,
    "info": N
  },
  "change_classification": {
    "is_user_facing": true|false,
    "is_breaking": true|false,
    "has_runtime_guard": true|false,
    "affected_areas": ["api", "extensions", ...]
  }
}
```

## Execution

1. Get modified files:
```bash
git diff --name-only <base>...HEAD
```

2. Classify change type

3. Verify changelog:
```bash
git diff <base>...HEAD -- changelogs/current.yaml
```

4. If user-facing and no changelog, ERROR

5. If there's changelog, verify format

6. Search for undocumented runtime guards

7. Generate report
