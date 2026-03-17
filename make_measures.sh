#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

CANDIDATE_COUNT_OVERRIDE="${1:-${CANDIDATE_COUNT:-}}"
SEED_OVERRIDE="${2:-${SEED:-}}"

./generate_twists.sh "${CANDIDATE_COUNT_OVERRIDE}" "${SEED_OVERRIDE}"
./run_measure_twists.sh "${SEED_OVERRIDE}"
