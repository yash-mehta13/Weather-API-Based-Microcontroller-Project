// secrets.example.h
//
// Copy this file to "secrets.h" (same folder as the .ino) and edit the copy.
//   cp secrets.example.h secrets.h
// secrets.h is listed in .gitignore. NEVER commit it, and never put real
// values in this example file.

#ifndef SECRETS_H
#define SECRETS_H

// Wi-Fi network (2.4 GHz; the ESP32 does not support 5 GHz).
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"

// OpenWeatherMap API key (https://openweathermap.org/api). Treat it as a secret.
#define WEATHER_API_KEY "YOUR_OPENWEATHERMAP_API_KEY"

// City passed to the "q" query parameter, e.g. "Vellore" or "Vellore,IN".
#define WEATHER_CITY    "Vellore"

// Transport. 0 = plain HTTP: the API key travels unencrypted (original behaviour).
// 1 = HTTPS with certificate verification. For 1 you must also define
// WEATHER_ROOT_CA_PEM below (see docs/weather-api-integration.md).
#define WEATHER_USE_HTTPS 0

#if WEATHER_USE_HTTPS
// Root CA certificate (PEM) that signs api.openweathermap.org.
// Define WEATHER_ROOT_CA_PEM as one string literal (PEM text with "\n" line ends):
//   "-----BEGIN CERTIFICATE-----\n"
//   "...base64 lines...\n"
//   "-----END CERTIFICATE-----\n"
#endif

#endif  // SECRETS_H
