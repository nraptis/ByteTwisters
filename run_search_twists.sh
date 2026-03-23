#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

CANDIDATE_COUNT_OVERRIDE="${1:-${CANDIDATE_COUNT:-20000}}"
SEED_OVERRIDE="${2:-${SEED:-}}"
SHORTLIST_COUNT="${SHORTLIST_COUNT:-64}"
FINAL_COUNT="${FINAL_COUNT:-16}"
TOP_REPORT_COUNT="${TOP_REPORT_COUNT:-25}"
BOTTOM_REPORT_COUNT="${BOTTOM_REPORT_COUNT:-25}"

SCREEN_LENGTH_FACTOR="${SCREEN_LENGTH_FACTOR:-256}"
SCREEN_STREAM_BYTES="${SCREEN_STREAM_BYTES:-}"
SCREEN_TRIAL_COUNT="${SCREEN_TRIAL_COUNT:-8}"
SCREEN_SAMPLE_WINDOWS="${SCREEN_SAMPLE_WINDOWS:-16384}"
SCREEN_SIGNATURE_BYTES="${SCREEN_SIGNATURE_BYTES:-1024}"
SCREEN_AVALANCHE_BLOCKS="${SCREEN_AVALANCHE_BLOCKS:-8}"
SCREEN_AVALANCHE_TRIALS="${SCREEN_AVALANCHE_TRIALS:-2}"

VERIFY_LENGTH_FACTOR="${VERIFY_LENGTH_FACTOR:-1024}"
VERIFY_STREAM_BYTES="${VERIFY_STREAM_BYTES:-}"
VERIFY_TRIAL_COUNT="${VERIFY_TRIAL_COUNT:-12}"
VERIFY_SAMPLE_WINDOWS="${VERIFY_SAMPLE_WINDOWS:-32768}"
VERIFY_SIGNATURE_BYTES="${VERIFY_SIGNATURE_BYTES:-4096}"
VERIFY_AVALANCHE_BLOCKS="${VERIFY_AVALANCHE_BLOCKS:-24}"
VERIFY_AVALANCHE_TRIALS="${VERIFY_AVALANCHE_TRIALS:-4}"
VERIFY_LONG_REPEAT_BYTES="${VERIFY_LONG_REPEAT_BYTES:-31457280}"
VERIFY_LONG_REPEAT_MIN_MATCH="${VERIFY_LONG_REPEAT_MIN_MATCH:-96}"

SEARCH_ROOT="generated/search"
SCREEN_ROOT="${SEARCH_ROOT}/screen"
VERIFY_ROOT="${SEARCH_ROOT}/verify"

./generate_twists.sh "${CANDIDATE_COUNT_OVERRIDE}" "${SEED_OVERRIDE}"

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

rm -rf build "${SEARCH_ROOT}"
mkdir -p build "${SCREEN_ROOT}" "${VERIFY_ROOT}"

clang++ -std=c++20 -O2 -I./src \
  ./src/TwistCandidateHarness.cpp \
  ./src/BaselineCandidates.cpp \
  ./src/PasswordExpander.cpp \
  ./src/LightningMatrix.cpp \
  ./src/HurricaneMatrix.cpp \
  ./src/TyphoonMatrix.cpp \
  ./generated/twist_candidates_generated_verbose.cpp \
  ./references/AESCounter.cpp \
  ./references/ARIA256Counter.cpp \
  ./references/ChaCha20Counter.cpp \
  ./references/MersenneCounter.cpp \
  -o ./build/twist_candidate_harness

echo "search_seed=${RUN_SEED}"
echo "candidate_count=${CANDIDATE_COUNT_OVERRIDE}"
echo "shortlist_count=${SHORTLIST_COUNT}"
echo "final_count=${FINAL_COUNT}"

for SUITE in stale pseudorandom structured; do
  OUT_DIR="${SCREEN_ROOT}/${SUITE}"
  mkdir -p "${OUT_DIR}"
  HARNESS_ARGS=(
    --seed "${RUN_SEED}"
    --input-suite "${SUITE}"
    --output-dir "${OUT_DIR}"
    --length-factor "${SCREEN_LENGTH_FACTOR}"
    --trial-count "${SCREEN_TRIAL_COUNT}"
    --sample-windows "${SCREEN_SAMPLE_WINDOWS}"
    --signature-bytes "${SCREEN_SIGNATURE_BYTES}"
    --avalanche-blocks "${SCREEN_AVALANCHE_BLOCKS}"
    --avalanche-trials "${SCREEN_AVALANCHE_TRIALS}"
    --long-repeat-top 0
    --top-n 8
  )
  if [[ -n "${SCREEN_STREAM_BYTES}" ]]; then
    HARNESS_ARGS+=(--stream-bytes "${SCREEN_STREAM_BYTES}")
  fi
  echo "screen_suite=${SUITE}"
  ./build/twist_candidate_harness "${HARNESS_ARGS[@]}"
done

python3 tools/aggregate_twist_search.py screen \
  --manifest generated/twist_candidates_manifest.json \
  --stale "${SCREEN_ROOT}/stale/twist_candidate_scores.csv" \
  --pseudorandom "${SCREEN_ROOT}/pseudorandom/twist_candidate_scores.csv" \
  --structured "${SCREEN_ROOT}/structured/twist_candidate_scores.csv" \
  --output-dir "${SEARCH_ROOT}" \
  --shortlist "${SHORTLIST_COUNT}" \
  --final-count "${FINAL_COUNT}" \
  --top-report "${TOP_REPORT_COUNT}" \
  --bottom-report "${BOTTOM_REPORT_COUNT}"

SHORTLIST_IDS=()
while IFS= read -r CANDIDATE_ID || [[ -n "${CANDIDATE_ID}" ]]; do
  [[ -z "${CANDIDATE_ID}" ]] && continue
  SHORTLIST_IDS+=("${CANDIDATE_ID}")
done < "${SEARCH_ROOT}/twist_search_shortlist_ids.txt"
echo "verify_candidates=${#SHORTLIST_IDS[@]}"

for CANDIDATE_ID in "${SHORTLIST_IDS[@]}"; do
  [[ -z "${CANDIDATE_ID}" ]] && continue
  for SUITE in stale pseudorandom structured; do
    OUT_DIR="${VERIFY_ROOT}/${SUITE}/candidate_${CANDIDATE_ID}"
    mkdir -p "${OUT_DIR}"
    HARNESS_ARGS=(
      --seed "${RUN_SEED}"
      --input-suite "${SUITE}"
      --candidate-id "${CANDIDATE_ID}"
      --limit 1
      --output-dir "${OUT_DIR}"
      --trial-count "${VERIFY_TRIAL_COUNT}"
      --sample-windows "${VERIFY_SAMPLE_WINDOWS}"
      --signature-bytes "${VERIFY_SIGNATURE_BYTES}"
      --avalanche-blocks "${VERIFY_AVALANCHE_BLOCKS}"
      --avalanche-trials "${VERIFY_AVALANCHE_TRIALS}"
      --long-repeat-bytes "${VERIFY_LONG_REPEAT_BYTES}"
      --long-repeat-top 1
      --long-repeat-min-match "${VERIFY_LONG_REPEAT_MIN_MATCH}"
      --top-n 1
    )
    if [[ -n "${VERIFY_STREAM_BYTES}" ]]; then
      HARNESS_ARGS+=(--stream-bytes "${VERIFY_STREAM_BYTES}")
    elif [[ -n "${VERIFY_LENGTH_FACTOR}" ]]; then
      HARNESS_ARGS+=(--length-factor "${VERIFY_LENGTH_FACTOR}")
    fi
    ./build/twist_candidate_harness "${HARNESS_ARGS[@]}"
  done
done

python3 tools/aggregate_twist_search.py finalize \
  --manifest generated/twist_candidates_manifest.json \
  --stale-dir "${VERIFY_ROOT}/stale" \
  --pseudorandom-dir "${VERIFY_ROOT}/pseudorandom" \
  --structured-dir "${VERIFY_ROOT}/structured" \
  --output-dir "${SEARCH_ROOT}" \
  --final-count "${FINAL_COUNT}" \
  --top-report "${TOP_REPORT_COUNT}" \
  --bottom-report "${BOTTOM_REPORT_COUNT}"

python3 tools/generate_twist_candidates.py export-top \
  --manifest generated/twist_candidates_manifest.json \
  --scores "${SEARCH_ROOT}/twist_search_final16_scores.csv" \
  --output "${SEARCH_ROOT}/final16_twist_candidates.cpp" \
  --top "${FINAL_COUNT}"

echo
echo "Search artifacts:"
echo "  ${SEARCH_ROOT}/twist_search_screen_combined.csv"
echo "  ${SEARCH_ROOT}/twist_search_top25.txt"
echo "  ${SEARCH_ROOT}/twist_search_bottom25.txt"
echo "  ${SEARCH_ROOT}/twist_search_verified_top25.txt"
echo "  ${SEARCH_ROOT}/twist_search_verified_bottom25.txt"
echo "  ${SEARCH_ROOT}/twist_search_final16.txt"
echo "  ${SEARCH_ROOT}/twist_search_final_report.html"
echo "  ${SEARCH_ROOT}/final16_twist_candidates.cpp"
