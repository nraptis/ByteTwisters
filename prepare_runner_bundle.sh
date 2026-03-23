#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

RUNNER_DIR="${1:-${ROOT_DIR}/runner}"

mkdir -p "${RUNNER_DIR}" "${RUNNER_DIR}/src" "${RUNNER_DIR}/tools"

script_files=(
  "34_ladder"
  "34_ladder.sh"
  "run_practrand_34blk_alphabet_gauntlet.sh"
  "run_practrand_34blk_in_place.sh"
  "run_practrand_passwords_and_list.sh"
  "run_registered_candidate_practrand_once.sh"
  "run_emit_registered_candidate_stream.sh"
  "run_build_twist_registered_stream.sh"
)

tool_files=(
  "tools/run_practrand_in_place.py"
  "tools/run_practrand_passwords_and_list.py"
)

src_files=(
  "src/ByteTwister.cpp"
  "src/ByteTwister.hpp"
  "src/HurricaneMatrix.cpp"
  "src/HurricaneMatrix.hpp"
  "src/Knobs.hpp"
  "src/LightningMatrix.cpp"
  "src/LightningMatrix.hpp"
  "src/PasswordExpander.cpp"
  "src/PasswordExpander.hpp"
  "src/Scrambler.hpp"
  "src/TwistRegisteredBatteryStream.cpp"
  "src/TwistBreakers.hpp"
  "src/TwistTypes.hpp"
  "src/TyphoonMatrix.cpp"
  "src/TyphoonMatrix.hpp"
)

for file in "${script_files[@]}"; do
  cp "${ROOT_DIR}/${file}" "${RUNNER_DIR}/$(basename "${file}")"
done

for file in "${tool_files[@]}"; do
  cp "${ROOT_DIR}/${file}" "${RUNNER_DIR}/${file}"
done

for file in "${src_files[@]}"; do
  cp "${ROOT_DIR}/${file}" "${RUNNER_DIR}/${file}"
done

if [[ -f "${ROOT_DIR}/runner/34_ladder_passwords.txt" ]]; then
  password_dest="${RUNNER_DIR}/34_ladder_passwords.txt"
  if [[ "$(cd "$(dirname "${ROOT_DIR}/runner/34_ladder_passwords.txt")" && pwd)/$(basename "${ROOT_DIR}/runner/34_ladder_passwords.txt")" != "$(cd "$(dirname "${password_dest}")" && pwd)/$(basename "${password_dest}")" ]]; then
    cp "${ROOT_DIR}/runner/34_ladder_passwords.txt" "${password_dest}"
  fi
fi

chmod +x \
  "${RUNNER_DIR}/34_ladder" \
  "${RUNNER_DIR}/34_ladder.sh" \
  "${RUNNER_DIR}/run_practrand_34blk_alphabet_gauntlet.sh" \
  "${RUNNER_DIR}/run_practrand_34blk_in_place.sh" \
  "${RUNNER_DIR}/run_practrand_passwords_and_list.sh" \
  "${RUNNER_DIR}/run_registered_candidate_practrand_once.sh" \
  "${RUNNER_DIR}/run_emit_registered_candidate_stream.sh" \
  "${RUNNER_DIR}/run_build_twist_registered_stream.sh"

cat > "${RUNNER_DIR}/RUNNER_BUNDLE_README.txt" <<'EOF'
Standalone runner bundle

Expected layout:
- ./practrand_a_34_passers
- ./shards_cpp
- ./src
- ./tools
- ./34_ladder
- ./34_ladder_passwords.txt
- ./run_practrand_34blk_alphabet_gauntlet.sh
- ./run_practrand_passwords_and_list.sh

Typical run:
MAX_FAIL=0 MAX_VERY_SUSPICIOUS=1000 NO_RESULT_FILES=0 \
./34_ladder \
./practrand_a_34_passers \
./34_ladder_passwords.txt \
34blk \
./secret_34_outputs

Alphabet gauntlet:
MAX_FAIL=0 MAX_VERY_SUSPICIOUS=1000 \
./run_practrand_34blk_alphabet_gauntlet.sh \
./practrand_a_34_passers \
./alphabet_gauntlet_34blk_passers

Notes:
- Metadata may still point at old absolute shard paths. The bundled tooling will
  fall back to ./shards_cpp/<source_shard_name>.cpp automatically.
- PractRand's RNG_test still needs to be available on PATH, or at ./build/practrand/RNG_test.
EOF

echo "runner_bundle_ready=${RUNNER_DIR}"
