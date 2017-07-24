#!/bin/bash -x
set -ev

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  sudo apt-get install -y llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev

  sudo apt-get install -y llvm-${LLVM_VERSION}-tools clang-${LLVM_VERSION}
  sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
  sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  # NOTE: We should not easily generalize, since we need the corresponding support of bottled formulas
  if [ "${LLVM_VERSION}" == "3.4" ]; then
    brew install llvm34
  else
    echo "Error: Requested to install LLVM ${LLVM_VERSION} on macOS, which is not supported"
    exit 1
  fi
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi
