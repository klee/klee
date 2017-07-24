#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

if [ "X${USE_CMAKE}" == "X1" ]; then
    # The New CMake build system just needs the GTest sources regardless
    # of LLVM version.
    wget https://github.com/google/googletest/archive/release-1.7.0.zip
    unzip release-1.7.0.zip
    exit 0
fi

# Using LLVM3.4 all we need is vanilla GoogleTest :)
wget https://github.com/google/googletest/archive/release-1.7.0.zip
unzip release-1.7.0.zip
cd googletest-release-1.7.0/
cmake .
make
# Normally I wouldn't do something like this but hey we're running on a temporary virtual machine, so who cares?
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
   sudo cp lib* /usr/lib/
   sudo cp -r include/gtest /usr/include
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
   sudo cp lib* /usr/local/lib/
   sudo cp -r include/gtest /usr/local/include
else
   echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
   exit 1
fi
