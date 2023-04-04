setup_build_variables_sqlite() {
  SQLITE_SRC_PATH="${BASE}/sqlite-amalgamation-${SQLITE_VERSION}"
  SQLITE_SUFFIX="${SANITIZER_SUFFIX}"
  SQLITE_INSTALL_PATH="${SQLITE_SRC_PATH}${SANITIZER_SUFFIX}"
}

download_sqlite() {
  # Get sqlite release
  cd "${BASE}"
  wget --no-check-certificate "https://www.sqlite.org/2022/sqlite-amalgamation-${SQLITE_VERSION}.zip"
  unzip "sqlite-amalgamation-${SQLITE_VERSION}.zip"
}

build_sqlite() {
  cd "${SQLITE_SRC_PATH}"

  # Check if sanitizers should be used to build sqlite
  if [[ "${SANITIZER_SUFFIX}x" != "x" ]]; then
    CFLAGS=("${SANITIZER_C_FLAGS[@]}")
    COMP="${SANITIZER_C_COMPILER}"
  else
    [[ -z "${COMP:-}" ]] && COMP="$(which gcc)"
  fi

  mkdir -p "${SQLITE_INSTALL_PATH}"
  "${COMP}" "${CFLAGS[@]}" -fPIC -o "${SQLITE_INSTALL_PATH}"/libsqlite3.so -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -shared sqlite3.c
  if [[ -n "${SANITIZER_SUFFIX:-}" ]]; then
    cp "${SQLITE_SRC_PATH}/"sqlite3.h "${SQLITE_INSTALL_PATH}/"
  fi
}

install_sqlite() {
  return 0
}

is_installed_sqlite() {
  (
    setup_build_variables_sqlite
    [[ -f "${SQLITE_INSTALL_PATH}"/libsqlite3.so ]]
  ) || return 1
}

setup_artifact_variables_sqlite() {
  setup_build_variables_sqlite
}

get_docker_config_id_sqlite() {
  (
    setup_build_variables_sqlite
    echo "${SQLITE_VERSION}${SQLITE_SUFFIX}"
  )
}

get_build_artifacts_sqlite() {
  (
    setup_build_variables_sqlite
    echo "${SQLITE_SRC_PATH}"
    echo "${SQLITE_INSTALL_PATH}"
  )
}
