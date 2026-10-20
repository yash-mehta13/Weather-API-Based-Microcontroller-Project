# Safety and Limitations

- **Safe state:** relay commanded to the OFF level at startup and on every control tick when any input is invalid. A software interlock is not a guarantee; independent protection is required in real use.
- **Boot glitch:** before firmware runs, GPIO 4 is not under software control. Verify the relay stays OFF through power-up, reset and upload.
- **Relay assumption:** active-low is inferred from source; confirm per module.
- **Command ≠ state:** no feedback exists; the LCD shows what was commanded.
- **Invalid inputs:** stale/invalid DHT, soil, weather, rain, or Wi-Fi loss → OFF. Weather failure is not "no rain".
- **Run limit:** `PUMP_MAX_RUN_MS` 60 s and cooldown 10 min are placeholders, not safe values. The correct values depend on pump flow, container size and plants and require testing.
- **Electrical:** use a separate pump supply, matching relay ratings, flyback diode for DC pumps, fuse, enclosure; keep water and any mains wiring separated; do not switch mains with a hobby relay unless it is rated and properly enclosed.
- **Water:** place the setup in a tray; add overflow protection and a manual cutoff.
- **Network dependence:** no Wi-Fi/weather means no irrigation.
- **Calibration:** soil ADC is uncalibrated; the threshold may not match your sensor (including its direction).
- **Validation:** no physical testing has been done for the refactored firmware.
