// ESP32-Based Weather Monitoring and Smart Irrigation System
//
// Reads a DHT11 and a soil-moisture sensor, fetches current weather from
// OpenWeatherMap, shows data on a 16x2 I2C LCD and commands an irrigation relay.
// Fail-safe rule: the relay is commanded OFF unless every input is valid.
//
// Setup: copy secrets.example.h to secrets.h and fill it in. See README.md and
// docs/setup-and-configuration.md. Hardware behaviour (pins, relay polarity,
// LCD address, soil ADC direction) is UNVERIFIED until checked on your hardware.

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "secrets.h"            // local, git-ignored (see secrets.example.h)
#include "irrigation_logic.h"   // pure decision logic, unit-tested on a host

#if WEATHER_USE_HTTPS
#include <WiFiClientSecure.h>
#ifndef WEATHER_ROOT_CA_PEM
#error "WEATHER_USE_HTTPS is 1 but WEATHER_ROOT_CA_PEM is not defined in secrets.h"
#endif
#endif

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

// Pins (source-defined; confirm against your board and wiring)
#define RELAY_PIN            4
#define MOISTURE_SENSOR_PIN  34     // ADC1 channel, input only
#define DHT_PIN              18
#define DHT_TYPE             DHT11

// Relay polarity: original firmware drives LOW for ON and HIGH for OFF.
// Verify on your relay module before connecting a pump.
#define RELAY_ON_LEVEL       LOW
#define RELAY_OFF_LEVEL      HIGH

// LCD
#define LCD_ADDRESS          0x27
#define LCD_COLUMNS          16
#define LCD_ROWS             2
// Uncomment if your LiquidCrystal_I2C library has init() instead of begin().
// #define LCD_USE_INIT
#define ENABLE_I2C_SCAN      1      // print I2C devices found at startup

// Irrigation policy. Soil values are RAW ADC counts (0-4095), not percentages.
// Defaults reproduce the original thresholds; all need calibration.
#define SOIL_DRY_ADC             2000
#define SOIL_HYSTERESIS_ADC      200    // starting value, not calibrated
#define SOIL_HIGHER_IS_DRIER     false  // original code: ADC below threshold = dry. VERIFY.
#define AMBIENT_TRIGGERS         false  // true restores original OR rule (heat/dry air alone)
#define TEMP_HIGH_C              30.0f
#define HUMIDITY_LOW_PCT         40.0f
// Placeholders: safe values depend on the pump and plant; test with real hardware.
#define PUMP_MAX_RUN_MS          (60UL * 1000UL)
#define PUMP_COOLDOWN_MS         (10UL * 60UL * 1000UL)

// Timing
#define SENSOR_PERIOD_MS         2000UL
#define SENSOR_MAX_AGE_MS        10000UL    // DHT data older than this is invalid
#define CONTROL_PERIOD_MS        1000UL
#define WEATHER_PERIOD_MS        (10UL * 60UL * 1000UL)
#define WEATHER_RETRY_MS         (60UL * 1000UL)
#define WEATHER_MAX_AGE_MS       (25UL * 60UL * 1000UL)
#define HTTP_TIMEOUT_MS          8000
#define LCD_PAGE_MS              3000UL
#define WIFI_ATTEMPT_TIMEOUT_MS  15000UL
#define WIFI_BACKOFF_MIN_MS      5000UL
#define WIFI_BACKOFF_MAX_MS      (5UL * 60UL * 1000UL)

// Plausibility limits
#define TEMP_MIN_C   (-40.0f)
#define TEMP_MAX_C   (80.0f)
#define ADC_MAX      4095

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

enum WeatherError : uint8_t {
    WX_OK = 0, WX_NONE_YET, WX_TRANSPORT, WX_AUTH, WX_NOT_FOUND,
    WX_RATE_LIMIT, WX_HTTP, WX_JSON, WX_FIELDS
};

struct WeatherData {
    float      tempC;
    float      humidityPct;
    RainStatus rain;
};

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

const IrrigationConfig kConfig = {
    SOIL_DRY_ADC, SOIL_HYSTERESIS_ADC, SOIL_HIGHER_IS_DRIER, AMBIENT_TRIGGERS,
    TEMP_HIGH_C, HUMIDITY_LOW_PCT, PUMP_MAX_RUN_MS, PUMP_COOLDOWN_MS
};

PumpState g_pump = pumpStateInit();
bool      g_pumpCommandedOn = false;   // last level written to the relay
Decision  g_decision = { false, REASON_NO_WEATHER };

// Local readings
float    g_localTemp = 0, g_localHumidity = 0;
uint32_t g_localReadMs = 0;
bool     g_localEverRead = false;
int      g_soilRaw = 0;
bool     g_soilValid = false;

// Weather
WeatherData  g_weather = { 0, 0, RAIN_UNKNOWN };
uint32_t     g_weatherOkMs = 0;
bool         g_weatherEverOk = false;
uint32_t     g_nextWeatherMs = 0;
WeatherError g_weatherErr = WX_NONE_YET;

// Wi-Fi state machine
bool     g_wifiAttempting = false;
uint32_t g_wifiAttemptStartMs = 0;
uint32_t g_wifiNextAttemptMs = 0;
uint32_t g_wifiBackoffMs = WIFI_BACKOFF_MIN_MS;

// Scheduler
uint32_t g_lastSensorMs = 0, g_lastControlMs = 0, g_lastPageMs = 0;
uint8_t  g_lcdPage = 0;
char     g_lcdCache[LCD_ROWS][LCD_COLUMNS + 1];

// ---------------------------------------------------------------------------
// Relay
// ---------------------------------------------------------------------------

// Only place that writes the relay pin. This records the COMMAND; it does not
// confirm that a pump is physically running.
void setPump(bool on) {
    digitalWrite(RELAY_PIN, on ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
    if (on != g_pumpCommandedOn) {
        Serial.println(on ? "Pump command: ON" : "Pump command: OFF");
    }
    g_pumpCommandedOn = on;
}

// ---------------------------------------------------------------------------
// LCD
// ---------------------------------------------------------------------------

// Writes a padded 16-char line only if it changed, avoiding lcd.clear() flicker.
void lcdLine(uint8_t row, const char* text) {
    char buf[LCD_COLUMNS + 1];
    snprintf(buf, sizeof(buf), "%-*.*s", LCD_COLUMNS, LCD_COLUMNS, text);
    if (strcmp(buf, g_lcdCache[row]) == 0) return;
    lcd.setCursor(0, row);
    lcd.print(buf);
    strcpy(g_lcdCache[row], buf);
}

bool localFresh(uint32_t now) {
    return g_localEverRead && elapsedMs(now, g_localReadMs) <= SENSOR_MAX_AGE_MS;
}

bool weatherFresh(uint32_t now) {
    return g_weatherEverOk && elapsedMs(now, g_weatherOkMs) <= WEATHER_MAX_AGE_MS;
}

void updateLcd(uint32_t now) {
    char a[24], b[24];
    bool lf = localFresh(now), wf = weatherFresh(now);
    switch (g_lcdPage) {
        case 0:
            if (lf) snprintf(a, sizeof(a), "Loc %.1fC %.0f%%", g_localTemp, g_localHumidity);
            else    snprintf(a, sizeof(a), "Loc DHT error");
            if (g_soilValid) snprintf(b, sizeof(b), "Soil ADC: %d", g_soilRaw);
            else             snprintf(b, sizeof(b), "Soil ADC: --");
            break;
        case 1:
            if (wf) {
                snprintf(a, sizeof(a), "API %.1fC %.0f%%", g_weather.tempC, g_weather.humidityPct);
                snprintf(b, sizeof(b), "Rain: %s",
                         g_weather.rain == RAIN_DETECTED ? "yes" :
                         g_weather.rain == RAIN_NONE ? "none" : "unknown");
            } else {
                snprintf(a, sizeof(a), "API: no data");
                snprintf(b, sizeof(b), "%s", WiFi.status() == WL_CONNECTED ? "Weather error" : "Wi-Fi down");
            }
            break;
        default:
            snprintf(a, sizeof(a), "Pump cmd: %s", g_pumpCommandedOn ? "ON" : "OFF");
            snprintf(b, sizeof(b), "%s", reasonText(g_decision.reason));
            break;
    }
    lcdLine(0, a);
    lcdLine(1, b);
}

// ---------------------------------------------------------------------------
// Wi-Fi (non-blocking, bounded attempts, exponential backoff)
// ---------------------------------------------------------------------------

void serviceWifi(uint32_t now) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
        if (g_wifiAttempting) {
            Serial.println("Wi-Fi connected");
            g_wifiAttempting = false;
        }
        g_wifiBackoffMs = WIFI_BACKOFF_MIN_MS;
        return;
    }
    if (g_wifiAttempting) {
        if (elapsedMs(now, g_wifiAttemptStartMs) >= WIFI_ATTEMPT_TIMEOUT_MS) {
            Serial.println("Wi-Fi attempt timed out");
            WiFi.disconnect();
            g_wifiAttempting = false;
            g_wifiNextAttemptMs = now + g_wifiBackoffMs;
            g_wifiBackoffMs = (g_wifiBackoffMs * 2 > WIFI_BACKOFF_MAX_MS) ? WIFI_BACKOFF_MAX_MS : g_wifiBackoffMs * 2;
        }
        return;
    }
    if ((int32_t)(now - g_wifiNextAttemptMs) >= 0) {
        Serial.println("Wi-Fi connecting...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        g_wifiAttempting = true;
        g_wifiAttemptStartMs = now;
    }
}

// ---------------------------------------------------------------------------
// Weather API
// ---------------------------------------------------------------------------

String urlEncode(const char* s) {
    String out;
    for (; *s; ++s) {
        char c = *s;
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~' || c == ',') out += c;
        else { char h[4]; snprintf(h, sizeof(h), "%%%02X", (unsigned char)c); out += h; }
    }
    return out;
}

// Reads rain.1h (or rain.3h). "rain" absent in a valid response = none reported.
RainStatus parseRain(JSONVar& w) {
    RainStatus status = RAIN_NONE;
    if (w.hasOwnProperty("rain")) {
        JSONVar r = w["rain"];
        const char* keys[] = { "1h", "3h" };
        status = RAIN_UNKNOWN;
        for (uint8_t i = 0; i < 2; i++) {
            if (JSON.typeof(r[keys[i]]) == "number") {
                double mm = (double)r[keys[i]];
                if (mm < 0 || mm > 1000) return RAIN_UNKNOWN;
                status = mm > 0 ? RAIN_DETECTED : RAIN_NONE;
                break;
            }
        }
    }
    // Weather condition ids 200-531 (thunderstorm, drizzle, rain) also count as rain.
    if (status != RAIN_DETECTED && JSON.typeof(w["weather"]) == "array" &&
        JSON.typeof(w["weather"][0]["id"]) == "number") {
        int id = (int)w["weather"][0]["id"];
        if (id >= 200 && id <= 531) status = RAIN_DETECTED;
    }
    return status;
}

WeatherError parseWeather(const String& payload, WeatherData& out) {
    JSONVar w = JSON.parse(payload);
    if (JSON.typeof(w) != "object") return WX_JSON;
    if (JSON.typeof(w["main"]) != "object" ||
        JSON.typeof(w["main"]["temp"]) != "number" ||
        JSON.typeof(w["main"]["humidity"]) != "number") return WX_FIELDS;
    double t = (double)w["main"]["temp"];
    double h = (double)w["main"]["humidity"];
    if (t < TEMP_MIN_C || t > TEMP_MAX_C || h < 0 || h > 100) return WX_FIELDS;
    out.tempC = (float)t;
    out.humidityPct = (float)h;
    out.rain = parseRain(w);
    return WX_OK;
}

WeatherError fetchWeather(WeatherData& out) {
    String url = String(WEATHER_USE_HTTPS ? "https" : "http") +
                 "://api.openweathermap.org/data/2.5/weather?q=" + urlEncode(WEATHER_CITY) +
                 "&appid=" + WEATHER_API_KEY + "&units=metric";   // never printed
    HTTPClient http;
#if WEATHER_USE_HTTPS
    WiFiClientSecure client;
    client.setCACert(WEATHER_ROOT_CA_PEM);   // certificate verification stays enabled
    bool began = http.begin(client, url);
#else
    bool began = http.begin(url);
#endif
    if (!began) return WX_TRANSPORT;
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    int code = http.GET();
    WeatherError result;
    if (code == 200)      result = parseWeather(http.getString(), out);
    else if (code <= 0)   result = WX_TRANSPORT;
    else if (code == 401) result = WX_AUTH;
    else if (code == 404) result = WX_NOT_FOUND;
    else if (code == 429) result = WX_RATE_LIMIT;
    else                  result = WX_HTTP;
    http.end();
    if (result != WX_OK) {
        Serial.print("Weather request failed, code "); Serial.print(code);
        Serial.print(", error "); Serial.println((int)result);
    }
    return result;
}

void serviceWeather(uint32_t now) {
    if (WiFi.status() != WL_CONNECTED) return;
    if ((int32_t)(now - g_nextWeatherMs) < 0) return;
    WeatherData wd;
    g_weatherErr = fetchWeather(wd);
    if (g_weatherErr == WX_OK) {
        g_weather = wd;
        g_weatherOkMs = millis();
        g_weatherEverOk = true;
        g_nextWeatherMs = millis() + WEATHER_PERIOD_MS;
        Serial.print("Weather OK: "); Serial.print(wd.tempC); Serial.print("C ");
        Serial.print(wd.humidityPct); Serial.println("%");
    } else {
        g_nextWeatherMs = millis() + WEATHER_RETRY_MS;
    }
}

// ---------------------------------------------------------------------------
// Local sensors
// ---------------------------------------------------------------------------

void readSensors(uint32_t now) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h) && t >= TEMP_MIN_C && t <= TEMP_MAX_C && h >= 0 && h <= 100) {
        g_localTemp = t; g_localHumidity = h;
        g_localReadMs = now; g_localEverRead = true;
    } else {
        Serial.println("DHT read failed");
    }
    int raw = analogRead(MOISTURE_SENSOR_PIN);
    g_soilValid = (raw >= 0 && raw <= ADC_MAX);
    g_soilRaw = raw;
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void runControl(uint32_t now) {
    IrrigationInputs in;
    in.wifiConnected = (WiFi.status() == WL_CONNECTED);
    in.localValid = localFresh(now);
    in.localTempC = g_localTemp;
    in.localHumidityPct = g_localHumidity;
    in.soilValid = g_soilValid;
    in.soilRaw = g_soilRaw;
    in.weatherFresh = weatherFresh(now);
    in.rain = in.weatherFresh ? g_weather.rain : RAIN_UNKNOWN;

    g_decision = decideIrrigation(kConfig, in, g_pump, now);
    setPump(g_decision.pumpOn);
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
    // Safe state first: relay OFF before anything else can fail.
    digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);

    Serial.begin(115200);
    Serial.println("\nWeather irrigation controller starting");

    Wire.begin();
#if ENABLE_I2C_SCAN
    for (byte address = 8; address < 120; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("I2C device at 0x"); Serial.println(address, HEX);
        }
    }
#endif

    dht.begin();
#ifdef LCD_USE_INIT
    lcd.init();
#else
    lcd.begin();
#endif
    lcd.backlight();
    lcd.clear();
    for (uint8_t r = 0; r < LCD_ROWS; r++) g_lcdCache[r][0] = '\0';
    lcdLine(0, "Starting...");
    lcdLine(1, "Pump OFF");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    pinMode(MOISTURE_SENSOR_PIN, INPUT);
}

void loop() {
    uint32_t now = millis();

    serviceWifi(now);
    serviceWeather(now);

    if (elapsedMs(now, g_lastSensorMs) >= SENSOR_PERIOD_MS || g_lastSensorMs == 0) {
        g_lastSensorMs = now ? now : 1;
        readSensors(now);
    }
    if (elapsedMs(now, g_lastControlMs) >= CONTROL_PERIOD_MS) {
        g_lastControlMs = now;
        runControl(now);
    }
    if (elapsedMs(now, g_lastPageMs) >= LCD_PAGE_MS) {
        g_lastPageMs = now;
        g_lcdPage = (g_lcdPage + 1) % 3;
    }
    updateLcd(now);
    delay(50);
}
