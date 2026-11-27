# Dependencies

This document lists all external dependencies of the Modest IoT Nano Framework and how they are monitored for updates.

## Dependency Summary

| Dependency | Type | Version | Monitored By |
|------------|------|---------|--------------|
| **MQTT** | Arduino Library | ≥2.5.3 | Automated (weekly status badge) |
| **ESP32 Arduino Core** | Platform Core | Latest; BLE HAL supports core 3.x and 4.x | Automated (weekly status badge) |
| **GitHub Actions** | CI/CD | Various | Dependabot (automated) |

### Status Badges

The README displays two dynamic shields.io badges that are updated weekly by the [Dependency Status](../../.github/workflows/dependency-status.yml) workflow:

- **MQTT badge**: Shows the current minimum version. Turns orange if a newer release is available on GitHub.
- **ESP32 badge**: Shows the latest ESP32 Arduino core release version. CI always installs the latest, so this is informational.

Badges are stored as JSON files in `.badges/` and updated automatically by the workflow.

## External Arduino Dependencies

### MQTT Library (≥2.5.3)

The only required external Arduino library dependency after W46 (ArduinoJson removal).

- **Repository**: [256dpi/arduino-mqtt](https://github.com/256dpi/arduino-mqtt)
- **Used in**: `library.properties` (depends field), `library.json` (dependencies), `compile-examples.yml` (CI install)
- **Minimum version**: 2.5.3
- **Update process**: Manual — check releases page periodically, update all 3 files simultaneously

On-device inference is also available through `EmlearnController`. Generated-model inference uses application-provided C code and does not require an additional package; tree and neural-network model APIs can link emlearn when needed. emlearn is optional and is not a required Arduino Library Manager dependency.

To update the MQTT library version:

1. Check [MQTT releases](https://github.com/256dpi/arduino-mqtt/releases) for the latest version.
2. Update `library.properties`:
   ```properties
   depends=MQTT (>=X.Y.Z)
   ```
3. Update `library.json`:
   ```json
   "dependencies": {
     "MQTT": ">=X.Y.Z"
   }
   ```
4. Update `.github/workflows/compile-examples.yml`:
   ```yaml
   arduino-cli lib install "MQTT@X.Y.Z"
   ```
5. Create a feature branch, run CI, and merge.

### ESP32 Arduino Core

The example-build workflow uses an explicit ESP32 Arduino core matrix rather than installing the latest version.

- **Repository**: [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
- **Current matrix**: Arduino-ESP32 3.3.13 and 4.0.0, each compiled for ESP32 and ESP32-S3 in `.github/workflows/compile-examples.yml`.
- **Update process**: Change the workflow matrix intentionally when adding or retiring a supported core version; this matrix does not imply compatibility with every upstream release.

## GitHub Actions Dependencies

GitHub Actions are monitored automatically by Dependabot (`.github/dependabot.yml`). When a new version is available, Dependabot opens a pull request with the version bump.

| Action | Current Version | Workflows |
|--------|-----------------|-----------|
| `actions/checkout` | v4 | All workflows |
| `actions/upload-artifact` | v4 | host-tests.yml (coverage) |
| `arduino/arduino-lint-action` | v1 | lint.yml |
| `arduino/setup-arduino-cli` | v1 | compile-examples.yml |

Dependabot checks for updates monthly. Review the Dependabot PR changelog for breaking changes before merging. CI validates all changes automatically.

## Dependency Status Workflow

The [Dependency Status](../../.github/workflows/dependency-status.yml) workflow runs weekly (Monday at 09:00 UTC) and can also be triggered manually via `workflow_dispatch`. It:

1. Fetches the latest MQTT release version from the [256dpi/arduino-mqtt](https://github.com/256dpi/arduino-mqtt) GitHub API.
2. Fetches the latest ESP32 Arduino core release version from the [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) GitHub API.
3. Compares the MQTT version against the minimum version in `library.properties`.
4. Updates `.badges/mqtt.json` and `.badges/esp32.json` with the current status.
5. Commits the changes and posts a summary to the GitHub Actions run.

The badges in the README are powered by shields.io endpoint badges that read these JSON files directly.

## Build Tool Dependencies

These tools are required for local development and CI. They are not framework dependencies but are needed to build and test the project.

| Tool | Purpose | Where |
|------|---------|-------|
| **g++** | Host test compilation | Local, CI (Ubuntu) |
| **lcov** | Code coverage reporting | Local (optional), CI (Ubuntu) |
| **bc** | Coverage math | CI (Ubuntu) |
| **arduino-cli** | Example compilation | CI |
| **CMake** | Alternative test build | Local (optional) |

## Dependency Philosophy

After W46 (ArduinoJson removal), the framework has only **one external Arduino dependency** (MQTT). This minimal dependency footprint reduces:

- **Flash usage**: ~30-40 KB saved by removing ArduinoJson
- **Update surface**: Only 1 library to monitor instead of 2
- **Breakage risk**: Fewer upstream changes can break the framework
- **Build time**: Faster compilation with fewer dependencies

The ESP32 Arduino core is a platform requirement, not a framework choice — all ESP32 Arduino projects require it.
