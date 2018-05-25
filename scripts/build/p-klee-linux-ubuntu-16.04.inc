install_build_dependencies_klee() {
  apt update -y

  dependencies=(
    build-essential
    cmake
    libsqlite3-dev
    python-pip #for lit
    python-setuptools #for lit
    python-wheel #for lit
    zlib1g-dev
    python3
    python3-pip
  )

  if [[ $(to_bool "${COVERAGE}") -eq 1 ]]; then
    dependencies+=(lcov curl)
  fi

  apt -y --no-install-recommends install "${dependencies[@]}"

  pip install lit
  pip3 install tabulate --user
}