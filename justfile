default: rebuild

uclibc:
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p 3rd
    if [ ! -d "3rd/klee-uclibc" ]; then
        git clone git@github.com:klee/klee-uclibc.git 3rd/klee-uclibc
    fi
    pushd 3rd/klee-uclibc
        ./configure --make-llvm-lib
        make -j$(nproc)
    popd

libcxx:
    LLVM_VERSION=13 BASE=$PWD/3rd/libcxx ENABLE_OPTIMIZED=1 DISABLE_ASSERTIONS=1 ENABLE_DEBUG=0 REQUIRES_RTTI=1 scripts/build/build.sh libcxx

gtest version="1.11.0":
    #!/usr/bin/env bash
    set -euxo pipefail
    mkdir -p 3rd
    if [ ! -d "3rd/gtest" ]; then
        curl -OL https://github.com/google/googletest/archive/release-{{version}}.zip
        unzip release-{{version}}.zip && rm release-{{version}}.zip
        mv googletest-release-{{version}} 3rd/gtest
    fi

klee: uclibc gtest
    cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_SOLVER_Z3=ON -DENABLE_SOLVER_STP=ON -DENABLE_POSIX_RUNTIME=ON -DKLEE_UCLIBC_PATH=$PWD/3rd/klee-uclibc -DENABLE_UNIT_TESTS=ON -DGTEST_SRC_DIR=$PWD/3rd/gtest -DLLVMCC=$(which clang) -DLLVMCXX=$(which clang++)
    cmake --build build

rebuild:
    cmake --build build

clean:
    rm -rf /tmp/klee-out/*
