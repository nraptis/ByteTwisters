#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_DIR="${INPUT_DIR:-generated/shard_measure}"
INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_DIR="${OUTPUT_DIR:-generated/winners}"
TOP_COUNT="${TOP_COUNT:-50}"

if [[ ! -d "${INPUT_DIR}" ]]; then
  echo "missing shard score directory: ${INPUT_DIR}"
  echo "run ./run_measure_twists_sharded.sh first"
  exit 1
fi

if [[ ! -f "${INDEX_PATH}" ]]; then
  echo "missing shard index: ${INDEX_PATH}"
  echo "run ./run_measure_twists_sharded.sh first"
  exit 1
fi

mkdir -p "${OUTPUT_DIR}"

MERGED_CSV="${OUTPUT_DIR}/twist_candidate_scores.csv"
MERGED_SUMMARY="${OUTPUT_DIR}/twist_candidate_summary.txt"
MERGED_HTML="${OUTPUT_DIR}/twist_candidate_report.html"
WINNERS_CPP="${OUTPUT_DIR}/top_${TOP_COUNT}_twist_candidates.cpp"

python3 tools/merge_twist_shard_scores.py \
  --input-dir "${INPUT_DIR}" \
  --output-csv "${MERGED_CSV}" \
  --output-summary "${MERGED_SUMMARY}" \
  --output-html "${MERGED_HTML}" \
  --top-count "${TOP_COUNT}"

python3 tools/export_top_from_shards.py \
  --scores "${MERGED_CSV}" \
  --index "${INDEX_PATH}" \
  --output "${WINNERS_CPP}" \
  --top "${TOP_COUNT}"

echo
echo "Winner artifacts:"
echo "  ${MERGED_CSV}"
echo "  ${MERGED_SUMMARY}"
echo "  ${MERGED_HTML}"
echo "  ${WINNERS_CPP}"
