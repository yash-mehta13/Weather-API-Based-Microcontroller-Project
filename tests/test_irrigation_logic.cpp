// Host-side unit tests for irrigation_logic.h. Verifies decision logic only;
// it is NOT evidence of physical relay or pump behaviour.
#include <cstdio>
#include "../irrigation_logic.h"

static int g_fail = 0, g_run = 0;
#define CHECK(cond) do { g_run++; if (!(cond)) { g_fail++; printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static IrrigationConfig cfg() {
    IrrigationConfig c = { 2000, 200, false, false, 30.0f, 40.0f, 60000, 600000 };
    return c;
}
static IrrigationInputs good() {
    IrrigationInputs i = { true, true, 25.0f, 60.0f, true, 3000, true, RAIN_NONE };
    return i;
}

int main() {
    IrrigationConfig c = cfg();
    PumpState s = pumpStateInit();
    CHECK(!s.on);  // startup safe state

    // normal / wet soil
    IrrigationInputs in = good();
    CHECK(!decideIrrigation(c, in, s, 1000).pumpOn);

    // dry soil -> ON; boundary strict
    in.soilRaw = 1999; CHECK(decideIrrigation(c, in, s, 2000).pumpOn);
    s = pumpStateInit(); in.soilRaw = 2000; CHECK(!decideIrrigation(c, in, s, 2000).pumpOn);

    // hysteresis: stays on until >= 2200
    s = pumpStateInit(); in.soilRaw = 1500; decideIrrigation(c, in, s, 0);
    in.soilRaw = 2100; CHECK(decideIrrigation(c, in, s, 1000).pumpOn);
    in.soilRaw = 2199; CHECK(decideIrrigation(c, in, s, 2000).pumpOn);
    in.soilRaw = 2200; CHECK(!decideIrrigation(c, in, s, 3000).pumpOn);

    // heat / dry air alone do not trigger by default
    s = pumpStateInit(); in = good(); in.localTempC = 40; in.localHumidityPct = 10;
    CHECK(!decideIrrigation(c, in, s, 0).pumpOn);
    // legacy OR rule
    IrrigationConfig lc = c; lc.ambientTriggers = true;
    s = pumpStateInit(); CHECK(decideIrrigation(lc, in, s, 0).pumpOn);
    in = good(); in.localTempC = 30.0f; CHECK(!decideIrrigation(lc, in, (s = pumpStateInit()), 0).pumpOn);
    in.localTempC = 30.1f; CHECK(decideIrrigation(lc, in, (s = pumpStateInit()), 0).pumpOn);
    in = good(); in.localHumidityPct = 40.0f; CHECK(!decideIrrigation(lc, in, (s = pumpStateInit()), 0).pumpOn);
    in.localHumidityPct = 39.9f; CHECK(decideIrrigation(lc, in, (s = pumpStateInit()), 0).pumpOn);

    // rain stops a running pump
    s = pumpStateInit(); in = good(); in.soilRaw = 500;
    CHECK(decideIrrigation(c, in, s, 0).pumpOn);
    in.rain = RAIN_DETECTED;
    Decision d = decideIrrigation(c, in, s, 1000);
    CHECK(!d.pumpOn && d.reason == REASON_RAIN && !s.on);

    // each failure path forces OFF even while running and dry
    struct { int which; PumpReason r; } faults[] = {
        {0, REASON_NO_WIFI}, {1, REASON_LOCAL_SENSOR}, {2, REASON_SOIL_SENSOR},
        {3, REASON_NO_WEATHER}, {4, REASON_RAIN_UNKNOWN} };
    for (auto& f : faults) {
        s = pumpStateInit(); in = good(); in.soilRaw = 500;
        CHECK(decideIrrigation(c, in, s, 0).pumpOn);
        switch (f.which) {
            case 0: in.wifiConnected = false; break;
            case 1: in.localValid = false; break;
            case 2: in.soilValid = false; break;
            case 3: in.weatherFresh = false; break;
            case 4: in.rain = RAIN_UNKNOWN; break;
        }
        d = decideIrrigation(c, in, s, 1000);
        CHECK(!d.pumpOn && d.reason == f.r && !s.on);
    }
    // from a fresh start, a failed input never starts the pump
    s = pumpStateInit(); in = good(); in.soilRaw = 0; in.weatherFresh = false;
    CHECK(!decideIrrigation(c, in, s, 0).pumpOn);

    // recovery after error
    in = good(); in.soilRaw = 500; CHECK(decideIrrigation(c, in, s, 5000).pumpOn);

    // max run -> OFF -> cooldown -> allowed again
    s = pumpStateInit(); in = good(); in.soilRaw = 500;
    CHECK(decideIrrigation(c, in, s, 1000).pumpOn);
    CHECK(decideIrrigation(c, in, s, 60999).pumpOn);
    d = decideIrrigation(c, in, s, 61000);
    CHECK(!d.pumpOn && d.reason == REASON_MAX_RUN);
    d = decideIrrigation(c, in, s, 61000 + 599999);
    CHECK(!d.pumpOn && d.reason == REASON_COOLDOWN);
    CHECK(decideIrrigation(c, in, s, 61000 + 600000).pumpOn);

    // millis() rollover during a run
    s = pumpStateInit(); uint32_t t0 = 0xFFFFFFFFu - 10000u;
    CHECK(decideIrrigation(c, in, s, t0).pumpOn);
    CHECK(decideIrrigation(c, in, s, t0 + 30000u).pumpOn);      // wrapped, 30 s elapsed
    CHECK(!decideIrrigation(c, in, s, t0 + 60000u).pumpOn);     // 60 s: limit hit

    // inverted soil polarity
    IrrigationConfig hc = c; hc.soilHigherIsDrier = true;
    s = pumpStateInit(); in = good(); in.soilRaw = 2500;
    CHECK(decideIrrigation(hc, in, s, 0).pumpOn);
    in.soilRaw = 1801; CHECK(decideIrrigation(hc, in, s, 1000).pumpOn);
    in.soilRaw = 1800; CHECK(!decideIrrigation(hc, in, s, 2000).pumpOn);

    // run-time limit disabled
    IrrigationConfig nc = c; nc.maxRunMs = 0;
    s = pumpStateInit(); in = good(); in.soilRaw = 500;
    decideIrrigation(nc, in, s, 0); CHECK(decideIrrigation(nc, in, s, 3600000).pumpOn);

    printf("%d checks, %d failed\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
