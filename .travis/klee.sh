#!/bin/bash -x
# Make sure we exit if there is a failure
set -e
: ${SOLVERS?"Solvers must be specified"}

coverage_setup()
{
  # Zero coverage for any file, e.g. previous tests
  lcov --directory . --no-external --zerocounters
  # Create a baseline by capturing any file used for compilation, no execution yet
  lcov --rc lcov_branch_coverage=1 --directory . --base-directory=${KLEE_SRC} --no-external --capture --initial --output-file coverage_base.info
  lcov --rc lcov_branch_coverage=1 --remove coverage_base.info 'test/*' --output-file coverage_base.info
  lcov --rc lcov_branch_coverage=1 --remove coverage_base.info 'unittests/*' --output-file coverage_base.info
}

coverageup()
{
  # Create report
  # (NOTE: "--rc lcov_branch_coverage=1" needs to be added in all calls, otherwise branch coverage gets dropped)
  lcov --rc lcov_branch_coverage=1 --directory . --base-directory=${KLEE_SRC} --no-external --capture --output-file coverage.info
  # Exclude uninteresting coverage goals (LLVM, googletest, and KLEE system and unit tests)
  lcov --rc lcov_branch_coverage=1 --remove coverage.info 'test/*' --output-file coverage.info
  lcov --rc lcov_branch_coverage=1 --remove coverage.info 'unittests/*' --output-file coverage.info
  # Combine baseline and measured coverage
  lcov --rc lcov_branch_coverage=1 -a coverage_base.info -a coverage.info -o coverage_all.info
  # Debug info
  lcov --rc lcov_branch_coverage=1 --list coverage_all.info
  # Uploading report to CodeCov
  bash <(curl -s https://codecov.io/bash) -R ${KLEE_SRC} -X gcov -y ${KLEE_SRC}/.codecov.yml -f coverage_all.info -F $1
}

###############################################################################
# Select the compiler to use to generate LLVM bitcode
###############################################################################
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  KLEE_CC=/usr/bin/clang-${LLVM_VERSION}
  KLEE_CXX=/usr/bin/clang++-${LLVM_VERSION}
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  KLEE_CC=/usr/local/bin/clang-${LLVM_VERSION}
  KLEE_CXX=/usr/local/bin/clang++-${LLVM_VERSION}
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi

###############################################################################
# klee-uclibc
###############################################################################
if [ "${KLEE_UCLIBC}" != "0" ]; then
    git clone --depth 1 -b ${KLEE_UCLIBC} git://github.com/klee/klee-uclibc.git
    cd klee-uclibc
    ./configure --make-llvm-lib --with-cc "${KLEE_CC}" --with-llvm-config /usr/bin/llvm-config-${LLVM_VERSION}
    make
    KLEE_UCLIBC_CONFIGURE_OPTION="-DENABLE_KLEE_UCLIBC=TRUE -DKLEE_UCLIBC_PATH=$(pwd) -DENABLE_POSIX_RUNTIME=TRUE"
    cd ../
else
    KLEE_UCLIBC_CONFIGURE_OPTION="-DENABLE_KLEE_UCLIBC=FALSE -DENABLE_POSIX_RUNTIME=FALSE"
fi

COVERAGE_FLAGS=""
if [ ${COVERAGE} -eq 1 ]; then
    COVERAGE_FLAGS='-fprofile-arcs -ftest-coverage'
fi


###############################################################################
# Handle setting up solver configure flags for KLEE
###############################################################################
SOLVER_LIST=$(echo "${SOLVERS}" | sed 's/:/ /')

# Set CMake configure options
KLEE_Z3_CONFIGURE_OPTION="-DENABLE_SOLVER_Z3=OFF"
KLEE_STP_CONFIGURE_OPTION="-DENABLE_SOLVER_STP=OFF"
KLEE_METASMT_CONFIGURE_OPTION="-DENABLE_SOLVER_METASMT=OFF"
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

###############################################################################
# Handle additional configure flags
###############################################################################
TCMALLOC_OPTION=$([ "${USE_TCMALLOC:-0}" == 1 ] && echo "-DENABLE_TCMALLOC=TRUE" || echo "-DENABLE_TCMALLOC=FALSE")

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
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  LLVM_CONFIG="/usr/local/bin/llvm-config-${LLVM_VERSION}"
  LLVM_BUILD_DIR="$(${LLVM_CONFIG} --src-root)"
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi

###############################################################################
# KLEE
###############################################################################
mkdir klee
cd klee

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

# TODO: We should support Ninja too
# Configure KLEE
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

###############################################################################
# Unit tests
###############################################################################
# Prepare coverage information if COVERAGE is set
if [ ${COVERAGE} -eq 1 ]; then
  coverage_setup
fi
make unittests

# Generate and upload coverage if COVERAGE is set
if [ ${COVERAGE} -eq 1 ]; then
  coverageup "unittests"
fi

###############################################################################
# lit tests
###############################################################################
if [ ${COVERAGE} -eq 1 ]; then
  coverage_setup
fi
make systemtests

# If metaSMT is the only solver, then rerun lit tests with non-default metaSMT backends
if [ "X${SOLVERS}" == "XmetaSMT" ]; then
  metasmt_default=`echo "${METASMT_DEFAULT,,}"` # klee_opts and kleaver_opts use lowercase
  available_metasmt_backends="btor stp z3 yices2 cvc4"
  for backend in $available_metasmt_backends; do
    if [ "X${metasmt_default}" != "X$backend" ]; then
      if [ "$backend" == "cvc4" ]; then
        for num in {1..5}; do sleep 120; echo 'Keep Travis alive'; done &
      fi
      lit -v --param klee_opts=-metasmt-backend=$backend --param kleaver_opts=-metasmt-backend=$backend test/
    fi
  done
fi

# Generate and upload coverage if COVERAGE is set
if [ ${COVERAGE} -eq 1 ]; then
  coverageup "systemtests"
fi
