#!/bin/bash
# Build scripts for STP

# Variables that any artifact of this package might depend on
setup_build_variables_stp() {
  STP_SUFFIX="${SANITIZER_SUFFIX}"

  MINISAT_BUILD_PATH="${BASE}/minisat-build${STP_SUFFIX}"
  MINISAT_INSTALL_PATH="${BASE}/minisat-install${STP_SUFFIX}"

  STP_BUILD_PATH="${BASE}/stp-${STP_VERSION}-build${STP_SUFFIX}"
  STP_INSTALL_PATH="${BASE}/stp-${STP_VERSION}-install${STP_SUFFIX}"
  stp_url="https://github.com/stp/stp.git"

  return 0
}

download_stp() {
  source "${DIR}/common-functions"
  # Download minisat
  git_clone_or_update https://github.com/stp/minisat "${BASE}/minisat" "master"

  # Download STP
  git_clone_or_update "${stp_url}" "${BASE}/stp-${STP_VERSION}" "${STP_VERSION}"
}

build_stp() {
  local CFLAGS=""
  local CXXFLAGS=""
  local LDFLAGS=""
  local cmake_general_args=()

  # Check if sanitizers should be used to build stp
  if [[ "${SANITIZER_SUFFIX}x" != "x" ]]; then
    CFLAGS="${SANITIZER_C_FLAGS[*]}"
    CXXFLAGS="${SANITIZER_CXX_FLAGS[*]}"
    LDFLAGS="${SANITIZER_LD_FLAGS[*]}"

    cmake_general_args=(
      "-DCMAKE_C_COMPILER=${SANITIZER_C_COMPILER}"
      "-DCMAKE_CXX_COMPILER=${SANITIZER_CXX_COMPILER}"
    )
  fi

  mkdir -p "${MINISAT_BUILD_PATH}"
  cd "${MINISAT_BUILD_PATH}" || return 1

  local minisat_cmake_args=(
    ${cmake_general_args[@]+"${cmake_general_args[@]}"}
    "-DCMAKE_INSTALL_PREFIX=${MINISAT_INSTALL_PATH}"
  )
  CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS}" \
    cmake "${minisat_cmake_args[@]}" "${BASE}/minisat"

  # Try to build parallel
  make "-j$(nproc)" || make 
  make install

  mkdir -p "${STP_BUILD_PATH}"
  cd "${STP_BUILD_PATH}" || return 1

  local stp_cmake_args=(
      ${cmake_general_args[@]+"${cmake_general_args[@]}"}
      # Don't build against boost because that is broken when mixing packaged boost libraries and gcc 4.8
      "-DNO_BOOST:BOOL=ON"
      "-DENABLE_PYTHON_INTERFACE:BOOL=OFF"
      "-DONLY_SIMPLE:BOOL=ON"
      "-DCMAKE_PREFIX_PATH=${MINISAT_INSTALL_PATH}"
      "-DCMAKE_INSTALL_PREFIX=${STP_INSTALL_PATH}"
  )

  CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS}" \
  cmake "${stp_cmake_args[@]}" "${BASE}/stp-${STP_VERSION}"

  # Try to build parallel
  make "-j$(nproc)" || make
  make install
  touch "${STP_INSTALL_PATH}/.stp_installed"
}

install_stp() {
  return 0
}

# Check if the binary artifact is installed
is_installed_stp() {
  (
    setup_build_variables_stp
    [[ -f "${STP_INSTALL_PATH}/.stp_installed" ]]
  ) || return 1
}

setup_artifact_variables_stp() {
  setup_build_variables_stp
}

get_build_artifacts_stp() {
  (
    setup_build_variables_stp
    echo "${MINISAT_INSTALL_PATH}"
    echo "${STP_INSTALL_PATH}"
  )
}

get_docker_config_id_stp() {
  (
    source "${DIR}/common-functions"
    setup_build_variables_stp

    stp_remote_commit="$(get_git_hash "${stp_url}" "${STP_VERSION}")"
    echo "${stp_remote_commit}${STP_SUFFIX}"
  )
}