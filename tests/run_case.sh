#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 && $# -ne 3 ]]; then
  echo "usage: $0 <binary> [--vm] <case_dir>" >&2
  exit 2
fi

BIN="$1"
EXTRA_ARGS=()
CASE_DIR="$2"

if [[ $# -eq 3 ]]; then
  EXTRA_ARGS=("$2")
  CASE_DIR="$3"
fi

MODE_FILE="${CASE_DIR}/mode"
if [[ -f "${MODE_FILE}" ]]; then
  MODE_VALUE="$(tr -d ' \t\r\n' < "${MODE_FILE}")"
  if [[ -n "${MODE_VALUE}" ]]; then
    EXTRA_ARGS=("${MODE_VALUE}")
  fi
fi

INPUT="${CASE_DIR}/input.lox"
EXP_OUT="${CASE_DIR}/expected.out"
EXP_ERR="${CASE_DIR}/expected.err"
EXP_EXIT="${CASE_DIR}/expected.exit"

for f in "${INPUT}" "${EXP_OUT}" "${EXP_ERR}" "${EXP_EXIT}"; do
  if [[ ! -f "${f}" ]]; then
    echo "[fail] missing required file: ${f}" >&2
    exit 2
  fi
done

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TMP_DIR}"' EXIT

ACT_OUT="${TMP_DIR}/actual.out"
ACT_ERR="${TMP_DIR}/actual.err"

set +e
if [[ ${#EXTRA_ARGS[@]} -gt 0 ]]; then
  "${BIN}" "${EXTRA_ARGS[@]}" "${INPUT}" >"${ACT_OUT}" 2>"${ACT_ERR}"
else
  "${BIN}" "${INPUT}" >"${ACT_OUT}" 2>"${ACT_ERR}"
fi
ACT_EXIT=$?
set -e

EXPECTED_EXIT="$(tr -d ' \t\r\n' < "${EXP_EXIT}")"
if [[ "${ACT_EXIT}" != "${EXPECTED_EXIT}" ]]; then
  echo "[fail] ${CASE_DIR}: exit code mismatch (expected ${EXPECTED_EXIT}, got ${ACT_EXIT})" >&2
  exit 1
fi

if ! diff -u "${EXP_OUT}" "${ACT_OUT}" >/dev/null; then
  echo "[fail] ${CASE_DIR}: stdout mismatch" >&2
  diff -u "${EXP_OUT}" "${ACT_OUT}" || true
  exit 1
fi

if ! diff -u "${EXP_ERR}" "${ACT_ERR}" >/dev/null; then
  echo "[fail] ${CASE_DIR}: stderr mismatch" >&2
  diff -u "${EXP_ERR}" "${ACT_ERR}" || true
  exit 1
fi

exit 0
