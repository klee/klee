install_build_dependencies_sqlite() { 
  apt update -y

  dependencies=(
  build-essential
    unzip
    git
    wget
  )

  apt -y --no-install-recommends install "${dependencies[@]}"
}
install_binary_artifact_sqlite() {
  [[ "${SANITIZER_SUFFIX}x" == "x" ]] || return 1

  apt update -y
  dependencies=(
    libsqlite3-dev
  )
  apt -y --no-install-recommends install "${dependencies[@]}"
}

# Check if the binary artifact is installed
is_installed_sqlite() {
  [[ -f /usr/lib/x86_64-linux-gnu/libsqlite3.so ]] || return 1
  return 0
}

get_docker_config_id_sqlite() {
  return 0
}
