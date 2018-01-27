#!/bin/bash -x
set -ev

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  sudo apt-get install -y llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev

  sudo apt-get install -y llvm-${LLVM_VERSION}-tools clang-${LLVM_VERSION}
  sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
  sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  # We use our own local cache if possible
  set +e
  brew install $HOME/Library/Caches/Homebrew/llvm\@${LLVM_VERSION}*.bottle.tar.gz
  RES=$?
  set -ev
  if [ ${RES} -ne 0 ]; then
    # This might build the llvm package use a time out to avoid dumping log information
    brew install -v --build-bottle "llvm@${LLVM_VERSION}"
    # Now bottle the brew
    brew bottle llvm\@${LLVM_VERSION}
    # Not make sure it's cached
    cp llvm\@${LLVM_VERSION}*.bottle.tar.gz $HOME/Library/Caches/Homebrew/
  fi
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi
