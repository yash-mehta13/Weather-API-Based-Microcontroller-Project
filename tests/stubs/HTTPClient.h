#pragma once
#include "Arduino.h"
#include "WiFiClientSecure.h"
struct HTTPClient { bool begin(String); bool begin(WiFiClient&, String); void setTimeout(int); void setConnectTimeout(int); int GET(); String getString(); void end(); };
