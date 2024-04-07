#!/bin/bash
#!/bin/bash
set -e
set -u
DIR="$(cd "$(dirname "$0")" && pwd)"


coverage_setup() {
  local build_dir="$1"
  # Zero coverage for any file, e.g. previous tests
  lcov -q --directory "${build_dir}" --no-external --zerocounters
  # Create a baseline by capturing any file used for compilation, no execution yet
  lcov -q --rc lcov_branch_coverage=1 --directory "${build_dir}" --base-directory="${KLEE_SRC}" --no-external --capture --initial --output-file coverage_base.info
  lcov -q --rc lcov_branch_coverage=1 --remove coverage_base.info 'mytests/*' --output-file coverage_base.info

}

coverage_update() {
  tags="$2"
  codecov_suffix=(${tags// /})
  build_dir="$1"
  # Create report
  # (NOTE: "--rc lcov_branch_coverage=1" needs to be added in all calls, otherwise branch coverage gets dropped)
  lcov -q --rc lcov_branch_coverage=1 --directory "${build_dir}" --base-directory="${KLEE_SRC}" --no-external --capture --output-file coverage.info
  # Exclude uninteresting coverage goals (LLVM, googletest, and KLEE system and unit tests)
  lcov -q --rc lcov_branch_coverage=1 --remove coverage.info 'mytests/*' --output-file coverage.info
  # Combine baseline and measured coverage
  lcov -q --rc lcov_branch_coverage=1 -a coverage_base.info -a coverage.info -o coverage_all.info."${codecov_suffix}"
  # Debug info
  lcov -q --rc lcov_branch_coverage=1 --list coverage_all.info."${codecov_suffix}"
}

run_tests() {
  build_dir="$1"
  KLEE_SRC="$(cd "${DIR}"/../../ && pwd)"

  # TODO change to pinpoint specific directory
  cd "${build_dir}"

pwr
echo $PATH
echo $(which klee)

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

  ###############################################################################
  # Unit tests
  ###############################################################################
  # Prepare coverage information if COVERAGE is set
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_setup "${build_dir}"
  fi

pwd
echo $PATH
echo $(which klee)

make mytests
  
  # If metaSMT is the only solver, then rerun lit tests with non-default metaSMT backends
  if [ "X${SOLVERS}" == "XmetaSMT" ]; then
    base_path="$(python3 -m site --user-base)"
    export PATH="$PATH:${base_path}/bin"
    available_metasmt_backends="btor stp z3 yices2 cvc4"
    for backend in $available_metasmt_backends; do
      if [ "X${METASMT_DEFAULT}" != "X$backend" ]; then
        lit -v --param klee_opts=-metasmt-backend="$backend" --param kleaver_opts=-metasmt-backend="$backend" test/
      fi
    done
  fi
  
}

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

  run_tests "${directory}"
 }

main "$@"