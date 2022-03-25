setup_build_variables_metasmt() {
  return 0
}

install_build_dependencies_metasmt() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    # libgmp, gperf (required by yices2)
    gperf
    libgmp-dev
    libbz2-dev
    libboost-dev
    build-essential
    git
    wget
    ca-certificates
    cmake
    zlib1g-dev
    bison
    flex
    unzip
    python
    autoconf
    python3
    curl
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}