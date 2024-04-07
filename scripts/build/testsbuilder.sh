#!/bin/bash

set -e
set -u

function run_docker() {
 docker_arguments=(docker run -u root --cap-add SYS_PTRACE -t -e SOLVERS -e METASMT_DEFAULT)
 script_arguments=("--debug" '"/tmp/klee_build"*')
 if [[ "${COVERAGE}" -eq 1 ]]; then
   script_arguments+=("--coverage")
 fi
 
 if [[ "${UPLOAD_COVERAGE}" -eq 1 ]]; then
   docker_arguments+=($(bash <(curl -s https://codecov.io/env)))
   script_arguments+=("--upload-coverage")
 fi

 # Run the image that was build last with extended capabilities to allow tracing tests
 "${docker_arguments[@]}" "$(docker images -q | head -n 1)" /bin/bash -i -c "ulimit -s 16384; source /home/klee/.bashrc; export; /tmp/klee_src/scripts/build/run-tests.sh ${script_arguments[*]}"
}

main() {
  local NAME
  NAME=$(basename "${0}")
  local RUN_DOCKER=0
  local COVERAGE=0
  local UPLOAD_COVERAGE=0
  local directory=""

  for i in "$@" ;do
  case $i in
    --debug)
      set -x
    ;;
    --run-docker)
    RUN_DOCKER=1
    ;;
    --coverage)
    COVERAGE=1
    ;;
    --upload-coverage)
    UPLOAD_COVERAGE=1
    ;;
    -*)
      echo "${NAME}: unknown argument: $i"
      exit 1
    ;;
    *)
    directory="${i}"
    ;;
  esac
  done


  if [[ "${RUN_DOCKER}" -eq 1 ]]; then
    run_docker "${COVERAGE}"
    return 0
  fi

cd ~/
cd /tmp/klee_src/mytests/deadlockmark

clang++ -emit-llvm -O0 -c -g deadlock.cpp
klee deadlock.bc

cd ~/
cd /tmp/klee_src/mytests/racecondmark

clang++ -emit-llvm -O0 -c -g racecond.cpp
klee racecond.bc

cd ~/
cd /tmp/klee_src/mytests/undefined_behaviour_mark

clang++ -emit-llvm -O0 -c -g undefined_behaviour.cpp
klee undefined_behaviour.bc
}

main "$@"