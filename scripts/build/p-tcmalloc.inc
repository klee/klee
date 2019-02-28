setup_build_variables_tcmalloc() {
  TCMALLOC_SRC_PATH="${BASE}/gperftools-${TCMALLOC_VERSION}"
  TCMALLOC_INSTALL_PATH="${BASE}/tcmalloc-install-${TCMALLOC_VERSION}"
  return 0
}

download_tcmalloc() {
  # Get tcmalloc release
  cd "${BASE}"
  wget "https://github.com/gperftools/gperftools/releases/download/gperftools-${TCMALLOC_VERSION}/gperftools-${TCMALLOC_VERSION}.tar.gz"
  tar xfz "gperftools-${TCMALLOC_VERSION}.tar.gz"
}

build_tcmalloc() { 
  cd "${TCMALLOC_SRC_PATH}"
  ./configure --disable-dependency-tracking --disable-cpu-profiler \
    --disable-heap-checker --disable-debugalloc  --enable-minimal \
    --prefix="${TCMALLOC_INSTALL_PATH}"
  make "-j$(nproc)" || make
}

install_tcmalloc() {
  cd "${TCMALLOC_SRC_PATH}"
  make install
  touch "${TCMALLOC_INSTALL_PATH}"/.is_installed
}

is_installed_tcmalloc() {
  (
    setup_build_variables_tcmalloc
    [[ -f "${TCMALLOC_INSTALL_PATH}"/.is_installed ]]
  ) || return 1
}

setup_artifact_variables_tcmalloc() {
  setup_build_variables_tcmalloc
}


get_docker_config_id_tcmalloc() {
  echo "${TCMALLOC_VERSION}"
}

get_build_artifacts_tcmalloc() {
  (
    setup_build_variables_tcmalloc
    echo "${TCMALLOC_INSTALL_PATH}"
  )
}