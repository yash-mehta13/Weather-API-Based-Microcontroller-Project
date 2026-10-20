#pragma once
#include "Arduino.h"
struct WiFiClient {};
struct WiFiClientSecure : WiFiClient { void setCACert(const char*); };
