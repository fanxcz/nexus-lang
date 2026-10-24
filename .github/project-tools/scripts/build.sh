#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -G Ninja
cmake --build "$ROOT_DIR/build" -j"$(nproc)"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure
