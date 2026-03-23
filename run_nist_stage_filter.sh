#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

STAGE_NAME="${STAGE_NAME:-}"
PASSWORD_TEXT="${PASSWORD_TEXT:-}"
STREAM_MIB="${STREAM_MIB:-0}"
STREAM_BITS="${STREAM_BITS:-0}"
INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
INPUT_CANDIDATES="${INPUT_CANDIDATES:-}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_stages}"
RESUME_MODE="${RESUME_MODE:-0}"
LIMIT="${LIMIT:-0}"
START_AT="${START_AT:-0}"

if [[ -z "${STAGE_NAME}" ]]; then
  echo "STAGE_NAME is required"
  exit 1
fi

if [[ -z "${PASSWORD_TEXT}" ]]; then
  echo "PASSWORD_TEXT is required"
  exit 1
fi

if [[ "${STREAM_BITS}" == "0" && "${STREAM_MIB}" == "0" ]]; then
  echo "set STREAM_BITS or STREAM_MIB"
  exit 1
fi

STAGE_DIR="${OUTPUT_ROOT}/${STAGE_NAME}"
PLAN_CSV="${STAGE_DIR}/candidate_plan.csv"
SUMMARY_CSV="${STAGE_DIR}/summary.csv"
PASSED_TXT="${STAGE_DIR}/passed_candidates.txt"
FAILED_TXT="${STAGE_DIR}/failed_candidates.txt"
BUILD_DIR="build/nist_stage_${STAGE_NAME}"

if [[ "${RESUME_MODE}" != "1" ]]; then
  rm -rf "${STAGE_DIR}" "${BUILD_DIR}"
fi

mkdir -p "${STAGE_DIR}" "${BUILD_DIR}"

python3 - <<PY
import csv
import json
import re
from pathlib import Path

index = json.loads(Path("${INDEX_PATH}").read_text(encoding="utf-8"))
candidate_ids = []
if "${INPUT_CANDIDATES}".strip():
    text = Path("${INPUT_CANDIDATES}").read_text(encoding="utf-8")
    candidate_ids = [int(match) for match in re.findall(r"\d+", text)]
else:
    candidate_ids = list(range(1, int(index["candidate_count"]) + 1))

if ${START_AT} > 0:
    candidate_ids = candidate_ids[${START_AT}:]
if ${LIMIT} > 0:
    candidate_ids = candidate_ids[:${LIMIT}]

rows = []
for candidate_id in candidate_ids:
    shard_row = None
    for shard in index["shards"]:
        if shard["start_candidate_id"] <= candidate_id <= shard["end_candidate_id"]:
            shard_row = shard
            break
    if shard_row is None:
        continue
    rows.append({
        "candidate_id": candidate_id,
        "shard_name": shard_row["name"],
        "shard_path": shard_row["path"],
        "local_index": candidate_id - shard_row["start_candidate_id"],
    })

with Path("${PLAN_CSV}").open("w", encoding="utf-8", newline="") as handle:
    writer = csv.DictWriter(handle, fieldnames=["candidate_id", "shard_name", "shard_path", "local_index"])
    writer.writeheader()
    writer.writerows(rows)
PY

if [[ ! -f "${SUMMARY_CSV}" || "${RESUME_MODE}" != "1" ]]; then
  echo "candidate_id,shard_name,local_index,pass_rows,fail_rows,total_rows,final_report" > "${SUMMARY_CSV}"
fi
if [[ ! -f "${PASSED_TXT}" || "${RESUME_MODE}" != "1" ]]; then
  : > "${PASSED_TXT}"
fi
if [[ ! -f "${FAILED_TXT}" || "${RESUME_MODE}" != "1" ]]; then
  : > "${FAILED_TXT}"
fi

echo "stage_name=${STAGE_NAME}"
echo "password_text=${PASSWORD_TEXT}"
echo "stream_mib=${STREAM_MIB}"
echo "stream_bits=${STREAM_BITS}"
echo "plan_csv=${PLAN_CSV}"

current_shard=""
current_bin=""
while IFS=, read -r candidate_id shard_name shard_path local_index; do
  if [[ "${candidate_id}" == "candidate_id" ]]; then
    continue
  fi
  candidate_id="${candidate_id//$'\r'/}"
  shard_name="${shard_name//$'\r'/}"
  shard_path="${shard_path//$'\r'/}"
  local_index="${local_index//$'\r'/}"

  if [[ "${current_shard}" != "${shard_name}" ]]; then
    current_shard="${shard_name}"
    current_bin="${BUILD_DIR}/${shard_name}_stream"
    echo
    echo "compiling ${shard_name}"
    INPUT_CPP="${shard_path}" OUTPUT_BIN="${current_bin}" bash ./run_build_twist_registered_stream.sh >/dev/null
  fi

  if [[ "${RESUME_MODE}" == "1" ]] && grep -q "^${candidate_id}," "${SUMMARY_CSV}"; then
    echo "skipping candidate_id=${candidate_id} (already summarized)"
    continue
  fi

  echo "testing candidate_id=${candidate_id} shard=${shard_name} local_index=${local_index}"
  run_output="$(
    INPUT_CPP="${shard_path}" \
    OUTPUT_BIN="${current_bin}" \
    CANDIDATE_ID="${candidate_id}" \
    PASSWORD_TEXT="${PASSWORD_TEXT}" \
    STREAM_MIB="${STREAM_MIB}" \
    STREAM_BITS="${STREAM_BITS}" \
    RUN_LABEL="${STAGE_NAME}_candidate_${candidate_id}" \
    OUTPUT_ROOT="${STAGE_DIR}/nist_reports" \
    KEEP_STREAM=0 \
    KEEP_REPORT=0 \
    bash ./run_registered_candidate_nist_once.sh
  )"

  pass_rows="$(printf '%s\n' "${run_output}" | awk -F= '/^pass_rows=/{print $2}')"
  fail_rows="$(printf '%s\n' "${run_output}" | awk -F= '/^fail_rows=/{print $2}')"
  total_rows="$(printf '%s\n' "${run_output}" | awk -F= '/^total_rows=/{print $2}')"
  final_report="$(printf '%s\n' "${run_output}" | awk -F= '/^final_report=/{print $2}')"

  echo "${candidate_id},${shard_name},${local_index},${pass_rows},${fail_rows},${total_rows},${final_report}" >> "${SUMMARY_CSV}"
  if [[ "${fail_rows}" == "0" && "${total_rows}" != "0" ]]; then
    echo "${candidate_id}" >> "${PASSED_TXT}"
  else
    echo "${candidate_id}" >> "${FAILED_TXT}"
  fi
done < "${PLAN_CSV}"

echo
echo "summary_csv=${SUMMARY_CSV}"
echo "passed_candidates=${PASSED_TXT}"
echo "failed_candidates=${FAILED_TXT}"
