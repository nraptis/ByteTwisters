#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SEED_OVERRIDE="${1:-${SEED:-}}"
LENGTH_FACTOR_OVERRIDE="${LENGTH_FACTOR:-}"
STREAM_BYTES_OVERRIDE="${STREAM_BYTES:-}"
SAMPLE_WINDOWS_OVERRIDE="${SAMPLE_WINDOWS:-}"
SIGNATURE_BYTES_OVERRIDE="${SIGNATURE_BYTES:-}"
AVALANCHE_BLOCKS_OVERRIDE="${AVALANCHE_BLOCKS:-}"
AVALANCHE_TRIALS_OVERRIDE="${AVALANCHE_TRIALS:-}"
BIC_SAMPLE_BITS_OVERRIDE="${BIC_SAMPLE_BITS:-}"
SECOND_ORDER_TRIALS_OVERRIDE="${SECOND_ORDER_TRIALS:-}"
CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE="${CROSS_INPUT_SIGNATURE_BYTES:-}"
LONG_REPEAT_BYTES_OVERRIDE="${LONG_REPEAT_BYTES:-}"
LONG_REPEAT_TOP_OVERRIDE="${LONG_REPEAT_TOP:-}"
LONG_REPEAT_WINDOW_A_OVERRIDE="${LONG_REPEAT_WINDOW_A:-}"
LONG_REPEAT_WINDOW_B_OVERRIDE="${LONG_REPEAT_WINDOW_B:-}"
TOP_N_OVERRIDE="${TOP_N:-16}"
LIMIT="${LIMIT:-0}"
CANDIDATE_ID="${CANDIDATE_ID:-}"
OUTPUT_DIR="${OUTPUT_DIR:-generated/top64_measure}"
INPUT_CPP="${INPUT_CPP:-src/TOP64.cpp}"

if [[ ! -f "${INPUT_CPP}" ]]; then
  echo "missing input file: ${INPUT_CPP}"
  echo "export the shortlist first"
  exit 1
fi

rm -rf build/top64_measure "${OUTPUT_DIR}"
mkdir -p build/top64_measure "${OUTPUT_DIR}"

if [[ -n "${SEED_OVERRIDE}" ]]; then
  RUN_SEED="${SEED_OVERRIDE}"
else
  RUN_SEED="$(python3 - <<'PY'
import pathlib
import re
import secrets

text = pathlib.Path("src/Knobs.hpp").read_text(encoding="utf-8")
randomize_match = re.search(r"inline constexpr bool kRandomizeSeedByDefault = (true|false);", text)
seed_match = re.search(r"inline constexpr std::uint64_t kRandomSeed = (\d+);", text)
randomize = randomize_match and randomize_match.group(1) == "true"
seed_value = int(seed_match.group(1)) if seed_match else 0

if seed_value != 0:
    print(seed_value)
elif randomize:
    print(secrets.randbits(64))
else:
    print(1337)
PY
)"
fi

echo "seed=${RUN_SEED}"
echo "input_cpp=${INPUT_CPP}"
echo "output_dir=${OUTPUT_DIR}"

clang++ -std=c++20 -O2 -I./src \
  ./src/TwistCandidateHarness.cpp \
  ./src/BaselineCandidates.cpp \
  ./src/PasswordExpander.cpp \
  ./src/LightningMatrix.cpp \
  ./src/HurricaneMatrix.cpp \
  ./src/TyphoonMatrix.cpp \
  "./${INPUT_CPP}" \
  ./references/AESCounter.cpp \
  ./references/ARIA256Counter.cpp \
  ./references/ChaCha20Counter.cpp \
  ./references/MersenneCounter.cpp \
  -o ./build/top64_measure/twist_candidate_harness

HARNESS_ARGS=(
  --seed "${RUN_SEED}"
  --input-suite stale
  --output-dir "${OUTPUT_DIR}"
)

if [[ -n "${STREAM_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--stream-bytes "${STREAM_BYTES_OVERRIDE}")
fi
if [[ -n "${LENGTH_FACTOR_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--length-factor "${LENGTH_FACTOR_OVERRIDE}")
fi
if [[ -n "${SAMPLE_WINDOWS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--sample-windows "${SAMPLE_WINDOWS_OVERRIDE}")
fi
if [[ -n "${SIGNATURE_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--signature-bytes "${SIGNATURE_BYTES_OVERRIDE}")
fi
if [[ -n "${AVALANCHE_BLOCKS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--avalanche-blocks "${AVALANCHE_BLOCKS_OVERRIDE}")
fi
if [[ -n "${AVALANCHE_TRIALS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--avalanche-trials "${AVALANCHE_TRIALS_OVERRIDE}")
fi
if [[ -n "${BIC_SAMPLE_BITS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--bic-sample-bits "${BIC_SAMPLE_BITS_OVERRIDE}")
fi
if [[ -n "${SECOND_ORDER_TRIALS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--second-order-trials "${SECOND_ORDER_TRIALS_OVERRIDE}")
fi
if [[ -n "${CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--cross-input-signature-bytes "${CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE}")
fi
if [[ -n "${LONG_REPEAT_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-bytes "${LONG_REPEAT_BYTES_OVERRIDE}")
fi
if [[ -n "${LONG_REPEAT_TOP_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-top "${LONG_REPEAT_TOP_OVERRIDE}")
fi
if [[ -n "${LONG_REPEAT_WINDOW_A_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-window-a "${LONG_REPEAT_WINDOW_A_OVERRIDE}")
fi
if [[ -n "${LONG_REPEAT_WINDOW_B_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-window-b "${LONG_REPEAT_WINDOW_B_OVERRIDE}")
fi
if [[ -n "${TOP_N_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--top-n "${TOP_N_OVERRIDE}")
fi
if [[ "${LIMIT}" != "0" ]]; then
  HARNESS_ARGS+=(--limit "${LIMIT}")
fi
if [[ -n "${CANDIDATE_ID}" ]]; then
  HARNESS_ARGS+=(--candidate-id "${CANDIDATE_ID}")
fi

./build/top64_measure/twist_candidate_harness "${HARNESS_ARGS[@]}"

python3 - <<'PY'
import csv
from pathlib import Path

scores_path = Path("generated/top64_measure/twist_candidate_scores.csv")
rows = list(csv.DictReader(scores_path.open()))

category_fields = [
    "aes_score",
    "chacha_score",
    "zeros_score",
    "ones_score",
    "predictable_a_score",
    "predictable_b_score",
    "predictable_c_score",
]

def metric(row, key):
    if key == "input_mean":
        return sum(float(row.get(field) or 0.0) for field in category_fields) / float(len(category_fields))
    if key == "input_floor":
        return min(float(row.get(field) or 0.0) for field in category_fields)
    return float(row.get(key) or 0.0)

rows.sort(
    key=lambda row: (
        -(0.35 * metric(row, "input_mean") + 0.35 * metric(row, "input_floor") +
          0.15 * metric(row, "avalanche_score") + 0.15 * metric(row, "bic_score")),
        -float(row.get("composite_score") or 0.0),
        int(row["candidate_id"]),
    )
)

print()
print("Measured top64 preview:")
print("rank  id    mix     comp    mean    floor   aval    bic     rejected  fail")
for rank, row in enumerate(rows[:16], start=1):
    print(
        f"{rank:>4}  "
        f"{int(row['candidate_id']):>4}  "
        f"{(0.35 * metric(row, 'input_mean') + 0.35 * metric(row, 'input_floor') + 0.15 * metric(row, 'avalanche_score') + 0.15 * metric(row, 'bic_score')):>6.3f}  "
        f"{float(row['composite_score']):>6.3f}  "
        f"{metric(row, 'input_mean'):>6.3f}  "
        f"{metric(row, 'input_floor'):>6.3f}  "
        f"{float(row['avalanche_score']):>6.3f}  "
        f"{float(row['bic_score']):>6.3f}  "
        f"{row['rejected']:>8}  "
        f"{row['failure_reason']}"
    )
PY

echo
echo "Measured shortlist:"
echo "  ${OUTPUT_DIR}/twist_candidate_scores.csv"
echo "  ${OUTPUT_DIR}/twist_candidate_summary.txt"
echo "  ${OUTPUT_DIR}/twist_candidate_report.html"
