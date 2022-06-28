install_build_dependencies_libcxx() {
  source "${DIR}/common-functions"

  with_sudo apt-get update -y
  dependencies=(
    build-essential
    ca-certificates
    git
    python3
    python3-pip
    curl
    file # Needed for wllvm
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  pip3 install --user wllvm
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}