#!/bin/bash -x
set -ev

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  sudo apt-get install -y llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev

  if [ "${LLVM_VERSION}" != "2.9" ]; then
    sudo apt-get install -y llvm-${LLVM_VERSION}-tools clang-${LLVM_VERSION}
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 20
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 20
  else
    # Get llvm-gcc. We don't bother installing it
    wget http://llvm.org/releases/2.9/llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
    tar -xjf llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
    mv llvm-gcc4.2-2.9-x86_64-linux llvm-gcc

    # Hack to make llvm-gcc capable of building a native executable
    OLD_DIR=$(pwd)
    cd llvm-gcc/lib/gcc/x86_64-unknown-linux-gnu/4.2.1
    ln -s /usr/lib/x86_64-linux-gnu/crt1.o
    ln -s /usr/lib/x86_64-linux-gnu/crti.o
    ln -s /usr/lib/x86_64-linux-gnu/crtn.o
    cd "$OLD_DIR"

    # Check it can compile hello world
    echo -e "#include <stdio.h> \n int main(int argc, char** argv) { printf(\"Hello World\"); return 0;}" > test.c
    export C_INCLUDE_PATH=/usr/include/x86_64-linux-gnu
    export CPLUS_INCLUDE_PATH=/usr/include/x86_64-linux-gnu
    llvm-gcc/bin/llvm-gcc test.c -o hello_world
    ./hello_world
  fi
else # OSX
  # NOTE: We should not easily generalize, since we need the corresponding support of bottled formulas
  if [ "${LLVM_VERSION}" == "3.4" ]; then
    brew install llvm34
  else
    echo "Error: Requested to install LLVM ${LLVM_VERSION} on macOS, which is not supported"
    exit 1
  fi
fi
