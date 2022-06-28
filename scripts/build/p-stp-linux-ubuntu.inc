# Build dependencies
install_build_dependencies_stp() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    build-essential

    bison
    ca-certificates
    cmake
    flex
    git
    zlib1g-dev
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}