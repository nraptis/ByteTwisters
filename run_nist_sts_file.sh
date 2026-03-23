#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

STS_DIR="${STS_DIR:-sts-2.1.2}"
INPUT_FILE="${INPUT_FILE:-}"
STREAM_BITS="${STREAM_BITS:-1000000}"
INPUT_MODE="${INPUT_MODE:-1}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_file_runs}"
RUN_LABEL="${RUN_LABEL:-file_run}"

if [[ -z "${INPUT_FILE}" ]]; then
  echo "INPUT_FILE is required"
  exit 1
fi

if [[ ! -f "${INPUT_FILE}" ]]; then
  echo "missing input file: ${INPUT_FILE}"
  exit 1
fi

if [[ ! -x "${STS_DIR}/assess" ]]; then
  (cd "${STS_DIR}" && make)
fi

mkdir -p "${OUTPUT_ROOT}"
mkdir -p /tmp/sts_inputs

stage_ext="bin"
if [[ "${INPUT_MODE}" == "0" ]]; then
  stage_ext="txt"
fi
STAGE_FILE="/tmp/sts_inputs/${RUN_LABEL}.${stage_ext}"
cp "${INPUT_FILE}" "${STAGE_FILE}"

rm -rf "${STS_DIR}/experiments/AlgorithmTesting"
mkdir -p "${STS_DIR}/experiments/AlgorithmTesting"
for test_dir in Frequency BlockFrequency Runs LongestRun Rank FFT NonOverlappingTemplate OverlappingTemplate Universal LinearComplexity Serial ApproximateEntropy CumulativeSums RandomExcursions RandomExcursionsVariant; do
  mkdir -p "${STS_DIR}/experiments/AlgorithmTesting/${test_dir}"
done

(cd "${STS_DIR}" && ./assess "${STREAM_BITS}" <<EOF
0
${STAGE_FILE}
1
0
1
${INPUT_MODE}
EOF
) || true

RUN_DIR="${OUTPUT_ROOT}/${RUN_LABEL}"
rm -rf "${RUN_DIR}"
mkdir -p "${RUN_DIR}"
cp -R "${STS_DIR}/experiments/AlgorithmTesting" "${RUN_DIR}/"

report_path="${RUN_DIR}/AlgorithmTesting/finalAnalysisReport.txt"
pass_rows=0
fail_rows=0
total_rows=0
parse_output="$(
python3 - "${report_path}" <<'PY'
import pathlib
import re
import sys

report_path = pathlib.Path(sys.argv[1])
ratio_re = re.compile(r'\b(\d+)/(\d+)\b')
pass_rows = 0
fail_rows = 0
total_rows = 0

if report_path.exists():
    for raw_line in report_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line or "STATISTICAL TEST" in line or "generator is" in line:
            continue
        match = ratio_re.search(line)
        if not match:
            continue
        numerator = int(match.group(1))
        denominator = int(match.group(2))
        if denominator <= 0:
            continue
        total_rows += 1
        if numerator >= denominator:
            pass_rows += 1
        else:
            fail_rows += 1

print(f"pass_rows={pass_rows}")
print(f"fail_rows={fail_rows}")
print(f"total_rows={total_rows}")
PY
)"

pass_rows="$(printf '%s\n' "${parse_output}" | awk -F= '/^pass_rows=/{print $2}')"
fail_rows="$(printf '%s\n' "${parse_output}" | awk -F= '/^fail_rows=/{print $2}')"
total_rows="$(printf '%s\n' "${parse_output}" | awk -F= '/^total_rows=/{print $2}')"

echo "input_file=${INPUT_FILE}"
echo "stream_bits=${STREAM_BITS}"
echo "input_mode=${INPUT_MODE}"
echo "results_dir=${RUN_DIR}/AlgorithmTesting"
echo "final_report=${report_path}"
echo "pass_rows=${pass_rows}"
echo "fail_rows=${fail_rows}"
echo "total_rows=${total_rows}"
