#!/bin/bash
# Build a KLEE docker image. If needed, build all dependent layers as well.
set -e

# We are going to build docker containers
export DOCKER_BUILD=1

# All scripts are located relative to this one
DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"
KLEEDIR="${DIR}/../.."

###############################################################################
# Setup Docker variables needed to build layers layers
###############################################################################

add_arg_variable() {
  name=$1
  name_var=${!name}
  if [[ (-z "$name_var") || ($name_var == "") ]] ; then
    val=""
  else
    val="--build-arg ${name}=${name_var} "
  fi
  echo "$val"
}

DOCKER_OPTS=()
for v in \
  LLVM_VERSION \
  LLVM_VERSION_SHORT \
  ENABLE_OPTIMIZED \
  DISABLE_ASSERTIONS \
  ENABLE_DEBUG \
  REQUIRES_RTTI \
  LLVM_SUFFIX \
  REPOSITORY \
  SANITIZER_BUILD \
  SANITIZER_SUFFIX \
  PACKAGED \
  KLEE_TRAVIS_BUILD \
  TRAVIS_OS_NAME \
  METASMT_VERSION \
  METASMT_DEFAULT \
  METASMT_BOOST_VERSION \
  SOLVER_SUFFIX \
  Z3_VERSION \
  STP_VERSION \
  KLEE_UCLIBC \
  DEPS_SUFFIX \
  USE_TCMALLOC \
  COVERAGE \
  KEEP_SRC \
  KEEP_BUILD \
  TCMALLOC_VERSION \
  GTEST_VERSION \
  SOLVERS \
  ; do
  DOCKER_OPTS+=($(add_arg_variable $v))
done

echo "DOCKER_OPTS: ${DOCKER_OPTS[@]}"
docker_build_and_push() {
  merge=$1
  image=$2
  script=$3
  set +e
  # Get or build layer
  echo "$image"
  if ! docker pull $image ;  then
    set -e
    # Update if needed
    echo "BUILD IMAGE"
    merge_arg=""
  #  if [[ "${merge}x" == "1x" ]]; then
  #    #merge_arg="--squash"
  #  fi
    docker build ${merge_arg} -f "${DIR}/$script" ${@:4} -t "$image" "${KLEEDIR}"
    set +e
    docker push "$image"
  fi
  return 0
}

###############################################################################
# Build different layers
###############################################################################

# Check if the build of dependencies only has been requested
if [[ -z ${DOCKER_BUILD_DEPS_ONLY} || "${DOCKER_BUILD_DEPS_ONLY}x" == "1x" ]]; then
  echo "Build dependencies"
  docker_build_and_push 1 ${REPOSITORY}/base Dockerfile_base "${DOCKER_OPTS[@]}"
  docker_build_and_push 1 ${REPOSITORY}/llvm_built:${LLVM_VERSION_SHORT}${LLVM_SUFFIX} Dockerfile_llvm_build "${DOCKER_OPTS[@]}"
  docker_build_and_push 1 ${REPOSITORY}/solver:${LLVM_VERSION_SHORT}${LLVM_SUFFIX}${SOLVER_SUFFIX} Dockerfile_solver_build "${DOCKER_OPTS[@]}"
  docker_build_and_push 1 ${REPOSITORY}/klee_deps:${LLVM_VERSION_SHORT}${LLVM_SUFFIX}${SOLVER_SUFFIX}${DEPS_SUFFIX} Dockerfile_klee_deps "${DOCKER_OPTS[@]}"
fi

if [[ -z ${DOCKER_BUILD_DEPS_ONLY} || "${DOCKER_BUILD_DEPS_ONLY}x" != "1x" ]]; then
  docker build -f "${DIR}/Dockerfile_klee" "${DOCKER_OPTS[@]}" -t ${REPOSITORY}/klee:${LLVM_VERSION_SHORT}${LLVM_SUFFIX}${SOLVER_SUFFIX}${DEPS_SUFFIX} "${KLEEDIR}"
fi
