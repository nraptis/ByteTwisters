#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

COUNT="${COUNT:-64}"
STREAM_BITS="${STREAM_BITS:-1000000}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_cat_runs}"
SEED_BASE="${SEED_BASE:-11400714819323198485}"
SUMMARY_CSV="${SUMMARY_CSV:-${OUTPUT_ROOT}/nist_summary.csv}"

mkdir -p "${OUTPUT_ROOT}"

echo "candidate_index,pass_rows,fail_rows,total_rows,final_report" > "${SUMMARY_CSV}"

echo "output_root=${OUTPUT_ROOT}"
echo "stream_bits=${STREAM_BITS}"
echo "count=${COUNT}"
echo "password_text=${PASSWORD_TEXT}"
echo

for ((i = 0; i < COUNT; ++i)); do
  seed=$((SEED_BASE + i))
  echo "[${i}/${COUNT}] candidate_index=${i}"
  STREAM_BITS="${STREAM_BITS}" \
  CANDIDATE_INDEX="${i}" \
  PASSWORD_TEXT="${PASSWORD_TEXT}" \
  SEED="${seed}" \
  OUTPUT_ROOT="${OUTPUT_ROOT}" \
  bash ./run_nist_sts_candidate.sh >/dev/null

  report_path="${OUTPUT_ROOT}/candidate_index_$(printf '%02d' "${i}")/AlgorithmTesting/finalAnalysisReport.txt"
  if [[ ! -f "${report_path}" ]]; then
    echo "${i},0,0,0,missing_report" >> "${SUMMARY_CSV}"
    echo "  report=missing"
    echo
    continue
  fi

  pass_rows=$(awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/ && /1\/1/ {count++} END {print count+0}' "${report_path}")
  fail_rows=$(awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/ && /0\/1/ {count++} END {print count+0}' "${report_path}")
  total_rows=$((pass_rows + fail_rows))

  echo "${i},${pass_rows},${fail_rows},${total_rows},${report_path}" >> "${SUMMARY_CSV}"
  echo "  pass_rows=${pass_rows} fail_rows=${fail_rows} total_rows=${total_rows}"
  echo "  final_report=${report_path}"
  echo
done

echo "summary_csv=${SUMMARY_CSV}"
