#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SEED_OVERRIDE="${1:-${SEED:-}}"
SHARD_SIZE_OVERRIDE="${2:-${SHARD_SIZE:-10000}}"
INPUT_SUITE_OVERRIDE="${INPUT_SUITE:-stale}"
LENGTH_FACTOR_OVERRIDE="${LENGTH_FACTOR:-}"
STREAM_BYTES_OVERRIDE="${STREAM_BYTES:-}"
TRIAL_COUNT_OVERRIDE="${TRIAL_COUNT:-}"
SAMPLE_WINDOWS_OVERRIDE="${SAMPLE_WINDOWS:-}"
SIGNATURE_BYTES_OVERRIDE="${SIGNATURE_BYTES:-}"
AVALANCHE_BLOCKS_OVERRIDE="${AVALANCHE_BLOCKS:-}"
AVALANCHE_TRIALS_OVERRIDE="${AVALANCHE_TRIALS:-}"
BIC_SAMPLE_BITS_OVERRIDE="${BIC_SAMPLE_BITS:-}"
SECOND_ORDER_TRIALS_OVERRIDE="${SECOND_ORDER_TRIALS:-}"
CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE="${CROSS_INPUT_SIGNATURE_BYTES:-}"
TRIAL_CAP_PER_CATEGORY_OVERRIDE="${TRIAL_CAP_PER_CATEGORY:-}"
LONG_REPEAT_BYTES_OVERRIDE="${LONG_REPEAT_BYTES:-}"
LONG_REPEAT_TOP_OVERRIDE="${LONG_REPEAT_TOP:-}"
LONG_REPEAT_WINDOW_A_OVERRIDE="${LONG_REPEAT_WINDOW_A:-}"
LONG_REPEAT_WINDOW_B_OVERRIDE="${LONG_REPEAT_WINDOW_B:-}"
TOP_N_OVERRIDE="${TOP_N:-}"
RESUME_MODE="${RESUME_MODE:-1}"
KEEP_SHARDS="${KEEP_SHARDS:-1}"
FAST_SHARD_MODE="${FAST_SHARD_MODE:-1}"
SHARD_LONG_REPEAT_TOP_OVERRIDE="${SHARD_LONG_REPEAT_TOP:-}"
START_SHARD_INDEX="${START_SHARD_INDEX:-0}"

INPUT_CPP="generated/twist_candidates_generated_verbose.cpp"
INDEX_PATH="generated/shards_index.json"
SHARD_CPP_DIR="generated/shards_cpp"
SHARD_OUTPUT_DIR="generated/shard_measure"

if [[ ! -f "${INPUT_CPP}" ]]; then
  echo "missing generated input"
  echo "expected: ${INPUT_CPP}"
  echo "run ./generate_twists.sh first"
  exit 1
fi

if [[ "${RESUME_MODE}" != "1" ]]; then
  rm -rf build/shards "${SHARD_CPP_DIR}" "${SHARD_OUTPUT_DIR}" "${INDEX_PATH}"
fi

if [[ "${RESUME_MODE}" == "1" && -f "${INDEX_PATH}" && "${INPUT_CPP}" -nt "${INDEX_PATH}" ]]; then
  echo "generated input is newer than shard index; rebuilding shards"
  rm -rf build/shards "${SHARD_CPP_DIR}" "${SHARD_OUTPUT_DIR}" "${INDEX_PATH}"
fi

if [[ "${RESUME_MODE}" == "1" && -f "${INDEX_PATH}" ]]; then
  EXISTING_SHARD_SIZE="$(python3 - <<PY
import json
from pathlib import Path
index = json.loads(Path("${INDEX_PATH}").read_text(encoding="utf-8"))
print(index.get("shard_size", ""))
PY
)"
  if [[ "${EXISTING_SHARD_SIZE}" != "${SHARD_SIZE_OVERRIDE}" ]]; then
    echo "requested shard size differs from shard index (${SHARD_SIZE_OVERRIDE} vs ${EXISTING_SHARD_SIZE}); rebuilding shards"
    rm -rf build/shards "${SHARD_CPP_DIR}" "${SHARD_OUTPUT_DIR}" "${INDEX_PATH}"
  fi
fi

mkdir -p build/shards "${SHARD_CPP_DIR}" "${SHARD_OUTPUT_DIR}"

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
echo "shard_size=${SHARD_SIZE_OVERRIDE}"
echo "resume_mode=${RESUME_MODE}"
echo "fast_shard_mode=${FAST_SHARD_MODE}"
echo "start_shard_index=${START_SHARD_INDEX}"

if [[ "${FAST_SHARD_MODE}" == "1" && -z "${SHARD_LONG_REPEAT_TOP_OVERRIDE}" ]]; then
  SHARD_LONG_REPEAT_TOP_OVERRIDE="0"
fi

if [[ "${FAST_SHARD_MODE}" == "1" ]]; then
  if [[ -z "${LENGTH_FACTOR_OVERRIDE}" ]]; then
    LENGTH_FACTOR_OVERRIDE="4096"
  fi
  if [[ -z "${TRIAL_CAP_PER_CATEGORY_OVERRIDE}" ]]; then
    TRIAL_CAP_PER_CATEGORY_OVERRIDE="1"
  fi
  if [[ -z "${SAMPLE_WINDOWS_OVERRIDE}" ]]; then
    SAMPLE_WINDOWS_OVERRIDE="32768"
  fi
  if [[ -z "${SIGNATURE_BYTES_OVERRIDE}" ]]; then
    SIGNATURE_BYTES_OVERRIDE="2048"
  fi
  if [[ -z "${AVALANCHE_BLOCKS_OVERRIDE}" ]]; then
    AVALANCHE_BLOCKS_OVERRIDE="12"
  fi
  if [[ -z "${AVALANCHE_TRIALS_OVERRIDE}" ]]; then
    AVALANCHE_TRIALS_OVERRIDE="2"
  fi
  if [[ -z "${BIC_SAMPLE_BITS_OVERRIDE}" ]]; then
    BIC_SAMPLE_BITS_OVERRIDE="64"
  fi
  if [[ -z "${SECOND_ORDER_TRIALS_OVERRIDE}" ]]; then
    SECOND_ORDER_TRIALS_OVERRIDE="2"
  fi
  if [[ -z "${CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE}" ]]; then
    CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE="1024"
  fi
  if [[ -z "${TOP_N_OVERRIDE}" ]]; then
    TOP_N_OVERRIDE="8"
  fi
fi

echo "effective_length_factor=${LENGTH_FACTOR_OVERRIDE:-default}"
echo "effective_trial_cap_per_category=${TRIAL_CAP_PER_CATEGORY_OVERRIDE:-full}"
echo "effective_sample_windows=${SAMPLE_WINDOWS_OVERRIDE:-default}"
echo "effective_signature_bytes=${SIGNATURE_BYTES_OVERRIDE:-default}"
echo "effective_avalanche_blocks=${AVALANCHE_BLOCKS_OVERRIDE:-default}"
echo "effective_avalanche_trials=${AVALANCHE_TRIALS_OVERRIDE:-default}"
echo "effective_bic_sample_bits=${BIC_SAMPLE_BITS_OVERRIDE:-default}"
echo "effective_second_order_trials=${SECOND_ORDER_TRIALS_OVERRIDE:-default}"
echo "effective_cross_input_signature_bytes=${CROSS_INPUT_SIGNATURE_BYTES_OVERRIDE:-default}"

if [[ "${RESUME_MODE}" != "1" || ! -f "${INDEX_PATH}" ]]; then
  python3 tools/split_twist_candidates.py \
    --input "${INPUT_CPP}" \
    --output-dir "${SHARD_CPP_DIR}" \
    --shard-size "${SHARD_SIZE_OVERRIDE}" \
    --index-output "${INDEX_PATH}"
else
  echo "reusing shard index: ${INDEX_PATH}"
fi

SHARD_SOURCES=()
while IFS= read -r SHARD_SOURCE; do
  SHARD_SOURCES+=("${SHARD_SOURCE}")
done < <(find "${SHARD_CPP_DIR}" -name 'shard_*.cpp' | sort)

if [[ "${#SHARD_SOURCES[@]}" -eq 0 ]]; then
  echo "no shard sources were created"
  exit 1
fi

TOTAL_SHARDS="${#SHARD_SOURCES[@]}"
CURRENT_SHARD=0

for SHARD_SOURCE in "${SHARD_SOURCES[@]}"; do
  CURRENT_SHARD=$((CURRENT_SHARD + 1))
  SHARD_INDEX=$((CURRENT_SHARD - 1))
  SHARD_NAME="$(basename "${SHARD_SOURCE}" .cpp)"
  SHARD_BUILD="build/shards/${SHARD_NAME}"
  SHARD_OUTPUT="${SHARD_OUTPUT_DIR}/${SHARD_NAME}"
  SHARD_DONE_FILE="${SHARD_OUTPUT}/twist_candidate_scores.csv"

  if (( SHARD_INDEX < START_SHARD_INDEX )); then
    echo
    echo "[${CURRENT_SHARD}/${TOTAL_SHARDS}] skipping ${SHARD_NAME} (before start_shard_index=${START_SHARD_INDEX})"
    continue
  fi

  if [[ "${RESUME_MODE}" == "1" && -f "${SHARD_DONE_FILE}" ]]; then
    echo
    echo "[${CURRENT_SHARD}/${TOTAL_SHARDS}] skipping ${SHARD_NAME} (already has scores)"
    continue
  fi

  echo
  echo "[${CURRENT_SHARD}/${TOTAL_SHARDS}] compiling ${SHARD_NAME}"
  clang++ -std=c++20 -O2 -I./src \
    ./src/TwistCandidateHarness.cpp \
    ./src/BaselineCandidates.cpp \
    ./src/PasswordExpander.cpp \
    ./src/LightningMatrix.cpp \
    ./src/HurricaneMatrix.cpp \
    ./src/TyphoonMatrix.cpp \
    "${SHARD_SOURCE}" \
    ./references/AESCounter.cpp \
    ./references/ARIA256Counter.cpp \
    ./references/ChaCha20Counter.cpp \
    ./references/MersenneCounter.cpp \
    -o "${SHARD_BUILD}"

  HARNESS_ARGS=(
    --seed "${RUN_SEED}"
    --input-suite "${INPUT_SUITE_OVERRIDE}"
    --output-dir "${SHARD_OUTPUT}"
  )

  if [[ -n "${STREAM_BYTES_OVERRIDE}" ]]; then
    HARNESS_ARGS+=(--stream-bytes "${STREAM_BYTES_OVERRIDE}")
  fi

  if [[ -n "${LENGTH_FACTOR_OVERRIDE}" ]]; then
    HARNESS_ARGS+=(--length-factor "${LENGTH_FACTOR_OVERRIDE}")
  fi

  if [[ -n "${TRIAL_COUNT_OVERRIDE}" ]]; then
    HARNESS_ARGS+=(--trial-count "${TRIAL_COUNT_OVERRIDE}")
  fi

  if [[ -n "${TRIAL_CAP_PER_CATEGORY_OVERRIDE}" ]]; then
    HARNESS_ARGS+=(--trial-cap-per-category "${TRIAL_CAP_PER_CATEGORY_OVERRIDE}")
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

  if [[ -n "${SHARD_LONG_REPEAT_TOP_OVERRIDE}" ]]; then
    HARNESS_ARGS+=(--long-repeat-top "${SHARD_LONG_REPEAT_TOP_OVERRIDE}")
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

  echo "[${CURRENT_SHARD}/${TOTAL_SHARDS}] evaluating ${SHARD_NAME}"
  "${SHARD_BUILD}" "${HARNESS_ARGS[@]}"
done

python3 tools/merge_twist_shard_scores.py \
  --input-dir "${SHARD_OUTPUT_DIR}" \
  --output-csv generated/twist_candidate_scores.csv \
  --output-summary generated/twist_candidate_summary.txt \
  --output-html generated/twist_candidate_report.html

EXPORT_TOP_COUNT="${TOP_N_OVERRIDE:-25}"
python3 tools/export_top_from_shards.py \
  --scores generated/twist_candidate_scores.csv \
  --index "${INDEX_PATH}" \
  --output generated/top_twist_candidates.cpp \
  --top "${EXPORT_TOP_COUNT}"

echo
echo "Merged scores: generated/twist_candidate_scores.csv"
echo "Merged summary: generated/twist_candidate_summary.txt"
echo "Merged HTML: generated/twist_candidate_report.html"
echo "Top export: generated/top_twist_candidates.cpp"

if [[ "${KEEP_SHARDS}" != "1" ]]; then
  rm -rf "${SHARD_CPP_DIR}"
fi
