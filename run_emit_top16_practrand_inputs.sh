#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

OUTPUT_DIR="${OUTPUT_DIR:-/Users/magneto/Desktop/Codex Playground/Twist/generated/practrand_inputs}"
BYTES_PER_CANDIDATE="${BYTES_PER_CANDIDATE:-268435456}"
COUNT="${COUNT:-16}"
SEED_BASE="${SEED_BASE:-11400714819323198485}"

mkdir -p "${OUTPUT_DIR}"
bash ./run_build_twist_battery_stream.sh >/dev/null

for ((i = 0; i < COUNT; ++i)); do
  seed=$((SEED_BASE + i))
  output_path="${OUTPUT_DIR}/candidate_$(printf '%02d' "${i}").bin"
  echo "[${i}/${COUNT}] ${output_path}"
  BYTES="${BYTES_PER_CANDIDATE}" \
  CANDIDATE_INDEX="${i}" \
  SEED="${seed}" \
  OUTPUT_PATH="${output_path}" \
  bash ./run_emit_top_candidate_stream.sh >/dev/null
done

echo
echo "PractRand input files:"
echo "  ${OUTPUT_DIR}"
