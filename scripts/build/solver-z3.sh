#!/bin/bash -x

# Make sure we exit if there is a failure
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

if [[ -z "${Z3_VERSION}" ]] ; then
  echo "Z3_VERSION is not defined"
  exit 1
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  mkdir -p "${BASE}/z3-${Z3_VERSION}"
  cd "${BASE}/z3-${Z3_VERSION}"
  wget -qO- https://github.com/Z3Prover/z3/archive/z3-${Z3_VERSION}.tar.gz | tar xz --strip-components=1
  LDFLAGS="${SANITIZER_LD_FLAGS}" \
  CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
  python scripts/mk_make.py --prefix "${BASE}/z3-${Z3_VERSION}-install"
  cd build
  make -j$(nproc)
  make install
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
