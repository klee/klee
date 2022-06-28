install_binary_artifact_cmake() {
  source "${DIR}/common-functions"

  # Check if the Ubuntu version is an LTS version - otherwise no cmake version available
  if grep -q LTS /etc/os-release; then
    if ! grep -qr "apt.kitware.com" /etc/apt; then
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
  fi

  with_sudo apt update -y

  dependencies=(
    cmake
  )
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}

get_build_artifacts_cmake() {
  return 0
}

# Check if the binary artifact is installed
is_installed_cmake() {
  if which cmake ; then
    return 0
  fi

  return 1
}

setup_artifact_variables_cmake() {
  return 0
}