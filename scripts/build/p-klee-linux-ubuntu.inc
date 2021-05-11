install_build_dependencies_klee() {
  apt update -y

  # Add Kitware's certificate for CMake
  dependencies=(
    apt-transport-https
    ca-certificates
    gnupg
    lsb-release
    software-properties-common
    wget
  )

  apt -y --no-install-recommends install "${dependencies[@]}"
  wget -O - "https://apt.kitware.com/keys/kitware-archive-latest.asc" 2> /dev/null \
      | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg > /dev/null

  # Add CMake repository
  codename="$(lsb_release --codename --short)"
  apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${codename} main"

  # Install KLEE's dependencies
  dependencies=(
    build-essential
    gcc-multilib # to generate 32bit
    cmake
    python-pip #for lit
    python-setuptools #for lit
    python-wheel #for lit
    zlib1g-dev
    python3
    python3-setuptools
    python3-pip
    python3-wheel
  )

  if [[ $(to_bool "${COVERAGE}") -eq 1 ]]; then
    dependencies+=(lcov curl)
  fi

  apt -y --no-install-recommends install "${dependencies[@]}"

  pip3 install lit
  pip3 install tabulate
}
