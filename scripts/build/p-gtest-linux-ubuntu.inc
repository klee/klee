install_build_dependencies_gtest() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    ca-certificates
    wget
    unzip
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}