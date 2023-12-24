#!/usr/bin/env bash
setup_build_variables_optional() {
  OPTIONAL_INSTALL_PATH="${BASE}/optional-${OPTIONAL_VERSION}"
  optional_url="https://github.com/martinmoene/optional-lite.git"
  return 0
}

download_optional() {
  source "${DIR}/common-functions"
  # Download optional
  git_clone_or_update "${optional_url}" "${OPTIONAL_INSTALL_PATH}" "${OPTIONAL_VERSION}"
  touch "${OPTIONAL_INSTALL_PATH}"/.is_installed
}

build_optional() {
  return 0
}

install_optional() {
  return 0
}

is_installed_optional() {
  (
    setup_build_variables_optional
    [[ -f "${OPTIONAL_INSTALL_PATH}"/.is_installed ]]
  ) || return 1
}

get_build_artifacts_optional() {
  (
    setup_build_variables_optional
    echo "${OPTIONAL_INSTALL_PATH}"
  )
}

setup_artifact_variables_optional() {
  setup_build_variables_optional
}

get_docker_config_id_optional() {
  (
    setup_build_variables_optional
    echo "${OPTIONAL_VERSION}"
  )
}
