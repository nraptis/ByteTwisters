#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

if [[ "$#" -lt 4 ]]; then
  echo "usage: $0 <input_candidates_dir> <output_dir> <password_text> <size_spec> [max_failed_tests] [max_weak_tests]"
  exit 1
fi

INPUT_DIR="$1"
OUTPUT_DIR="$2"
PASSWORD_TEXT="$3"
SIZE_SPEC="$4"
MAX_FAILED_TESTS="${5:-0}"
MAX_WEAK_TESTS="${6:-999999}"

python3 -u tools/execute_dieharder_filter.py \
  "${INPUT_DIR}" \
  "${OUTPUT_DIR}" \
  "${PASSWORD_TEXT}" \
  "${SIZE_SPEC}" \
  --max-failed-tests "${MAX_FAILED_TESTS}" \
  --max-weak-tests "${MAX_WEAK_TESTS}"
