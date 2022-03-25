install_build_dependencies_klee() {
  source "${DIR}/common-functions"

  # Check if the Ubuntu version is an LTS version - otherwise no cmake version available
  if [[ $(grep -q LTS /etc/os-release) ]]; then
    with_sudo apt update -y
    # Add Kitware's certificate for CMake
    dependencies=(
      apt-transport-https
      ca-certificates
      gnupg
      lsb-release
      software-properties-common
      wget
    )

    with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
    wget -O - "https://apt.kitware.com/keys/kitware-archive-latest.asc" 2> /dev/null \
        | gpg --dearmor - | with_sudo tee /etc/apt/trusted.gpg.d/kitware.gpg > /dev/null

    # Add CMake repository
    codename="$(lsb_release --codename --short)"
    with_sudo apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ ${codename} main"
  fi

  with_sudo apt update -y

  # Install KLEE's dependencies
  dependencies=(
    build-essential
    gcc-multilib # to generate 32bit
    cmake
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
