#!/usr/bin/env bash
setup_build_variables_immer() {
  IMMER_INSTALL_PATH="${BASE}/immer-${IMMER_VERSION}"
  immer_url="https://github.com/arximboldi/immer.git"
  return 0
}

download_immer() {
  source "${DIR}/common-functions"
  # Download immer
  git_clone_or_update "${immer_url}" "${IMMER_INSTALL_PATH}" "${IMMER_VERSION}"
  touch "${IMMER_INSTALL_PATH}"/.is_installed
}

build_immer() {
  return 0
}

install_immer() {
  return 0
}

is_installed_immer() {
  (
    setup_build_variables_immer
    [[ -f "${IMMER_INSTALL_PATH}"/.is_installed ]]
  ) || return 1
}

get_build_artifacts_immer() {
  (
    setup_build_variables_immer
    echo "${IMMER_INSTALL_PATH}"
  )
}

setup_artifact_variables_immer() {
  setup_build_variables_immer
}

get_docker_config_id_immer() {
  (
    setup_build_variables_immer
    echo "${IMMER_VERSION}"
  )
}
