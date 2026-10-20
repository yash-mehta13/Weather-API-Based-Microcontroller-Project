# CLAUDE.md

ESP32 (Arduino framework) weather monitoring and irrigation controller. Single sketch `weatherapi_mpmcproject_01.ino` plus `irrigation_logic.h` (pure logic, host-tested). Toolchain: Arduino IDE/CLI with the Espressif esp32 core. Do not migrate to PlatformIO or add frameworks.

## Commands
- Host checks (g++/bash only): `tests/run_tests.sh`
- Board build (needs core, libraries, local `secrets.h`): `arduino-cli compile --fqbn esp32:esp32:esp32 .`

## Rules
- **Secrets:** never put real Wi-Fi credentials or API keys in any tracked file, log, test or doc. Real values live only in git-ignored `secrets.h`. Do not print them. A key was exposed in the git history of this repo; treat it as compromised.
- **Hardware safety:** relay must be OFF at startup and on every invalid-input path. Only `setPump()` writes the relay pin. Never change relay polarity, pins or thresholds on assumption; document as unverified.
- Weather failure is never "no rain". Never disable TLS verification.
- Keep logic in `irrigation_logic.h` and add a test for every policy change.
- **Docs:** keep README, docs/ and macros consistent; mark unverified hardware facts as unverified; never claim hardware testing, live API success or compile success that did not happen.
- Verify implementation (run the tests) before claiming a feature works. No commits/pushes without the owner's request.
