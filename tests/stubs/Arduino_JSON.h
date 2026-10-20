#pragma once
#include "Arduino.h"
struct JSONVar {
    JSONVar(); JSONVar operator[](const char*); JSONVar operator[](int);
    bool hasOwnProperty(const char*); operator double(); operator int();
};
// The real library exposes typeof_ and maps typeof onto it (typeof is a GNU keyword).
struct JSONClass { JSONVar parse(const String&); String typeof_(JSONVar&); String typeof_(JSONVar&&); };
#define typeof typeof_
extern JSONClass JSON;
inline bool operator!=(const String& a, const char* b) { return std::string(a) != b; }
inline bool operator==(const String& a, const char* b) { return std::string(a) == b; }
