install_build_dependencies_klee() {
  source "${DIR}/common-functions"

  # Install KLEE's dependencies
  dependencies=(
    build-essential
    gcc-multilib # to generate 32bit
    zlib1g-dev
    python3
    python3-setuptools
    python3-pip
    python3-venv # Needed to install lit and tabulate in a venv
    python3-wheel
    libtinfo-dev
  )

  if [[ "${SOLVERS:-}" == "metaSMT" ]]; then
    dependencies+=(
      libboost-dev
      libgmp-dev
    )
  fi
  if [[ $(to_bool "${COVERAGE}") -eq 1 ]]; then
    dependencies+=(lcov curl git)
  fi

  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    dependencies+=(doxygen graphviz)
  fi

  with_sudo apt-get -y --no-install-recommends install "${dependencies[@]}"

  # Prepend the venv to PATH, so its python3 (with tabulate) is used by klee-stats
  local klee_venv="${HOME}/.venvs/klee"
  python3 -m venv "${klee_venv}" || return 1
  "${klee_venv}/bin/python" -m pip install lit tabulate || return 1
  export PATH="${klee_venv}/bin:${PATH}"
}
