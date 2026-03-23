#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_CPP="${INPUT_CPP:-generated/twist_candidates_generated_verbose.cpp}"
OUTPUT_BIN="${OUTPUT_BIN:-build/twist_registered_battery_stream}"
PRACTRAND_BIN="${PRACTRAND_BIN:-}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-0}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
SEED="${SEED:-11400714819323198485}"
TLMIN="${TLMIN:-1KB}"
TLMAX="${TLMAX:-1GB}"

if [[ -z "${PRACTRAND_BIN}" ]]; then
  if command -v RNG_test >/dev/null 2>&1; then
    PRACTRAND_BIN="$(command -v RNG_test)"
  elif [[ -x "${ROOT_DIR}/build/practrand/RNG_test" ]]; then
    PRACTRAND_BIN="${ROOT_DIR}/build/practrand/RNG_test"
  else
    echo "Set PRACTRAND_BIN to the path of RNG_test"
    exit 1
  fi
fi

if [[ ! -x "${PRACTRAND_BIN}" ]]; then
  echo "PractRand binary is not executable: ${PRACTRAND_BIN}"
  exit 1
fi

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" bash ./run_build_twist_registered_stream.sh >/dev/null
fi

STREAM_ARGS=(--bytes 0 --seed "${SEED}" --password-text "${PASSWORD_TEXT}")
if [[ -n "${CANDIDATE_ID}" ]]; then
  STREAM_ARGS+=(--candidate-id "${CANDIDATE_ID}")
else
  STREAM_ARGS+=(--candidate-index "${CANDIDATE_INDEX}")
fi

echo "PractRand run:"
echo "  input_cpp=${INPUT_CPP}"
if [[ -n "${CANDIDATE_ID}" ]]; then
  echo "  candidate_id=${CANDIDATE_ID}"
else
  echo "  candidate_index=${CANDIDATE_INDEX}"
fi
echo "  password_text=${PASSWORD_TEXT}"
echo "  practrand_bin=${PRACTRAND_BIN}"
echo "  tlmin=${TLMIN}"
echo "  tlmax=${TLMAX}"
echo

"${OUTPUT_BIN}" "${STREAM_ARGS[@]}" | "${PRACTRAND_BIN}" stdin8 -tlmin "${TLMIN}" -tlmax "${TLMAX}"
