# Hardware and Wiring

> Nothing here has been physically verified by this documentation effort. Pins come from the firmware source. Treat everything as "to be confirmed on your hardware".

## Source-defined connections

| Component | Signal | Connection | Status |
|---|---|---|---|
| DHT11 | DATA | GPIO 18 | From source |
| Soil-moisture sensor | Analog output | GPIO 34 (input-only, ADC1) | From source |
| Relay module | Control input | GPIO 4 | From source |
| I2C LCD | SDA / SCL | `Wire.begin()` board defaults (often 21 / 22 on ESP32 DevKit) | **Verify for your board** |
| I2C LCD | Address | 0x27 | From source; confirm with scanner |
| Serial | Baud | 115200 | From source |

Power and ground: connect all grounds together. Supply voltages for the DHT module, soil sensor, relay module and LCD backpack depend on the exact modules (many LCD backpacks and relay modules need 5 V while ESP32 GPIO is 3.3 V). Check each module's datasheet. The pump supply must be separate from the ESP32 3.3 V rail and sized for the pump's current (unknown here).

## Verification procedures

**Board and GPIOs.** Identify your board's pinout. GPIO 34 is input-only and on ADC1 (works while Wi-Fi is active; ADC2 pins do not). GPIO 4 is a normal output but check your board for on-board use.

**LCD I2C address.** Leave `ENABLE_I2C_SCAN 1`, open the Serial Monitor at 115200 and read `I2C device at 0x..`. Update `LCD_ADDRESS` if it differs (0x3F is a common alternative). Adjust contrast on the backpack if the screen is blank.

**Relay polarity (do this with NO pump or load connected).** The firmware drives LOW for ON, HIGH for OFF (`RELAY_ON_LEVEL`/`RELAY_OFF_LEVEL`). Power the system and watch the relay LED/click: it must be OFF after startup. If it is ON, your module is active-high; swap the two macros. Also watch the relay during a reset and during upload: GPIO levels during boot are not under firmware control and some modules may click.

**Soil sensor calibration.** Print the ADC value (LCD page 1 or add a temporary serial print). Record: (a) sensor in dry air, (b) in dry soil, (c) in soil at the wetness you want to maintain, (d) in water to the line. Note whether the number rises or falls as wetness increases and set `SOIL_HIGHER_IS_DRIER` accordingly. Set `SOIL_DRY_ADC` between the "dry" and "good" readings and `SOIL_HYSTERESIS_ADC` wider than the reading noise. Values are specific to the sensor, supply voltage and soil; do not convert to a percentage without your own calibration.

**Supply compatibility.** Confirm each module's logic level against 3.3 V, the relay coil current against what the relay input can source, and the relay contact rating against pump voltage/current. Use a flyback diode across a DC pump if the module does not provide one. Drive a bare relay coil only through a transistor driver.

## Common wiring problems

| Symptom | Likely cause |
|---|---|
| LCD blank | wrong address, contrast, SDA/SCL swapped, backpack needs 5 V |
| `I2C scan` finds nothing | wiring/pull-ups/power |
| DHT always errors | wrong pin, no pull-up (bare sensor), unpowered |
| Soil ADC stuck at 0 or 4095 | floating/miswired signal, wrong supply |
| Relay inverted | module is active-high |
| ESP32 resets when pump starts | pump shares the ESP32 supply; use separate supply |
