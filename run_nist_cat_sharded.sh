#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SHARD_SIZE_OVERRIDE="${1:-${SHARD_SIZE:-500}}"
RESUME_MODE="${RESUME_MODE:-1}"
START_SHARD_INDEX="${START_SHARD_INDEX:-0}"
START_CANDIDATE_OFFSET="${START_CANDIDATE_OFFSET:-0}"
MAX_CANDIDATES_PER_SHARD="${MAX_CANDIDATES_PER_SHARD:-0}"
MAX_SHARDS="${MAX_SHARDS:-0}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
STREAM_BITS="${STREAM_BITS:-1073741824}"
INPUT_CPP="generated/twist_candidates_generated_verbose.cpp"
INDEX_PATH="generated/shards_index.json"
SHARD_CPP_DIR="generated/shards_cpp"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_cat_sharded}"
PASSERS_PATH="${PASSERS_PATH:-${OUTPUT_ROOT}/nist_cat_passers.txt}"
SUMMARY_CSV="${SUMMARY_CSV:-${OUTPUT_ROOT}/nist_cat_summary.csv}"
BUILD_DIR="build/nist_shards"

if [[ ! -f "${INPUT_CPP}" ]]; then
  echo "missing generated input"
  echo "expected: ${INPUT_CPP}"
  echo "run ./generate_twists.sh first"
  exit 1
fi

if [[ "${RESUME_MODE}" != "1" ]]; then
  rm -rf "${OUTPUT_ROOT}" "${BUILD_DIR}"
fi

mkdir -p "${OUTPUT_ROOT}" "${BUILD_DIR}" "${SHARD_CPP_DIR}"

if [[ ! -f "${INDEX_PATH}" || "${INPUT_CPP}" -nt "${INDEX_PATH}" ]]; then
  python3 tools/split_twist_candidates.py \
    --input "${INPUT_CPP}" \
    --output-dir "${SHARD_CPP_DIR}" \
    --shard-size "${SHARD_SIZE_OVERRIDE}" \
    --index-output "${INDEX_PATH}"
fi

echo "password_text=${PASSWORD_TEXT}"
echo "stream_bits=${STREAM_BITS}"
echo "shard_size=${SHARD_SIZE_OVERRIDE}"
echo "resume_mode=${RESUME_MODE}"
echo "start_shard_index=${START_SHARD_INDEX}"
echo "start_candidate_offset=${START_CANDIDATE_OFFSET}"
echo "max_candidates_per_shard=${MAX_CANDIDATES_PER_SHARD:-all}"
echo "max_shards=${MAX_SHARDS:-all}"
echo "output_root=${OUTPUT_ROOT}"

if [[ ! -f "${SUMMARY_CSV}" || "${RESUME_MODE}" != "1" ]]; then
  echo "shard_name,global_candidate_id,local_candidate_index,pass_rows,fail_rows,total_rows,final_report" > "${SUMMARY_CSV}"
fi

if [[ ! -f "${PASSERS_PATH}" || "${RESUME_MODE}" != "1" ]]; then
  cat > "${PASSERS_PATH}" <<EOF
NIST cat passers
password_text=${PASSWORD_TEXT}
stream_bits=${STREAM_BITS}
summary_csv=${SUMMARY_CSV}

EOF
fi

SHARD_SOURCES=()
while IFS= read -r shard_source; do
  SHARD_SOURCES+=("${shard_source}")
done < <(find "${SHARD_CPP_DIR}" -name 'shard_*.cpp' | sort)
TOTAL_SHARDS="${#SHARD_SOURCES[@]}"
if [[ "${TOTAL_SHARDS}" -eq 0 ]]; then
  echo "no shard sources were created"
  exit 1
fi

for ((shard_pos = 0; shard_pos < TOTAL_SHARDS; ++shard_pos)); do
  if (( shard_pos < START_SHARD_INDEX )); then
    continue
  fi
  if (( MAX_SHARDS > 0 )) && (( shard_pos >= START_SHARD_INDEX + MAX_SHARDS )); then
    break
  fi

  SHARD_SOURCE="${SHARD_SOURCES[shard_pos]}"
  SHARD_NAME="$(basename "${SHARD_SOURCE}" .cpp)"
  SHARD_OUTPUT_DIR="${OUTPUT_ROOT}/${SHARD_NAME}"
  SHARD_BIN="${BUILD_DIR}/${SHARD_NAME}_stream"
  SHARD_SUMMARY_CSV="${SHARD_OUTPUT_DIR}/nist_summary.csv"

  mkdir -p "${SHARD_OUTPUT_DIR}"

  echo
  echo "[${shard_pos}/${TOTAL_SHARDS}] compiling ${SHARD_NAME}"
  INPUT_CPP="${SHARD_SOURCE}" OUTPUT_BIN="${SHARD_BIN}" bash ./run_build_twist_registered_stream.sh >/dev/null

  candidate_count="$("${SHARD_BIN}" --count-only)"
  if [[ -z "${candidate_count}" ]]; then
    echo "failed to determine candidate count for ${SHARD_NAME}"
    exit 1
  fi

  if [[ ! -f "${SHARD_SUMMARY_CSV}" || "${RESUME_MODE}" != "1" ]]; then
    echo "local_candidate_index,pass_rows,fail_rows,total_rows,final_report" > "${SHARD_SUMMARY_CSV}"
  fi

  start_local_index=0
  if (( shard_pos == START_SHARD_INDEX )); then
    start_local_index="${START_CANDIDATE_OFFSET}"
  fi

  max_local_index="${candidate_count}"
  if (( MAX_CANDIDATES_PER_SHARD > 0 )); then
    capped_end=$((start_local_index + MAX_CANDIDATES_PER_SHARD))
    if (( capped_end < max_local_index )); then
      max_local_index="${capped_end}"
    fi
  fi

  for ((local_index = start_local_index; local_index < max_local_index; ++local_index)); do
    if [[ "${RESUME_MODE}" == "1" && -f "${SHARD_OUTPUT_DIR}/candidate_index_$(printf '%04d' "${local_index}")/AlgorithmTesting/finalAnalysisReport.txt" ]]; then
      echo "  skipping ${SHARD_NAME} candidate_index=${local_index} (already has report)"
      continue
    fi

    global_candidate_id="$(python3 - <<PY
import json
from pathlib import Path
index = json.loads(Path("${INDEX_PATH}").read_text(encoding="utf-8"))
for shard in index["shards"]:
    if shard["path"] == "${SHARD_SOURCE}":
        print(shard["start_candidate_id"] + ${local_index})
        break
PY
)"

    echo "  ${SHARD_NAME} candidate_index=${local_index} candidate_id=${global_candidate_id}"
    INPUT_CPP="${SHARD_SOURCE}" \
    OUTPUT_BIN="${SHARD_BIN}" \
    OUTPUT_ROOT="${SHARD_OUTPUT_DIR}" \
    STREAM_BITS="${STREAM_BITS}" \
    CANDIDATE_INDEX="${local_index}" \
    PASSWORD_TEXT="${PASSWORD_TEXT}" \
    RUN_LABEL="candidate_index_$(printf '%04d' "${local_index}")" \
    bash ./run_nist_sts_registered_candidate.sh >/dev/null

    report_path="${SHARD_OUTPUT_DIR}/candidate_index_$(printf '%04d' "${local_index}")/AlgorithmTesting/finalAnalysisReport.txt"
    if [[ ! -f "${report_path}" ]]; then
      echo "${local_index},0,0,0,missing_report" >> "${SHARD_SUMMARY_CSV}"
      echo "${SHARD_NAME},${global_candidate_id},${local_index},0,0,0,missing_report" >> "${SUMMARY_CSV}"
      continue
    fi

    pass_rows=$(awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/ && /1\/1/ {count++} END {print count+0}' "${report_path}")
    fail_rows=$(awk '/^[[:space:]]*[0-9]+[[:space:]]+[0-9]+/ && /0\/1/ {count++} END {print count+0}' "${report_path}")
    total_rows=$((pass_rows + fail_rows))

    echo "${local_index},${pass_rows},${fail_rows},${total_rows},${report_path}" >> "${SHARD_SUMMARY_CSV}"
    echo "${SHARD_NAME},${global_candidate_id},${local_index},${pass_rows},${fail_rows},${total_rows},${report_path}" >> "${SUMMARY_CSV}"

    if (( fail_rows == 0 )); then
      echo "candidate_id=${global_candidate_id} shard=${SHARD_NAME} local_index=${local_index} pass_rows=${pass_rows} fail_rows=${fail_rows} final_report=${report_path}" >> "${PASSERS_PATH}"
    fi
  done
done

echo
echo "summary_csv=${SUMMARY_CSV}"
echo "passers_path=${PASSERS_PATH}"
