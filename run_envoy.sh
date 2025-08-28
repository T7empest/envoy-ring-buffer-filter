#!/bin/bash
set -euo pipefail

ENVOY_BIN="$(bazel info bazel-bin)/envoy"

"$ENVOY_BIN" \
  -c ring_cache_server.yaml \
  --log-level error \
  --component-log-level filter:debug,main:debug \
  --concurrency 1