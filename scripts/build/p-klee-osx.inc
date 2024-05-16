# Build dependencies
install_build_dependencies_klee() {
  brew upgrade python               # upgrade to Python 3
  brew link --overwrite python

  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    brew install doxygen graphviz
  fi

  brew install python-tabulate lit

  # Get path of package location
  base_path=$(python3 -m site --user-base)
  export PATH="${base_path}/bin:$PATH"
}