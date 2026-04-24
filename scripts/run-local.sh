#!/usr/bin/env bash
# Convenience wrapper: run plugin-harness with sane ASan defaults.
#
# Usage: scripts/run-local.sh <plugin> <scenario> [extra args...]
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "usage: $0 <plugin-path> <scenario> [extra args...]" >&2
    exit 2
fi

PLUGIN="$1"; shift
SCENARIO="$1"; shift

export ASAN_OPTIONS="${ASAN_OPTIONS:-abort_on_error=1:symbolize=1:print_stacktrace=1:detect_leaks=0}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"
export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1}"

# Find the harness binary.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

for candidate in \
    "$ROOT/build-asan/plugin-harness_artefacts/Debug/plugin-harness" \
    "$ROOT/build/plugin-harness_artefacts/Debug/plugin-harness" \
    "$ROOT/build/plugin-harness_artefacts/Release/plugin-harness" \
    "$ROOT/build/plugin-harness"; do
    if [[ -x "$candidate" ]]; then
        BIN="$candidate"
        break
    fi
done

if [[ -z "${BIN:-}" ]]; then
    echo "error: no built plugin-harness binary found. Build first:" >&2
    echo "  cmake -S . -B build-asan -G Ninja -DHARNESS_ENABLE_ASAN=ON" >&2
    echo "  cmake --build build-asan -j" >&2
    exit 2
fi

echo "[run-local] BIN=$BIN"
echo "[run-local] ASAN_OPTIONS=$ASAN_OPTIONS"
exec "$BIN" --plugin "$PLUGIN" --scenario "$SCENARIO" "$@"
