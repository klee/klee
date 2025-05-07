install_binary_artifact_z3 () {
  brew install z3
}

is_installed_z3() {
    [[ -f "/opt/homebrew/bin/z3" ]]
}

setup_artifact_variables_z3() {
  # shellcheck disable=SC2034
  Z3_INSTALL_PATH="/opt/homebrew/"
}
