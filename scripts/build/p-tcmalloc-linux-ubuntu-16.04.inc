install_build_dependencies_tcmalloc() { 
  apt update -y

  dependencies=(
  build-essential
    ca-certificates
    cmake
    git
    wget
  )

  apt -y --no-install-recommends install "${dependencies[@]}"
}