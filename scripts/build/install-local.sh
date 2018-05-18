#!/bin/bash
# Install all dependencies to build KLEE in a directory ${BASE}
set -e
if [[ "${BASE}x" == "x" ]]; then
  echo "\${BASE} not set"
  exit 1
fi

# Create base directory if not exists
mkdir -p "${BASE}"
DIR=$(dirname "$(readlink -f "$0" || stat -f "$0")")

# Install dependencies
"${DIR}/install-klee-deps.sh"

# Build KLEE
"${DIR}/klee.sh"
