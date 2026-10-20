# Project Overview

**Purpose:** an ESP32 prototype that combines local sensing (DHT11, soil moisture) with online weather data (OpenWeatherMap) to command an irrigation relay and show status on a 16x2 LCD.

**Scope:** one sketch (`weatherapi_mpmcproject_01.ino`) plus `irrigation_logic.h`. Single city, single relay/pump channel, no cloud storage.

**Hardware architecture (intended):** see [system-architecture.md](system-architecture.md). A photograph of the author's prototype is in [`assets/images/prototype-photo.png`](../assets/images/prototype-photo.png); it appears to show an ESP32 board, relay module, small pump, resistive-style soil probe, DHT module, LCD and a 2-cell battery holder. Exact part numbers, supply voltages and wiring are not established.

**Users:** students and hobbyists using the Arduino IDE.

**Constraints:** 2.4 GHz Wi-Fi required for weather data; free API rate limits; DHT11 accuracy; uncalibrated soil ADC.

**Development status:** prototype. Original firmware worked as a demonstration (per its author; not verified here). The refactored firmware has been reviewed and unit-tested for logic only; it still needs a board compile and hardware validation.
