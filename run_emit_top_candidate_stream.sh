#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

OUTPUT_BIN="${OUTPUT_BIN:-build/twist_battery_stream}"
BYTES="${BYTES:-104857600}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-0}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
SEED="${SEED:-11400714819323198485}"
PASSWORD_TEXT="${PASSWORD_TEXT:-}"
BITS_MODE="${BITS_MODE:-0}"
OUTPUT_PATH="${OUTPUT_PATH:-}"

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  bash ./run_build_twist_battery_stream.sh
fi

ARGS=(--bytes "${BYTES}" --seed "${SEED}")
if [[ -n "${CANDIDATE_ID}" ]]; then
  ARGS+=(--candidate-id "${CANDIDATE_ID}")
else
  ARGS+=(--candidate-index "${CANDIDATE_INDEX}")
fi
if [[ -n "${PASSWORD_TEXT}" ]]; then
  ARGS+=(--password-text "${PASSWORD_TEXT}")
fi
if [[ "${BITS_MODE}" == "1" ]]; then
  ARGS+=(--bits)
fi
if [[ -n "${OUTPUT_PATH}" ]]; then
  ARGS+=(--output "${OUTPUT_PATH}")
fi

"${OUTPUT_BIN}" "${ARGS[@]}"
