# Build dependencies
install_build_dependencies_klee() {
  brew upgrade python               # upgrade to Python 3
  brew link --overwrite python
  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    brew install doxygen graphviz
  fi
  pip3 install lit tabulate
}