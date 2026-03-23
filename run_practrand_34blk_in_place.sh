#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

DEFAULT_INPUT_DIR="${ROOT_DIR}/candidates"
if [[ -d "${ROOT_DIR}/generated/candidates" ]]; then
  DEFAULT_INPUT_DIR="${ROOT_DIR}/generated/candidates"
fi
INPUT_DIR="${1:-${DEFAULT_INPUT_DIR}}"
PASSWORD_TEXT="${2:-a}"
SIZE_SPEC="${3:-32blk}"
PASSERS_DIR="${4:-}"
ZERO_FAILERS_DIR="${ZERO_FAILERS_DIR:-}"
if [[ -z "${ZERO_FAILERS_DIR}" && -n "${PASSERS_DIR}" ]]; then
  if [[ "${PASSERS_DIR}" == *_passers ]]; then
    ZERO_FAILERS_DIR="${PASSERS_DIR%_passers}_zero_failers"
  else
    ZERO_FAILERS_DIR="${PASSERS_DIR}_zero_failers"
  fi
fi
MAX_FAIL="${MAX_FAIL:-0}"
MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS:-1000}"
MAX_SUSPICIOUS="${MAX_SUSPICIOUS:-0}"
MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS:-999999}"
MAX_UNUSUAL="${MAX_UNUSUAL:-999999}"
RESUME_FLAG="${RESUME_FLAG:-0}"
NO_RESULT_FILES="${NO_RESULT_FILES:-1}"

cmd=(
  python3 -u tools/run_practrand_in_place.py
  "${INPUT_DIR}"
  "${PASSWORD_TEXT}"
  "${SIZE_SPEC}"
  --max-fail "${MAX_FAIL}"
  --max-very-suspicious "${MAX_VERY_SUSPICIOUS}"
  --max-suspicious "${MAX_SUSPICIOUS}"
  --max-mildly-suspicious "${MAX_MILDLY_SUSPICIOUS}"
  --max-unusual "${MAX_UNUSUAL}"
)

if [[ -n "${PASSERS_DIR}" ]]; then
  cmd+=(--passers-dir "${PASSERS_DIR}")
fi

if [[ -n "${ZERO_FAILERS_DIR}" ]]; then
  cmd+=(--zero-failers-dir "${ZERO_FAILERS_DIR}")
fi

if [[ "${RESUME_FLAG}" == "1" ]]; then
  cmd+=(--resume)
fi

if [[ "${NO_RESULT_FILES}" == "1" ]]; then
  cmd+=(--no-result-files)
fi

"${cmd[@]}"
