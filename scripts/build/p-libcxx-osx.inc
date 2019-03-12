# Build dependencies
install_build_dependencies_libcxx() {
  set +e
  brew upgrade python               # upgrade to Python 3
  set -e
  pip3 install wllvm
}