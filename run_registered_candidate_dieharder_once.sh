#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_CPP="${INPUT_CPP:-}"
OUTPUT_BIN="${OUTPUT_BIN:-}"
DIEHARDER_BIN="${DIEHARDER_BIN:-}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
STREAM_BYTES="${STREAM_BYTES:-0}"
STREAM_KIB="${STREAM_KIB:-0}"
STREAM_MIB="${STREAM_MIB:-0}"
SEED="${SEED:-11400714819323198485}"
RUN_LABEL="${RUN_LABEL:-}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/dieharder_single_runs}"
OUTPUT_STREAM_PATH="${OUTPUT_STREAM_PATH:-}"
SCORE_FILE="${SCORE_FILE:-}"
KEEP_STREAM="${KEEP_STREAM:-0}"
KEEP_REPORT="${KEEP_REPORT:-1}"
DIEHARDER_ARGS="${DIEHARDER_ARGS:--a}"

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

if [[ -z "${DIEHARDER_BIN}" ]]; then
  if command -v dieharder >/dev/null 2>&1; then
    DIEHARDER_BIN="$(command -v dieharder)"
  else
    echo "Set DIEHARDER_BIN to the path of dieharder"
    exit 1
  fi
fi

if [[ ! -x "${DIEHARDER_BIN}" ]]; then
  echo "dieharder is not executable: ${DIEHARDER_BIN}"
  exit 1
fi

if [[ "${STREAM_BYTES}" != "0" ]]; then
  :
elif [[ "${STREAM_KIB}" != "0" ]]; then
  STREAM_BYTES="$((STREAM_KIB * 1024))"
elif [[ "${STREAM_MIB}" != "0" ]]; then
  STREAM_BYTES="$((STREAM_MIB * 1024 * 1024))"
else
  echo "set STREAM_BYTES, STREAM_KIB, or STREAM_MIB"
  exit 1
fi

if (( STREAM_BYTES <= 0 )); then
  echo "invalid stream size"
  exit 1
fi

if [[ -z "${OUTPUT_BIN}" ]]; then
  code_base="$(basename "${INPUT_CPP}" .cpp)"
  OUTPUT_BIN="build/dieharder_single/${code_base}_stream"
fi

mkdir -p "$(dirname "${OUTPUT_BIN}")" "${OUTPUT_ROOT}"

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" bash ./run_build_twist_registered_stream.sh >/dev/null
fi

if [[ -z "${RUN_LABEL}" ]]; then
  size_tag_kib="$((STREAM_BYTES / 1024))"
  if [[ -n "${CANDIDATE_ID}" ]]; then
    RUN_LABEL="candidate_id_${CANDIDATE_ID}_${PASSWORD_TEXT}_${size_tag_kib}KiB"
  else
    RUN_LABEL="candidate_index_${CANDIDATE_INDEX}_${PASSWORD_TEXT}_${size_tag_kib}KiB"
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
  "BYTES=${STREAM_BYTES}"
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

RUN_DIR="${OUTPUT_ROOT}/${RUN_LABEL}"
report_file="${RUN_DIR}/dieharder_output.txt"
mkdir -p "${RUN_DIR}"

# `-g 200` consumes raw binary bytes from stdin.
cat "${stream_path}" | "${DIEHARDER_BIN}" -g 200 ${DIEHARDER_ARGS} > "${report_file}" 2>&1 || true

parse_output="$(
python3 - "${report_file}" <<'PY'
import pathlib
import re
import sys

report_path = pathlib.Path(sys.argv[1])
passed = 0
weak = 0
failed = 0
total = 0

status_re = re.compile(r'\b(PASSED|WEAK|FAILED)\b')
if report_path.exists():
    for line in report_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        match = status_re.search(line)
        if not match:
            continue
        total += 1
        status = match.group(1)
        if status == "PASSED":
            passed += 1
        elif status == "WEAK":
            weak += 1
        else:
            failed += 1

print(f"passed_tests={passed}")
print(f"weak_tests={weak}")
print(f"failed_tests={failed}")
print(f"total_tests={total}")
PY
)"

passed_tests="$(printf '%s\n' "${parse_output}" | awk -F= '/^passed_tests=/{print $2}')"
weak_tests="$(printf '%s\n' "${parse_output}" | awk -F= '/^weak_tests=/{print $2}')"
failed_tests="$(printf '%s\n' "${parse_output}" | awk -F= '/^failed_tests=/{print $2}')"
total_tests="$(printf '%s\n' "${parse_output}" | awk -F= '/^total_tests=/{print $2}')"

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
    echo "stream_bytes=${STREAM_BYTES}"
    echo "stream_file=${stream_path}"
    echo "dieharder_bin=${DIEHARDER_BIN}"
    echo "report_file=${report_file}"
    echo "passed_tests=${passed_tests}"
    echo "weak_tests=${weak_tests}"
    echo "failed_tests=${failed_tests}"
    echo "total_tests=${total_tests}"
    echo
    if [[ -f "${report_file}" ]]; then
      cat "${report_file}"
    fi
  } > "${SCORE_FILE}"
fi

if [[ "${KEEP_REPORT}" != "1" ]]; then
  rm -rf "${RUN_DIR}"
fi

if [[ "${KEEP_STREAM}" != "1" && ( "${generated_temp_stream}" == "1" || -z "${OUTPUT_STREAM_PATH}" ) ]]; then
  rm -f "${stream_path}"
fi

echo "input_cpp=${INPUT_CPP}"
echo "stream_bytes=${STREAM_BYTES}"
echo "report_file=${report_file}"
echo "passed_tests=${passed_tests}"
echo "weak_tests=${weak_tests}"
echo "failed_tests=${failed_tests}"
echo "total_tests=${total_tests}"
