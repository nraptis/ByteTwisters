#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

CANDIDATE_COUNT_OVERRIDE="${1:-${CANDIDATE_COUNT:-}}"
SEED_OVERRIDE="${2:-${SEED:-}}"

mkdir -p generated

if [[ -n "${SEED_OVERRIDE}" ]]; then
  RUN_SEED="${SEED_OVERRIDE}"
else
  RUN_SEED="$(python3 - <<'PY'
import pathlib
import re
import secrets

text = pathlib.Path("src/Knobs.hpp").read_text(encoding="utf-8")
randomize_match = re.search(r"inline constexpr bool kRandomizeSeedByDefault = (true|false);", text)
seed_match = re.search(r"inline constexpr std::uint64_t kRandomSeed = (\d+);", text)
randomize = randomize_match and randomize_match.group(1) == "true"
seed_value = int(seed_match.group(1)) if seed_match else 0

if seed_value != 0:
    print(seed_value)
elif randomize:
    print(secrets.randbits(64))
else:
    print(1337)
PY
)"
fi

echo "seed=${RUN_SEED}"

GENERATOR_ARGS=(
  generate
  --seed "${RUN_SEED}"
  --output-dir generated
)

if [[ -n "${CANDIDATE_COUNT_OVERRIDE}" ]]; then
  GENERATOR_ARGS+=(--count "${CANDIDATE_COUNT_OVERRIDE}")
fi

python3 tools/generate_twist_candidates.py "${GENERATOR_ARGS[@]}"

python3 - <<'PY'
from datetime import datetime
import json
from pathlib import Path

manifest_path = Path("generated/twist_candidates_manifest.json")
input_path = Path("generated/twist_candidates_generated_verbose.cpp")

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
mtime = datetime.fromtimestamp(input_path.stat().st_mtime).astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")

print(f"generated_input={input_path}")
print(f"generated_input_mtime={mtime}")
print(f"generated_test_count={manifest.get('generated_candidate_count', manifest.get('candidate_count', 0))}")
print(f"baseline_test_count={manifest.get('baseline_candidate_count', 0)}")
print(f"total_manifest_count={manifest.get('candidate_count', 0)}")
PY

echo
echo "Artifacts:"
echo "  generated/twist_candidates_generated.cpp"
echo "  generated/twist_candidates_generated_verbose.cpp"
echo "  generated/twist_candidates_generated_verbose.txt"
echo "  generated/twist_candidates_manifest.json"
