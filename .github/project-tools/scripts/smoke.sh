#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
"$ROOT_DIR/build/nexus" check "$ROOT_DIR/examples/hello.nx"
"$ROOT_DIR/build/nexus" run "$ROOT_DIR/examples/hello.nx"
"$ROOT_DIR/build/nexus" run "$ROOT_DIR/examples/factorial.nx"
"$ROOT_DIR/build/nexus" run "$ROOT_DIR/examples/control_flow.nx"
