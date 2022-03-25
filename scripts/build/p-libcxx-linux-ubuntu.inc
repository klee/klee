install_build_dependencies_libcxx() {
  source "${DIR}/common-functions"

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

  with_sudo apt-get update -y
  dependencies=(
    build-essential
    ca-certificates
    git
    python3
    python3-pip
    cmake
    curl
    file # Needed for wllvm
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  pip3 install --user wllvm
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}