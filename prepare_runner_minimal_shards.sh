#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

INPUT_DIR="${INPUT_DIR:-${1:-practrand_a_34_passers}}"
RUNNER_DIR="${RUNNER_DIR:-${2:-${ROOT_DIR}/runner}}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${RUNNER_DIR}}"
TARGET_SHARD_KB="${TARGET_SHARD_KB:-2048}"
MAX_CANDIDATES_PER_SHARD="${MAX_CANDIDATES_PER_SHARD:-0}"
PACK_MODE="${PACK_MODE:-balanced}"
COMPILE_TEST="${COMPILE_TEST:-0}"

LOCAL_OUTPUT_ROOT="${OUTPUT_ROOT}/consolidated_winners"
LOCAL_OUTPUT_DIR="${LOCAL_OUTPUT_ROOT}/shards_cpp"
LOCAL_INDEX_OUTPUT="${OUTPUT_ROOT}/shards_index.json"

mkdir -p "${RUNNER_DIR}"

INPUT_DIR="${INPUT_DIR}" \
OUTPUT_ROOT="${LOCAL_OUTPUT_ROOT}" \
OUTPUT_DIR="${LOCAL_OUTPUT_DIR}" \
INDEX_OUTPUT="${LOCAL_INDEX_OUTPUT}" \
TARGET_SHARD_KB="${TARGET_SHARD_KB}" \
MAX_CANDIDATES_PER_SHARD="${MAX_CANDIDATES_PER_SHARD}" \
PACK_MODE="${PACK_MODE}" \
COMPILE_TEST="${COMPILE_TEST}" \
bash "${ROOT_DIR}/consolidate_winners.sh"

echo
echo "Minimal runner shards ready:"
echo "  runner_dir=${RUNNER_DIR}"
echo "  shards_dir=${LOCAL_OUTPUT_DIR}"
echo "  shards_index=${LOCAL_INDEX_OUTPUT}"
