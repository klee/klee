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

  local wllvm_venv="${HOME}/.venvs/wllvm"
  python -m venv "${wllvm_venv}" || return 1
  "${wllvm_venv}/bin/python" -m pip install wllvm || return 1
  export PATH="${wllvm_venv}/bin:${PATH}"
}
