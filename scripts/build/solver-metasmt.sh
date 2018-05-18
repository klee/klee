#!/bin/bash -x
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

: ${METASMT_VERSION?"METASMT_VERSION not specified"}

# Get Z3, libgmp, gperf (required by yices2)
apt update
apt -y --no-install-recommends install gperf libgmp-dev
apt clean
rm -rf /var/lib/apt/lists/*

# Clone
git clone --single-branch --depth 1 https://github.com/hoangmle/metaSMT.git "${BASE}/metaSMT"
cd "${BASE}/metaSMT"
git submodule update --init

if [ "X${IS_SANITIZED_BUILD}" != "X0" ]; then
  echo "Error: Requested Sanitized build but sanitized build of metaSMT is not implemented"
  exit 1
fi

# Bootstrap
git clone https://github.com/agra-uni-bremen/dependencies.git
./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off \
  --build stp-git-basic --build boolector-2.2.0 --build minisat-git \
  --build lingeling-ayv-86bf266-140429 --build yices-2.5.1 --build Z3-4.4.1 \
  --build cvc4-1.5

# Build
make -C build install

# Cleanup
if [[ "${STORAGE_SPACE_OPTIMIZED}x" == "1x" ]]; then
  rm -rf deps/build
  rm -rf deps/cache
fi
