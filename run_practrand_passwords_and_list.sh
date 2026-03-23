#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

DEFAULT_INPUT_DIR="${ROOT_DIR}/candidates"
if [[ -d "${ROOT_DIR}/generated/candidates" ]]; then
  DEFAULT_INPUT_DIR="${ROOT_DIR}/generated/candidates"
fi

INPUT_DIR="${1:-${DEFAULT_INPUT_DIR}}"
PASSWORD_FILE="${2:-}"
SIZE_SPEC="${3:-34blk}"
OUTPUT_DIR="${4:-}"

MAX_FAIL="${MAX_FAIL:-0}"
MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS:-1000}"
MAX_SUSPICIOUS="${MAX_SUSPICIOUS:-0}"
MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS:-999999}"
MAX_UNUSUAL="${MAX_UNUSUAL:-999999}"
KEEP_FAILING_OUTPUT="${KEEP_FAILING_OUTPUT:-0}"
WORK_DIR="${WORK_DIR:-}"

if [[ -z "${PASSWORD_FILE}" ]]; then
  echo "missing password file argument" >&2
  exit 1
fi

if [[ ! -d "${INPUT_DIR}" ]]; then
  echo "missing input dir: ${INPUT_DIR}" >&2
  exit 1
fi

if [[ ! -f "${PASSWORD_FILE}" ]]; then
  echo "missing password file: ${PASSWORD_FILE}" >&2
  exit 1
fi

if [[ -z "${OUTPUT_DIR}" ]]; then
  OUTPUT_DIR="${ROOT_DIR}/passwords_and_list_passers"
fi

cmd=(
  python3 -u tools/run_practrand_passwords_and_list.py
  "${INPUT_DIR}"
  "${PASSWORD_FILE}"
  "${SIZE_SPEC}"
  "${OUTPUT_DIR}"
  --max-fail "${MAX_FAIL}"
  --max-very-suspicious "${MAX_VERY_SUSPICIOUS}"
  --max-suspicious "${MAX_SUSPICIOUS}"
  --max-mildly-suspicious "${MAX_MILDLY_SUSPICIOUS}"
  --max-unusual "${MAX_UNUSUAL}"
)

if [[ -n "${WORK_DIR}" ]]; then
  cmd+=(--work-dir "${WORK_DIR}")
fi

if [[ "${KEEP_FAILING_OUTPUT}" == "1" ]]; then
  cmd+=(--keep-failing-output)
fi

"${cmd[@]}"
