install_build_dependencies_libcxx() {
  source "${DIR}/common-functions"

  with_sudo apt-get update -y
  dependencies=(
    ca-certificates
    base-devel
    git
    python
    python-pip
    curl
    file # Needed for wllvm
  )

  with_sudo pacman --no-confirm -S "${dependencies[@]}"

  pip install --user wllvm
  base_path="$(python -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}
