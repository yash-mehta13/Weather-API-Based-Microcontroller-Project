#pragma once
#include "Arduino.h"
struct TwoWire { void begin(); void beginTransmission(int); int endTransmission(); };
extern TwoWire Wire;
