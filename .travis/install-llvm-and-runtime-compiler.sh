#!/bin/bash -x
set -ev

sudo apt-get install llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev

if [ "${LLVM_VERSION}" != "2.9" ]; then
    sudo apt-get install llvm-${LLVM_VERSION}-tools clang-${LLVM_VERSION}
else
    # Get llvm-gcc. We don't bother installing it
    wget http://llvm.org/releases/2.9/llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
    tar -xjf llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
    mv llvm-gcc4.2-2.9-x86_64-linux llvm-gcc
fi
