#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_CPP="${INPUT_CPP:-generated/twist_candidates_generated_verbose.cpp}"
OUTPUT_BIN="${OUTPUT_BIN:-build/twist_registered_battery_stream}"
CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA:--O3 -march=native -DNDEBUG}"

mkdir -p "$(dirname "${OUTPUT_BIN}")"

clang++ -std=c++20 ${CXXFLAGS_EXTRA} \
  -I./src \
  ./src/TwistRegisteredBatteryStream.cpp \
  ./src/PasswordExpander.cpp \
  ./src/LightningMatrix.cpp \
  ./src/HurricaneMatrix.cpp \
  ./src/TyphoonMatrix.cpp \
  "${INPUT_CPP}" \
  -o "${OUTPUT_BIN}"

echo "built ${OUTPUT_BIN}"
