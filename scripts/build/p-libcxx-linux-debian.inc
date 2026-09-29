install_build_dependencies_libcxx() {
  source "${DIR}/common-functions"

  with_sudo apt-get update -y
  dependencies=(
    build-essential
    ca-certificates
    git
    python3
    python3-pip
    python3-venv # Needed to install wllvm in a venv
    curl
    file # Needed for wllvm
  )

  with_sudo apt -y --no-install-recommends install "${dependencies[@]}"

  local wllvm_venv="${HOME}/.venvs/wllvm"
  python3 -m venv "${wllvm_venv}" || return 1
  "${wllvm_venv}/bin/python" -m pip install wllvm || return 1
  export PATH="${wllvm_venv}/bin:${PATH}"
}