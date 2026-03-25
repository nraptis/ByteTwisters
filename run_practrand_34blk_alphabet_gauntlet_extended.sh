#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

DEFAULT_INPUT_DIR="${ROOT_DIR}/candidates"
if [[ -d "${ROOT_DIR}/generated/candidates" ]]; then
  DEFAULT_INPUT_DIR="${ROOT_DIR}/generated/candidates"
fi

INPUT_DIR="${1:-${DEFAULT_INPUT_DIR}}"
OUTPUT_DIR="${2:-${ROOT_DIR}/alphabet_gauntlet_34blk_extended_passers}"
PASSWORD_FILE="${PASSWORD_FILE:-}"

temp_password_file=""
cleanup() {
  if [[ -n "${temp_password_file}" && -f "${temp_password_file}" ]]; then
    rm -f "${temp_password_file}"
  fi
}
trap cleanup EXIT

if [[ -z "${PASSWORD_FILE}" ]]; then
  temp_password_file="$(mktemp "${TMPDIR:-/tmp}/alphabet_gauntlet_extended_XXXXXX.txt")"
  python3 - <<'PY' > "${temp_password_file}"
for value in range(ord("a"), ord("z") + 1):
    print(chr(value))

for value in range(ord("A"), ord("Z") + 1):
    print(chr(value))

letters = "abcdefghijklmnopqrstuvwxyz"
for index, letter in enumerate(letters):
    print(letter + letters[(index + 1) % len(letters)])
for index, letter in enumerate(letters):
    print(letters[(index + 1) % len(letters)] + letter)


PY
  PASSWORD_FILE="${temp_password_file}"
fi

MAX_FAIL="${MAX_FAIL:-0}"
MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS:-10}"
MAX_SUSPICIOUS="${MAX_SUSPICIOUS:-10}"
MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS:-10}"
MAX_UNUSUAL="${MAX_UNUSUAL:-10}"
KEEP_FAILING_OUTPUT="${KEEP_FAILING_OUTPUT:-0}"
WORK_DIR="${WORK_DIR:-}"

env \
  MAX_FAIL="${MAX_FAIL}" \
  MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS}" \
  MAX_SUSPICIOUS="${MAX_SUSPICIOUS}" \
  MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS}" \
  MAX_UNUSUAL="${MAX_UNUSUAL}" \
  KEEP_FAILING_OUTPUT="${KEEP_FAILING_OUTPUT}" \
  WORK_DIR="${WORK_DIR}" \
  "${ROOT_DIR}/run_practrand_passwords_and_list.sh" \
  "${INPUT_DIR}" \
  "${PASSWORD_FILE}" \
  "34blk" \
  "${OUTPUT_DIR}"
