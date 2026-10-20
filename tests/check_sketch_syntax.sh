#!/usr/bin/env bash
# Syntax/type check of the sketch against host stubs (tests/stubs).
# NOT a board build. Uses secrets.example.h as secrets.h in a temp dir, so
# a real local secrets.h is never read or printed.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
cp "$root/weatherapi_mpmcproject_01.ino" "$tmp/sketch.cpp"
cp "$root/irrigation_logic.h" "$tmp/"
cp "$root/secrets.example.h" "$tmp/secrets.h"
for https in 0 1; do
  extra=()
  if [ "$https" = 1 ]; then
    sed -i 's/#define WEATHER_USE_HTTPS 0/#define WEATHER_USE_HTTPS 1\n#define WEATHER_ROOT_CA_PEM "PLACEHOLDER"/' "$tmp/secrets.h"
  fi
  g++ -std=gnu++11 -fsyntax-only -Wall -Wextra -Wno-unused-parameter -include "$root/tests/stubs/Arduino.h" \
      -I"$root/tests/stubs" -I"$tmp" "$tmp/sketch.cpp"
  echo "syntax check OK (WEATHER_USE_HTTPS=$https)"
done
