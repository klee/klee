install_build_dependencies_libcxx() { 
  apt-get update -y

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

  apt -y --no-install-recommends install "${dependencies[@]}"

  pip3 install wllvm
}