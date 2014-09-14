#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

# Calculate LLVM branch name to retrieve missing files from
SVN_BRANCH="release_$( echo ${LLVM_VERSION} | sed 's/\.//g')"

###############################################################################
# Select the compiler to use to generate LLVM bitcode
###############################################################################
if [ "${LLVM_VERSION}" != "2.9" ]; then
    KLEE_CC=/usr/bin/clang-${LLVM_VERSION}
    KLEE_CXX=/usr/bin/clang++-${LLVM_VERSION}
else
    # Just use pre-built llvm-gcc downloaded earlier
    KLEE_CC=${BUILD_DIR}/llvm-gcc/bin/llvm-gcc
    KLEE_CXX=${BUILD_DIR}/llvm-gcc/bin/llvm-g++
    export C_INCLUDE_PATH=/usr/include/x86_64-linux-gnu
    export CPLUS_INCLUDE_PATH=/usr/include/x86_64-linux-gnu

    # Add symlinks to fix llvm-2.9-dev package so KLEE can configure properly
    # Because of the way KLEE's configure script works this must be a relative
    # symlink, **not** absolute!
    sudo sh -c 'cd /usr/lib/llvm-2.9/build/ && ln -s ../ Release'
    sudo sh -c 'cd /usr/lib/llvm-2.9/build/ && ln -s ../include include'
fi

###############################################################################
# klee-uclibc
###############################################################################
if [ "${KLEE_UCLIBC}" -eq 1 ]; then
    git clone --depth 1 git://github.com/klee/klee-uclibc.git
    cd klee-uclibc
    ./configure --make-llvm-lib --with-cc "${KLEE_CC}" --with-llvm-config /usr/bin/llvm-config-${LLVM_VERSION}
    make
    KLEE_UCLIBC_CONFIGURE_OPTION="--with-uclibc=$(pwd) --with-posix-runtime"
    cd ../
else
    KLEE_UCLIBC_CONFIGURE_OPTION=""
fi

###############################################################################
# KLEE
###############################################################################
mkdir klee
cd klee

# Build KLEE
# Note: ENABLE_SHARED=0 is required because llvm-2.9 is incorectly packaged
# and is missing the shared library that was supposed to be built that the build
# system will try to use by default.
${KLEE_SRC}/configure --with-llvmsrc=/usr/lib/llvm-${LLVM_VERSION}/build \
            --with-llvmobj=/usr/lib/llvm-${LLVM_VERSION}/build \
            --with-llvmcc=${KLEE_CC} \
            --with-llvmcxx=${KLEE_CXX} \
            --with-stp="${BUILD_DIR}/stp/build" \
            ${KLEE_UCLIBC_CONFIGURE_OPTION} \
            && make DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
                    ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
                    ENABLE_SHARED=0

###############################################################################
# Testing
###############################################################################
set +e # We want to let all the tests run before we exit

###############################################################################
# Unit tests
###############################################################################
# The unittests makefile doesn't seem to have been packaged so get it from SVN
sudo mkdir -p /usr/lib/llvm-${LLVM_VERSION}/build/unittests/
svn export  http://llvm.org/svn/llvm-project/llvm/branches/${SVN_BRANCH}/unittests/Makefile.unittest \
    ../Makefile.unittest
sudo mv ../Makefile.unittest /usr/lib/llvm-${LLVM_VERSION}/build/unittests/

make unittests \
    DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
    ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
    ENABLE_SHARED=0
RETURN="$?"

###############################################################################
# lit tests
###############################################################################
# Note can't use ``make check`` because llvm-lit is not available
cd test
# The build system needs to generate this file before we can run lit
make lit.site.cfg \
    DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
    ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
    ENABLE_SHARED=0

set +e # We want to let all the tests run before we exit
lit -v .
RETURN="${RETURN}$?"

###############################################################################
# Result
###############################################################################
if [ "${RETURN}" != "00" ]; then
    echo "Running tests failed"
    exit 1
fi
