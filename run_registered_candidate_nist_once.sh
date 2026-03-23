#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_CPP="${INPUT_CPP:-}"
OUTPUT_BIN="${OUTPUT_BIN:-}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
STREAM_KIB="${STREAM_KIB:-0}"
STREAM_BYTES="${STREAM_BYTES:-0}"
STREAM_MIB="${STREAM_MIB:-0}"
STREAM_BITS="${STREAM_BITS:-0}"
SEED="${SEED:-11400714819323198485}"
RUN_LABEL="${RUN_LABEL:-}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_single_runs}"
OUTPUT_STREAM_PATH="${OUTPUT_STREAM_PATH:-}"
SCORE_FILE="${SCORE_FILE:-}"
KEEP_STREAM="${KEEP_STREAM:-0}"
KEEP_REPORT="${KEEP_REPORT:-1}"

if [[ -z "${INPUT_CPP}" ]]; then
  echo "INPUT_CPP is required"
  exit 1
fi

if [[ ! -f "${INPUT_CPP}" ]]; then
  echo "missing code file: ${INPUT_CPP}"
  exit 1
fi

if [[ -z "${CANDIDATE_INDEX}" && -z "${CANDIDATE_ID}" ]]; then
  echo "set CANDIDATE_INDEX or CANDIDATE_ID"
  exit 1
fi

if [[ "${STREAM_BYTES}" != "0" ]]; then
  STREAM_BITS="$((STREAM_BYTES * 8))"
elif [[ "${STREAM_KIB}" != "0" ]]; then
  STREAM_BITS="$((STREAM_KIB * 1024 * 8))"
elif [[ "${STREAM_MIB}" != "0" ]]; then
  STREAM_BITS="$((STREAM_MIB * 1024 * 1024 * 8))"
elif [[ "${STREAM_BITS}" == "0" ]]; then
  echo "set STREAM_BYTES, STREAM_KIB, STREAM_MIB, or STREAM_BITS"
  exit 1
fi

BYTES="$((STREAM_BITS / 8))"
if (( BYTES <= 0 )); then
  echo "invalid stream size"
  exit 1
fi

if [[ -z "${OUTPUT_BIN}" ]]; then
  code_base="$(basename "${INPUT_CPP}" .cpp)"
  OUTPUT_BIN="build/nist_single/${code_base}_stream"
fi

mkdir -p "$(dirname "${OUTPUT_BIN}")" "${OUTPUT_ROOT}"

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" bash ./run_build_twist_registered_stream.sh >/dev/null
fi

if [[ -z "${RUN_LABEL}" ]]; then
  size_tag_mib=$((BYTES / 1024 / 1024))
  if [[ -n "${CANDIDATE_ID}" ]]; then
    RUN_LABEL="candidate_id_${CANDIDATE_ID}_${PASSWORD_TEXT}_${size_tag_mib}MiB"
  else
    RUN_LABEL="candidate_index_${CANDIDATE_INDEX}_${PASSWORD_TEXT}_${size_tag_mib}MiB"
  fi
fi

stream_path="${OUTPUT_STREAM_PATH}"
generated_temp_stream=0
if [[ -z "${stream_path}" ]]; then
  stream_path="/tmp/${RUN_LABEL}.bin"
  generated_temp_stream=1
fi

emit_args=(
  "INPUT_CPP=${INPUT_CPP}"
  "OUTPUT_BIN=${OUTPUT_BIN}"
  "BYTES=${BYTES}"
  "SEED=${SEED}"
  "PASSWORD_TEXT=${PASSWORD_TEXT}"
  "OUTPUT_PATH=${stream_path}"
)
if [[ -n "${CANDIDATE_ID}" ]]; then
  emit_args+=("CANDIDATE_ID=${CANDIDATE_ID}")
else
  emit_args+=("CANDIDATE_INDEX=${CANDIDATE_INDEX}")
fi

env "${emit_args[@]}" bash ./run_emit_registered_candidate_stream.sh >/dev/null

sts_output="$(
  INPUT_FILE="${stream_path}" \
  STREAM_BITS="${STREAM_BITS}" \
  INPUT_MODE=1 \
  OUTPUT_ROOT="${OUTPUT_ROOT}" \
  RUN_LABEL="${RUN_LABEL}" \
  bash ./run_nist_sts_file.sh
)"

pass_rows="$(printf '%s\n' "${sts_output}" | awk -F= '/^pass_rows=/{print $2}')"
fail_rows="$(printf '%s\n' "${sts_output}" | awk -F= '/^fail_rows=/{print $2}')"
total_rows="$(printf '%s\n' "${sts_output}" | awk -F= '/^total_rows=/{print $2}')"
report_path="$(printf '%s\n' "${sts_output}" | awk -F= '/^final_report=/{print $2}')"
results_dir="$(printf '%s\n' "${sts_output}" | awk -F= '/^results_dir=/{print $2}')"

if [[ -n "${SCORE_FILE}" ]]; then
  mkdir -p "$(dirname "${SCORE_FILE}")"
  {
    echo "input_cpp=${INPUT_CPP}"
    if [[ -n "${CANDIDATE_ID}" ]]; then
      echo "candidate_id=${CANDIDATE_ID}"
    fi
    if [[ -n "${CANDIDATE_INDEX}" ]]; then
      echo "candidate_index=${CANDIDATE_INDEX}"
    fi
    echo "password_text=${PASSWORD_TEXT}"
    echo "stream_bits=${STREAM_BITS}"
    echo "stream_bytes=${BYTES}"
    echo "stream_file=${stream_path}"
    echo "results_dir=${results_dir}"
    echo "final_report=${report_path}"
    echo "pass_rows=${pass_rows}"
    echo "fail_rows=${fail_rows}"
    echo "total_rows=${total_rows}"
    if [[ -f "${report_path}" ]]; then
      echo
      cat "${report_path}"
    fi
  } > "${SCORE_FILE}"
fi

if [[ "${KEEP_REPORT}" != "1" && -n "${results_dir}" ]]; then
  rm -rf "$(dirname "${results_dir}")"
fi

if [[ "${KEEP_STREAM}" != "1" && ( "${generated_temp_stream}" == "1" || -z "${OUTPUT_STREAM_PATH}" ) ]]; then
  rm -f "${stream_path}"
fi

printf '%s\n' "${sts_output}"
