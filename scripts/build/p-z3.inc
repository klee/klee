setup_build_variables_z3() {
  Z3_SUFFIX="${SANITIZER_SUFFIX}"
  Z3_SRC_PATH="${BASE}/z3-${Z3_VERSION}"
  Z3_INSTALL_PATH="${BASE}/z3-${Z3_VERSION}-install${Z3_SUFFIX}"
}

download_z3() {
  mkdir -p "${Z3_SRC_PATH}"
  cd "${Z3_SRC_PATH}"
  wget -qO- https://github.com/Z3Prover/z3/archive/z3-${Z3_VERSION}.tar.gz | tar xz --strip-components=1
}

build_z3() {
  local CFLAGS=""
  local CXXFLAGS=""
  local LDFLAGS=""
  local CC=""
  local CXX=""
  
  # Check if sanitizers should be used to build z3
  if [[ "${SANITIZER_SUFFIX}x" != "x" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS[*]}"
    CXXFLAGS="${SANITIZER_CXX_FLAGS[*]}"
    LDFLAGS="${SANITIZER_LD_FLAGS[*]}"
    
    CC="${SANITIZER_C_COMPILER}"
    CXX="${SANITIZER_CXX_COMPILER}"
  fi
  CFLAGS="${CFLAGS}" \
  CXXFLAGS="${CXXFLAGS}" \
  LDFLAGS="${LDFLAGS}" \
  CC="${CC}" \
  CXX="${CXX}" \
  python scripts/mk_make.py --prefix "${Z3_INSTALL_PATH}" -b "build${Z3_SUFFIX}"
  cd "build${Z3_SUFFIX}"
  make -j$(nproc) || make
}

install_z3() {
  cd "${Z3_SRC_PATH}/build${Z3_SUFFIX}"
  make install
  touch "${Z3_INSTALL_PATH}"/.is_installed
}

is_installed_z3() {
  (
    setup_build_variables_z3
    [[ -f "${Z3_INSTALL_PATH}"/.is_installed ]]
  ) || return 1
}

get_build_artifacts_z3() {
  (
    setup_build_variables_z3
    echo "${Z3_INSTALL_PATH}"
  )
}

setup_artifact_variables_z3() {
  setup_build_variables_z3
}

get_docker_config_id_z3() {
  (
    setup_build_variables_z3
    echo "${Z3_VERSION}${Z3_SUFFIX}"
  )
}
