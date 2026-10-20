// irrigation_logic.h
//
// Pure irrigation decision logic. It has no Arduino or hardware dependencies,
// so the same code is compiled into the sketch and into the host-side unit
// tests (tests/test_irrigation_logic.cpp).
//
// A decision is only a COMMAND for the relay. It is not confirmation that a
// pump is running.
//
// Fail-safe policy: the pump may only be commanded ON when every input is
// valid and fresh. Anything else yields OFF.

#ifndef IRRIGATION_LOGIC_H
#define IRRIGATION_LOGIC_H

#include <stdint.h>

enum RainStatus : uint8_t {
    RAIN_UNKNOWN = 0,  // no usable weather data, or the rain field could not be read
    RAIN_NONE,         // valid response, no precipitation reported
    RAIN_DETECTED      // valid response, precipitation reported
};

enum PumpReason : uint8_t {
    REASON_SOIL_OK = 0,        // soil is wet enough
    REASON_IRRIGATING,         // soil is dry, pump commanded ON
    REASON_RAIN,               // rain reported
    REASON_MAX_RUN,            // run-time limit reached
    REASON_COOLDOWN,           // waiting after a run-time limit stop
    REASON_NO_WIFI,            // Wi-Fi not connected
    REASON_LOCAL_SENSOR,       // DHT reading missing, invalid or stale
    REASON_SOIL_SENSOR,        // soil ADC reading invalid or stale
    REASON_NO_WEATHER,         // no fresh weather data
    REASON_RAIN_UNKNOWN        // fresh weather data, but rain state unreadable
};

// Short text, at most 16 characters (one line of the 16x2 LCD).
inline const char* reasonText(PumpReason r) {
    switch (r) {
        case REASON_SOIL_OK:      return "Soil OK";
        case REASON_IRRIGATING:   return "Soil dry";
        case REASON_RAIN:         return "Rain reported";
        case REASON_MAX_RUN:      return "Max run reached";
        case REASON_COOLDOWN:     return "Cooldown";
        case REASON_NO_WIFI:      return "No Wi-Fi";
        case REASON_LOCAL_SENSOR: return "DHT sensor fault";
        case REASON_SOIL_SENSOR:  return "Soil sensor flt";
        case REASON_NO_WEATHER:   return "No weather data";
        case REASON_RAIN_UNKNOWN: return "Rain unknown";
    }
    return "?";
}

struct IrrigationConfig {
    int      soilDryAdc;          // raw ADC threshold (NOT a calibrated percentage)
    int      soilHysteresisAdc;   // band that must be crossed before the pump stops
    bool     soilHigherIsDrier;   // false: ADC below threshold means dry (original firmware)
    bool     ambientTriggers;     // true: legacy rule, heat or dry air alone can start the pump
    float    tempHighC;           // legacy trigger, strictly greater than
    float    humidityLowPct;      // legacy trigger, strictly less than
    uint32_t maxRunMs;            // 0 disables the run-time limit
    uint32_t cooldownMs;          // wait after a max-run stop
};

struct IrrigationInputs {
    bool       wifiConnected;
    bool       localValid;        // fresh, in-range DHT reading
    float      localTempC;
    float      localHumidityPct;
    bool       soilValid;
    int        soilRaw;
    bool       weatherFresh;      // weather response parsed OK and not older than the limit
    RainStatus rain;
};

struct PumpState {
    bool     on;                  // pump currently commanded ON
    uint32_t onSinceMs;
    bool     cooldownActive;
    uint32_t cooldownStartMs;
};

struct Decision {
    bool       pumpOn;
    PumpReason reason;
};

inline PumpState pumpStateInit() {
    PumpState s;
    s.on = false;
    s.onSinceMs = 0;
    s.cooldownActive = false;
    s.cooldownStartMs = 0;
    return s;
}

// True when the soil reading is "dry enough to start". Strict comparison,
// matching the original `soilMoisture < 2000`.
inline bool soilIsDry(const IrrigationConfig& c, int raw) {
    return c.soilHigherIsDrier ? (raw > c.soilDryAdc) : (raw < c.soilDryAdc);
}

// True when the soil has recovered past the hysteresis band.
inline bool soilIsWetEnough(const IrrigationConfig& c, int raw) {
    return c.soilHigherIsDrier ? (raw <= c.soilDryAdc - c.soilHysteresisAdc)
                               : (raw >= c.soilDryAdc + c.soilHysteresisAdc);
}

inline bool ambientTriggerActive(const IrrigationConfig& c, const IrrigationInputs& in) {
    return in.localTempC > c.tempHighC || in.localHumidityPct < c.humidityLowPct;
}

// Elapsed time that stays correct across millis() rollover (unsigned wrap).
inline uint32_t elapsedMs(uint32_t now, uint32_t since) {
    return (uint32_t)(now - since);
}

inline Decision decideIrrigation(const IrrigationConfig& cfg, const IrrigationInputs& in,
                                 PumpState& st, uint32_t nowMs) {
    Decision d;
    d.pumpOn = false;

    // 1. Invalid or unsuitable inputs: always OFF.
    PumpReason fault = REASON_SOIL_OK;
    bool hasFault = true;
    if (!in.wifiConnected)                 fault = REASON_NO_WIFI;
    else if (!in.localValid)               fault = REASON_LOCAL_SENSOR;
    else if (!in.soilValid)                fault = REASON_SOIL_SENSOR;
    else if (!in.weatherFresh)             fault = REASON_NO_WEATHER;
    else if (in.rain == RAIN_UNKNOWN)      fault = REASON_RAIN_UNKNOWN;
    else if (in.rain == RAIN_DETECTED)     fault = REASON_RAIN;
    else                                   hasFault = false;

    if (hasFault) {
        st.on = false;
        d.reason = fault;
        return d;
    }

    // 2. Cooldown after a run-time limit stop.
    if (st.cooldownActive) {
        if (elapsedMs(nowMs, st.cooldownStartMs) < cfg.cooldownMs) {
            st.on = false;
            d.reason = REASON_COOLDOWN;
            return d;
        }
        st.cooldownActive = false;
    }

    // 3. Want-to-irrigate, with hysteresis once running.
    bool want;
    if (st.on) {
        want = !soilIsWetEnough(cfg, in.soilRaw);
    } else {
        want = soilIsDry(cfg, in.soilRaw);
    }
    if (cfg.ambientTriggers && ambientTriggerActive(cfg, in)) want = true;

    if (!want) {
        st.on = false;
        d.reason = REASON_SOIL_OK;
        return d;
    }

    // 4. Run-time limit.
    if (st.on && cfg.maxRunMs != 0 && elapsedMs(nowMs, st.onSinceMs) >= cfg.maxRunMs) {
        st.on = false;
        st.cooldownActive = true;
        st.cooldownStartMs = nowMs;
        d.reason = REASON_MAX_RUN;
        return d;
    }

    if (!st.on) {
        st.on = true;
        st.onSinceMs = nowMs;
    }
    d.pumpOn = true;
    d.reason = REASON_IRRIGATING;
    return d;
}

#endif  // IRRIGATION_LOGIC_H
