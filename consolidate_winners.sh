#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_DIR="${INPUT_DIR:-${1:-practrand_a_34_passers}}"
OUTPUT_ROOT="${OUTPUT_ROOT:-generated/consolidated_winners}"
OUTPUT_DIR="${OUTPUT_DIR:-${OUTPUT_ROOT}/shards_cpp}"
INDEX_OUTPUT="${INDEX_OUTPUT:-${OUTPUT_ROOT}/shards_index.json}"
TARGET_SHARD_KB="${TARGET_SHARD_KB:-2048}"
MAX_CANDIDATES_PER_SHARD="${MAX_CANDIDATES_PER_SHARD:-0}"
PACK_MODE="${PACK_MODE:-balanced}"
COMPILE_TEST="${COMPILE_TEST:-0}"
CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA:--O3 -march=native -DNDEBUG}"

if [[ ! -d "${INPUT_DIR}" ]]; then
  echo "missing input directory: ${INPUT_DIR}"
  echo "expected candidate folders like ${INPUT_DIR}/candidate_*/code.cpp"
  exit 1
fi

python3 tools/consolidate_winners.py \
  --input-dir "${INPUT_DIR}" \
  --output-dir "${OUTPUT_DIR}" \
  --index-output "${INDEX_OUTPUT}" \
  --target-shard-kb "${TARGET_SHARD_KB}" \
  --max-candidates-per-shard "${MAX_CANDIDATES_PER_SHARD}" \
  --pack-mode "${PACK_MODE}" \
  --clean

if [[ "${COMPILE_TEST}" == "1" ]]; then
  BUILD_DIR="${OUTPUT_ROOT}/build"
  mkdir -p "${BUILD_DIR}"
  SHARD_SOURCES=()
  while IFS= read -r shard_source; do
    SHARD_SOURCES+=("${shard_source}")
  done < <(find "${OUTPUT_DIR}" -name 'shard_*.cpp' | sort)
  if [[ "${#SHARD_SOURCES[@]}" -eq 0 ]]; then
    echo "no shard sources found in ${OUTPUT_DIR}"
    exit 1
  fi
  for shard_source in "${SHARD_SOURCES[@]}"; do
    shard_name="$(basename "${shard_source}" .cpp)"
    echo "compile-testing ${shard_name}"
    INPUT_CPP="${shard_source}" \
    OUTPUT_BIN="${BUILD_DIR}/${shard_name}_stream" \
    CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
    bash ./run_build_twist_registered_stream.sh >/dev/null
  done
  echo "compile_tested=${#SHARD_SOURCES[@]}"
  echo "build_dir=${BUILD_DIR}"
fi

echo
echo "Consolidated winner artifacts:"
echo "  input_dir=${INPUT_DIR}"
echo "  output_dir=${OUTPUT_DIR}"
echo "  index_output=${INDEX_OUTPUT}"
echo "  target_shard_kb=${TARGET_SHARD_KB}"
echo "  pack_mode=${PACK_MODE}"
