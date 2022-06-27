#!/usr/bin/env bash
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

  # Remove klee from PATH
  export PATH=${PATH%":/home/klee/klee_build/bin"}
  if which klee; then
    return 1 # should not happen
  fi

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

  make systemtests || return 1

  metasmt_backends="btor stp z3 yices2 cvc4"
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
  for s in "${SOLVERS[@]}"; do
    case $(tr '[:upper:]' '[:lower:]' <<< "$s") in
      stp)
        lit -v --param klee_opts=-solver-backend=stp --param kleaver_opts=-solver-backend=stp test/ || return 1
      ;;
      z3)
        lit -v --param klee_opts=-solver-backend=z3 --param kleaver_opts=-solver-backend=z3 test/ || return 1
      ;;
      metasmt)
        lit -v --param klee_opts=-solver-backend=metasmt --param kleaver_opts=-solver-backend=metasmt test/ || return 1
        for backend in "${METASMT_BACKENDS[@]}" ; do
          lit -v --param klee_opts="-solver-backend=metasmt -metasmt-backend=$backend" --param kleaver_opts="-solver-backend=metasmt -metasmt-backend=$backend" test/ || return 1
        done
      ;;
      *)
        echo "Unknown smt solver: $s" >2
        exit 2
    esac
  done
  
  # Generate and upload coverage if COVERAGE is set
  if [ "${COVERAGE}" -eq 1 ]; then
    coverage_update "${build_dir}" "systemtests"
  fi
}


function upload_coverage() {
  file="$1"
  tags="$2"
  cd /home/klee/klee_src
  bash <(curl -s https://codecov.io/bash) -X gcov -R /tmp/klee_src/ -f /home/klee/klee_build/coverage_all.info."${file}" -F "$tags"
}

function run_docker() {
  docker_arguments=(docker run -u root --cap-add SYS_PTRACE -t)
  script_arguments=("--debug" '"/tmp/klee_build"*')

  if [[ ${#SOLVERS[@]} -gt 0 ]]; then
    script_arguments+=("--solvers=$(IFS=':' && echo "${SOLVERS[*]}")")
  fi

  if [[ ${#METASMT_BACKENDS[@]} -gt 0 ]]; then
    script_arguments+=("--metasmt-backends=$(IFS=':' && echo "${METASMT_BACKENDS[*]}")")
  fi

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
  local SOLVERS=()
  local METASMT_BACKENDS=()

  for i in "$@" ; do
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
      --solvers=*)
        readarray -d ':' -t SOLVERS <<< "${i#*=}"
      ;;
      --metasmt-backends=*)
        readarray -d ':' -t METASMT_BACKENDS <<< "${i#*=}"
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
