#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

# The New CMake build system just needs the GTest sources regardless
# of LLVM version.
cd "${BASE}"

wget https://github.com/google/googletest/archive/release-${GTEST_VERSION}.zip
unzip -o release-${GTEST_VERSION}.zip
rm release-${GTEST_VERSION}.zip
