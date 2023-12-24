#!/usr/bin/env bash
install_build_dependencies_json() {
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
    ca-certificates
    wget
    unzip
    git
  )

  #Install essential dependencies
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}
