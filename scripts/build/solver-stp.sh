#!/bin/bash -x

# TODO: For OSX we can prepare bottled formulas for STP and minisat

# Make sure we exit if there is a failure
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

cd ${BASE}
if [ "x${STP_VERSION}" != "x" ]; then
  # Build minisat
  git_clone_or_update https://github.com/stp/minisat "${BASE}/minisat"
  mkdir -p "${BASE}/minisat/build"
  cd "${BASE}/minisat/build"
  MINISAT_DIR="$(pwd)"
  if [[ "$TRAVIS_OS_NAME" == "linux" || "$TRAVIS_OS_NAME" == "osx" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS}" \
    CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
    LDFLAGS="${SANITIZER_LD_FLAGS}" \
    cmake -DCMAKE_INSTALL_PREFIX="${BASE}/minisat-install" \
      ${SANITIZER_CMAKE_C_COMPILER} \
      ${SANITIZER_CMAKE_CXX_COMPILER} \
      "${BASE}/minisat"
  else
    echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
    exit 1
  fi
  make
  make install

  # Build STP
  git_clone_or_update git://github.com/stp/stp.git "${BASE}/stp-${STP_VERSION}" ${STP_VERSION}
  mkdir -p "${BASE}/stp-${STP_VERSION}-build"
  cd "${BASE}/stp-${STP_VERSION}-build"

  STP_CMAKE_FLAGS=( \
     "-DENABLE_PYTHON_INTERFACE:BOOL=OFF" \
     -DNO_BOOST:BOOL=ON \
     -DENABLE_PYTHON_INTERFACE:BOOL=OFF \
     ${SANITIZER_CMAKE_C_COMPILER} \
     ${SANITIZER_CMAKE_CXX_COMPILER} \
  )
  # Disabling building of shared libs is a workaround.
  # Don't build against boost because that is broken when mixing packaged boost libraries and gcc 4.8
  CFLAGS="${SANITIZER_C_FLAGS}" \
  CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
  LDFLAGS="${SANITIZER_LD_FLAGS}" \
  cmake ${STP_CMAKE_FLAGS} \
        -DCMAKE_PREFIX_PATH="${BASE}/minisat-install" "${BASE}/stp-${STP_VERSION}" \
        -DCMAKE_INSTALL_PREFIX="${BASE}/stp-${STP_VERSION}-install"
  make
  make install
else
  echo "No STP_VERSION given or empty"
  exit 1
fi
