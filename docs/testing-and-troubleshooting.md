# Testing and Troubleshooting

## Automated (host) checks: `tests/run_tests.sh`
| Script | What it checks | Limits |
|---|---|---|
| `test_irrigation_logic.cpp` | Decision logic: dry/wet, boundaries, hysteresis, legacy rule, rain, every fault path, recovery, max-run/cooldown, millis rollover, inverted polarity | Logic only, not relay/pump behaviour |
| `check_sketch_syntax.sh` | Compiles the `.ino` with `-fsyntax-only` against `tests/stubs` (HTTP and HTTPS variants) | Stubs are hand-written assumptions about the APIs; **not** a board build |
| `check_no_secrets.sh` | Key-like strings, non-placeholder credential defines, `secrets.h` ignored/untracked | Heuristic; does not scan git history |

Needs only `g++` and `bash`.

## Board build (not yet performed by the maintainers of this change)
`arduino-cli compile --fqbn esp32:esp32:esp32 .` from the sketch folder after installing the core and libraries and creating `secrets.h`.

## Hardware validation checklist (all unchecked)
- [ ] Exact ESP32 board identified
- [ ] Stable power (no brownouts when pump starts)
- [ ] GPIO connections match table
- [ ] DHT readings plausible
- [ ] Soil ADC direction and calibration recorded
- [ ] LCD address and pages correct
- [ ] Relay polarity: OFF at boot and during reset/upload
- [ ] Pump voltage/current within relay rating
- [ ] Wi-Fi unplugged → pump OFF
- [ ] Invalid key / bad city → pump OFF, LCD shows no data
- [ ] Rain response parsed (compare against a real response)
- [ ] Controlled pump on/off with water containment and run-time limit confirmed

## Troubleshooting
| Symptom | Check |
|---|---|
| `secrets.h: No such file` | create it from the example |
| `begin` / `init` compile error on LCD | `LCD_USE_INIT` |
| "No Wi-Fi" | 2.4 GHz network, credentials |
| "Weather error" | Serial `error` code: 1 none yet, 2 transport, 3 key (401), 4 city (404), 5 rate limit, 6 other HTTP, 7 bad JSON, 8 bad fields (enum order in the sketch) |
| "DHT sensor fault" | wiring/pin; DHT11 needs >1 s between reads |
| Pump never starts | by design unless every input is valid; read LCD page 3 reason |
