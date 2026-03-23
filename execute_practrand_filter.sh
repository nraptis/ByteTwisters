#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

if [[ "$#" -lt 4 ]]; then
  echo "usage: $0 <input_candidates_dir> <output_dir> <password_text> <size_spec> [max_fail] [max_very_suspicious] [max_suspicious] [max_mildly_suspicious] [max_unusual]"
  exit 1
fi

INPUT_DIR="$1"
OUTPUT_DIR="$2"
PASSWORD_TEXT="$3"
SIZE_SPEC="$4"
MAX_FAIL="${5:-0}"
MAX_VERY_SUSPICIOUS="${6:-0}"
MAX_SUSPICIOUS="${7:-0}"
MAX_MILDLY_SUSPICIOUS="${8:-999999}"
MAX_UNUSUAL="${9:-999999}"

python3 -u tools/execute_practrand_filter.py \
  "${INPUT_DIR}" \
  "${OUTPUT_DIR}" \
  "${PASSWORD_TEXT}" \
  "${SIZE_SPEC}" \
  --max-fail "${MAX_FAIL}" \
  --max-very-suspicious "${MAX_VERY_SUSPICIOUS}" \
  --max-suspicious "${MAX_SUSPICIOUS}" \
  --max-mildly-suspicious "${MAX_MILDLY_SUSPICIOUS}" \
  --max-unusual "${MAX_UNUSUAL}"
