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
  lcov -q --rc lcov_branch_coverage=1 --remove coverage_base.info 'test/*' --output-file coverage_base.info
  lcov -q --rc lcov_branch_coverage=1 --remove coverage_base.info 'unittests/*' --output-file coverage_base.info
}

coverage_update() {
  tags="$2"
  codecov_suffix=(${tags// /})
  build_dir="$1"
  # Create report
  # (NOTE: "--rc lcov_branch_coverage=1" needs to be added in all calls, otherwise branch coverage gets dropped)
  lcov -q --rc lcov_branch_coverage=1 --directory "${build_dir}" --base-directory="${KLEE_SRC}" --no-external --capture --output-file coverage.info
  # Exclude uninteresting coverage goals (LLVM, googletest, and KLEE system and unit tests)
  lcov -q --rc lcov_branch_coverage=1 --remove coverage.info 'test/*' --output-file coverage.info
  lcov -q --rc lcov_branch_coverage=1 --remove coverage.info 'unittests/*' --output-file coverage.info
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

  ###############################################################################
  # Unit tests
  ###############################################################################
  # Prepare coverage information if COVERAGE is set
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_setup "${build_dir}"
  fi
  make unittests
  
  # Generate and upload coverage if COVERAGE is set
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_update "${build_dir}" "unittests"
  fi

  ###############################################################################
  # lit tests
  ###############################################################################
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_setup "${build_dir}"
  fi

  if [[ -n "${SANITIZER_BUILD+x}" ]]; then
    for num in {1..10}; do sleep 120; echo 'Keep Travis alive'; done &
  fi

  make systemtests || return 1
  
  # If metaSMT is the only solver, then rerun lit tests with non-default metaSMT backends
  if [ "X${SOLVERS}" == "XmetaSMT" ]; then
    available_metasmt_backends="btor stp z3 yices2 cvc4"
    for backend in $available_metasmt_backends; do
      if [ "X${METASMT_DEFAULT}" != "X$backend" ]; then
        if [ "$backend" == "cvc4" ]; then
          for num in {1..10}; do sleep 120; echo 'Keep Travis alive'; done &
        fi
        lit -v --param klee_opts=-metasmt-backend="$backend" --param kleaver_opts=-metasmt-backend="$backend" test/
      fi
    done
  fi
  
  # Generate and upload coverage if COVERAGE is set
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_update "${build_dir}" "systemtests"
  fi
}



function upload_coverage() {
  file="$1"
  tags="$2"
  cd /home/klee/klee_src
  bash <(curl -s https://codecov.io/bash) -X gcov -R /tmp/klee_src/ -y .codecov.yml -f /home/klee/klee_build/coverage_all.info."${file}" -F "$tags"
}

function run_docker() {
 docker_arguments=(docker run -u root --cap-add SYS_PTRACE -ti)
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
  
  # FIXME Enable separated coverage tags again
  if [[ "${UPLOAD_COVERAGE}" -eq 1 ]]; then
    upload_coverage systemtests systemtests_unittests
    upload_coverage unittests systemtests_unittests
  fi
 }

main "$@"