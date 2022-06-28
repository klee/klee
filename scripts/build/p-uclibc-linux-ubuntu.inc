# Build dependencies
install_build_dependencies_uclibc() { 
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    build-essential
    ca-certificates
    git
    python
    libncurses5-dev
    wget
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}