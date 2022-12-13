validate_build_config_klee() {
  return 0
}

setup_build_variables_klee() {
  source "${DIR}/common-functions"
  KLEE_SUFFIX="${LLVM_VERSION_SHORT}${SOLVER_SUFFIX}${SANITIZER_SUFFIX}"
  KLEE_BUILD_DIR="${BASE}/klee_build${KLEE_SUFFIX}"
  KLEE_SRC="$DIR/../../"
}

download_klee() {
  return 0
}

build_klee() {
  CMAKE_PREFIX_PATH=("")

  local CMAKE_ARGUMENTS=(
    "-DLLVM_CONFIG_BINARY=${LLVM_CONFIG}"
    "-DLLVMCC=${BITCODE_CC}"
    "-DLLVMCXX=${BITCODE_CXX}"
    "-DGTEST_SRC_DIR=${GTEST_INSTALL_PATH}"
    "-DENABLE_UNIT_TESTS=TRUE"
    "-DENABLE_SYSTEM_TESTS=TRUE"
    "-DENABLE_DOXYGEN=${ENABLE_DOXYGEN}"
    "-DLIT_ARGS=\"-v;--time-tests\""
  )


###############################################################################
# klee-uclibc
###############################################################################

  if [ "${UCLIBC_VERSION}" != "0" ]; then
    CMAKE_ARGUMENTS+=(
      "-DENABLE_KLEE_UCLIBC=TRUE"
      "-DKLEE_UCLIBC_PATH=${BASE}/klee-uclibc-${LLVM_VERSION_SHORT}"
      "-DENABLE_POSIX_RUNTIME=TRUE"
    )
  else
    CMAKE_ARGUMENTS+=(
      "-DENABLE_KLEE_UCLIBC=FALSE"
      "-DENABLE_POSIX_RUNTIME=FALSE"
    )
  fi

###############################################################################
# libc++
###############################################################################

if [ "${USE_LIBCXX}" -eq 1 ]; then
  CMAKE_ARGUMENTS+=(
    "-DENABLE_KLEE_LIBCXX=TRUE"
    "-DKLEE_LIBCXX_DIR=${LIBCXX_INSTALL}"
    "-DKLEE_LIBCXX_INCLUDE_DIR=${LIBCXX_INSTALL}/include/c++/v1"
  )
  local LLVM_VERSION_MAJOR="${LLVM_VERSION/.*/}"
  [[ "${LLVM_VERSION_MAJOR}" -ge 8 ]] && CMAKE_ARGUMENTS+=(
    "-DENABLE_KLEE_EH_CXX=TRUE"
    "-DKLEE_LIBCXXABI_SRC_DIR=${LIBCXX_SRC}/libcxxabi"
  )
else
  CMAKE_ARGUMENTS+=(
    "-DENABLE_KLEE_LIBCXX=FALSE"
  )
fi


###############################################################################
# Handle setting up solver configure flags for KLEE
###############################################################################

  local KLEE_Z3_CONFIGURE_OPTION=("-DENABLE_SOLVER_Z3=OFF")
  local KLEE_STP_CONFIGURE_OPTION=("-DENABLE_SOLVER_STP=OFF")
  local KLEE_METASMT_CONFIGURE_OPTION=("-DENABLE_SOLVER_METASMT=OFF")
  KLEE_FLOATING_POINT=(
    "-DENABLE_FLOATING_POINT=FALSE"
    "-DENABLE_FP_RUNTIME=FALSE"
  )
  
  
  for solver in "${SELECTED_SOLVERS[@]}"; do
    echo "Setting CMake configuration option for ${solver}"
    case "${solver}" in
    stp)
      KLEE_STP_CONFIGURE_OPTION=(
        "-DENABLE_SOLVER_STP=TRUE"
        "-DSTP_DIR=${STP_INSTALL_PATH}/"
      )
      CMAKE_PREFIX_PATH+=("${STP_INSTALL_PATH}")
      ;;
    z3)
      echo "Z3"
      KLEE_Z3_CONFIGURE_OPTION=(
        "-DENABLE_SOLVER_Z3=TRUE"
        "-DZ3_INCLUDE_DIRS=${Z3_INSTALL_PATH}/include"
        "-DZ3_LIBRARIES=${Z3_INSTALL_PATH}/lib/libz3.so"
      )
      KLEE_FLOATING_POINT=(
        "-DENABLE_FLOATING_POINT=TRUE"
        "-DENABLE_FP_RUNTIME=TRUE"
      )
      ;;
    metasmt)
      echo "metaSMT"
      if [ "X${METASMT_DEFAULT}" == "X" ]; then
        METASMT_DEFAULT=STP
      fi
      KLEE_METASMT_CONFIGURE_OPTION=(
        "-DENABLE_SOLVER_METASMT=TRUE"
        "-DmetaSMT_DIR=${METASMT_BUILD_PATH}"
        "-DMETASMT_DEFAULT_BACKEND=${METASMT_DEFAULT}"
      )
      ;;
    *)
      echo "Unknown solver ${solver}"
      exit 1
    esac
  done
  
  CMAKE_ARGUMENTS+=(
    "${KLEE_Z3_CONFIGURE_OPTION[@]}"
    "${KLEE_STP_CONFIGURE_OPTION[@]}"
    "${KLEE_METASMT_CONFIGURE_OPTION[@]}"
    "${KLEE_FLOATING_POINT[@]}"
  )

###############################################################################
# Handle additional configure flags
###############################################################################

  if [[ $(to_bool "${USE_TCMALLOC}") -eq 1 ]] ; then
    CMAKE_PREFIX_PATH+=("${TCMALLOC_INSTALL_PATH}")
    CMAKE_ARGUMENTS+=("-DENABLE_TCMALLOC=TRUE")
  else
    CMAKE_ARGUMENTS+=("-DENABLE_TCMALLOC=FALSE")
  fi

  CMAKE_PREFIX_PATH+=("${SQLITE_INSTALL_PATH}")
###############################################################################
# KLEE
###############################################################################

  if [ "X${DISABLE_ASSERTIONS}" == "X1" ]; then
    CMAKE_ARGUMENTS+=("-DENABLE_KLEE_ASSERTS=FALSE")
  else
    CMAKE_ARGUMENTS+=("-DENABLE_KLEE_ASSERTS=TRUE")
  fi
  
  if [ "X${ENABLE_OPTIMIZED}" == "X1" ]; then
    CMAKE_ARGUMENTS+=("-DCMAKE_BUILD_TYPE=RelWithDebInfo")
  else
    CMAKE_ARGUMENTS+=("-DCMAKE_BUILD_TYPE=Debug")
  fi

  CMAKE_ARGUMENTS+=("-DKLEE_RUNTIME_BUILD_TYPE=${KLEE_RUNTIME_BUILD}")
  
# TODO: We should support Ninja too
# Configure KLEE

  local CXX_FLAGS=("")
  local C_FLAGS=("")
  local LD_FLAGS=("")
  
  if [ "${COVERAGE}" -eq 1 ]; then
    CXX_FLAGS+=("-fprofile-arcs" "-ftest-coverage")
    C_FLAGS+=("-fprofile-arcs" "-ftest-coverage")
    LD_FLAGS+=("-fprofile-arcs" "-ftest-coverage")
  fi

  if [[ -n "${SANITIZER_BUILD}" ]]; then
    C_FLAGS+=("${SANITIZER_C_FLAGS[@]}")
    CXX_FLAGS+=("${SANITIZER_CXX_FLAGS[@]}")
    LD_FLAGS+=("${SANITIZER_LD_FLAGS[@]}")

    CMAKE_ARGUMENTS+=(
      "-DCMAKE_C_COMPILER=${SANITIZER_C_COMPILER}"
      "-DCMAKE_CXX_COMPILER=${SANITIZER_CXX_COMPILER}"
    )
  fi



  mkdir -p "${KLEE_BUILD_DIR}" || return 1
  cd "${KLEE_BUILD_DIR}"

  {
    echo "CXXFLAGS=\"${CXX_FLAGS[*]}\" \\"
    echo "CFLAGS=\"${C_FLAGS[*]}\" \\"
    echo "LDFLAGS=\"${LD_FLAGS[*]}\" \\"

    if [[ -n "${CMAKE_PREFIX_PATH[*]}" ]]; then
      cmake_concatenated=$(IFS=: ; echo "${CMAKE_PREFIX_PATH[*]}")
      echo "CMAKE_PREFIX_PATH=\"${cmake_concatenated}\" \\"
    fi

    echo "cmake ${CMAKE_ARGUMENTS[*]} \"${KLEE_SRC}\""
  } > "${KLEE_BUILD_DIR}/.build_command"

  source "${KLEE_BUILD_DIR}/.build_command"
  make -j$(nproc) || make || return 1

  touch "${KLEE_BUILD_DIR}/.is_installed"
}

install_klee() {
  return 0
}

get_docker_config_id_klee() {
  (
    setup_build_variables_klee
    echo "${KLEE_SUFFIX}"
  )
}

get_docker_context_klee() {
  # KLEE's source code is two levels above this script
  echo "${DIR}/../../"
}

get_docker_container_context_klee() {
  echo "${BASE}/klee_src/"
}

get_build_artifacts_klee() {
  (
    setup_build_variables_klee
    echo "${KLEE_BUILD_DIR}"
  )
}

# Check if the binary artifact is installed
is_installed_klee() {
  (
    setup_build_variables_klee
    [[ -f "${KLEE_BUILD_DIR}"/.is_installed ]]
  ) || return 1
}

setup_artifact_variables_klee() {
  setup_build_variables_klee
}