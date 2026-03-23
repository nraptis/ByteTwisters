#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SCORES_PATH="${SCORES_PATH:-generated/twist_candidate_scores.csv}"
INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_PATH="${OUTPUT_PATH:-/Users/magneto/Desktop/Codex Playground/Twist/generated/top_64_balanced_shortlist.cpp}"
TOP_COUNT="${TOP_COUNT:-64}"

python3 tools/export_top_from_shards.py \
  --scores "${SCORES_PATH}" \
  --index "${INDEX_PATH}" \
  --output "${OUTPUT_PATH}" \
  --top "${TOP_COUNT}" \
  --rank-mode balanced-shortlist \
  --standalone

python3 - <<'PY'
from pathlib import Path
from tools.export_top_from_shards import load_top_rows

scores_path = Path("generated/twist_candidate_scores.csv")
rows = load_top_rows(scores_path, 16, "balanced-shortlist")
print()
print("Balanced shortlist preview:")
print("rank  id    score   mean    floor   tail    aval    bic     name")
for rank, row in enumerate(rows, start=1):
    print(
        f"{rank:>4}  "
        f"{int(row['candidate_id']):>4}  "
        f"{float(row['_rank_score']):>6.3f}  "
        f"{float(row['_input_mean']):>6.3f}  "
        f"{float(row['_input_floor']):>6.3f}  "
        f"{float(row['_input_tail']):>6.3f}  "
        f"{float(row['avalanche_score']):>6.3f}  "
        f"{float(row['bic_score']):>6.3f}  "
        f"{row['function_name']}"
    )
PY

echo
echo "Top shortlist export:"
echo "  ${OUTPUT_PATH}"
