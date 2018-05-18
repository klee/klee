# This file is meant to be included by shell scripts to provide common default
# values.
# It is used by: travis builds; docker builds and local builds
# Change defaults to setup different development environment
# Defines defaults used to build a default docker image outside of Travis
if [[ "${DOCKER_BUILD}x" == "1x" ]]; then
  # Default for docker: Do not keep build and source files for small images
  : "${KEEP_BUILD:=0}"
  : "${KEEP_SRC:=0}"
  echo "DOCKER_BUILD"
fi
if [[ "${KLEE_TRAVIS_BUILD}x" != "1x" ]]; then
: "${REPOSITORY:=klee}"
: "${LLVM_VERSION:=3.4}"
: "${DISABLE_ASSERTIONS:=0}"
: "${ENABLE_DEBUG:=1}"
: "${ENABLE_OPTIMIZED:=1}"
: "${SOLVERS:=STP}"
: "${STP_VERSION:=2.1.2}"
: "${Z3_VERSION:=4.4.1}"
: "${METASMT_VERSION:=v4.rc1}"
: "${METASMT_DEFAULT:=STP}"
: "${METASMT_BOOST_VERSION:=}"
: "${REQUIRES_RTTI:=0}"
: "${KLEE_UCLIBC:=klee_uclibc_v1.0.0}"
: "${USE_TCMALLOC:=1}"
: "${PACKAGED:=0}"
# undefined, address, memory
: "${SANITIZER_BUILD:=}"
# Defines if source and build artifacts should be preserved
: "${KEEP_BUILD:=1}"
: "${KEEP_SRC:=1}"
OS="$(uname)"
case $OS in
  'Linux')
    TRAVIS_OS_NAME='linux'
    ;;
  'Darwin')
    TRAVIS_OS_NAME='osx'
    ;;
  *) ;;
esac
fi

# General default values
: "${TCMALLOC_VERSION:=2.4}"
: "${GTEST_VERSION:=1.7.0}"

if [[ "${DIR}x" == "x" ]]; then
  echo "\${DIR} not set"
  exit 1
fi
source "${DIR}/common-functions"
