#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

OUTPUT_BIN="${OUTPUT_BIN:-${ROOT_DIR}/build/twist_registered_battery_stream}"
PRACTRAND_BIN="${PRACTRAND_BIN:-}"
CANDIDATE_ID="${CANDIDATE_ID:--101}"
PASSWORD_TEXT="${PASSWORD_TEXT:-a}"
STREAM_BYTES="${STREAM_BYTES:-61440}"
SEED="${SEED:-11400714819323198485}"
RUN_LABEL="${RUN_LABEL:-}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${ROOT_DIR}/generated/practrand_dummy_aes_runs}"
OUTPUT_STREAM_PATH="${OUTPUT_STREAM_PATH:-}"
SCORE_FILE="${SCORE_FILE:-}"
KEEP_STREAM="${KEEP_STREAM:-0}"
KEEP_REPORT="${KEEP_REPORT:-1}"

if [[ -z "${PRACTRAND_BIN}" ]]; then
  if command -v RNG_test >/dev/null 2>&1; then
    PRACTRAND_BIN="$(command -v RNG_test)"
  elif [[ -x "${ROOT_DIR}/build/practrand/RNG_test" ]]; then
    PRACTRAND_BIN="${ROOT_DIR}/build/practrand/RNG_test"
  else
    echo "Set PRACTRAND_BIN to the path of RNG_test"
    exit 1
  fi
fi

if [[ ! -x "${PRACTRAND_BIN}" ]]; then
  echo "PractRand binary is not executable: ${PRACTRAND_BIN}"
  exit 1
fi

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  echo "missing stream binary: ${OUTPUT_BIN}"
  echo "This wrapper expects a built twist_registered_battery_stream that already includes candidate_id=${CANDIDATE_ID}."
  exit 1
fi

if (( STREAM_BYTES <= 0 )); then
  echo "invalid stream size"
  exit 1
fi

if (( STREAM_BYTES % 1024 != 0 )); then
  echo "PractRand stage size must be a multiple of 1024 bytes; got ${STREAM_BYTES}"
  exit 1
fi

probe_file="/tmp/practrand_dummy_aes_probe.bin"
if ! "${OUTPUT_BIN}" --candidate-id "${CANDIDATE_ID}" --bytes 16 --password-text "${PASSWORD_TEXT}" --output "${probe_file}" >/dev/null 2>&1; then
  rm -f "${probe_file}"
  echo "candidate_id=${CANDIDATE_ID} is not available from ${OUTPUT_BIN}"
  exit 1
fi
rm -f "${probe_file}"

size_kib="$((STREAM_BYTES / 1024))"
size_arg="${size_kib}KB"

if [[ -z "${RUN_LABEL}" ]]; then
  RUN_LABEL="dummy_aes_${PASSWORD_TEXT}_${size_kib}KB"
fi

mkdir -p "${OUTPUT_ROOT}"
RUN_DIR="${OUTPUT_ROOT}/${RUN_LABEL}"
mkdir -p "${RUN_DIR}"

stream_path="${OUTPUT_STREAM_PATH}"
generated_temp_stream=0
if [[ -z "${stream_path}" ]]; then
  stream_path="/tmp/${RUN_LABEL}.bin"
  generated_temp_stream=1
fi

"${OUTPUT_BIN}" \
  --candidate-id "${CANDIDATE_ID}" \
  --bytes "${STREAM_BYTES}" \
  --seed "${SEED}" \
  --password-text "${PASSWORD_TEXT}" \
  --output "${stream_path}"

report_file="${RUN_DIR}/practrand_output.txt"
"${PRACTRAND_BIN}" stdin8 -tlmin "${size_arg}" -tlmax "${size_arg}" < "${stream_path}" > "${report_file}" 2>&1 || true

parse_output="$(
python3 - "${report_file}" <<'PY'
import pathlib
import re
import sys

report_path = pathlib.Path(sys.argv[1])
counts = {
    "fail": 0,
    "very_suspicious": 0,
    "suspicious": 0,
    "mildly_suspicious": 0,
    "unusual": 0,
    "normalish": 0,
    "normal": 0,
    "total_results": 0,
}

if report_path.exists():
    for raw_line in report_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.rstrip()
        if "Test Name" in line or "length=" in line or not line.strip():
            continue
        match = re.search(r"\.\.\.and\s+(\d+)\s+test result\(s\) without anomalies", line)
        if match:
            counts["normal"] += int(match.group(1))
            counts["total_results"] += int(match.group(1))
            continue
        if "FAIL" in line:
            counts["fail"] += 1
            counts["total_results"] += 1
        elif "VERY SUSPICIOUS" in line:
            counts["very_suspicious"] += 1
            counts["total_results"] += 1
        elif "very suspicious" in line:
            counts["very_suspicious"] += 1
            counts["total_results"] += 1
        elif "mildly suspicious" in line:
            counts["mildly_suspicious"] += 1
            counts["total_results"] += 1
        elif "suspicious" in line:
            counts["suspicious"] += 1
            counts["total_results"] += 1
        elif "unusual" in line:
            counts["unusual"] += 1
            counts["total_results"] += 1
        elif "normalish" in line:
            counts["normalish"] += 1
            counts["total_results"] += 1
        elif "normal" in line:
            counts["normal"] += 1
            counts["total_results"] += 1

for key, value in counts.items():
    print(f"{key}={value}")
PY
)"

fail_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^fail=/{print $2}')"
very_suspicious_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^very_suspicious=/{print $2}')"
suspicious_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^suspicious=/{print $2}')"
mildly_suspicious_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^mildly_suspicious=/{print $2}')"
unusual_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^unusual=/{print $2}')"
normalish_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^normalish=/{print $2}')"
normal_count="$(printf '%s\n' "${parse_output}" | awk -F= '/^normal=/{print $2}')"
total_results="$(printf '%s\n' "${parse_output}" | awk -F= '/^total_results=/{print $2}')"

if [[ -n "${SCORE_FILE}" ]]; then
  mkdir -p "$(dirname "${SCORE_FILE}")"
  {
    echo "candidate_id=${CANDIDATE_ID}"
    echo "password_text=${PASSWORD_TEXT}"
    echo "stream_bytes=${STREAM_BYTES}"
    echo "stream_file=${stream_path}"
    echo "output_bin=${OUTPUT_BIN}"
    echo "practrand_bin=${PRACTRAND_BIN}"
    echo "report_file=${report_file}"
    echo "fail=${fail_count}"
    echo "very_suspicious=${very_suspicious_count}"
    echo "suspicious=${suspicious_count}"
    echo "mildly_suspicious=${mildly_suspicious_count}"
    echo "unusual=${unusual_count}"
    echo "normalish=${normalish_count}"
    echo "normal=${normal_count}"
    echo "total_results=${total_results}"
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

echo "candidate_id=${CANDIDATE_ID}"
echo "password_text=${PASSWORD_TEXT}"
echo "stream_bytes=${STREAM_BYTES}"
echo "report_file=${report_file}"
echo "fail=${fail_count}"
echo "very_suspicious=${very_suspicious_count}"
echo "suspicious=${suspicious_count}"
echo "mildly_suspicious=${mildly_suspicious_count}"
echo "unusual=${unusual_count}"
echo "normalish=${normalish_count}"
echo "normal=${normal_count}"
echo "total_results=${total_results}"
