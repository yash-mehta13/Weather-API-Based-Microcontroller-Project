#pragma once
#include "Arduino.h"
typedef enum { WL_IDLE_STATUS, WL_CONNECTED, WL_DISCONNECTED } wl_status_t;
enum { WIFI_STA };
struct WiFiClass { wl_status_t status(); void begin(const char*, const char*); void disconnect(); void mode(int); void setAutoReconnect(bool); };
extern WiFiClass WiFi;
