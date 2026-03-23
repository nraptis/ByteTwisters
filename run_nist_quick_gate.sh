#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

PROJECT_ROOT="${PROJECT_ROOT:-${ROOT_DIR}}"
INPUT_DIR="${INPUT_DIR:-${PROJECT_ROOT}/candidates}"
QUICK_STREAM_BYTES="${QUICK_STREAM_BYTES:-61440}"
QUICK_MAX_FAIL_ROWS="${QUICK_MAX_FAIL_ROWS:-24}"

# Cheap first gate for huge candidate counts.
# These sizes are intentionally small and are meant to catch obvious startup bias,
# not certify quality. The default stream is 8 full 7680-byte twister blocks.
STAGE_SPECS="${STAGE_SPECS:-a:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},b:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},c:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},d:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},e:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},f:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},g:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},h:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},i:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},j:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},k:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},l:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},m:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},n:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},o:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},p:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},q:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},r:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},s:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},t:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},u:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},v:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},w:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},x:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},y:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},z:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},A:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},B:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},C:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},D:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},E:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},F:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},G:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},H:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},I:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},J:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},K:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},L:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},M:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},N:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},O:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},P:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},Q:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},R:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},S:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},T:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},U:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},V:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},W:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},X:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},Y:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS},Z:${QUICK_STREAM_BYTES}b:${QUICK_MAX_FAIL_ROWS}}"

format_stage_tag() {
  local size_spec="$1"
  local lower
  lower="$(printf '%s' "${size_spec}" | tr '[:upper:]' '[:lower:]' | tr -d ' ')"
  local digits="${lower%%[!0-9]*}"
  local unit="${lower#$digits}"
  if [[ -z "${unit}" || "${unit}" == "mb" || "${unit}" == "m" || "${unit}" == "mib" ]]; then
    printf '%03dMB' "${digits}"
  elif [[ "${unit}" == "blk" || "${unit}" == "block" || "${unit}" == "blocks" ]]; then
    printf '%03dBLK' "${digits}"
  elif [[ "${unit}" == "kb" || "${unit}" == "k" || "${unit}" == "kib" ]]; then
    printf '%03dKB' "${digits}"
  elif [[ "${unit}" == "gb" || "${unit}" == "g" || "${unit}" == "gib" ]]; then
    printf '%03dGB' "${digits}"
  elif [[ "${unit}" == "b" || "${unit}" == "byte" || "${unit}" == "bytes" ]]; then
    printf '%05dB' "${digits}"
  else
    printf '%s' "${size_spec}"
  fi
}

if [[ ! -d "${INPUT_DIR}" ]]; then
  echo "missing input candidate folder: ${INPUT_DIR}"
  exit 1
fi

current_input="${INPUT_DIR}"
IFS=',' read -r -a stage_specs <<< "${STAGE_SPECS}"

echo "quick_nist_gate=1"
echo "input_dir=${INPUT_DIR}"
echo "quick_stream_bytes=${QUICK_STREAM_BYTES}"
echo "quick_stream_blocks=$(( QUICK_STREAM_BYTES / 7680 ))"
echo "quick_max_fail_rows=${QUICK_MAX_FAIL_ROWS}"
echo "note=tiny NIST stages are a coarse startup-bias filter, not a final quality metric"

for stage_spec in "${stage_specs[@]}"; do
  password_text="${stage_spec%%:*}"
  remainder="${stage_spec#*:}"
  size_spec="${remainder%%:*}"
  max_fail_rows="${remainder##*:}"
  size_tag="$(format_stage_tag "${size_spec}")"
  output_dir="${PROJECT_ROOT}/candidates_filtered_quick_nist_${password_text}_${size_tag}"
  summary_csv="${output_dir}/summary_${password_text}_${size_tag}.csv"
  histogram_txt="${output_dir}/failure_histogram_${password_text}_${size_tag}.txt"
  histogram_csv="${output_dir}/failure_histogram_${password_text}_${size_tag}.csv"

  echo
  echo "stage password=${password_text} size_spec=${size_spec}"
  echo "max_fail_rows=${max_fail_rows}"
  echo "input_dir=${current_input}"
  echo "output_dir=${output_dir}"

  bash ./execute_nist_filter.sh "${current_input}" "${output_dir}" "${password_text}" "${size_spec}" "${max_fail_rows}"

  python3 tools/nist_fail_histogram.py \
    "${summary_csv}" \
    --output-txt "${histogram_txt}" \
    --output-csv "${histogram_csv}"

  echo "passed_candidates=$(wc -l < "${output_dir}/passed_candidates.txt" | tr -d ' ')"
  echo "failed_candidates=$(wc -l < "${output_dir}/failed_candidates.txt" | tr -d ' ')"
  echo "histogram_txt=${histogram_txt}"

  if [[ ! -s "${output_dir}/passed_candidates.txt" ]]; then
    echo "no candidates passed stage ${password_text}_${size_tag}; stopping quick gate"
    exit 0
  fi

  current_input="${output_dir}"
done

echo
echo "final_output_dir=${current_input}"
