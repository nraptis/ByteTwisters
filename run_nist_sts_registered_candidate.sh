#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

STS_DIR="${STS_DIR:-sts-2.1.2}"
INPUT_CPP="${INPUT_CPP:-generated/twist_candidates_generated_verbose.cpp}"
OUTPUT_BIN="${OUTPUT_BIN:-build/twist_registered_battery_stream}"
OUTPUT_ROOT="${OUTPUT_ROOT:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_registered_runs}"
STREAM_BITS="${STREAM_BITS:-1073741824}"
CANDIDATE_INDEX="${CANDIDATE_INDEX:-0}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
SEED="${SEED:-11400714819323198485}"
PASSWORD_TEXT="${PASSWORD_TEXT:-cat}"
RUN_LABEL="${RUN_LABEL:-}"

if [[ ! -x "${STS_DIR}/assess" ]]; then
  (cd "${STS_DIR}" && make)
fi

if [[ ! -x "${OUTPUT_BIN}" ]]; then
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" bash ./run_build_twist_registered_stream.sh >/dev/null
fi

bytes=$((STREAM_BITS / 8))
if (( bytes <= 0 )); then
  echo "STREAM_BITS must be at least 8"
  exit 1
fi

mkdir -p "${OUTPUT_ROOT}"
mkdir -p /tmp/sts_inputs

if [[ -n "${RUN_LABEL}" ]]; then
  STAGE_FILE="/tmp/sts_inputs/${RUN_LABEL}.txt"
else
  if [[ -n "${CANDIDATE_ID}" ]]; then
    STAGE_FILE="/tmp/sts_inputs/candidate_${CANDIDATE_ID}.txt"
    RUN_LABEL="candidate_id_${CANDIDATE_ID}"
  else
    STAGE_FILE="/tmp/sts_inputs/candidate_$(printf '%04d' "${CANDIDATE_INDEX}").txt"
    RUN_LABEL="candidate_index_$(printf '%04d' "${CANDIDATE_INDEX}")"
  fi
fi

if [[ -n "${CANDIDATE_ID}" ]]; then
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" BYTES="${bytes}" CANDIDATE_ID="${CANDIDATE_ID}" SEED="${SEED}" PASSWORD_TEXT="${PASSWORD_TEXT}" BITS_MODE=1 OUTPUT_PATH="${STAGE_FILE}" bash ./run_emit_registered_candidate_stream.sh >/dev/null
else
  INPUT_CPP="${INPUT_CPP}" OUTPUT_BIN="${OUTPUT_BIN}" BYTES="${bytes}" CANDIDATE_INDEX="${CANDIDATE_INDEX}" SEED="${SEED}" PASSWORD_TEXT="${PASSWORD_TEXT}" BITS_MODE=1 OUTPUT_PATH="${STAGE_FILE}" bash ./run_emit_registered_candidate_stream.sh >/dev/null
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
echo "NIST STS registered run complete:"
echo "  input_cpp=${INPUT_CPP}"
echo "  stream_bits=${STREAM_BITS}"
echo "  password_text=${PASSWORD_TEXT}"
echo "  input_file=${STAGE_FILE}"
echo "  results_dir=${RUN_DIR}/AlgorithmTesting"
echo "  final_report=${RUN_DIR}/AlgorithmTesting/finalAnalysisReport.txt"
