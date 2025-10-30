# Generating compile_commands.json for Envoy

This guide explains how to generate `compile_commands.json` for Envoy using Hedron's Compile Commands Extractor with hermetic Python toolchain support.

## Overview

The `compile_commands.json` file enables full LSP/clangd support in IDEs and editors, providing features like:

- Accurate code completion
- Go-to-definition across the entire codebase
- Real-time error detection
- Symbol navigation
- Refactoring support

## Prerequisites

- Envoy repository cloned locally
- Bazel installed (compatible with Envoy's requirements)
- Git configured
- Clang toolchain configured (recommended - see Envoy's CLAUDE.md)

---

## Quick Start

For those familiar with the process:

```bash
# Clone Hedron into third_party
mkdir -p third_party
git clone https://github.com/hedronvision/bazel-compile-commands-extractor.git third_party/hedron_compile_commands
cd third_party/hedron_compile_commands
git checkout feature/optional-hermetic-python-support
cd ../..

# Configure WORKSPACE and BUILD (see detailed steps below)

# Generate compile_commands.json
bazel run //:compdb
```

---

## Detailed Setup Instructions

### Step 1: Clone Hedron with Hermetic Python Support

Envoy uses hermetic Python toolchains which require a specific branch of Hedron that includes workaround support.

```bash
# From Envoy repository root
mkdir -p third_party

# Clone Hedron
git clone https://github.com/hedronvision/bazel-compile-commands-extractor.git third_party/hedron_compile_commands

# Switch to the branch with hermetic Python support
cd third_party/hedron_compile_commands
git checkout feature/optional-hermetic-python-support
cd ../..
```

**Verification:**

```bash
cd third_party/hedron_compile_commands
git branch --show-current
# Should output: feature/optional-hermetic-python-support

git log --oneline -1
# Should show commit: 5840f1d Add optional hermetic Python workaround support
cd ../..
```

---

### Step 2: Configure WORKSPACE

Edit the `WORKSPACE` file at the repository root and add the following **at the top** (right after `workspace(name = "envoy")`):

```python
load("@bazel_tools//tools/build_defs/repo:local.bzl", "local_repository")

local_repository(
    name = "hedron_compile_commands",
    path = "third_party/hedron_compile_commands",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()

load("@hedron_compile_commands//:workspace_setup_transitive.bzl", "hedron_compile_commands_setup_transitive")
hedron_compile_commands_setup_transitive()

load("@hedron_compile_commands//:workspace_setup_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive()

load("@hedron_compile_commands//:workspace_setup_transitive_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive_transitive()
```

**Note:** Once Hedron's hermetic Python support is merged upstream, you can use `http_archive` instead of `local_repository` to fetch from GitHub directly.

---

### Step 3: Add Compilation Database Target

Edit the `BUILD` file at the repository root and add:

```python
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "compdb",
    targets = {
        "//source/...": "",
    },
    exclude_headers = "all",
    exclude_external_sources = True,
    use_hermetic_python_workaround = True,  # Required for Envoy's hermetic Python toolchain
)
```

**Important parameter explanations:**

- **`use_hermetic_python_workaround = True`**: **REQUIRED** for Envoy. This enables the workaround for hermetic Python toolchains like `@python3_12_host//:python`. Without this, generation will fail with rules_python placeholder expansion errors.

- **`exclude_headers = "all"`**: Significantly speeds up generation by not including header files. Recommended for faster iteration. Clangd can infer header compilation commands from source files.

- **`exclude_external_sources = True`**: Excludes external dependencies from the compilation database, reducing file size and generation time.

- **`targets`**: Specifies which targets to analyze. `"//source/..."` covers all Envoy source code. You can add `"//test/..."` if you need test coverage.

---

### Step 4: Configure .bazelignore

Create or edit `.bazelignore` at the repository root and add:

```
ci/build
build
```

This prevents Bazel from attempting to parse build output directories, which can cause conflicts.

---

### Step 5: Add CI Convenience Command

For easier invocation, you can add a command to `ci/do_ci.sh`:

```bash
hedron_compdb)
    echo "Running Hedron's Compile Commands Extractor (with hermetic Python workaround)..."
    setup_clang_toolchain
    echo "Executing: bazel run //:compdb (excludes headers for speed and stability)"
    bazel run "${BAZEL_BUILD_OPTIONS[@]}" //:compdb
    echo "Hedron execution completed"

    # Verify compile_commands.json was generated
    if [[ -f compile_commands.json ]]; then
        echo "✓ compile_commands.json generated successfully"
        ENTRY_COUNT=$(grep -c '"file":' compile_commands.json || echo "unknown")
        FILE_SIZE=$(du -h compile_commands.json | cut -f1)
        echo "  - File size: $FILE_SIZE"
        echo "  - Number of entries: $ENTRY_COUNT"
        echo "  - Location: $(pwd)/compile_commands.json"
    else
        echo "ERROR: compile_commands.json was not generated!"
        exit 1
    fi

    pkill clangd || :
    ;;
```

Add this case before the closing of the main `case` statement.

---

## Generating compile_commands.json

### Method 1: Using CI Script (Recommended)

```bash
ENVOY_DOCKER_BUILD_DIR=./build ./ci/run_envoy_docker.sh './ci/do_ci.sh hedron_compdb'
```

### Method 2: Inside Envoy's Docker Environment

```bash
# Start development container
./ci/run_envoy_docker.sh '/bin/bash'

# Inside the container
bazel run //:compdb

# Or with CI script
./ci/do_ci.sh hedron_compdb
```

**Expected output:**

- File `compile_commands.json` created at repository root
- Size: approximately 15-20 MB
- Entries: ~1200-1500 compilation units
- Generation time: 2-5 minutes (hardware dependent)

---

## Verification

```bash
# Check file exists and size
ls -lh compile_commands.json

# Count compilation entries
grep -c '"file":' compile_commands.json

# Preview first few entries
head -n 30 compile_commands.json

# Validate JSON syntax
python3 -m json.tool compile_commands.json > /dev/null && echo "Valid JSON"
```

---

## IDE/Editor Configuration

### Neovim with clangd

The `compile_commands.json` file should be automatically detected by clangd when placed at the repository root.

```bash
# Restart clangd to pick up new compilation database
pkill clangd

# Open an Envoy source file
nvim source/common/http/http1/codec_impl.cc
```

### Visual Studio Code

1. Install the `clangd` extension
2. The compilation database should be detected automatically
3. Restart VSCode if necessary

### CLion

CLion should auto-detect the file. If not:

1. **File → Settings → Build, Execution, Deployment → Compilation Database**
2. Point to `compile_commands.json` in the repository root

### Emacs with LSP Mode

```elisp
;; In your Emacs configuration
(use-package lsp-mode
  :hook (c++-mode . lsp)
  :config
  (setq lsp-clients-clangd-args '("--compile-commands-dir=." "--background-index")))
```

The compilation database will be detected automatically when opening C++ files.

---

## Troubleshooting

### Error: "rules_python placeholder expansion failed"

**Cause:** The `use_hermetic_python_workaround` parameter is not enabled in the BUILD file.

**Solution:** Verify that your `BUILD` file includes `use_hermetic_python_workaround = True` (see Step 3).

### No compile_commands.json generated

**Debug steps:**

```bash
# Verify Hedron is on correct branch
cd third_party/hedron_compile_commands
git branch --show-current
# Should show: feature/optional-hermetic-python-support

# Check that the wrapper template exists
ls -l refresh_wrapper.sh.template
# Should exist

# Clean and retry
cd ../../
bazel clean --expunge
bazel run //:compdb
```

### Compilation errors during generation

```bash
# Ensure Clang toolchain is configured
bazel/setup_clang.sh /path/to/clang

# Verify .bazelrc configuration
grep "build --config=clang" user.bazelrc

# If missing, add it
echo "build --config=clang" >> user.bazelrc
```

### File is empty or very small

**Check target configuration:**

```python
# In BUILD file, ensure targets are correct
targets = {
    "//source/...": "",  # Should include all source
},

# If needed, add more targets
targets = {
    "//source/...": "",
    "//test/...": "",  # Include tests if desired
},
```

### Performance issues / slow generation

**Optimization tips:**

1. Keep `exclude_headers = "all"` enabled (major speedup)
2. Use `exclude_external_sources = True` to skip dependencies
3. Specify minimal target set in `targets` parameter
4. Use `--config=opt` or `--config=fastbuild` if needed

```python
# Minimal configuration for fastest generation
refresh_compile_commands(
    name = "compdb",
    targets = {"//source/server:envoy": ""},  # Just main binary
    exclude_headers = "all",
    exclude_external_sources = True,
    use_hermetic_python_workaround = True,
)
```

---

## Updating the Compilation Database

After making significant changes to the codebase (new files, modified BUILD files, dependency changes), regenerate:

```bash
bazel run //:compdb
```

**Note:** You don't need to regenerate for every small change. Clangd can infer compilation commands for new files based on similar existing files.

---

## Git Integration

Consider adding to `.gitignore`:

```gitignore
# Generated compilation database
compile_commands.json
```

This prevents accidental commits of the generated file, which can be large and machine-specific.

---

## Advanced Configuration

### Including Test Targets

```python
refresh_compile_commands(
    name = "compdb",
    targets = {
        "//source/...": "",
        "//test/...": "",  # Add test coverage
    },
    exclude_headers = "all",
    exclude_external_sources = True,
    use_hermetic_python_workaround = True,
)
```

### Including Headers (slower, more complete)

```python
refresh_compile_commands(
    name = "compdb",
    targets = {"//source/...": ""},
    exclude_headers = "",  # Include all headers
    exclude_external_sources = True,
    use_hermetic_python_workaround = True,
)
```

**Warning:** This significantly increases generation time as Bazel must preprocess all sources to determine header dependencies.

### Including External Sources

```python
refresh_compile_commands(
    name = "compdb",
    targets = {"//source/...": ""},
    exclude_headers = "all",
    exclude_external_sources = False,  # Include externals
    use_hermetic_python_workaround = True,
)
```

Useful for navigating into third-party dependencies, but increases file size considerably.

---

## Technical Background

### Why the Hermetic Python Workaround?

Envoy uses Bazel's hermetic Python toolchains (e.g., `@python3_12_host//:python`) for reproducible builds. However, Hedron's default implementation uses `py_binary`, which relies on `rules_python`'s template placeholder expansion. This expansion fails with hermetic interpreters.

The workaround uses `sh_binary` with a bash wrapper (`refresh_wrapper.sh.template`) that:

1. Locates the Python script in Bazel's runfiles directory
2. Executes it directly without rules_python templating
3. Maintains full functionality while bypassing the incompatibility

This is a **temporary workaround** until rules_python provides better hermetic toolchain support.

### Related Hedron Issues

- [#165](https://github.com/hedronvision/bazel-compile-commands-extractor/issues/165): Hermetic Python support
- [#245](https://github.com/hedronvision/bazel-compile-commands-extractor/issues/245): rules_python compatibility
- [#168](https://github.com/hedronvision/bazel-compile-commands-extractor/issues/168): Placeholder expansion errors

---

## Future Improvements

Once [Hedron PR with hermetic Python support] is merged upstream:

1. Switch from `local_repository` to `http_archive` in WORKSPACE:

```python
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "hedron_compile_commands",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/<commit>.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-<commit>",
    # sha256 = "...",
)
```

2. Remove `third_party/hedron_compile_commands` directory
3. Continue using `use_hermetic_python_workaround = True` in BUILD

---

## References

- **Hedron Repository**: https://github.com/hedronvision/bazel-compile-commands-extractor
- **Feature Branch**: `feature/optional-hermetic-python-support`
- **Clangd Documentation**: https://clangd.llvm.org/
- **JSON Compilation Database Specification**: https://clang.llvm.org/docs/JSONCompilationDatabase.html
- **Envoy Build Documentation**: See `CLAUDE.md` and `DEVELOPER.md` in this repository

---

## Summary

**Key points to remember:**

1. ✅ Use the `feature/optional-hermetic-python-support` branch of Hedron
2. ✅ Always set `use_hermetic_python_workaround = True` in Envoy's BUILD
3. ✅ Keep `exclude_headers = "all"` for best performance
4. ✅ Regenerate after significant codebase changes
5. ✅ Add `compile_commands.json` to `.gitignore`

With these steps, you'll have full LSP/clangd support for Envoy development!
