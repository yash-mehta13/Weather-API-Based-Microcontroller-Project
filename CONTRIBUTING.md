# Contributing

1. Follow [docs/setup-and-configuration.md](docs/setup-and-configuration.md); create a local `secrets.h` (never commit it, never paste credentials into issues or PRs).
2. Firmware changes: keep decision logic in `irrigation_logic.h`, add/adjust tests in `tests/test_irrigation_logic.cpp`, run `tests/run_tests.sh`.
3. Pin, wiring or relay-polarity changes must update [docs/hardware-and-wiring.md](docs/hardware-and-wiring.md), the README pin table and the macros together.
4. Say what you tested and how: host tests, board compile (board, core and library versions), and physical tests (what hardware, what you observed). Do not claim hardware results you did not obtain.
5. Describe safety implications of any control-logic change. Open a PR using the template.
