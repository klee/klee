#!/usr/bin/env bash
setup_build_variables_json() {
  JSON_INSTALL_PATH="${BASE}/json-${JSON_VERSION}"
  json_url="https://github.com/nlohmann/json.git"
  return 0
}

download_json() {
  source "${DIR}/common-functions"
  # Download json
  git_clone_or_update "${json_url}" "${JSON_INSTALL_PATH}" "${JSON_VERSION}"
  touch "${JSON_INSTALL_PATH}"/.is_installed
}

build_json() {
  return 0
}

install_json() {
  return 0
}

is_installed_json() {
  (
    setup_build_variables_json
    [[ -f "${JSON_INSTALL_PATH}"/.is_installed ]]
  ) || return 1
}

get_build_artifacts_json() {
  (
    setup_build_variables_json
    echo "${JSON_INSTALL_PATH}"
  )
}

setup_artifact_variables_json() {
  setup_build_variables_json
}

get_docker_config_id_json() {
  (
    setup_build_variables_json
    echo "${JSON_VERSION}"
  )
}
