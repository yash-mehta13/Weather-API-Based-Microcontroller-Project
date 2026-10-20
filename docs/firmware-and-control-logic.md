# Firmware and Control Logic

Describes what the current source does (not a test report).

## Files
- `weatherapi_mpmcproject_01.ino`: hardware, networking, LCD, scheduler.
- `irrigation_logic.h`: pure decision logic, also compiled by the host tests.

## Startup (`setup()`)
Relay pin is written to the OFF level, set as output, and written again before Serial/LCD/Wi-Fi start. Then Serial (115200), I2C scan (optional), DHT, LCD, Wi-Fi station mode. Failures in later steps cannot switch the pump on, because only `runControl()` ever commands ON.

## Main loop (millis-based, `delay(50)`)
| Task | Period |
|---|---|
| Wi-Fi service | every pass |
| Weather fetch | 10 min; retry 60 s after failure |
| DHT + soil read | 2 s |
| Control decision + relay write | 1 s |
| LCD page change | 3 s (text redrawn only if changed) |

Time differences use unsigned subtraction so `millis()` rollover is handled.

## Wi-Fi
One non-blocking attempt at a time, 15 s attempt timeout, backoff 5 s doubling to 5 min, reset on success. While disconnected the control step commands OFF.

## Weather
See [weather-api-integration.md](weather-api-integration.md). Data older than 25 min is treated as unavailable.

## Decision (`decideIrrigation`)
Order of evaluation, first match wins:
1. Wi-Fi down, DHT stale/invalid (>10 s), soil invalid, weather stale, rain unknown, rain detected → **OFF**.
2. Cooldown active → OFF.
3. Soil dry (strict `<` threshold, hysteresis once running) → want ON. With `AMBIENT_TRIGGERS`, temp > 30 or humidity < 40 also → want ON.
4. Running longer than `PUMP_MAX_RUN_MS` → OFF and cooldown starts.

### Original behaviour vs now
Original: rain>0 → OFF; else `soil<2000 || temp>30 || humidity<40` → ON; weather failure just returned early, **leaving the relay in its previous state**; a DHT failure did the same; the loop slept ~78 s between decisions.
Assessment: the OR means hot/dry air waters moist soil, and the early returns could leave the pump running indefinitely. Now: soil gates irrigation by default (legacy rule available), every control tick rewrites the relay, and all failure paths command OFF. These thresholds are not validated agronomic rules.

### Rationale for policy choices
- Hysteresis/max-run/cooldown values are placeholders needing hardware testing.
- A DHT fault forces OFF even in soil-only mode: conservative, keeps one simple rule.

## Relay and LCD
`setPump()` is the only writer of the relay pin and logs changes. The LCD says "Pump cmd", the commanded state; no feedback sensor exists.

## Error handling summary
| Event | Result |
|---|---|
| DHT NaN/out of range | keep last good value up to 10 s, then OFF |
| Wi-Fi lost | OFF, reconnect with backoff |
| HTTP/JSON/field error | previous weather valid up to 25 min, then OFF; retry in 60 s |
| Rain field unreadable | `RAIN_UNKNOWN` → OFF |
