#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_PATH="${OUTPUT_PATH:-src/ByteTwister.cpp}"
SELECTION_SEED="${SELECTION_SEED:-1337}"
BYTE_TWISTER_COUNT="${BYTE_TWISTER_COUNT:-16}"
START_DIGIT_STATE_PATH="${START_DIGIT_STATE_PATH:-generated/byte_twister_start_digit.txt}"

if [[ ! -f "${INDEX_PATH}" ]]; then
  echo "missing shard index: ${INDEX_PATH}"
  echo "run ./run_measure_twists_sharded.sh first"
  exit 1
fi

if [[ -n "${START_DIGIT+x}" ]]; then
  EFFECTIVE_START_DIGIT="${START_DIGIT}"
else
  if [[ -f "${START_DIGIT_STATE_PATH}" ]]; then
    EFFECTIVE_START_DIGIT="$(tr -d '[:space:]' < "${START_DIGIT_STATE_PATH}")"
  else
    EFFECTIVE_START_DIGIT="0"
  fi
fi

python3 tools/export_random_byte_twister_from_shards.py \
  --index "${INDEX_PATH}" \
  --output "${OUTPUT_PATH}" \
  --seed "${SELECTION_SEED}" \
  --count "${BYTE_TWISTER_COUNT}" \
  --start-digit "${EFFECTIVE_START_DIGIT}"

NEXT_START_DIGIT="$((EFFECTIVE_START_DIGIT + BYTE_TWISTER_COUNT))"
mkdir -p "$(dirname "${START_DIGIT_STATE_PATH}")"
printf '%s\n' "${NEXT_START_DIGIT}" > "${START_DIGIT_STATE_PATH}"

echo
echo "ByteTwister export:"
echo "  ${OUTPUT_PATH}"
echo "  start_digit=${EFFECTIVE_START_DIGIT}"
echo "  next_start_digit=${NEXT_START_DIGIT}"
echo "  state_file=${START_DIGIT_STATE_PATH}"
