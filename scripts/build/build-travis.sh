#!/bin/bash
# Build/setup all Travis dependencies if needed
set -e
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  export DOCKER_BUILD_DEPS_ONLY=1
  "${KLEE_SRC_TRAVIS}/scripts/build/build-docker.sh"
elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  brew install ccache
  set +e
  brew upgrade python               # upgrade to Python 3
  brew install python@2             # install Python 2 (OSX version outdated + pip missing)
  brew link --overwrite python@2    # keg-only => link manually
  rm -f /usr/local/bin/{python,pip} # re-link python/pip to Python 2
  py2path=$(find /usr/local/Cellar/python@2/ -name "python" -type f | head -n 1)
  pip2path=$(find /usr/local/Cellar/python@2/ -name "pip" -type f | head -n 1)
  ln -s "${py2path}" /usr/local/bin/python
  ln -s "${pip2path}" /usr/local/bin/pip
  set -e
  pip install lit==0.6.0
  export PATH="/usr/local/opt/ccache/libexec:$PATH"
  "${KLEE_SRC_TRAVIS}/scripts/build/install-klee-deps.sh"
else
  echo "UNKNOWN OS ${TRAVIS_OS_NAME}"
  exit 1
fi
