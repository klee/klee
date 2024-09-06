# Build dependencies
install_build_dependencies_klee() {
  brew upgrade python               # upgrade to Python 3
  brew link --overwrite python
  export PATH="$(brew --prefix python)/libexec/bin:$PATH"

  if [[ $(to_bool "${ENABLE_DOXYGEN}") -eq 1 ]]; then
    brew install doxygen graphviz
  fi

  brew install python-tabulate lit

  # brew does not seem to update this properly for 0.9.0_1
  export PYTHONPATH=/usr/local/opt/python-tabulate/libexec/lib/python3.12/site-packages

  # Get path of package location
  base_path=$(python -m site --user-base)
  export PATH="${base_path}/bin:$PATH"
}