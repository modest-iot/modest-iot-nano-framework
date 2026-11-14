# Contributing to Modest IoT Nano Framework

Thank you for your interest in contributing to the Modest IoT Nano Framework! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Framework Philosophy](#framework-philosophy)
- [Git Flow Branching Strategy](#git-flow-branching-strategy)
- [Semantic Versioning](#semantic-versioning)
- [Reporting Issues](#reporting-issues)
- [Submitting Changes](#submitting-changes)
- [Coding Standards](#coding-standards)
- [Testing Requirements](#testing-requirements)
- [Documentation Requirements](#documentation-requirements)

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/2/0/code_of_conduct/). By participating, you are expected to uphold this code.

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
3. **Install dependencies**: ArduinoJson ≥7.0.0, MQTT ≥2.5.3
4. **Run tests**: `bash tests/run_all.sh` to verify your environment
5. **Create a branch** for your changes (see [Git Flow Branching Strategy](#git-flow-branching-strategy))

## Framework Philosophy

The Modest IoT Nano Framework follows a set of core principles that guide all design decisions. Contributions should align with these principles.

### Architecture Patterns

- **Mediator Pattern**: The `Device` class is the central mediator that orchestrates sensors, actuators, and gateway clients. Components never communicate directly — all event and command routing flows through the mediator.
- **CQRS (Command Query Responsibility Segregation)**: Sensors emit events (queries); actuators handle commands. The separation is enforced via `Event`/`EventHandler` and `Command`/`CommandHandler` interfaces.
- **Template Method Pattern**: `Sensor::processEvent()` and `Actuator::executeCommand()` are template method hooks that subclasses override to transform or suppress events/commands before propagation.
- **Strategy Pattern**: `ConnectivityDriver` abstracts physical transport (WiFi, BLE); `GatewayClient` abstracts application protocol (MQTT, HTTP, BLE NUS). Clean separation between link-layer and protocol-layer concerns.

### Design Principles

- **Loop-less Architecture**: All async execution is handled by the framework's background tasks. Application `loop()` is always empty — no manual polling, no blocking calls.
- **FreeRTOS Task Isolation**: Sampler task handles all sensor/actuator I/O on one context; network task handles asynchronous telemetry on a separate context. Prevents blocking I/O from starving network operations.
- **Memory Safety**: `unique_ptr` ownership semantics, bounded buffers, no dynamic allocation in hot paths. ESP32-constrained environments require predictable memory usage.
- **Zero Magic Numbers**: All numeric constants are named with descriptive suffixes (e.g., `SERIAL_BAUD_RATE`, `MAIN_LOOP_DELAY_IN_MILLISECONDS`). Raw numbers in code are prohibited.
- **Minimalist Value Objects**: `Event` and `Command` are simple value objects. Commands carry an optional `int32_t` payload for parameterized operations.

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `WiFiConnectivityDriver`)
- **Variables**: `camelCase` (e.g., `transportDriver`)
- **Constants**: `UPPER_SNAKE_CASE` with descriptive suffixes (e.g., `SERIAL_BAUD_RATE`, `MAIN_LOOP_DELAY_IN_MILLISECONDS`)
- **Member variables**: Plain `camelCase` without trailing underscores (e.g., `preferences`, not `preferences_`)
- **Boolean members**: No `is` prefix on the member (e.g., `initialized`); the `is` prefix is reserved for getter methods (e.g., `isInitialized()`)
- **Pin definitions**: `SENSOR_NAME_PIN` (e.g., `TEMPERATURE_SENSOR_PIN`)
- **Event identifiers**: `EVENT_NAME_IDENTIFIER` (e.g., `SENSOR_FAULT_EVENT_IDENTIFIER`)
- **Command identifiers**: `ACTION_COMMAND_IDENTIFIER` (e.g., `TOGGLE_LED_COMMAND_IDENTIFIER`)

### Documentation Standards

- **Doxygen Comments**: All public APIs must have `/** @brief ... */` comments with `@param`, `@return`, and `@runs_on{task}` tags.
- **Threading Context**: Every method that runs on a specific FreeRTOS task must document the context with `@runs_on{sampler}` or `@runs_on{network}`.
- **Examples**: All examples must have Doxygen file headers with a grounded real-world device purpose and a "Modest IoT Nano Framework aspects illustrated" section.

## Git Flow Branching Strategy

The Modest IoT Nano Framework uses [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/) for collaboration. This ensures a stable `main` branch, organized feature development, and predictable releases.

### Branch Types

| Branch | Purpose | Source | Target |
|--------|---------|--------|--------|
| `main` | Production-ready releases | — | — |
| `develop` | Integration branch for next release | `main` | `main` |
| `feature/*` | New features or enhancements | `develop` | `develop` |
| `bugfix/*` | Bug fixes for next release | `develop` | `develop` |
| `release/*` | Pre-release stabilization | `develop` | `main`, `develop` |
| `hotfix/*` | Emergency fixes to production | `main` | `main`, `develop` |

### Branch Naming Conventions

- **Feature branches**: `feature/W20-configuration-store` or `feature/add-bme680-sensor`
- **Bugfix branches**: `bugfix/fix-mqtt-tls-handshake` or `bugfix/resolve-command-queue-race`
- **Release branches**: `release/v1.1.0`
- **Hotfix branches**: `hotfix/v1.0.1-fix-esp32-crash`

### Workflow

#### Starting a New Feature

```bash
# 1. Ensure develop is up to date
git checkout develop
git pull origin develop

# 2. Create a feature branch
git checkout -b feature/W20-configuration-store

# 3. Develop, commit, and push
git add .
git commit -m "feat(config): add ConfigurationStore NVS wrapper"
git push origin feature/W20-configuration-store

# 4. Create a pull request on GitHub
```

#### Merging a Feature

```bash
# 1. Review and approve the pull request on GitHub
# 2. Squash and merge into develop (via GitHub UI)
# 3. Delete the feature branch after merge
```

#### Creating a Release

```bash
# 1. Create a release branch from develop
git checkout develop
git pull origin develop
git checkout -b release/v1.1.0

# 2. Final stabilization: fix bugs, update docs, bump version
# 3. Update CHANGELOG.md, library.properties version
# 4. Commit all changes
git add .
git commit -m "chore(release): prepare v1.1.0 release"

# 5. Merge release into main and develop
git checkout main
git merge --no-ff release/v1.1.0
git checkout develop
git merge --no-ff release/v1.1.0

# 6. Tag the release
git tag -a v1.1.0 -m "Modest IoT Nano Framework v1.1.0"

# 7. Push everything
git push origin main develop v1.1.0
git branch -d release/v1.1.0
```

#### Fixing a Production Bug

```bash
# 1. Create a hotfix branch from main
git checkout main
git pull origin main
git checkout -b hotfix/v1.0.1-fix-crash

# 2. Fix the bug, update CHANGELOG.md, bump patch version
# 3. Merge into main and develop
git checkout main
git merge --no-ff hotfix/v1.0.1-fix-crash
git checkout develop
git merge --no-ff hotfix/v1.0.1-fix-crash

# 4. Tag the hotfix release
git tag -a v1.0.1 -m "Hotfix: fix ESP32 crash on deep sleep wake"

# 5. Push everything
git push origin main develop v1.0.1
git branch -d hotfix/v1.0.1-fix-crash
```

### Pull Request Guidelines

- **Title**: Clear, descriptive title following Conventional Commits format
- **Description**: Explain the problem, solution, and any trade-offs
- **Checks**: All CI workflows must pass (ESP32 compilation, host tests, lint)
- **Review**: At least one maintainer approval required
- **Merge**: Squash and merge into `develop` (features/bugfixes) or no-fast-forward merge (releases/hotfixes)

## Semantic Versioning

The Modest IoT Nano Framework follows [Semantic Versioning 2.0.0](https://semver.org/). Version numbers follow the format `MAJOR.MINOR.PATCH`.

### Version Bump Rules

| Version Part | When to Bump | Example |
|-------------|--------------|---------|
| **MAJOR** | Incompatible API changes (breaking changes) | `1.0.0` → `2.0.0` |
| **MINOR** | Backward-compatible feature additions | `1.0.0` → `1.1.0` |
| **PATCH** | Backward-compatible bug fixes | `1.0.0` → `1.0.1` |

### What Constitutes a Breaking Change?

- Removing or renaming a public method, class, or constant
- Changing the signature of a public method (parameters, return type)
- Changing the behavior of a public method in a way that breaks existing code
- Removing a component from `ModestIoT.h` umbrella header

### What Constitutes a Minor Release?

- Adding a new sensor, actuator, or connectivity driver
- Adding a new public method to an existing class
- Adding a new example
- Adding a new test suite
- Documentation improvements that don't change behavior

### What Constitutes a Patch Release?

- Fixing a compilation error
- Fixing a runtime bug
- Fixing incorrect documentation
- Fixing a test that was incorrectly asserting

### Version Management

The version is managed in two places:

1. **`library.properties`**: `version=1.0.0` — used by Arduino Library Manager
2. **Git tags**: `v1.0.0` — used for release tracking

Both must be updated together when creating a release.

## Reporting Issues

### Bug Reports

Use the GitHub Issues page to report bugs. Include:

- **ESP32 board model** (e.g., ESP32 DevKit, ESP32-S3)
- **Arduino core version** (e.g., 3.3.12)
- **Steps to reproduce** the issue
- **Expected behavior**
- **Actual behavior**
- **Error messages** or stack traces if applicable

### Feature Requests

Use the GitHub Issues page to suggest features. Include:

- **Use case**: What problem does this solve?
- **Proposed solution**: How should it work?
- **Alternatives considered**: Any other approaches?

## Submitting Changes

All contributions are made via pull requests on GitHub. Follow the [Git Flow Branching Strategy](#git-flow-branching-strategy) for branch creation and merging.

### Pull Request Process

1. **Create a branch** from `develop` (features/bugfixes) or `main` (hotfixes)
2. **Make your changes** following the [Framework Philosophy](#framework-philosophy) and [Coding Standards](#coding-standards)
3. **Add tests** for new functionality — proportional to complexity
4. **Update documentation** — Doxygen comments, README.md, CHANGELOG.md
5. **Run all tests** to ensure nothing is broken: `bash tests/run_all.sh`
6. **Run Doxygen** to verify documentation: `doxygen docs/Doxyfile`
7. **Submit a pull request** with a clear description linking to any related issue

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/) format:

```
type(scope): description

Optional body with more details.
```

Types:
- `feat`: New feature (triggers MINOR version bump)
- `fix`: Bug fix (triggers PATCH version bump)
- `docs`: Documentation changes
- `test`: Adding or updating tests
- `refactor`: Code refactoring (no behavior change)
- `chore`: Maintenance tasks (e.g., CI, dependencies)
- `BREAKING CHANGE`: Incompatible API change (triggers MAJOR version bump)

Examples:
```
feat(sensor): add BME680 VOC sensor support
fix(device): resolve race condition in command queue
docs(readme): update installation instructions
test(config): add ConfigurationStore namespace isolation tests
refactor(rgb): extract brightness scaling to static helper
BREAKING CHANGE: remove deprecated Sensor::getRawValue method
```

### Pre-Submission Checklist

Before submitting a pull request, verify:

- [ ] All tests pass: `bash tests/run_all.sh`
- [ ] Doxygen generates without warnings: `doxygen docs/Doxyfile`
- [ ] Code follows [Framework Philosophy](#framework-philosophy) (mediator pattern, CQRS, loop-less)
- [ ] Naming conventions are consistent ([Naming Conventions](#naming-conventions))
- [ ] All public APIs have Doxygen comments with `@runs_on{task}` tags
- [ ] No magic numbers — all constants are named with descriptive suffixes
- [ ] CHANGELOG.md is updated under the appropriate section
- [ ] `library.properties` version is updated (if applicable)
- [ ] New components are added to `ModestIoT.h` umbrella header
- [ ] New components have entries in `keywords.txt`

## Coding Standards

See also: [Framework Philosophy — Naming Conventions](#naming-conventions) for the complete naming reference.

### Code Style

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 120 characters maximum
- **Braces**: Allman style (opening brace on new line)
- **Comments**: Use Doxygen format (`/** @brief ... */`)
- **Magic numbers**: Never use raw numbers; always define named constants

### Doxygen Documentation

All public APIs must have Doxygen comments:

```cpp
/**
 * @brief Short description.
 * @param name Description of parameter.
 * @return Description of return value.
 * @runs_on{sampler} Threading context (if applicable).
 */
```

## Testing Requirements

### Unit Tests

- **New components** must have dedicated test suites
- **Existing tests** must pass after your changes
- **Test coverage** should be proportional to complexity

### Running Tests

```bash
# Run all tests
bash tests/run_all.sh

# Run specific test
g++ -std=c++17 -Wall -Wextra -Werror -Itests/fakes -Isrc \
    -fsanitize=address,undefined tests/GasSensor/test.cpp src/GasSensor.cpp \
    tests/fakes/ArduinoFake.cpp -o test_GasSensor && ./test_GasSensor
```

### Test Quality

- Use **assertions** for all expected outcomes
- Test **edge cases** and error conditions
- Use **mock objects** for hardware dependencies
- Keep tests **independent** and **deterministic**

## Documentation Requirements

### Code Documentation

- **All public methods** must have Doxygen comments
- **All classes** must have `@file` and `@ingroup` tags
- **Threading context** must be documented with `@runs_on{task}` tags

### Example Documentation

- **New examples** must have Doxygen file headers with:
  - Grounded real-world device purpose
  - "Modest IoT Nano Framework aspects illustrated" section
  - `@runs_on{sampler}` tags on `on()` and `handle()` methods

### README and CHANGELOG

- **Update README.md** if your changes affect the user-facing API
- **Update CHANGELOG.md** with your changes under the appropriate section

## License

By contributing, you agree that your contributions will be licensed under the [Apache License 2.0](LICENSE.md).

## Questions?

If you have questions about contributing, please open an issue with the `question` label.
