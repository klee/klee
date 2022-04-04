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
    autoconf
    python3
    curl
    python-is-python3 # Need for old Z3
    python3-distutils
    python3-pip
    python3-toml
    default-jre # Needed for ANTLR
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}