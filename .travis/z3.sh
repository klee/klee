#!/bin/bash -x

# Make sure we exit if there is a failure
set -e

source ${KLEE_SRC}/.travis/sanitizer_flags.sh
if [ "X${IS_SANITIZED_BUILD}" != "X0" ]; then
  echo "Error: Requested Sanitized build but Z3 being used is not sanitized"
  exit 1
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  # Should we install libz3-dbg too?
  sudo apt-get -y install libz3 libz3-dev
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  set +e
  brew install python@2
  if [[ "X$?" != "X0" ]]; then
     brew link --overwrite python@2
  fi
  set -e
  brew install z3
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi
