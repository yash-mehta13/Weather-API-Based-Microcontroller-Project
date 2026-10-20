# System Architecture

## Block diagram

```mermaid
flowchart TB
    subgraph Local["Local readings"]
      DHT[DHT11 - GPIO 18]
      SOIL[Soil sensor analog - GPIO 34]
    end
    subgraph Net["Network data"]
      API[OpenWeatherMap current weather]
    end
    DHT --> ESP
    SOIL --> ESP
    API <-- "Wi-Fi" --> ESP
    ESP[ESP32 firmware] -- "I2C 0x27" --> LCD[16x2 LCD]
    ESP -- "GPIO 4 (active-low assumed)" --> RELAY[Relay module] --> PUMP[Pump - if installed]
```

## Data flow

```mermaid
flowchart LR
    A[Read DHT + soil every 2 s] --> D
    B[Fetch weather every 10 min] --> V[Validate + parse] --> D
    W[Wi-Fi status] --> D
    D{decideIrrigation} -->|ON only if all valid| R[Relay command]
    D --> L[LCD reason text]
```

**Local vs API data:** local temperature, humidity and soil ADC come from the sensors; API temperature, humidity and rain come from OpenWeatherMap for the configured city. API temperature/humidity are displayed only; they are not used in the pump decision. API rain is. Local temperature/humidity are used only by the optional legacy rule (`AMBIENT_TRIGGERS`).

The pump and its wiring are the *intended* architecture; the source cannot confirm them.
