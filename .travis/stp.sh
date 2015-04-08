#!/bin/bash -x

# Make sure we exit if there is a failure
set -e

STP_LOG="$(pwd)/stp-build.log"

if [ "${STP_VERSION}" == "UPSTREAM" ]; then
    # Build STP
    git clone --depth 1 git://github.com/stp/stp.git src
    mkdir build
    cd build
    # Disabling building of shared libs is a workaround
    cmake -DBUILD_SHARED_LIBS:BOOL=OFF -DENABLE_PYTHON_INTERFACE:BOOL=OFF ../src
    # Don't try to build stp executable, there's an issue with using gcc4.8 with boost libraries built with gcc4.6

    set +e # Do not exit if build fails because we need to display the log
    make libstp CopyPublicHeaders >> "${STP_LOG}" 2>&1

elif [ "${STP_VERSION}" == "r940" ]; then
    # Building the old "r940" version that for some reason we love so much!
    git clone git://github.com/stp/stp.git src_build
    mkdir build # This is actually the install directory
    cd src_build/
    git checkout bc78d1f9f06fc095bd1ddad90eacdd1f05f64dae # r940

    # Fixes for GCC
    # We don't try to fix clang compilation because there too many things that need
    # fixing and it isn't really r940 anymore if we start doing that
    git config --global user.name "travis"
    git config --global  user.email "travis@travis.123"
    git cherry-pick ece1a55fb367bd905078baca38476e35b4df06c3
    patch -p1 -i ${KLEE_SRC}/.travis/stp-r940-smtlib2.y.patch

    # Oh man this project is so broken. The binary build directory is missing
    mkdir -p bin

    export CC=gcc
    export CXX=g++
    ./scripts/configure --with-prefix=${BUILD_DIR}/stp/build --with-cryptominisat2
    echo "WARNING FORCING GCC TO BE USED TO COMPILE STP"

    set +e # Do not exit if build fails because we need to display the log
    make OPTIMIZE=-O2 CFLAGS_M32=    install >> "${STP_LOG}" 2>&1
else
    echo "Unsupported STP_VERSION"
    exit 1
fi

# Only show build output if something went wrong to keep log output short
if [ $? -ne 0 ]; then
    echo "Build error"
    cat "${STP_LOG}"
fi
