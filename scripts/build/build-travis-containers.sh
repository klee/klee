#!/bin/bash
# Build all the docker containers utilized, that would be
# also build by TravisCI.
#
# Every line starting with " - LLVM_VERSION=" is a single setup
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"

# Build dependencies only
export DOCKER_BUILD_DEPS_ONLY=1
export KLEE_TRAVIS_BUILD="1"
export REPOSITORY="klee"
export TRAVIS_OS_NAME="linux"

while read -r line
do
    name=$(echo $line| grep "\- LLVM" | xargs -L 1 | cut -d "-" -f 2)
    if [[ "x$name" == "x" ]]; then
      continue
    fi
    /bin/bash -c "$name ./scripts/build/build-docker.sh"
done < "${DIR}/../../.travis.yml"
