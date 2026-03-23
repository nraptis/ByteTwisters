#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

STS_DIR="${STS_DIR:-sts-2.1.2}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_runs}"
STREAM_BITS="${STREAM_BITS:-1000000}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-0}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
SEED="${SEED:-11400714819323198485}"
PASSWORD_TEXT="${PASSWORD_TEXT:-}"

if [[ ! -x "${STS_DIR}/assess" ]]; then
  (cd "${STS_DIR}" && make)
fi

bytes=$((STREAM_BITS / 8))
if (( bytes <= 0 )); then
  echo "STREAM_BITS must be at least 8"
  exit 1
fi

mkdir -p "${OUTPUT_ROOT}"
mkdir -p /tmp/sts_inputs

if [[ -n "${CANDIDATE_ID}" ]]; then
  STAGE_FILE="/tmp/sts_inputs/candidate_${CANDIDATE_ID}.txt"
else
  STAGE_FILE="/tmp/sts_inputs/candidate_$(printf '%02d' "${CANDIDATE_INDEX}").txt"
fi

if [[ -n "${CANDIDATE_ID}" ]]; then
  BYTES="${bytes}" CANDIDATE_ID="${CANDIDATE_ID}" SEED="${SEED}" PASSWORD_TEXT="${PASSWORD_TEXT}" BITS_MODE=1 OUTPUT_PATH="${STAGE_FILE}" bash ./run_emit_top_candidate_stream.sh >/dev/null
  RUN_LABEL="candidate_id_${CANDIDATE_ID}"
else
  BYTES="${bytes}" CANDIDATE_INDEX="${CANDIDATE_INDEX}" SEED="${SEED}" PASSWORD_TEXT="${PASSWORD_TEXT}" BITS_MODE=1 OUTPUT_PATH="${STAGE_FILE}" bash ./run_emit_top_candidate_stream.sh >/dev/null
  RUN_LABEL="candidate_index_$(printf '%02d' "${CANDIDATE_INDEX}")"
fi

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
0
EOF
) || true

RUN_DIR="${OUTPUT_ROOT}/${RUN_LABEL}"
rm -rf "${RUN_DIR}"
mkdir -p "${RUN_DIR}"
cp -R "${STS_DIR}/experiments/AlgorithmTesting" "${RUN_DIR}/"

echo
echo "NIST STS run complete:"
echo "  stream_bits=${STREAM_BITS}"
if [[ -n "${PASSWORD_TEXT}" ]]; then
  echo "  password_text=${PASSWORD_TEXT}"
fi
echo "  input_file=${STAGE_FILE}"
echo "  results_dir=${RUN_DIR}/AlgorithmTesting"
echo "  final_report=${RUN_DIR}/AlgorithmTesting/finalAnalysisReport.txt"
