#!/bin/bash
# Build dependencies
install_build_dependencies_uclibc() { 
  apt update -y

  dependencies=(
    build-essential
    ca-certificates
    git
    python
    libncurses5-dev
    wget
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}