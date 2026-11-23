# Modest IoT Nano Framework — Code Coverage

**Version**: 1.2.0  
**Date**: 2026-11-20  
**Toolchain**: GCC gcov + lcov/genhtml  
**Platform**: Linux (Ubuntu)

---

## Overview

The Modest IoT Nano Framework uses **gcov** and **lcov** to measure test coverage across all 51 host test suites. The coverage report shows which lines of source code are exercised by the test suite, providing evidence of test quality and identifying untested code paths.

## Running Coverage Locally

### Prerequisites

- **GCC** (not Clang) — gcov requires GCC for coverage instrumentation.
- **lcov** package — provides the `genhtml` tool for HTML reports.
- **bc** — for coverage threshold comparison.

Install on Ubuntu:
```bash
sudo apt-get install -y g++ lcov bc
```

Install on macOS:
```bash
brew install gcc lcov bc
```

On macOS, specify the GCC compiler explicitly:
```bash
CXX=g++-16 ./tests/run_coverage.sh
```

### Execution

Generate a coverage report:
```bash
./tests/run_coverage.sh
```

This compiles all 51 test suites with coverage instrumentation (`-fprofile-arcs -ftest-coverage`), runs them, and generates an HTML report in `build/coverage/index.html`.

Set a custom minimum coverage threshold (default: 80%):
```bash
MIN_COVERAGE=70 ./tests/run_coverage.sh
```

Enforce the threshold (fail if not met):
```bash
ENFORCE_COVERAGE=1 ./tests/run_coverage.sh
```

### Report Structure

The HTML report in `build/coverage/` contains:

- **`index.html`** — Overall summary with per-file coverage percentages.
- **Per-file pages** — Line-by-line view showing executed (green), partially executed (yellow), and unexecuted (red) lines.
- **`coverage.summary`** — Machine-readable summary file with overall and per-file statistics.

## CI Integration

The coverage job runs automatically on every push and pull request. Key details:

| Aspect | Configuration |
|--------|--------------|
| **Platform** | Ubuntu (gcov requires GCC) |
| **Trigger** | Every push to `main`, `develop`, `release/*`, `feature/*`; every PR |
| **Artifact** | HTML report uploaded as `coverage-report` (14-day retention) |
| **Summary** | Coverage summary posted to GitHub step summary |
| **Threshold** | 80% line coverage (warning only, not enforced in CI) |

### Viewing CI Coverage Reports

1. Navigate to the workflow run in GitHub Actions.
2. Find the "Code Coverage (Ubuntu)" job.
3. Download the `coverage-report` artifact.
4. Open `index.html` in a browser.

Alternatively, check the job's step summary for the text coverage summary.

## Coverage Threshold

The default minimum coverage threshold is **80% line coverage**. This threshold:

- **Warns** when coverage falls below the threshold.
- **Fails the build** only when `ENFORCE_COVERAGE=1` is set.
- Can be adjusted via the `MIN_COVERAGE` environment variable.

The threshold applies to **line coverage only** — function coverage is not measured, as line coverage is the primary indicator of test quality for this framework.

## Why gcov and Not Other Tools?

| Tool | Pros | Cons | Decision |
|------|------|------|----------|
| **gcov** | Native GCC integration, no external dependencies, simple output | Linux-only for consistent results | ✓ Selected |
| **Clang Coverage** | Works with Clang, LLVM-based | Requires Clang, different workflow | Not selected |
| **GCOV + LCOV** | HTML reports, easy to read | Requires lcov package | ✓ Selected for reports |
| **Coverage.py** | Python ecosystem | Not applicable for C++ | Not applicable |

gcov was chosen because:
1. The framework already uses GCC for host tests.
2. gcov integrates seamlessly with the existing `tests/run_all.sh` pattern.
3. No additional dependencies beyond the lcov package for report generation.

## Coverage vs. Sanitizers

The coverage script runs **without sanitizers** (`-fsanitize=address,undefined`). This is intentional:

- Sanitizers and gcov can interfere with each other in some configurations.
- The regular test runner (`tests/run_all.sh`) runs with sanitizers enabled by default.
- Coverage focuses on **which lines are executed**, not on memory safety — that's the sanitizer's job.

## Interpreting the Report

### Coverage Colors

- **Green**: Line was executed during testing.
- **Yellow**: Line was partially executed (e.g., one branch of a conditional).
- **Red**: Line was not executed.
- **Gray**: Line is not countable (e.g., comments, blank lines).

### What Counts as Good Coverage?

| Coverage | Interpretation |
|----------|---------------|
| **90%+** | Excellent — most code paths are tested. |
| **80-89%** | Good — core paths are covered; some edge cases may be missing. |
| **70-79%** | Acceptable — significant gaps exist; prioritize testing uncovered areas. |
| **<70%** | Concerning — large portions of code are untested. |

### Expected Gaps

Some unexecuted lines are expected and acceptable:

- **Platform-specific code**: ESP32-specific macros and hardware registers that can't run on host.
- **Error paths**: Rare error conditions that require specific hardware states.
- **Mock-dependent code**: Code that only executes with real hardware peripherals.

### Platform-Specific File Exclusions

The coverage report excludes files that use Arduino.h, FreeRTOS, or LittleFS APIs that cannot execute in host tests. These files are filtered out by the coverage script so the reported number reflects only testable code:

| File | Reason |
|------|--------|
| OneWireTemperatureSensor.cpp | 1-Wire bit-banging protocol (GPIO timing) |
| DhtSensor.cpp | DHT sensor protocol (GPIO timing) |
| UltrasonicSensor.cpp | Ultrasonic timing (GPIO, delays) |
| LoadCellAmplifier.cpp | ADC calibration (hardware-specific) |
| TelemetryBuffer.cpp | LittleFS file operations |
| PowerManager.cpp | ESP32 deep sleep API |
| GatewayClient.h | Abstract interface (pure-virtual methods) |

Additionally, `LCOV_EXCL_START/END` markers in `Device.cpp` exclude FreeRTOS-specific functions (ISR, task creation, queue operations, power management) that cannot run in host tests.

The coverage script ensures these exclusions are visible in the final HTML report by deleting the raw `.gcda`/`.gcno` files for excluded sources before `genhtml` runs, preventing `genhtml` from re-discovering them.

### Current Coverage Numbers

| Scope | Line Coverage | Functions |
|-------|---------------|-----------|  
| **Testable code (filtered)** | **91.5%** | **84.4%** |

The filtered coverage (91.5%) is the most meaningful metric, as it excludes test code, GCC standard library headers, and the 7 platform-specific files listed above. The `lcov --capture` step uses `--exclude` patterns to filter out system include paths (Homebrew GCC, Xcode SDK, Linux system headers) so only project source files appear in the report. The genhtml HTML report reflects only this filtered set — the excluded files do not appear.

## Adding Coverage to New Tests

When adding new source files, ensure they are included in the coverage script:

1. Add the new test suite to `tests/run_coverage.sh` (same pattern as existing tests).
2. Include the new source files in the compilation command.
3. Run `./tests/run_coverage.sh` to verify the report includes the new files.

## Limitations

1. **Host-only**: Coverage measures only the host test suites, not the ESP32 firmware behavior.
2. **No branch coverage**: Only line coverage is measured; branch coverage would require additional configuration.
3. **Mock influence**: Tests use fakes and mocks, so some production code paths may not be exercised identically to hardware.
4. **Platform exclusions**: Platform-specific files are excluded from the report. The reported coverage reflects only testable code, not the full source base.

## Future Improvements

- **Branch coverage**: Enable `-fbranch-protection` for branch-level coverage.
- **Coverage diff**: Compare coverage between PRs to detect regressions.
- **Enforced threshold**: Set `ENFORCE_COVERAGE=1` in CI when coverage stabilizes above 85%.
- **Badges**: Add a coverage badge to README.md when CI coverage is stable.

---

*This document will be updated as the coverage infrastructure evolves.*
