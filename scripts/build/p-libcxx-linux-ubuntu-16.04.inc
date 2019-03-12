install_build_dependencies_libcxx() { 
  apt-get update -y

  dependencies=(
    build-essential
    ca-certificates
    git
    python3
    cmake
    curl
    
    file # Needed for wllvm
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"

  curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
  python3 get-pip.py --user
  export PATH="${HOME}/.local/bin:$PATH"
  pip3 install wllvm
}