#!/bin/bash
# test_render_baseline.sh — Render E1M1 and compare against baseline PPMs
# Exit 0 on pass, 1 on failure.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
# Optional first arg: absolute path to the build dir (where render_test was
# built). Defaults to "$PROJECT_DIR/build" for backwards-compatible local
# `task test` runs; CMake passes ${CMAKE_BINARY_DIR} so out-of-tree builds
# (e.g. build-linux/) work too.
BUILD_DIR="${1:-$PROJECT_DIR/build}"
RENDER_TEST="$BUILD_DIR/render_test"
WAD="$PROJECT_DIR/resources/DOOM1.WAD"
BASELINES="$SCRIPT_DIR/baselines"
TMPDIR="$BUILD_DIR/test_output"

mkdir -p "$TMPDIR"
trap "rm -rf $TMPDIR" EXIT

if [ ! -f "$RENDER_TEST" ]; then
    echo "FAIL: render_test not built (run cmake --build build first)"
    exit 1
fi

if [ ! -f "$WAD" ]; then
    echo "FAIL: DOOM1.WAD not found at $WAD"
    exit 1
fi

# Run the renderer
cd "$TMPDIR"
"$RENDER_TEST" "$WAD" e1m1_test.ppm > /dev/null 2>&1
RESULT=$?

if [ $RESULT -ne 0 ]; then
    echo "FAIL: render_test exited with code $RESULT"
    exit 1
fi

# Compare against baselines
PASS=0
FAIL=0

for baseline in "$BASELINES"/*.ppm; do
    name=$(basename "$baseline")
    if [ ! -f "$TMPDIR/$name" ]; then
        echo "FAIL: $name not generated"
        FAIL=$((FAIL + 1))
        continue
    fi

    if cmp -s "$baseline" "$TMPDIR/$name"; then
        echo "PASS: $name matches baseline"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $name differs from baseline"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "$PASS passed, $FAIL failed"

if [ $FAIL -gt 0 ]; then
    exit 1
fi
