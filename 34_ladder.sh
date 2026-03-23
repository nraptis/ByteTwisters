#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT_DIR}"

default_source_dir="${ROOT_DIR}/runner/practrand_a_34_passers"
if [[ -d "${ROOT_DIR}/practrand_a_34_passers" ]]; then
  default_source_dir="${ROOT_DIR}/practrand_a_34_passers"
fi

default_password_file="${ROOT_DIR}/runner/34_ladder_passwords.txt"
if [[ -f "${ROOT_DIR}/34_ladder_passwords.txt" ]]; then
  default_password_file="${ROOT_DIR}/34_ladder_passwords.txt"
fi

default_output_root="${ROOT_DIR}/runner/34_ladder_runs"
if [[ -d "${ROOT_DIR}/practrand_a_34_passers" || -f "${ROOT_DIR}/34_ladder_passwords.txt" ]]; then
  default_output_root="${ROOT_DIR}/34_ladder_runs"
fi

SOURCE_DIR="${1:-${default_source_dir}}"
PASSWORD_FILE="${2:-${default_password_file}}"
SIZE_SPEC="${3:-34blk}"
OUTPUT_ROOT="${4:-${default_output_root}}"

WORKSPACE_DIR="${WORKSPACE_DIR:-${OUTPUT_ROOT}/workspace}"
LOG_DIR="${LOG_DIR:-${OUTPUT_ROOT}/logs}"
SETUP_ONLY="${SETUP_ONLY:-0}"
RESUME_FLAG="${RESUME_FLAG:-0}"

MAX_FAIL="${MAX_FAIL:-0}"
MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS:-1000}"
MAX_SUSPICIOUS="${MAX_SUSPICIOUS:-0}"
MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS:-999999}"
MAX_UNUSUAL="${MAX_UNUSUAL:-999999}"
NO_RESULT_FILES="${NO_RESULT_FILES:-0}"

if [[ ! -d "${SOURCE_DIR}" ]]; then
  echo "missing source dir: ${SOURCE_DIR}" >&2
  exit 1
fi

if [[ ! -f "${PASSWORD_FILE}" ]]; then
  echo "missing password file: ${PASSWORD_FILE}" >&2
  exit 1
fi

mkdir -p "${OUTPUT_ROOT}" "${LOG_DIR}"

clone_workspace() {
  if [[ -d "${WORKSPACE_DIR}" ]]; then
    echo "workspace_exists=${WORKSPACE_DIR}"
    return
  fi

  echo "cloning_workspace_from=${SOURCE_DIR}"
  cp -R "${SOURCE_DIR}" "${WORKSPACE_DIR}"
  find "${WORKSPACE_DIR}" -type d -name ".practrand_stage" -prune -exec rm -rf {} +
  find "${WORKSPACE_DIR}" -type d -name ".practrand_build" -prune -exec rm -rf {} +
  mkdir -p "${WORKSPACE_DIR}/.practrand_stage" "${WORKSPACE_DIR}/.practrand_build"
  echo "workspace_created=${WORKSPACE_DIR}"
}

sanitize_name() {
  python3 - "$1" <<'PY'
import re
import sys

text = sys.argv[1]
slug = re.sub(r"[^A-Za-z0-9]+", "_", text).strip("_").lower()
if not slug:
    slug = "blank"
print(slug[:48])
PY
}

clone_workspace

count_candidate_dirs() {
  find "$1" -maxdepth 1 -type d -name 'candidate_*' | wc -l | tr -d '[:space:]'
}

if [[ "${SETUP_ONLY}" == "1" ]]; then
  echo "setup_only=1"
  exit 0
fi

run_count=0
current_input_dir="${WORKSPACE_DIR}"
while IFS= read -r password || [[ -n "${password}" ]]; do
  if [[ -z "${password}" || "${password}" == \#* ]]; then
    continue
  fi

  input_candidates="$(count_candidate_dirs "${current_input_dir}")"
  if [[ "${input_candidates}" == "0" ]]; then
    echo
    echo "stopping_ladder=no_candidates_remaining"
    echo "last_input_dir=${current_input_dir}"
    break
  fi

  run_count="$((run_count + 1))"
  safe_name="$(sanitize_name "${password}")"
  run_tag="$(printf '%05d' "${run_count}")"
  passers_dir="${OUTPUT_ROOT}/ladder_passers_${run_tag}_${safe_name}"
  zero_failers_dir="${OUTPUT_ROOT}/ladder_zero_failers_${run_tag}_${safe_name}"
  log_file="${LOG_DIR}/ladder_${run_tag}_${safe_name}.log"

  echo
  echo "=== ladder run ${run_tag} password=${password} ==="
  echo "input_dir=${current_input_dir}"
  echo "input_candidates=${input_candidates}"
  echo "workspace=${WORKSPACE_DIR}"
  echo "passers_dir=${passers_dir}"
  echo "zero_failers_dir=${zero_failers_dir}"
  echo "log_file=${log_file}"

  env \
    MAX_FAIL="${MAX_FAIL}" \
    MAX_VERY_SUSPICIOUS="${MAX_VERY_SUSPICIOUS}" \
    MAX_SUSPICIOUS="${MAX_SUSPICIOUS}" \
    MAX_MILDLY_SUSPICIOUS="${MAX_MILDLY_SUSPICIOUS}" \
    MAX_UNUSUAL="${MAX_UNUSUAL}" \
    NO_RESULT_FILES="${NO_RESULT_FILES}" \
    RESUME_FLAG="${RESUME_FLAG}" \
    ZERO_FAILERS_DIR="${zero_failers_dir}" \
    "${ROOT_DIR}/run_practrand_34blk_in_place.sh" \
    "${current_input_dir}" \
    "${password}" \
    "${SIZE_SPEC}" \
    "${passers_dir}" | tee "${log_file}"

  current_input_dir="${passers_dir}"
done < "${PASSWORD_FILE}"

echo
echo "runs_completed=${run_count}"
echo "workspace=${WORKSPACE_DIR}"
echo "output_root=${OUTPUT_ROOT}"
