install_build_dependencies_klee() {
  apt update -y

  dependencies=(
    build-essential
    cmake
    python-pip #for lit
    python-setuptools #for lit
    python-wheel #for lit
    zlib1g-dev
    python3
  )

  if [[ $(to_bool "${COVERAGE}") -eq 1 ]]; then
    dependencies+=(lcov curl)
  fi

  apt -y --no-install-recommends install "${dependencies[@]}"

  pip install lit tabulate
}