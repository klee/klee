# Build dependencies
install_build_dependencies_klee() {
  brew upgrade python               # upgrade to Python 3
  pip3 install lit tabulate
}