#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

SEED_OVERRIDE="${1:-${SEED:-}}"
INPUT_SUITE_OVERRIDE="${INPUT_SUITE:-stale}"
LENGTH_FACTOR_OVERRIDE="${LENGTH_FACTOR:-}"
STREAM_BYTES_OVERRIDE="${STREAM_BYTES:-}"
TRIAL_COUNT_OVERRIDE="${TRIAL_COUNT:-}"
CYCLE_BLOCK_COUNT_OVERRIDE="${CYCLE_BLOCK_COUNT:-}"
SAMPLE_WINDOWS_OVERRIDE="${SAMPLE_WINDOWS:-}"
SIGNATURE_BYTES_OVERRIDE="${SIGNATURE_BYTES:-}"
AVALANCHE_BLOCKS_OVERRIDE="${AVALANCHE_BLOCKS:-}"
AVALANCHE_TRIALS_OVERRIDE="${AVALANCHE_TRIALS:-}"
LONG_REPEAT_BYTES_OVERRIDE="${LONG_REPEAT_BYTES:-}"
LONG_REPEAT_TOP_OVERRIDE="${LONG_REPEAT_TOP:-}"
LONG_REPEAT_WINDOW_A_OVERRIDE="${LONG_REPEAT_WINDOW_A:-}"
LONG_REPEAT_WINDOW_B_OVERRIDE="${LONG_REPEAT_WINDOW_B:-}"
TOP_N_OVERRIDE="${TOP_N:-}"
LIMIT="${LIMIT:-0}"
CANDIDATE_ID="${CANDIDATE_ID:-}"

INPUT_CPP="generated/twist_candidates_generated_verbose.cpp"
MANIFEST_PATH="generated/twist_candidates_manifest.json"

if [[ ! -f "${INPUT_CPP}" || ! -f "${MANIFEST_PATH}" ]]; then
  echo "missing generated inputs"
  echo "expected: ${INPUT_CPP}"
  echo "expected: ${MANIFEST_PATH}"
  echo "run ./generate_twists.sh first"
  exit 1
fi

rm -rf build
mkdir -p build generated

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
export INPUT_SUITE="${INPUT_SUITE_OVERRIDE}"

python3 - <<'PY'
import json
import os
import re
from datetime import datetime
from pathlib import Path

knobs_path = Path("src/Knobs.hpp")
manifest_path = Path("generated/twist_candidates_manifest.json")
input_path = Path("generated/twist_candidates_generated_verbose.cpp")

knob_pattern = re.compile(
    r"inline constexpr (?:std::size_t|std::uint64_t|int|bool)\s+(k[A-Za-z0-9_]+)\s*=\s*([^;]+);"
)
knobs = {}
for match in knob_pattern.finditer(knobs_path.read_text(encoding="utf-8")):
    knobs[match.group(1)] = match.group(2).strip()

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
input_text = input_path.read_text(encoding="utf-8")
function_count = len(re.findall(r"^void TwistCandidate_\d{4}\(", input_text, flags=re.MULTILINE))
mtime = datetime.fromtimestamp(input_path.stat().st_mtime).astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")
generated_count = int(manifest.get("generated_candidate_count", manifest.get("candidate_count", 0)))
baseline_count = int(manifest.get("baseline_candidate_count", 0))
total_count = int(manifest.get("candidate_count", generated_count + baseline_count))

print(f"input_file={input_path}")
print(f"input_file_mtime={mtime}")
print("input_mix=mixed")
print(f"manifest_generated_count={generated_count}")
print(f"manifest_baseline_count={baseline_count}")
print(f"manifest_total_count={total_count}")
print(f"input_function_count={function_count}")
print(f"kCandidateCount={knobs.get('kCandidateCount', 'unknown')}")
print(f"kTopCandidateCount={knobs.get('kTopCandidateCount', 'unknown')}")
print(f"kLengthFactor={knobs.get('kLengthFactor', 'unknown')}")
print(f"kTrialCountAES={knobs.get('kTrialCountAES', 'unknown')}")
print(f"kTrialCountChaCha={knobs.get('kTrialCountChaCha', 'unknown')}")
print(f"kTrialCountZeros={knobs.get('kTrialCountZeros', 'unknown')}")
print(f"kTrialCountOnes={knobs.get('kTrialCountOnes', 'unknown')}")
print(f"kTrialCountPredictableA={knobs.get('kTrialCountPredictableA', 'unknown')}")
print(f"kTrialCountPredictableB={knobs.get('kTrialCountPredictableB', 'unknown')}")
print(f"kTrialCountPredictableC={knobs.get('kTrialCountPredictableC', 'unknown')}")

knob_count = int(knobs["kCandidateCount"]) if "kCandidateCount" in knobs else None
if knob_count is not None and generated_count != knob_count:
    print(f"warning=manifest_generated_count differs from kCandidateCount ({generated_count} vs {knob_count})")
if function_count != generated_count:
    print(f"warning=input_function_count differs from manifest_generated_count ({function_count} vs {generated_count})")
PY

clang++ -std=c++20 -O2 -I./src \
  ./src/TwistCandidateHarness.cpp \
  ./src/BaselineCandidates.cpp \
  ./src/LightningMatrix.cpp \
  "./${INPUT_CPP}" \
  ./references/AESCounter.cpp \
  ./references/ARIA256Counter.cpp \
  ./references/ChaCha20Counter.cpp \
  ./references/MersenneCounter.cpp \
  -o ./build/twist_candidate_harness

HARNESS_ARGS=(
  --seed "${RUN_SEED}"
  --input-suite "${INPUT_SUITE_OVERRIDE}"
  --output-dir generated
)

if [[ -n "${STREAM_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--stream-bytes "${STREAM_BYTES_OVERRIDE}")
fi

if [[ -n "${LENGTH_FACTOR_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--length-factor "${LENGTH_FACTOR_OVERRIDE}")
fi

if [[ -n "${TRIAL_COUNT_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--trial-count "${TRIAL_COUNT_OVERRIDE}")
fi

if [[ -n "${CYCLE_BLOCK_COUNT_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--cycle-block-count "${CYCLE_BLOCK_COUNT_OVERRIDE}")
fi

if [[ -n "${SAMPLE_WINDOWS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--sample-windows "${SAMPLE_WINDOWS_OVERRIDE}")
fi

if [[ -n "${SIGNATURE_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--signature-bytes "${SIGNATURE_BYTES_OVERRIDE}")
fi

if [[ -n "${AVALANCHE_BLOCKS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--avalanche-blocks "${AVALANCHE_BLOCKS_OVERRIDE}")
fi

if [[ -n "${AVALANCHE_TRIALS_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--avalanche-trials "${AVALANCHE_TRIALS_OVERRIDE}")
fi

if [[ -n "${LONG_REPEAT_BYTES_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-bytes "${LONG_REPEAT_BYTES_OVERRIDE}")
fi

if [[ -n "${LONG_REPEAT_TOP_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-top "${LONG_REPEAT_TOP_OVERRIDE}")
fi

if [[ -n "${LONG_REPEAT_WINDOW_A_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-window-a "${LONG_REPEAT_WINDOW_A_OVERRIDE}")
fi

if [[ -n "${LONG_REPEAT_WINDOW_B_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--long-repeat-window-b "${LONG_REPEAT_WINDOW_B_OVERRIDE}")
fi

if [[ -n "${TOP_N_OVERRIDE}" ]]; then
  HARNESS_ARGS+=(--top-n "${TOP_N_OVERRIDE}")
fi

if [[ "${LIMIT}" != "0" ]]; then
  HARNESS_ARGS+=(--limit "${LIMIT}")
fi

if [[ -n "${CANDIDATE_ID}" ]]; then
  HARNESS_ARGS+=(--candidate-id "${CANDIDATE_ID}")
fi

./build/twist_candidate_harness "${HARNESS_ARGS[@]}"

EXPORT_ARGS=(
  export-top
  --manifest "${MANIFEST_PATH}"
  --scores generated/twist_candidate_scores.csv
  --output generated/top_twist_candidates.cpp
)

if [[ -n "${TOP_N_OVERRIDE}" ]]; then
  EXPORT_ARGS+=(--top "${TOP_N_OVERRIDE}")
fi

python3 tools/generate_twist_candidates.py "${EXPORT_ARGS[@]}"

echo
echo "HTML report: generated/twist_candidate_report.html"
echo "Top export: generated/top_twist_candidates.cpp"
