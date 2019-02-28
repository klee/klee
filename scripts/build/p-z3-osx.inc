install_binary_artifact_z3 () {
  set +e
  brew install python@2
  if [[ "X$?" != "X0" ]]; then
     brew link --overwrite python@2
  fi
  set -e
  brew install z3
}

is_installed_z3() {
    [[ -f "/usr/local/opt/z3/bin/z3" ]]
}