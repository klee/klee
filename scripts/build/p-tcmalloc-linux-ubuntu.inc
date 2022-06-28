install_build_dependencies_tcmalloc() { 
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
  build-essential
    ca-certificates
    cmake
    git
    wget
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}