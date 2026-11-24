# Release Process

This document describes the complete release process for the Modest IoT Nano Framework, from preparation to registry publishing.

## Table of Contents

- [Overview](#overview)
- [Release Workflow](#release-workflow)
- [Git Flow Helper Plugin Process](#git-flow-helper-plugin-process)
- [Automated Release Workflow](#automated-release-workflow)
- [Documentation Alignment](#documentation-alignment)
- [Pre-Release Checklist](#pre-release-checklist)
- [Version Bump](#version-bump)
- [Registry Publishing](#registry-publishing)
- [Post-Release](#post-release)

## Overview

The Modest IoT Nano Framework uses [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/) with the **Git Flow Helper Plugin** in CLion for release management. The release process combines manual Git Flow steps with automated GitHub Actions workflows. Validation runs on the release branch via existing CI workflows, and a tag-triggered workflow generates documentation and creates the GitHub Release.

### Release Architecture

```
Git Flow Helper Plugin                    GitHub Actions CI (on release/* push)
─────────────────────────                 ───────────────────────────────────
                                         
1. Release Start                          (Existing workflows validate)
   - Create release branch               - Host tests (53 suites)
   - Specify version tag                 - Code coverage (gcov/lcov)
                                         
2. Release Publish                        - Compile examples (28 × 4 board/core combinations)
   - Push release branch to remote       - Lint (strict compliance)
   → Triggers CI validation              - Dependency status (weekly)
                                        
3. Manual merge:                          (All must pass before merging)
   - git checkout main
   - git merge release/vX.X.X
   - git checkout develop
   - git merge release/vX.X.X
                                        
4. Release Finish                         GitHub Actions Release (on tag push)
   - Close and delete branch             ───────────────────────────────────
   - Create tag vX.X.X                   - Generate Doxygen HTML
   - Push tag to remote                  - Create GitHub Release
                                          - Upload ZIP + Doxygen artifacts
                                         
5. Publish to PlatformIO                 (Manual step after tag workflow)
   - pio pkg publish                     - Library available on PlatformIO Registry
```

## Git Flow Helper Plugin Process

The Git Flow Helper Plugin in CLion provides the following release commands:

| Command | Description | When |
|---------|-------------|------|
| **Release Start** | Creates the release branch from `develop` and specifies the version tag | Begin release preparation |
| **Release Publish** | Pushes the release branch to the remote (or creates it on first publish) | After finalizing changes |
| **Manual merge** | Merge the release branch into `main` and `develop` | After Release Publish |
| **Release Finish** | Closes the release branch, creates the tag, and deletes the branch | Final step — triggers automated workflow |

### Step-by-Step

1. **Release Start**
   - In CLion: `Git → Git Flow → Release Start`
   - Specify the version tag (e.g., `v1.3.0`)
   - This creates the `release/v1.3.0` branch from `develop`

2. **Prepare the Release**
   - Update version numbers (see [Version Bump](#version-bump))
   - Update `CHANGELOG.md` — move `[Unreleased]` section to the new version
   - Review documentation alignment (see [Documentation Alignment](#documentation-alignment))
   - Verify all tests pass: `bash tests/run_all.sh`
   - Verify Doxygen generates: `doxygen docs/Doxyfile`
   - Commit all changes

3. **Release Publish**
   - In CLion: `Git → Git Flow → Release Publish`
   - This pushes the release branch to the remote

4. **Manual Merge**
   ```bash
   # Merge into main
   git checkout main
   git merge --no-ff release/v1.3.0
   
   # Merge into develop
   git checkout develop
   git merge --no-ff release/v1.3.0
   
   # Push both branches
   git push origin main develop
   ```

5. **Release Finish**
   - In CLion: `Git → Git Flow → Release Finish`
   - Select the release branch (`release/v1.3.0`)
   - This closes the branch, creates the tag (`v1.3.0`), and deletes the branch
   - **The tag push triggers the automated Release workflow**

6. **Publish to PlatformIO Registry**
   - Wait for the Release workflow to complete (Doxygen + GitHub Release)
   - Publish to PlatformIO Registry (see [Registry Publishing](#registry-publishing))
   - Verify the new version is available on both registries

## Automated Release Workflow

The release process uses two phases of automated checks:

### Phase 1: Release Branch Validation (existing CI)

When you push the release branch (step 2: Release Publish), the existing CI workflows automatically run on the `release/*` branch:

| Workflow | Purpose |
|----------|---------|
| **Host Unit Tests** | Runs all 53 test suites with AddressSanitizer and UBSan (Ubuntu + macOS) |
| **Code Coverage** | Generates gcov/lcov coverage report (80% threshold) |
| **Compile Examples** | Compiles all 28 examples for esp32 and esp32s3 on ESP32 Arduino cores 3.3.13 and 4.0.0 |
| **Lint** | Validates library metadata with strict compliance |

**Important:** Verify all CI workflows pass on the release branch before proceeding to the manual merge step. If any job fails, fix the issues on the release branch and push again.

### Phase 2: Tag Release (release.yml)

When the tag is pushed by **Release Finish**, the `.github/workflows/release.yml` workflow runs automatically. It assumes the validation checks from Phase 1 passed, and focuses on generating documentation and creating the release:

| Job | Purpose | Dependencies |
|-----|---------|--------------|
| **doxygen** | Generates HTML API reference using Doxyfile | None |
| **release** | Creates GitHub Release with changelog and artifacts | doxygen |

### Job Flow

```
Tag push (v1.3.0)
  │
  ├──→ doxygen ──→ release
  │    (HTML)      (GitHub Release)
  │
  └──→ (Validation already passed on release branch via existing CI)
```

### Artifacts

The release workflow creates two artifacts:

| Artifact | Description | Retention |
|----------|-------------|-----------|
| **Modest-IoT-Nano-Framework.zip** | Library source ZIP following `library.json` export rules | Permanent (GitHub Release asset) |
| **doxygen-html.zip** | Complete HTML API reference with Doxygen Awesome theme | Permanent (GitHub Release asset) |
| **coverage-report** | gcov/lcov HTML coverage report | 14 days (workflow artifact) |

## Documentation Alignment

Before finalizing the release, review all documentation for effects of the changes being released. A version bump alone is not enough — decisions made during the release (e.g., API migration, dependency removal, new features) may affect other documentation files.

### What to Review

| Document | Review for |
|----------|-----------|
| **docs/adrs.md** | Stale references to removed dependencies or old APIs; mark superseded ADRs if decisions were reversed; record new architectural decisions where warranted and verify the ADR index and count |
| **docs/class-diagram.puml** | Current classes, APIs, member types, relationships, ownership, and cardinality match the codebase |
| **Doxygen API documentation** (`docs/doxygen-mainpage.md` and public headers) | Documented behavior, API details, and module grouping match the current implementation; generated reference builds successfully |
| **CONTRIBUTING.md** | Dependency changes, version management updates, bug report template |
| **docs/quick-start.md** | Minimum version requirements, hardware prerequisites, BLE simulation limits |
| **docs/dependencies.md** | Added/removed dependencies, monitoring changes |
| **docs/evolution.md** | New W-items or resolutions that should be documented |
| **docs/improvements.md** | Comparison tables, current-state accuracy |
| **docs/user-stories.md** | Implementation status and method name updates; verify acceptance criteria against current API behavior and constraints |
| **docs/troubleshooting.md** | New error patterns, platform-specific issues |
| **docs/coverage.md** | Coverage number changes, new exclusions |
| **docs/benchmarks.md** | Version, date, platform; new metrics or comparison data; distinguish newly measured values from carried-forward estimates |

### Common Triggers

- **API migration** (e.g., ESP32 4.0.0 BLE): Check class diagram, ADRs, quick-start, CONTRIBUTING.md
- **Dependency removal** (e.g., ArduinoJson): Check ADRs, CONTRIBUTING.md, SECURITY.md, dependencies.md
- **New feature** (e.g., EventAggregator): Check class diagram, user stories, evolution.md, improvements.md
- **CI changes** (e.g., Node.js 24): Check benchmarks.md, dependencies.md
- **Coverage changes**: Check coverage.md, benchmarks.md

### Review Process

1. Identify the changes in the release branch
2. For each change, determine which documentation files reference the affected code
3. Update the documentation to reflect the current state
4. Mark historical ADRs as superseded if decisions were reversed
5. Ensure all version references are consistent

## Pre-Release Checklist

Before starting the release process, verify:

- [ ] All feature branches are merged into `develop`
- [ ] `CHANGELOG.md` `[Unreleased]` section is complete
- [ ] All CI workflows pass on `develop` (host tests, coverage, compile examples, lint)
- [ ] `library.properties` version is updated
- [ ] `library.json` version is updated
- [ ] `docs/Doxyfile` `PROJECT_NUMBER` is updated
- [ ] `CMakeLists.txt` `VERSION` is updated
- [ ] Host test entry points (`tests/run_all.sh`, `tests/run_coverage.sh`, and CMake/CTest) execute the same complete suite set and report a consistent suite count
- [ ] `README.md` `**Version**` is updated
- [ ] `README.md` `**Last updated**` date is updated
- [ ] `SECURITY.md` Supported Versions table is updated
- [ ] `docs/doxygen-mainpage.md` version reference is updated
- [ ] Doxygen API documentation is aligned with current codebase behavior and `doxygen docs/Doxyfile` succeeds
- [ ] `docs/benchmarks.md` version, date, and platform are updated
- [ ] `docs/class-diagram.puml` matches the current codebase structure and relationships
- [ ] `keywords.txt` entries match the current public API
- [ ] Documentation alignment review is complete (see [Documentation Alignment](#documentation-alignment))
- [ ] No blocking issues or open pull requests
- [ ] Dependency status badges are current (check README badges)

**After pushing the release branch (Release Publish), verify:**

- [ ] All CI workflows pass on the `release/*` branch
- [ ] Host tests: 53 suites pass on Ubuntu and macOS
- [ ] Code coverage: 80% threshold met
- [ ] Compile examples: All 28 examples compile for esp32 and esp32s3 on ESP32 Arduino cores 3.3.13 and 4.0.0
- [ ] Lint: Strict compliance passes

**Only proceed to the manual merge step after all CI checks pass on the release branch.**

## Version Bump

The version is managed in four core files, with five documentation references. Update all nine simultaneously during the release preparation phase:

**Core version files:**

| File | Field | Example |
|------|-------|---------|
| `library.properties` | `version=1.3.0` | Arduino Library Manager |
| `library.json` | `"version": "1.3.0"` | PlatformIO Registry |
| `docs/Doxyfile` | `PROJECT_NUMBER = 1.3.0` | Doxygen API reference |
| `CMakeLists.txt` | `VERSION 1.3.0` | CMake host test builds |

**Documentation version references:**

| File | Field | Example |
|------|-------|---------|
| `README.md` | `**Version**: 1.3.0` | Front matter version |
| `README.md` | `**Last updated**: 2026-11-24` | Front matter date |
| `SECURITY.md` | Supported Versions table | Current version row |
| `docs/doxygen-mainpage.md` | `(v1.3.0)` in first paragraph | API reference version |
| `docs/benchmarks.md` | `**Version**: 1.3.0` | Benchmark table version |
| `docs/benchmarks.md` | `**Date**: 2026-11-24` | Benchmark table date |
| `docs/benchmarks.md` | `**Platform**: ESP32 (Arduino core 4.0.0)` | Benchmark table platform |

### Version Bump Rules

Follow [Semantic Versioning 2.0.0](https://semver.org/):

| Version Part | When to Bump | Example |
|-------------|--------------|---------|
| **MAJOR** | Incompatible API changes | `1.0.0` → `2.0.0` |
| **MINOR** | Backward-compatible features | `1.0.0` → `1.1.0` |
| **PATCH** | Backward-compatible bug fixes | `1.0.0` → `1.0.1` |

After updating the version files, commit:

```bash
git add library.properties library.json docs/Doxyfile CMakeLists.txt README.md SECURITY.md docs/doxygen-mainpage.md docs/benchmarks.md
git commit -m "chore(release): bump version to 1.3.0"
```

## Registry Publishing

After the Release workflow completes (Doxygen + GitHub Release), publish to both registries:

| Registry | Automation | Timing |
|----------|-----------|--------|
| **Arduino Library Manager** | Fully automatic | Within ~1 hour after tag push |
| **PlatformIO Registry** | **Manual step required** | After Release workflow completes |

### Arduino Library Manager

**Fully automatic.** The Arduino Library Manager indexer checks repositories hourly for new tags. After the tag is pushed:

1. The Arduino indexer detects the new tag (`v1.3.0`)
2. The library is validated against the Arduino Library Manager requirements
3. The new version appears in the Library Manager within ~1 hour

**No manual action needed** for updates — the library is already approved since v1.1.2.

### PlatformIO Registry

**Manual step required.** PlatformIO does not auto-index GitHub tags. You must explicitly publish after the Release workflow completes:

#### Option 1: Manual Publish (recommended)

After the GitHub Release is created:

```bash
# Install PlatformIO CLI if not already installed
pip install platformio

# Login to your PlatformIO account
pio account login

# Publish the library
cd /path/to/Modest-IoT-Nano-Framework
pio pkg publish --owner modest-iot
```

> **Note**: The package is currently published under `avelasquezn` (personal account). To migrate to the `modest-iot` organization, contact `support@platformio.org` before the next release.

#### Option 2: Automated with Token

To automate PlatformIO publishing in CI:

1. Generate a personal access token: [PlatformIO Account Settings](https://account.platformio.org/)
2. Add the token as a repository secret: `PLATFORMIO_ACCOUNT_TOKEN`
3. Add a job to the release workflow:

```yaml
- name: Publish to PlatformIO
  env:
    PLATFORMIO_ACCOUNT_TOKEN: ${{ secrets.PLATFORMIO_ACCOUNT_TOKEN }}
  run: |
    pip install platformio
    pio account login --token $PLATFORMIO_ACCOUNT_TOKEN
    pio pkg publish --owner modest-iot
```

## Post-Release

After the release is complete:

1. **Publish to PlatformIO Registry** (required): Follow the steps in [PlatformIO Registry](#platformio-registry) above. This is the only manual registry step — Arduino Library Manager updates automatically.
2. **Verify Arduino Library Manager**: Check that the new version appears in the Library Manager within ~1 hour.
3. **Verify PlatformIO Registry**: Check that the new version is available at [registry.platformio.org](https://registry.platformio.org/libraries/modest-iot/Modest-IoT-Nano-Framework).
4. **Update CHANGELOG.md**: Add a new `[Unreleased]` section at the top for future changes.
5. **Bump develop version**: Update `library.properties`, `library.json`, `docs/Doxyfile`, `CMakeLists.txt`, `README.md`, `SECURITY.md`, `docs/doxygen-mainpage.md`, and `docs/benchmarks.md` to the next development version (e.g., `1.3.0-dev`).
6. **Announce the release**: Update any relevant communication channels (GitHub Discussions, social media, etc.).

### Post-Release Version Bump on develop

After the release, update `develop` to the next development version:

```bash
git checkout develop
# Update version to 1.3.0-dev in library.properties, library.json, docs/Doxyfile, CMakeLists.txt, README.md, SECURITY.md, docs/doxygen-mainpage.md, and docs/benchmarks.md
git add library.properties library.json docs/Doxyfile CMakeLists.txt README.md SECURITY.md docs/doxygen-mainpage.md docs/benchmarks.md
git commit -m "chore: bump develop version to 1.3.0-dev"
git push origin develop
```

## Troubleshooting

### Release branch CI fails

- Check the GitHub Actions run log for the specific workflow that failed on the `release/*` branch.
- Fix the issues on the release branch and push again: `git push origin release/v1.3.0`
- **Do not merge to `main` or `develop` until all CI checks pass on the release branch.**
- If host tests fail, fix the tests and push the fix to the release branch.
- If example compilation fails, verify the example compiles locally with `arduino-cli`.
- If lint fails, check the [Arduino Library Manager requirements](https://github.com/arduino/arduino-lint-action).

### Release workflow (tag) fails

- The tag workflow only generates Doxygen and creates the GitHub Release.
- If Doxygen generation fails, check the `docs/Doxyfile` configuration and ensure all source files are properly documented.
- If the GitHub Release creation fails, verify the `GITHUB_TOKEN` has `contents: write` permissions.

### Arduino Library Manager doesn't pick up the tag

- The Arduino indexer runs hourly. Wait up to 1 hour.
- Verify the tag exists: `git ls-remote --tags origin`
- Check the [Arduino Library Manager status](https://github.com/arduino/Arduino-Library-Index).

### PlatformIO publish fails

- Verify you are logged in: `pio account info`
- Check that the package owner matches your account.
- For migration from `avelasquezn` to `modest-iot`, contact `support@platformio.org`.
