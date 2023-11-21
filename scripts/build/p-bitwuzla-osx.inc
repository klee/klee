install_build_dependencies_bitwuzla() {
  dependencies=(
    ninja
    python3
    python3-pip
    cmake
    git
    pkg-config
  )
  brew install "${dependencies[@]}"
  pip3 install --user meson
  base_path="$(python3 -m site --user-base)"
  export PATH="$PATH:${base_path}/bin"
}
