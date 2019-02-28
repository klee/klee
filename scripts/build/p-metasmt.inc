setup_build_variables_metasmt() {
  METASMT_SRC_PATH="${BASE}/metaSMT-${METASMT_VERSION}"
  METASMT_BUILD_PATH="${BASE}/metaSMT-${METASMT_VERSION}-build"
  METASMT_INSTALL_PATH="${BASE}/metaSMT-${METASMT_VERSION}-install"
  METASMT_DEPS_PATH="${BASE}/metaSMT-${METASMT_VERSION}-deps"
}

download_metasmt() {
  # Clone
  git clone --single-branch --depth 1 https://github.com/hoangmle/metaSMT.git "${METASMT_SRC_PATH}"
  cd "${METASMT_SRC_PATH}"
  git submodule update --init
  # Bootstrap
  git clone https://github.com/agra-uni-bremen/dependencies.git
}

build_metasmt() {
  cd "${METASMT_SRC_PATH}"
  ./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off \
    --build stp-git-basic --build boolector-2.2.0 --build minisat-git \
    --build lingeling-ayv-86bf266-140429 --build yices-2.5.1 --build Z3-4.4.1 \
    --build cvc4-1.5 \
    --deps "${METASMT_DEPS_PATH}/" \
    --install "${METASMT_INSTALL_PATH}/" \
    -j "$(nproc)" \
    "${METASMT_BUILD_PATH}"

  # Build
  cd "${METASMT_BUILD_PATH}"
  make
}

install_metasmt() {
  cd "${METASMT_BUILD_PATH}"
  make install
  touch "${METASMT_INSTALL_PATH}"/.is_installed
}

get_docker_config_id_metasmt() {
  echo "${METASMT_VERSION}"
}

# Check if the binary artifact is installed
is_installed_metasmt() {
  (
  setup_build_variables_metasmt
  [[ -f "${METASMT_INSTALL_PATH}"/.is_installed ]]
  )
}

get_docker_config_id_metasmt() {
  (
    echo "${METASMT_VERSION}"
  )
}

get_build_artifacts_metasmt() {
  (
    setup_build_variables_metasmt
    echo "${METASMT_INSTALL_PATH}/"
    echo "${METASMT_BUILD_PATH}/"
    echo "${METASMT_DEPS_PATH}/"
  )
}