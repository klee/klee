#!/bin/bash
# Build dependencies
install_build_dependencies_stp() { 
  apt update -y

  dependencies=(
    build-essential

    bison
    ca-certificates
    cmake
    flex
    git
    zlib1g-dev
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}