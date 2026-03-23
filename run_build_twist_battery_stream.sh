#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

OUTPUT_BIN="${OUTPUT_BIN:-build/twist_battery_stream}"

mkdir -p "$(dirname "${OUTPUT_BIN}")"

clang++ -std=c++20 -O2 \
  -I./src \
  ./src/TwistBatteryStream.cpp \
  ./src/LightningMatrix.cpp \
  -o "${OUTPUT_BIN}"

echo "built ${OUTPUT_BIN}"
