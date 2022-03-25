# Build dependencies Z3
install_build_dependencies_z3() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    build-essential
    python
    git
    wget
    ca-certificates
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}