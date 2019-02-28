install_build_dependencies_gtest() {
  apt update -y

  dependencies=(
    ca-certificates
    wget
    unzip
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}