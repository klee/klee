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
    python3-wheel
  )

  if [[ $(to_bool "${COVERAGE}") -eq 1 ]]; then
    dependencies+=(lcov curl git)
  fi

  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    dependencies+=(doxygen graphviz)
  fi

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  pip3 install --user lit
  pip3 install --user tabulate
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}
