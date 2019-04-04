setup_build_variables_metasmt() {
  return 0
}

install_build_dependencies_metasmt() {
  apt update -y

  dependencies=(
    # libgmp, gperf (required by yices2)
    gperf
    libgmp-dev

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
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}