# Setup and Configuration

1. **Arduino IDE 2.x** (or Arduino CLI).
2. **ESP32 board package**: Boards Manager → "esp32 by Espressif Systems". The version the author used is unknown; use a current stable release (the sketch uses `HTTPClient::begin(WiFiClient&, url)` and `setConnectTimeout`, present in core 2.x/3.x).
3. **Libraries** (Library Manager): `Arduino_JSON`, `DHT sensor library` (Adafruit, plus `Adafruit Unified Sensor` if prompted), `LiquidCrystal_I2C` (a version providing `begin()` with no arguments, otherwise uncomment `LCD_USE_INIT`). Exact working versions are not established.
4. **Sketch folder:** Arduino requires the sketch folder name to match the `.ino`; move the files into a folder `weatherapi_mpmcproject_01/` (the IDE offers to). Keep `irrigation_logic.h` and `secrets.h` beside it.
5. **Secrets:** `cp secrets.example.h secrets.h`, edit values. Without `secrets.h` compilation fails with "secrets.h: No such file". Never commit it; `git status` should not list it. Rotate any key that has been published.
6. **City:** `WEATHER_CITY` (e.g. `"Vellore"` or `"Vellore,IN"`).
7. Select board (e.g. "ESP32 Dev Module") and port; compile; upload.
8. Serial Monitor at 115200.
9. **Initial test, relay load disconnected:** check I2C scan output, LCD pages, "Pump command: OFF" at start, Wi-Fi connect message, `Weather OK` line. Then follow [hardware-and-wiring.md](hardware-and-wiring.md) before attaching a pump.

Tunable macros are grouped at the top of the `.ino`.
