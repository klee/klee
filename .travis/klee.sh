#!/bin/bash -x
# Make sure we exit if there is a failure
set -e
: ${SOLVERS?"Solvers must be specified"}

# Calculate LLVM branch name to retrieve missing files from
SVN_BRANCH="release_$( echo ${LLVM_VERSION} | sed 's/\.//g')"

###############################################################################
# Select the compiler to use to generate LLVM bitcode
###############################################################################
if [ "${LLVM_VERSION}" != "2.9" ]; then
  if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    KLEE_CC=/usr/bin/clang-${LLVM_VERSION}
    KLEE_CXX=/usr/bin/clang++-${LLVM_VERSION}
  else # OSX
    KLEE_CC=/usr/local/bin/clang-${LLVM_VERSION}
    KLEE_CXX=/usr/local/bin/clang++-${LLVM_VERSION}
  fi
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
if [ "${KLEE_UCLIBC}" != "0" ]; then
    git clone --depth 1 -b ${KLEE_UCLIBC} git://github.com/klee/klee-uclibc.git
    cd klee-uclibc
    ./configure --make-llvm-lib --with-cc "${KLEE_CC}" --with-llvm-config /usr/bin/llvm-config-${LLVM_VERSION}
    make
    if [ "X${USE_CMAKE}" == "X1" ]; then
      KLEE_UCLIBC_CONFIGURE_OPTION="-DENABLE_KLEE_UCLIBC=TRUE -DKLEE_UCLIBC_PATH=$(pwd) -DENABLE_POSIX_RUNTIME=TRUE"
    else
      KLEE_UCLIBC_CONFIGURE_OPTION="--with-uclibc=$(pwd) --enable-posix-runtime"
    fi
    cd ../
else
    if [ "X${USE_CMAKE}" == "X1" ]; then
      KLEE_UCLIBC_CONFIGURE_OPTION="-DENABLE_KLEE_UCLIBC=FALSE -DENABLE_POSIX_RUNTIME=FALSE"
    else
      KLEE_UCLIBC_CONFIGURE_OPTION=""
    fi
fi

COVERAGE_FLAGS=""
if [ ${COVERAGE} -eq 1 ]; then
    COVERAGE_FLAGS='-fprofile-arcs -ftest-coverage'
fi


###############################################################################
# Handle setting up solver configure flags for KLEE
###############################################################################
KLEE_Z3_CONFIGURE_OPTION=""
KLEE_STP_CONFIGURE_OPTION=""
KLEE_METASMT_CONFIGURE_OPTION=""
SOLVER_LIST=$(echo "${SOLVERS}" | sed 's/:/ /')

if [ "X${USE_CMAKE}" == "X1" ]; then
  # Set CMake configure options
  for solver in ${SOLVER_LIST}; do
    echo "Setting CMake configuration option for ${solver}"
    case ${solver} in
    STP)
      KLEE_STP_CONFIGURE_OPTION="-DENABLE_SOLVER_STP=TRUE -DSTP_DIR=${BUILD_DIR}/stp/build"
      ;;
    Z3)
      echo "Z3"
      KLEE_Z3_CONFIGURE_OPTION="-DENABLE_SOLVER_Z3=TRUE"
      ;;
    metaSMT)
      echo "metaSMT"
      if [ "X${METASMT_DEFAULT}" == "X" ]; then
        METASMT_DEFAULT=STP
      fi
      KLEE_METASMT_CONFIGURE_OPTION="-DENABLE_SOLVER_METASMT=TRUE -DmetaSMT_DIR=${BUILD_DIR}/metaSMT/build -DMETASMT_DEFAULT_BACKEND=${METASMT_DEFAULT}"
      ;;
    *)
      echo "Unknown solver ${solver}"
      exit 1
    esac
  done
  TCMALLOC_OPTION=$([ "${USE_TCMALLOC:-0}" == 1 ] && echo "-DENABLE_TCMALLOC=TRUE" || echo "-DENABLE_TCMALLOC=FALSE")
else
  for solver in ${SOLVER_LIST}; do
    echo "Setting configuration option for ${solver}"
    case ${solver} in
    STP)
      KLEE_STP_CONFIGURE_OPTION="--with-stp=${BUILD_DIR}/stp/build"
      ;;
    Z3)
      echo "Z3"
      KLEE_Z3_CONFIGURE_OPTION="--with-z3=/usr"
      ;;
    metaSMT)
      echo "metaSMT"
      KLEE_METASMT_CONFIGURE_OPTION="--with-metasmt=${BUILD_DIR}/metaSMT/build/root --with-metasmt-default-backend=${METASMT_DEFAULT}"
      ;;
    *)
      echo "Unknown solver ${solver}"
      exit 1
    esac
  done

  TCMALLOC_OPTION=$([ "${USE_TCMALLOC:-0}" == 1 ] && echo "--with-tcmalloc" || echo "--without-tcmalloc")
fi

###############################################################################
# Handle Sanitizer flags
###############################################################################
source ${KLEE_SRC}/.travis/sanitizer_flags.sh

###############################################################################
# Handling LLVM configuration
###############################################################################
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  LLVM_CONFIG="/usr/lib/llvm-${LLVM_VERSION}/bin/llvm-config"
  LLVM_BUILD_DIR="/usr/lib/llvm-${LLVM_VERSION}/build"
else # OSX
  LLVM_CONFIG="/usr/local/bin/llvm-config-${LLVM_VERSION}"
  LLVM_BUILD_DIR="$(${LLVM_CONFIG} --src-root)"
fi

###############################################################################
# KLEE
###############################################################################
mkdir klee
cd klee

if [ "X${USE_CMAKE}" == "X1" ]; then
  GTEST_SRC_DIR="${BUILD_DIR}/test-utils/googletest-release-1.7.0/"
  if [ "X${DISABLE_ASSERTIONS}" == "X1" ]; then
    KLEE_ASSERTS_OPTION="-DENABLE_KLEE_ASSERTS=FALSE"
  else
    KLEE_ASSERTS_OPTION="-DENABLE_KLEE_ASSERTS=TRUE"
  fi

  if [ "X${ENABLE_OPTIMIZED}" == "X1" ]; then
    CMAKE_BUILD_TYPE="RelWithDebInfo"
  else
    CMAKE_BUILD_TYPE="Debug"
  fi
  # Compute CMake build type
  CXXFLAGS="${COVERAGE_FLAGS} ${SANITIZER_CXX_FLAGS}" \
  CFLAGS="${COVERAGE_FLAGS} ${SANITIZER_C_FLAGS}" \
  cmake \
    -DLLVM_CONFIG_BINARY="${LLVM_CONFIG}" \
    -DLLVMCC="${KLEE_CC}" \
    -DLLVMCXX="${KLEE_CXX}" \
    ${KLEE_STP_CONFIGURE_OPTION} \
    ${KLEE_Z3_CONFIGURE_OPTION} \
    ${KLEE_METASMT_CONFIGURE_OPTION} \
    ${KLEE_UCLIBC_CONFIGURE_OPTION} \
    ${TCMALLOC_OPTION} \
    -DGTEST_SRC_DIR=${GTEST_SRC_DIR} \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    ${KLEE_ASSERTS_OPTION} \
    -DENABLE_UNIT_TESTS=TRUE \
    -DENABLE_SYSTEM_TESTS=TRUE \
    -DLIT_ARGS="-v" \
    ${KLEE_SRC}
  make
else
  # Build KLEE
  # Note: ENABLE_SHARED=0 is required because llvm-2.9 is incorectly packaged
  # and is missing the shared library that was supposed to be built that the build
  # system will try to use by default.
  ${KLEE_SRC}/configure --with-llvmsrc=${LLVM_BUILD_DIR} \
              --with-llvmobj=${LLVM_BUILD_DIR} \
              --with-llvmcc=${KLEE_CC} \
              --with-llvmcxx=${KLEE_CXX} \
              ${KLEE_STP_CONFIGURE_OPTION} \
              ${KLEE_Z3_CONFIGURE_OPTION} \
              ${KLEE_METASMT_CONFIGURE_OPTION} \
              ${KLEE_UCLIBC_CONFIGURE_OPTION} \
              ${TCMALLOC_OPTION} \
              CXXFLAGS="${COVERAGE_FLAGS} ${SANITIZER_CXX_FLAGS}" \
              CFLAGS="${COVERAGE_FLAGS} ${SANITIZER_C_FLAGS}" \
              LDFLAGS="${SANITIZER_LD_FLAGS}"
  make  DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
        ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
        ENABLE_SHARED=0
fi

###############################################################################
# Unit tests
###############################################################################
if [ "X${USE_CMAKE}" == "X1" ]; then
  make unittests
else
  # The unittests makefile doesn't seem to have been packaged so get it from SVN
  sudo mkdir -p /usr/lib/llvm-${LLVM_VERSION}/build/unittests/
  svn export  http://llvm.org/svn/llvm-project/llvm/branches/${SVN_BRANCH}/unittests/Makefile.unittest \
      ../Makefile.unittest
  sudo mv ../Makefile.unittest /usr/lib/llvm-${LLVM_VERSION}/build/unittests/

  make unittests \
      DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
      ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
      ENABLE_SHARED=0
fi

###############################################################################
# lit tests
###############################################################################
if [ "X${USE_CMAKE}" == "X1" ]; then
  make systemtests
else
  # Note can't use ``make check`` because llvm-lit is not available
  cd test
  # The build system needs to generate this file before we can run lit
  make lit.site.cfg \
      DISABLE_ASSERTIONS=${DISABLE_ASSERTIONS} \
      ENABLE_OPTIMIZED=${ENABLE_OPTIMIZED} \
      ENABLE_SHARED=0
  cd ../
  lit -v test/
fi

# If metaSMT is the only solver, then rerun lit tests with non-default metaSMT backends
if [ "X${SOLVERS}" == "XmetaSMT" ]; then
  metasmt_default=`echo "${METASMT_DEFAULT,,}"` # klee_opts and kleaver_opts use lowercase
  available_metasmt_backends="btor stp z3"
  for backend in $available_metasmt_backends; do
    if [ "X${metasmt_default}" != "X$backend" ]; then
      lit -v --param klee_opts=-metasmt-backend=$backend --param kleaver_opts=-metasmt-backend=$backend test/
    fi
  done
fi

#generate and upload coverage if COVERAGE is set
if [ ${COVERAGE} -eq 1 ]; then

#get zcov that works with gcov v4.8
    git clone https://github.com/ddunbar/zcov.git
    cd zcov
    sudo python setup.py install
    sudo mkdir /usr/local/lib/python2.7/dist-packages/zcov-0.3.0.dev0-py2.7.egg/zcov/js
    cd zcov

#these files are not where zcov expects them to be after install so we move them
    sudo cp js/sorttable.js /usr/local/lib/python2.7/dist-packages/zcov-0.3.0.dev0-py2.7.egg/zcov/js/sorttable.js
    sudo cp js/sourceview.js /usr/local/lib/python2.7/dist-packages/zcov-0.3.0.dev0-py2.7.egg/zcov/js/sourceview.js
    sudo cp style.css /usr/local/lib/python2.7/dist-packages/zcov-0.3.0.dev0-py2.7.egg/zcov/style.css

#install zcov dependency
    sudo apt-get install -y enscript

#update gcov from v4.6 to v4.8. This is becauase gcda files are made for v4.8 and cause
#a segmentation fault in v4.6
    sudo apt-get install -y ggcov
    sudo rm /usr/bin/gcov
    sudo ln -s /usr/bin/gcov-4.8 /usr/bin/gcov

#scan and generate coverage
    zcov scan output.zcov ${BUILD_DIR}
    zcov genhtml output.zcov coverage/
#upload the coverage data, currently to a random ftp server
    tar -zcvf coverage.tar.gz coverage/
    curl --form "file=@coverage.tar.gz" -u ${USER}:${PASSWORD} ${COVERAGE_SERVER}
fi
