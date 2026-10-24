#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BUILD_DIR="${ROOT_DIR}/build"
ASAN_BUILD_DIR="${ROOT_DIR}/build-asan"

usage() {
  cat <<'EOF'
Usage: ./scripts/build.sh [default|asan|leak|clean|help] [build|test|all]

Options:
  default   Configure and build the standard project in ./build
  asan      Configure and build the sanitizer build in ./build-asan
  leak      Run an optional leak check using valgrind (Linux) or leaks (macOS)
  clean     Remove the generated build directories
  help      Show this help text

Actions:
  build     Build only (default action)
  test      Run the tests for the selected build
  all       Build and test

Examples:
  ./scripts/build.sh default
  ./scripts/build.sh default test
  ./scripts/build.sh asan all
  ./scripts/build.sh leak
  ./scripts/build.sh clean
EOF
}

cpu_count() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu 2>/dev/null || echo 4
  else
    echo 4
  fi
}

build_default() {
  cmake -S "${ROOT_DIR}" -B "${DEFAULT_BUILD_DIR}" -DCMAKE_BUILD_TYPE=RelWithDebInfo
  cmake --build "${DEFAULT_BUILD_DIR}" --parallel "$(cpu_count)"
}

build_asan() {
  cmake -S "${ROOT_DIR}" -B "${ASAN_BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer' \
    -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address' \
    -DCMAKE_SHARED_LINKER_FLAGS='-fsanitize=address'
  cmake --build "${ASAN_BUILD_DIR}" --parallel "$(cpu_count)"
}

run_default_tests() {
  ctest --test-dir "${DEFAULT_BUILD_DIR}" --output-on-failure
}

run_asan_tests() {
  ctest --test-dir "${ASAN_BUILD_DIR}" --output-on-failure
}

run_leak_check() {
  build_default

  if command -v valgrind >/dev/null 2>&1; then
    echo "[info] Running leak check with valgrind"
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 "${DEFAULT_BUILD_DIR}/litecode_unit_tests"
    return 0
  fi

  if command -v leaks >/dev/null 2>&1; then
    echo "[info] Running leak check with leaks on macOS"
    leaks --atExit -- "${DEFAULT_BUILD_DIR}/litecode_unit_tests"
    return 0
  fi

  echo "[warn] No valgrind/leaks tool found on this system."
  echo "[warn] The recommended cross-platform memory check is the ASan build:"
  echo "[warn]   ./scripts/build.sh asan test"
  return 1
}

TARGET="${1:-default}"
ACTION="${2:-build}"

case "${TARGET}" in
  default)
    case "${ACTION}" in
      build) build_default ;;
      test) build_default; run_default_tests ;;
      all) build_default; run_default_tests ;;
      *) echo "Unknown action: ${ACTION}"; usage; exit 2 ;;
    esac
    ;;
  asan)
    case "${ACTION}" in
      build) build_asan ;;
      test) build_asan; run_asan_tests ;;
      all) build_asan; run_asan_tests ;;
      *) echo "Unknown action: ${ACTION}"; usage; exit 2 ;;
    esac
    ;;
  leak)
    case "${ACTION}" in
      build) run_leak_check ;;
      test) run_leak_check ;;
      all) run_leak_check ;;
      *) echo "Unknown action: ${ACTION}"; usage; exit 2 ;;
    esac
    ;;
  clean)
    rm -rf "${DEFAULT_BUILD_DIR}" "${ASAN_BUILD_DIR}" "${ROOT_DIR}/build_fresh"
    ;;
  help|-h|--help)
    usage
    ;;
  *)
    echo "Unknown target: ${TARGET}"
    usage
    exit 2
    ;;
 esac
