#!/bin/bash
# Build/setup all Travis dependencies if needed
set -e
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  export DOCKER_BUILD_DEPS_ONLY=1
  "${KLEE_SRC_TRAVIS}/scripts/build/build-docker.sh"
elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  brew tap klee/klee
  brew update
  brew install ccache
  set +e
  brew install python@2
  if [[ "X$?" != "X0" ]]; then
     brew link --overwrite python@2
  fi
  set -e
  pip install lit
  export PATH="/usr/local/opt/ccache/libexec:$PATH"
  "${KLEE_SRC_TRAVIS}/scripts/build/install-klee-deps.sh"
else
  echo "UNKNOWN OS ${TRAVIS_OS_NAME}"
  exit 1
fi
