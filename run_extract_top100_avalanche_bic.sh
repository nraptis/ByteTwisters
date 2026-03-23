#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SCORES_PATH="${SCORES_PATH:-generated/twist_candidate_scores.csv}"
INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_PATH="${OUTPUT_PATH:-generated/top_100_avalanche_bic_equal.cpp}"
TOP_COUNT="${TOP_COUNT:-100}"

if [[ ! -f "${SCORES_PATH}" ]]; then
  echo "missing scores: ${SCORES_PATH}"
  echo "run shard measurement or merge first"
  exit 1
fi

if [[ ! -f "${INDEX_PATH}" ]]; then
  echo "missing shard index: ${INDEX_PATH}"
  echo "run ./run_measure_twists_sharded.sh first"
  exit 1
fi

python3 tools/export_top_from_shards.py \
  --scores "${SCORES_PATH}" \
  --index "${INDEX_PATH}" \
  --output "${OUTPUT_PATH}" \
  --top "${TOP_COUNT}" \
  --rank-mode avalanche-bic-equal \
  --standalone

echo
echo "Top export:"
echo "  ${OUTPUT_PATH}"
