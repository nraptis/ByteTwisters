#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

OUTPUT_DIR="${OUTPUT_DIR:-/Users/magneto/Desktop/Codex Playground/Twist/generated/nist_inputs}"
BITS_PER_CANDIDATE="${BITS_PER_CANDIDATE:-100000000}"
COUNT="${COUNT:-16}"
SEED_BASE="${SEED_BASE:-11400714819323198485}"

mkdir -p "${OUTPUT_DIR}"
bash ./run_build_twist_battery_stream.sh >/dev/null

bytes_per_candidate=$((BITS_PER_CANDIDATE / 8))
if (( bytes_per_candidate <= 0 )); then
  echo "BITS_PER_CANDIDATE must be at least 8"
  exit 1
fi

for ((i = 0; i < COUNT; ++i)); do
  seed=$((SEED_BASE + i))
  output_path="${OUTPUT_DIR}/candidate_$(printf '%02d' "${i}").txt"
  echo "[${i}/${COUNT}] ${output_path}"
  BYTES="${bytes_per_candidate}" \
  CANDIDATE_INDEX="${i}" \
  SEED="${seed}" \
  BITS_MODE=1 \
  OUTPUT_PATH="${output_path}" \
  bash ./run_emit_top_candidate_stream.sh >/dev/null
done

echo
echo "NIST input files:"
echo "  ${OUTPUT_DIR}"
