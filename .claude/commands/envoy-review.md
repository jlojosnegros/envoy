---
description: Review code changes with Envoy-specific standards and policies
---

Please perform a comprehensive code review of my recent changes using the **Envoy Code Reviewer** agent.

# Review Scope

Analyze all changes in my current branch compared to `main` and verify compliance with Envoy's contribution standards:

## Required Checks

1. **Test Coverage** - Verify 100% coverage for all new/modified code
2. **Release Notes** - Check if `changelogs/current.yaml` updated for user-facing changes
3. **Code Style** - Validate compliance with STYLE.md (naming, patterns, error handling)
4. **Build System** - Verify BUILD files, extension registration, dependencies
5. **Breaking Changes** - Detect and validate deprecation policy compliance
6. **Documentation** - Check API docs, user docs, code comments
7. **Test Quality** - Verify hermetic, deterministic tests with proper coverage

## Analysis Process

Please:

1. Identify all modified files (source, tests, build, docs)
2. For each source file, verify corresponding test file exists
3. Check test coverage is complete (including error paths)
4. Validate code style and Envoy-specific patterns
5. Run format checks and build verification
6. Check for common issues (missing runtime guards, thread safety, etc.)
7. Generate a detailed report with:
   - ❌ Critical issues (must fix before merge)
   - ⚠️ Warnings (should fix)
   - 💡 Suggestions (consider)
   - ✅ Passing checks

## Output Format

Provide:

- Executive summary of changes
- Detailed issue list with file:line references
- Specific, actionable fixes for each issue
- Commands to run for verification
- Checklist of action items

## Focus Areas

Pay special attention to:

- Coverage gaps (any untested code paths)
- Missing release notes for user-visible changes
- Breaking changes without proper deprecation
- Shared pointers where unique_ptr would work
- Missing thread safety annotations
- Direct time() calls instead of TimeSystem
- Missing runtime guards for behavioral changes

## Reference Documents

Use these for verification:

- `CONTRIBUTING.md` - Contribution guidelines
- `STYLE.md` - Code style requirements
- `CLAUDE.md` - Development best practices
- `bazel/README.md` - Build system docs

---

**Note:** This command invokes the specialized code-reviewer agent with deep knowledge of Envoy's development policies and patterns.
