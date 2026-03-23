#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SCORES_PATH="${SCORES_PATH:-generated/twist_candidate_scores.csv}"
INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_PATH="${OUTPUT_PATH:-/Users/magneto/Desktop/Codex Playground/Twist/generated/top_16_finalists.cpp}"
TOP_COUNT="${TOP_COUNT:-16}"

python3 tools/export_top_from_shards.py \
  --scores "${SCORES_PATH}" \
  --index "${INDEX_PATH}" \
  --output "${OUTPUT_PATH}" \
  --top "${TOP_COUNT}" \
  --rank-mode finalist-strict \
  --standalone

echo
echo "Top finalist export:"
echo "  ${OUTPUT_PATH}"
