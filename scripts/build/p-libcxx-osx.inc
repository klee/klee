# Build dependencies
install_build_dependencies_libcxx() {
  set +e
  brew upgrade python               # upgrade to Python 3
  set -e
  pip3 install --user wllvm
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}