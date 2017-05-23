#!/bin/bash -x

# TODO: For OSX we can prepare bottled formulas for STP and minisat

# Make sure we exit if there is a failure
set -e

STP_LOG="$(pwd)/stp-build.log"

if [ "x${STP_VERSION}" != "x" ]; then
  # Get Sanitizer flags
  source ${KLEE_SRC}/.travis/sanitizer_flags.sh

  # Build minisat
  git clone https://github.com/stp/minisat
  cd minisat
  mkdir build
  cd build
  MINISAT_DIR=`pwd`
  if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS}" \
    CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS}" \
    CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
  else
    echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
    exit 1
  fi
  make
  sudo make install
  cd ../../

  # Determine STP build flags
  if [ "X${STP_VERSION}" = "Xmaster" ]; then
    # 7e0b096ee79d59bb5c344b7dd4d51b5b8d226221 s/NO_BOOST/ONLY_SIMPLE/
    # 5e9ca6339a2b3b000aa7a90c18601fdcf1212fe1 Silently BUILD_SHARED_LIBS removed. STATICCOMPILE does something similar
    STP_CMAKE_FLAGS=( \
      "-DENABLE_PYTHON_INTERFACE:BOOL=OFF" \
      "-DONLY_SIMPLE:BOOL=ON" \
      "-DSTATICCOMPILE:BOOL=ON" \
    )
  else
    STP_CMAKE_FLAGS=( \
      "-DENABLE_PYTHON_INTERFACE:BOOL=OFF" \
      "-DNO_BOOST:BOOL=ON" \
      "-DBUILD_SHARED_LIBS:BOOL=OFF" \
    )
  fi

  # Build STP
  git clone --depth 1 -b "${STP_VERSION}" git://github.com/stp/stp.git src
  mkdir build
  cd build
  # Disabling building of shared libs is a workaround.
  # Don't build against boost because that is broken when mixing packaged boost libraries and gcc 4.8
  CFLAGS="${SANITIZER_C_FLAGS}" \
  CXXFLAGS="${SANITIZER_CXX_FLAGS}" \
  cmake "${STP_CMAKE_FLAGS[@]}" ../src

  set +e # Do not exit if build fails because we need to display the log
  make >> "${STP_LOG}" 2>&1
else
  echo "No STP_VERSION given or empty"
  exit 1
fi

# Only show build output if something went wrong to keep log output short
if [ $? -ne 0 ]; then
  echo "Build error"
  cat "${STP_LOG}"
fi
