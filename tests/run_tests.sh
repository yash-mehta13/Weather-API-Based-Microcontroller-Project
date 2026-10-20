#!/usr/bin/env bash
# Runs all host-side checks. See docs/testing-and-troubleshooting.md.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
g++ -std=c++11 -Wall -Wextra -o build/test_irrigation_logic test_irrigation_logic.cpp
./build/test_irrigation_logic
./check_sketch_syntax.sh
./check_no_secrets.sh
