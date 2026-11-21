# Security Policy

## Supported Versions

| Version | Supported          |
|---------|------------------|
| 1.2.1   | :white_check_mark: |
| < 1.2.1 | :x:               |

## Reporting a Vulnerability

We take the security of the Modest IoT Nano Framework seriously. If you discover a security vulnerability, please report it responsibly so we can address it before affected devices are compromised.

### How to Report

1. **Open a GitHub Issue** with the `security` label by navigating to the [Issues tab](https://github.com/modest-iot/modest-iot-nano-framework/issues/new/choose) of the repository, selecting "Bug Report", and adding the `security` label. The maintainer will address the issue promptly and may close the public issue to limit exposure while working on a fix.

2. **Include the following information** in your report:
   - A clear description of the vulnerability
   - Steps to reproduce the issue
   - The affected version(s) of the framework
   - The ESP32 board model and Arduino core version used
   - Any potential impact on deployed devices
   - Suggested fixes if you have them

### What to Expect

This project is maintained by a single developer. Response times may vary, but we commit to the following:

- **Acknowledgment**: We will acknowledge receipt of your report as soon as possible, typically within **1 week**.
- **Assessment**: We will assess the report and determine the severity within **2 weeks**.
- **Resolution**: We will work on a fix and aim to release a patch in the next scheduled release. Critical vulnerabilities may trigger an out-of-band patch release.
- **Disclosure**: We will coordinate with you on public disclosure timing. The vulnerability will be disclosed after a fix is available and users have had reasonable time to update.

### Security Scope

The following areas are considered in scope for security reporting:

- **OTA update handling** (`OtaUpdateHandler`) — authentication, integrity, and update process
- **TLS/SSL connectivity** — certificate validation, secure socket creation
- **MQTT/HTTP gateway clients** — authentication headers, token handling
- **BLE connectivity** — pairing, encryption, and data transmission
- **ConfigurationStore** — persistent storage of sensitive data (credentials, tokens)
- **Memory safety** — buffer overflows, use-after-free, or other memory corruption issues

### Security Exclusions

The following are out of scope for security reporting:

- **Denial of service** on the Arduino IDE or Library Manager
- **Social engineering** attacks
- **Physical access** to the ESP32 device
- **Third-party library vulnerabilities** (report these to the respective library maintainers)

### Security Best Practices

When using the Modest IoT Nano Framework in production, follow these security best practices:

- **Use OTA password protection**: Always call `OtaUpdateHandler::setPassword()` with a strong password before `begin()`.
- **Enable TLS/SSL**: Use `WiFiConnectivityDriver::createSecureSocket()` for HTTPS connections instead of plain HTTP.
- **Validate certificates**: Ensure your ESP32 firmware validates server certificates for TLS connections.
- **Secure credentials**: Store sensitive data (API tokens, passwords) in `ConfigurationStore` using separate NVS namespaces.
- **Keep dependencies updated**: Regularly update the MQTT library dependency to its latest version. The framework monitors dependencies automatically via Dependabot (GitHub Actions) and weekly version checks (MQTT, ESP32 core) with shields.io status badges.

## Acknowledgments

We appreciate the security research community's help in keeping the Modest IoT Nano Framework secure. If you contribute to identifying and fixing vulnerabilities, we are happy to acknowledge your contribution in the release notes (with your permission).
