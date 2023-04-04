install_build_dependencies_sqlite() { 
  source "${DIR}/common-functions"
  with_sudo apt update -y

  dependencies=(
  build-essential
    unzip
    git
    wget
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}
install_binary_artifact_sqlite() {
  [[ "${SANITIZER_SUFFIX}x" == "x" ]] || return 1
  [[ "${SQLITE_VERSION}" == "322" ]] || return 1

  source "${DIR}/common-functions"
  with_sudo apt update -y
  dependencies=(
    libsqlite3-dev
  )
  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"
}

# Check if the binary artifact is installed
is_installed_sqlite() {
  [[ -z "${SANITIZER_SUFFIX:-}" ]] || return 1
  [[ -f /usr/lib/x86_64-linux-gnu/libsqlite3.so ]] || return 1
  [[ -f /usr/include/sqlite3.h ]] || return 1
}