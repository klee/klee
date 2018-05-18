#!/bin/bash -x
set -ev

DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  # Get tcmalloc release
  if [[ ! -e "${BASE}/gperftools-${TCMALLOC_VERSION}" ]]; then
    cd "${BASE}"
    wget "https://github.com/gperftools/gperftools/releases/download/gperftools-${TCMALLOC_VERSION}/gperftools-${TCMALLOC_VERSION}.tar.gz"
    tar xfz "gperftools-${TCMALLOC_VERSION}.tar.gz"
  fi
  cd "${BASE}/gperftools-${TCMALLOC_VERSION}"
  ./configure --disable-dependency-tracking --disable-cpu-profiler \
    --disable-heap-checker --disable-debugalloc  --enable-minimal \
    --prefix="${BASE}/tcmalloc-install"
  make
  make install
  if [[ "${KEEP_BUILD}x" != "1x" ]]; then
    rm -rf "${BASE}/gperftools-${TCMALLOC_VERSION}"
  fi

  if [[ "${KEEP_SRC}x" != "1x" ]]; then
    rm -f "${BASE}/gperftools-${TCMALLOC_VERSION}.tar.gz"
  fi
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  if [ ${USE_TCMALLOC:-0} -eq 1 ] ; then
    echo "Error: Requested to install TCMalloc on macOS, which is not supported"
    exit 1
  fi
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi
