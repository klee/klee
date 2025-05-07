install_binary_artifact_z3 () {
  brew install z3
}

is_installed_z3() {
    [[ -f "/opt/homebrew/bin/z3" ]]
}