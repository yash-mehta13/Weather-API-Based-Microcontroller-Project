# Changelog

## Unreleased
Changes made during the documentation/firmware overhaul (not a release; nothing here is hardware-tested).

### Security
- Removed hard-coded Wi-Fi credentials and API key from the sketch; added git-ignored `secrets.h` and `secrets.example.h`. Previously committed values remain in Git history; rotate them.
- Stopped printing the full API response; request URL is never logged.

### Firmware
- Relay forced OFF on every invalid-input path; relay written every control tick; startup OFF before other init.
- Non-blocking millis-based scheduler replacing ~78 s of blocking delays; bounded Wi-Fi reconnect with backoff.
- Weather: timeout, status handling, JSON/field validation, three-state rain, optional verified HTTPS.
- Irrigation default is soil-gated (original OR rule via `AMBIENT_TRIGGERS`), with hysteresis, max run and cooldown.
- LCD pages without `lcd.clear()` flicker; shows pump command and fault reason.

### Added
- `irrigation_logic.h`, `tests/` (logic tests, syntax check against stubs, secret scan), docs/, README rewrite, GitHub templates, CLAUDE.md, SECURITY.md, CONTRIBUTING.md, `.gitignore`.
- Moved the prototype screenshot to `assets/images/prototype-photo.png`.
