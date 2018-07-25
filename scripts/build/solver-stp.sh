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
  MINISAT_BUILD_PATH="${BASE}/minisat-build${SANITIZER_SUFFIX}"
  MINISAT_INSTALL_PATH="${BASE}/minisat-install${SANITIZER_SUFFIX}"
  mkdir -p "${MINISAT_BUILD_PATH}"
  cd "${MINISAT_BUILD_PATH}"
  if [[ "$TRAVIS_OS_NAME" == "linux" || "$TRAVIS_OS_NAME" == "osx" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS[@]}" \
    CXXFLAGS="${SANITIZER_CXX_FLAGS[@]}" \
    LDFLAGS="${SANITIZER_LD_FLAGS[@]}" \
    cmake -DCMAKE_INSTALL_PREFIX="${MINISAT_INSTALL_PATH}" \
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
  mkdir -p "${BASE}/stp-${STP_VERSION}-build${SANITIZER_SUFFIX}"
  cd "${BASE}/stp-${STP_VERSION}-build${SANITIZER_SUFFIX}"

  STP_CMAKE_FLAGS=( \
     -DNO_BOOST:BOOL=ON \
     -DENABLE_PYTHON_INTERFACE:BOOL=OFF \
     ${SANITIZER_CMAKE_C_COMPILER} \
     ${SANITIZER_CMAKE_CXX_COMPILER} \
  )
  # Disabling building of shared libs is a workaround.
  # Don't build against boost because that is broken when mixing packaged boost libraries and gcc 4.8
  CFLAGS="${SANITIZER_C_FLAGS[@]}" \
  CXXFLAGS="${SANITIZER_CXX_FLAGS[@]}" \
  LDFLAGS="${SANITIZER_LD_FLAGS[@]}" \
  cmake "${STP_CMAKE_FLAGS[@]}" \
        -DCMAKE_PREFIX_PATH="${MINISAT_INSTALL_PATH}" "${BASE}/stp-${STP_VERSION}" \
        -DCMAKE_INSTALL_PREFIX="${BASE}/stp-${STP_VERSION}-install${SANITIZER_SUFFIX}"
  make
  make install
else
  echo "No STP_VERSION given or empty"
  exit 1
fi
