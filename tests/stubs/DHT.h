#pragma once
#include "Arduino.h"
#define DHT11 11
struct DHT { DHT(int, int); void begin(); float readTemperature(); float readHumidity(); };
