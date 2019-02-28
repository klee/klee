# Build dependencies Z3
install_build_dependencies_z3() { 
  apt update -y

  dependencies=(
    build-essential

	python
    git
    wget
    ca-certificates
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}