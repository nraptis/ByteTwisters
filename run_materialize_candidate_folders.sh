#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INDEX_PATH="${INDEX_PATH:-generated/shards_index.json}"
OUTPUT_DIR="${OUTPUT_DIR:-/Users/magneto/Desktop/Codex Playground/Twist/generated/candidates}"
CANDIDATE_IDS="${CANDIDATE_IDS:-}"
LIMIT="${LIMIT:-0}"

ARGS=(--index "${INDEX_PATH}" --output-dir "${OUTPUT_DIR}")
if [[ -n "${CANDIDATE_IDS}" ]]; then
  ARGS+=(--candidate-ids "${CANDIDATE_IDS}")
fi
if [[ "${LIMIT}" != "0" ]]; then
  ARGS+=(--limit "${LIMIT}")
fi

python3 tools/materialize_candidate_folders.py "${ARGS[@]}"
