#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

PROJECT_ROOT="${PROJECT_ROOT:-${ROOT_DIR}}"
INPUT_DIR="${INPUT_DIR:-${PROJECT_ROOT}/candidates}"
STAGE_SPECS="${STAGE_SPECS:-a:1blk:24,b:1blk:24,c:1blk:24,d:1blk:24,e:1blk:24,f:1blk:24,g:1blk:24,ape:2blk:24,bat:2blk:24,cat:2blk:24,dog:2blk:24,elk:2blk:24,fat:2blk:24,gun:2blk:24,aces:4blk:12,bear:4blk:12,camp:4blk:12,dark:4blk:12,eats:4blk:12,fang:4blk:12,goat:4blk:12,amped:8blk:7,barbs:8blk:7,champ:8blk:7,deals:8blk:7,ember:8blk:7,foals:8blk:7,goose:8blk:7}"

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

for stage_spec in "${stage_specs[@]}"; do
  password_text="${stage_spec%%:*}"
  remainder="${stage_spec#*:}"
  size_spec="${remainder%%:*}"
  max_fail_rows="${remainder##*:}"
  size_tag="$(format_stage_tag "${size_spec}")"
  output_dir="${PROJECT_ROOT}/candidates_filtered_${password_text}_${size_tag}"
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
    echo "no candidates passed stage ${password_text}_${size_tag}; stopping ladder"
    exit 0
  fi

  current_input="${output_dir}"
done

echo
echo "final_output_dir=${current_input}"
