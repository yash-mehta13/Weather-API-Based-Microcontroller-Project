// Minimal host stubs for syntax/type checking only. They encode MY assumptions
// about the Arduino/ESP32 APIs and are not a substitute for a real board build.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <math.h>
#include <ctype.h>
typedef uint8_t byte;
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define HEX 16
unsigned long millis(); void delay(unsigned long); int analogRead(int);
void pinMode(int, int); void digitalWrite(int, int);
struct String : std::string {
    String() {} String(const char* s) : std::string(s) {} String(const std::string& s) : std::string(s) {}
    String& operator+=(char c) { append(1, c); return *this; }
    String& operator+=(const char* c) { append(c); return *this; }
    String operator+(const char* c) const { return String(std::string(*this) + c); }
    String operator+(const String& c) const { return String(std::string(*this) + std::string(c)); }
    const char* c_str() const { return std::string::c_str(); }
};
inline String operator+(const char* a, const String& b) { return String(std::string(a) + std::string(b)); }
struct SerialClass { void begin(long); void println(const char*); void println(int); void println(unsigned, int); void println(float); void print(const char*); void print(int); void print(float); };
extern SerialClass Serial;
