install_build_dependencies_stp() {
   dependencies=(
     bison
     flex
   )
   brew install "${dependencies[@]}"
}
