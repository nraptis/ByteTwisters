#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_CPP="${INPUT_CPP:-generated/twist_candidates_generated_verbose.cpp}"
OUTPUT_BIN="${OUTPUT_BIN:-build/twist_registered_battery_stream}"
TESTU01_LIBS="${TESTU01_LIBS:-}"
TESTU01_CFLAGS="${TESTU01_CFLAGS:-}"
ADAPTER_BIN="${ADAPTER_BIN:-build/twist_bigcrush_adapter}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-0}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
SEED="${SEED:-11400714819323198485}"
BATTERY="${BATTERY:-BigCrush}"

if [[ -z "${TESTU01_LIBS}" ]]; then
  TESTU01_LIBS="$(pkg-config --libs testu01 2>/dev/null || true)"
fi

if [[ -z "${TESTU01_CFLAGS}" ]]; then
  TESTU01_CFLAGS="$(pkg-config --cflags testu01 2>/dev/null || true)"
fi

if [[ -z "${TESTU01_LIBS}" ]]; then
  echo "TestU01 not detected. Install TestU01 or set TESTU01_LIBS/TESTU01_CFLAGS."
  exit 1
fi

mkdir -p "$(dirname "${ADAPTER_BIN}")"

clang++ -std=c++20 -O3 -march=native -DNDEBUG \
  ${TESTU01_CFLAGS} \
  -I./src \
  ./src/TwistBigCrushAdapter.cpp \
  ./src/PasswordExpander.cpp \
  ./src/LightningMatrix.cpp \
  ./src/HurricaneMatrix.cpp \
  ./src/TyphoonMatrix.cpp \
  "${INPUT_CPP}" \
  ${TESTU01_LIBS} \
  -o "${ADAPTER_BIN}"

ADAPTER_ARGS=(--seed "${SEED}" --password-text "${PASSWORD_TEXT}" --battery "${BATTERY}")
if [[ -n "${CANDIDATE_ID}" ]]; then
  ADAPTER_ARGS+=(--candidate-id "${CANDIDATE_ID}")
else
  ADAPTER_ARGS+=(--candidate-index "${CANDIDATE_INDEX}")
fi

"${ADAPTER_BIN}" "${ADAPTER_ARGS[@]}"
