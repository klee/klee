#!/bin/bash

set -e

# We are going to build docker containers
export DOCKER_BUILD=1

# All scripts are located relative to this one
DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

if [[ "a$COVERAGE" != "a1" ]]; then
  exit 0
fi

function upload_coverage() {
  tags=$1
  codecov_suffix=(${tags// /})
  ci_env=$(bash <(curl -s https://codecov.io/env))
  docker run ${ci_env} -ti ${REPOSITORY}/klee:${LLVM_VERSION_SHORT}${LLVM_SUFFIX}${SOLVER_SUFFIX}${DEPS_SUFFIX} /bin/bash -c "cd /home/klee/klee_src && bash <(curl -s https://codecov.io/bash) -X gcov -y /home/klee/klee_src/.codecov.yml -f /home/klee/klee_build/coverage_all.info.${codecov_suffix} -F $tags"
}

upload_coverage systemtests
upload_coverage unittests
