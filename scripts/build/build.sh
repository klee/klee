#!/usr/bin/env bash
set -e
set -u
set -o pipefail
# Build script for different components that are needed

DIR="$(cd "$(dirname "$0")" && pwd)"

show_help() {
  local name="$1"
  echo "$name [--docker] [--keep-dockerfile] [--install-system-deps] component*" 
  echo "Available components:"
  for component_path in "$DIR/v-"*.inc; do
    [ -e "$component_path" ] || continue

    component_path=${component_path#"$DIR/v-"}
    component=${component_path%".inc"}

    echo "$component"
  done

  return 0
}

function to_upper() {
  echo "$1" | tr '[:lower:]' '[:upper:]'
}

function to_lower() {
  echo "$1" | tr '[:upper:]' '[:lower:]'
}

to_bool() {
  [[ "$1" == "1" || "$1" == "ON" || "$1" == "YES" || "$1" == "TRUE" || "$1" == "T" ]] && echo 1 && return 0
  [[ "$1" == "0" || "$1" == "OFF" || "$1" == "NO" || "$1" == "FALSE" || "$1" == "F" ]] && echo 0 && return 0
  # Error case - value unknown
  exit 1
}

check_bool() {
  local v="$1"
  local result
  result=$(to_bool "${!v}" ) || { echo "${v} has invalid boolean value: ${!v}"; exit 1; }
}

validate_build_variables() {
  # Check general default variables
  if [[ -z ${BASE+x} ]] || [[ -z "${BASE}" ]]; then
    echo "BASE not set. - BASE directory defines target where every component should be installed"
    exit 1
  fi
}

validate_component() {
  local component="$1"
  local c_type="$2"

  local component_path="$DIR/${c_type}-${component}.inc"
  if [[ ! -f "${component_path}" ]]; then
    echo "Component ${component} not found."
    return 1
  fi
}

load_component() {
  local component="$1"
  local c_type="$2"

  local component_path="$DIR/${c_type}-${component}.inc"
  if [[ ! -f "${component_path}" ]]; then
    echo "Component ${component} not found."
    return 2 # File not found
  fi
  source "${component_path}" || { echo "Error in file ${component_path}"; return 1; }
}


check_docker_os() {
  local docker_base=$1
  local os_info=""
  # start docker using the base image, link this directory and get the os information
  os_info=$(docker run -v "${DIR}:/tmp" "${docker_base}" /tmp/build.sh --check-os) || { echo "Docker execution failed: $os_info"; exit 1;}

  local line
  while read -r line; do
  case $line in
    OS=*)
    OS="${line#*=}"
    ;;
    DISTRIBUTION=*)
      DISTRIBUTION="${line#*=}"
    ;;
    DISTRIBUTION_VER=*)
      DISTRIBUTION_VER="${line#*=}"
    ;;
  esac
  done <<< "$os_info"

  echo "Docker OS=${OS}"
  echo "Docker DISTRIBUTION=${DISTRIBUTION}"
  echo "Docker DISTRIBUTION_VER=${DISTRIBUTION_VER}"
}


build_docker() {
  local component=$1
  local is_dependency=$2
  
  load_component "${component}" "v" || { echo "Loading component failed ${component}"; return 1; }

  # Extract repository, image, tag from base image
  local base_repository="${BASE_IMAGE%/*}"
  [[ "${base_repository}" == "${BASE_IMAGE}" ]] && base_repository="" # No base repository provided
  local base_image="${BASE_IMAGE##*/}"
  local base_tag="${base_image##*:}"
  if [[ "${base_tag}" == "${base_image}" ]]; then
    base_tag="" # No tag provided
  else
    base_image="${base_image%:*}"
  fi

  local BASE="/tmp"

  # Check all input variables
  validate_required_variables "${component}" "required_variables"

  # Get the list of build dependencies and check them
  local depending_artifact_components=("")
  check_list artifact_dependency_"${component}"
  mapfile -t depending_artifact_components <<< "$(get_list artifact_dependency_"${component}")"
  local v
  for v in "${depending_artifact_components[@]}"; do
    [[ -z "${v}" ]] && continue
    { build_docker "${v}" 1; } || { echo "Not all compile dependencies are available: ${v}"; return 1; }
  done

  # Get the list of runtime dependencies
  local depending_runtime_artifact_components=("")
  mapfile -t depending_runtime_artifact_components <<< "$(get_list runtime_artifact_dependency_"${component}")"
  local v
  for v in "${depending_runtime_artifact_components[@]}"; do
    [[ -z "${v}" ]] && continue
    { build_docker "${v}" 1;} || { echo "Not all runtime dependencies are available: ${v}"; return 1; }
  done

  echo "Build component: ${component}"

  # Setup general component variables
  gather_dependencies "${component}" "artifact" "setup_variables"

  # Get the docker image id if available
  local config_id=""
  config_id=$(execution_action "get_docker_config_id" "${component}")
  res="$?"
  [[ "$res" -eq 1 ]] && echo "Building docker dependency ${component} failed: get_docker_config_id_${component}" && return 1

  # An empty config id indicates nothing to build
  [[ -z "${config_id}" ]] && return 0

  # Use the config id to find a pre-build docker image
  local target_image="${component}:${config_id}_${base_image}_${base_tag}"
  [[ -z $(docker images -q "${REPOSITORY}/${target_image}") ]] || { echo "Docker image exists: ${REPOSITORY}/${target_image}"; return 0; }
  docker pull "${REPOSITORY}/${target_image}" || /bin/true
  [[ -z $(docker images -q "${REPOSITORY}/${target_image}") ]] || { echo "Docker image exists: ${REPOSITORY}/${target_image}"; return 0; }

  # Gather all indirect depending components
  local all_depending_artifact_components=("")
  mapfile -t all_depending_artifact_components <<< "$(get_all_dependencies "${component}" "artifact")"

  local all_depending_runtime_artifact_components=("")
  mapfile -t all_depending_runtime_artifact_components <<< "$(get_all_dependencies "${component}" "runtime_artifact")"

  local docker_dependencies=("")
  docker_dependencies+=("${all_depending_artifact_components[@]}")

  for c in "${depending_runtime_artifact_components[@]}"; do
    local found=0
    for i in "${docker_dependencies[@]}"; do
      [[ -z "$i" ]] && continue
      [[ "$i" == "$c" ]] && found=1 && break
    done

    # Attach item if new
    [[ "$found" -eq 0 ]] && docker_dependencies+=("$c")
  done

  # Start to build image
  local temp_dir
  temp_dir="$(mktemp -d)"
  [[ -d "${temp_dir}" ]] || (echo "Could not create temp_dir"; exit 1)

  local docker_context=""
  docker_context=$(execution_action "get_docker_context" "${component}") || docker_context=""

  local docker_container_context=""
  docker_container_context=$(execution_action "get_docker_container_context" "${component}") || docker_container_context=""
  
  # Prepare Dockerfile
  local dockerfile="${temp_dir}/Dockerfile"

  {
    # Add all depending images as layers
    for v in "${docker_dependencies[@]}"; do
      [[ -z "${v}" ]] && continue
      local dep_config_id
      dep_config_id=$(execution_action "get_docker_config_id" "${v}") || true
      # An empty config id indicates nothing to build
      [[ -z "${dep_config_id}" ]] && continue

      echo "FROM ${REPOSITORY}/${v}:${dep_config_id}_${base_image}_${base_tag} as ${v}_base"
    done

    # If multiple source images are available use the first one as a base image
    local hasBaseImage=0
    for v in "${all_depending_artifact_components[@]}"; do
      [[ -z "${v}" ]] && continue
      dep_config_id=$(execution_action "get_docker_config_id" "${v}") || true

      # An empty config id indicates nothing to build
      [[ -z "${dep_config_id}" ]] && continue
      
      if [[ "${hasBaseImage}" -eq 0 ]]; then
        hasBaseImage=1
        # Use base image as building base
        echo "FROM ${v}_base as intermediate"
      else
        # Copy artifacts
        # TODO: maybe get list of specific directories to copys
        echo "COPY --from=${v}_base ${BASE} ${BASE}/"
      fi
    done

    if [[ "${hasBaseImage}" -eq 0 ]]; then
      hasBaseImage=1
      # Use base image as building base
      echo "FROM ${BASE_IMAGE} as intermediate"
    fi

    # Retrieve list of required variables and make them available in Docker
    local docker_vars=[]
    mapfile -t docker_vars <<< "$(get_list required_variables_"$component")"
    docker_vars+=("BASE")
    for v in "${all_depending_artifact_components[@]}"; do
      [[ -z "${v}" ]] && continue
      local dep_docker_vars=()
      mapfile -t dep_docker_vars <<< "$(get_list required_variables_"${v}")"
      docker_vars+=("${dep_docker_vars[@]}")
    done

    # Export docker variables explicitly
    for d_v in "${docker_vars[@]}"; do
      [[ -z "${d_v}" ]] && continue
      local d_v_value
      d_v_value=${!d_v}
      echo "ENV ${d_v}=${d_v_value}"
    done

    # Run the build script
    if [[ -z "${docker_context}" ]]; then
      # Copy content from the temporary directory to the docker image
      echo "COPY . ."
      echo "RUN ./build.sh --debug --install-system-deps ${component}"
    else
      echo "COPY . ${docker_container_context}"
      # TODO: this is quite specific to this script: should use common value
      echo "RUN ${docker_container_context}/scripts/build/build.sh --debug --install-system-deps ${component}"
    fi

    # Copy all the build artifacts to the final image: build artifact is the full path
    # and will be the same in the destination docker image
    if [[ "${is_dependency}" -eq 1 || "${CREATE_FINAL_IMAGE}" -eq 0 ]]; then
      echo "FROM ${BASE_IMAGE} as final"
      local build_artifacts
      mapfile -t build_artifacts <<< "$(execution_action "get_build_artifacts" "${component}")"
      for artifact in "${build_artifacts[@]}"; do
        [[ -z "${artifact}" ]] && continue
        echo "COPY --from=intermediate ${artifact} ${artifact}"
      done
    
      # Copy all the runtime dependencies
      for dep_component in "${all_depending_runtime_artifact_components[@]}"; do
        [[ -z "${dep_component}" ]] && continue
        local build_artifacts
        mapfile -t build_artifacts <<< "$(execution_action "get_build_artifacts" "${dep_component}")"
        for artifact in "${build_artifacts[@]}"; do
          [[ -z "${artifact}" ]] && continue
          echo "COPY --from=intermediate ${artifact} ${artifact}"
        done
      done
    fi
    # TODO Add description labels
    echo "LABEL maintainer=\"KLEE Developers\""
  } >> "${dockerfile}"

  # Append template file if exists
  docker_template_files=(
      "${DIR}/d-${component}-${OS}-${DISTRIBUTION}-${DISTRIBUTION_VER}.inc"
      "${DIR}/d-${component}-${OS}-${DISTRIBUTION}.inc"
      "${DIR}/d-${component}-${OS}.inc"
      "${DIR}/d-${component}.inc"
    )
  for f in "${docker_template_files[@]}"; do
     [[ ! -f "$f" ]] && continue
     cat "$f" >> "${dockerfile}"
     break
  done

  # Copy docker context and dependencies
  shopt -s nullglob # allow nullglobing
  echo "${all_depending_artifact_components[@]}"
  for v in "${all_depending_artifact_components[@]}" "${component}"; do
    [[ -z "${v}" ]] && continue
    local files=(
      "${DIR}/"*"-${v}.inc" \
      "${DIR}/"*"-${v}-${OS}.inc" \
      "${DIR}/"*"-${v}-${OS}-${DISTRIBUTION}.inc" \
      "${DIR}/"*"-${v}-${OS}-${DISTRIBUTION}-${DISTRIBUTION_VER}.inc" \
      "${DIR}/build.sh" \
      "${DIR}/common-functions"
    )
    local f
    for f in  "${files[@]}"; do
      [ -f "$f" ] || continue
      cp -r "$f" "${temp_dir}/"
    done

    mkdir -p "${temp_dir}/patches"
    for f in "${DIR}/patches/${v}"*.patch; do
      cp -r "$f" "${temp_dir}/patches/"
    done
  done
  shopt -u nullglob # disallow nullglobing

  if [[ -z "${docker_context}" ]]; then
    docker_context="${temp_dir}"
  fi

  # Build Docker container
  docker build -t "${REPOSITORY}/${target_image}" -f "${temp_dir}"/Dockerfile "${docker_context}" || return 1
  if [[ "${PUSH_DOCKER_DEPS}" -eq 1 && "${is_dependency}" -eq 1 ]]; then
    docker push "${REPOSITORY}/${target_image}" || true
  fi

  if [[ "${KEEP_DOCKERFILE}" -eq 0 ]]; then
    # Clean-up docker build directory
    rm -rf "${temp_dir}"
  else
    echo "Preserved docker directory: ${temp_dir}"
  fi
}

check_os() {
  if [[ "$OSTYPE" == "linux-gnu" ]]; then
    # Host is Linux
    OS="linux"
    DISTRIBUTION="$(grep ^ID= /etc/os-release)"
    DISTRIBUTION=${DISTRIBUTION#"ID="}
    DISTRIBUTION_VER="$(grep ^VERSION_ID= /etc/os-release || echo "")"
    DISTRIBUTION_VER=${DISTRIBUTION_VER#"VERSION_ID="}
    # Remove `"`, if exists
    DISTRIBUTION_VER=${DISTRIBUTION_VER#\"}
    DISTRIBUTION_VER=${DISTRIBUTION_VER%\"}

  elif [[ "$OSTYPE" == "darwin"* ]]; then
    # Host is Mac OS X
    OS="osx"
    DISTRIBUTION="$(sw_vers -productName)"
    DISTRIBUTION_VER="$(sw_vers -productVersion)"
  else
    echo "Operating System unknown $OSTYPE"
    exit 1
  fi
  echo "Detected OS: ${OS}"
  echo "OS=${OS}"
  echo "Detected distribution: ${DISTRIBUTION}"
  echo "DISTRIBUTION=${DISTRIBUTION}"
  echo "Detected version: ${DISTRIBUTION_VER}"
  echo "DISTRIBUTION_VER=${DISTRIBUTION_VER}"
}


#
try_execute() {
  # Check if a function exists and execute it
  fct=$1
  # promote that function does not exist
  [[ "$(type -t "${fct}")" != "function" ]] && return 2

  # promote the return value of the function
  local failed=0
  "${fct}" || failed=1
  return "${failed}"
}

try_execute_if_exists() {
  # Check if a function exists and execute it
  fct=$1
  # Ignore if function does not exist
  [[ "$(type -t "${fct}")" != "function" ]] && return 0

  # promote the return value of the function
  local failed=0
  "${fct}" || failed=1
  return "${failed}"
}

# Execute the most system specific action possible.
# If an action executed successfuly, return 0
# The function will return 1 if an action was not successful
execution_action() {
    local action="$1"
    local component="$2"
    local found=0

    # Execute most specific action: os + distribution + distribution version
    if [[ -f "${DIR}/p-${component}-${OS}-${DISTRIBUTION}-${DISTRIBUTION_VER}.inc" ]]; then
        found=1
        source "${DIR}/p-${component}-${OS}-${DISTRIBUTION}-${DISTRIBUTION_VER}.inc"
        local failed=0
        try_execute "${action}"_"${component}" || failed=1
        [[ "$failed" -eq 0 ]] && return 0
    fi

    # Execute most specific action: os + distribution
    if [[ -f "${DIR}/p-${component}-${OS}-${DISTRIBUTION}.inc" ]]; then
        found=1
        source "${DIR}/p-${component}-${OS}-${DISTRIBUTION}.inc"
        local failed=0
        try_execute "${action}"_"${component}" || failed=1
        [[ "$failed" -eq 0 ]] && return 0
    fi

    # Execute most specific action: os
    if [[ -f "${DIR}/p-${component}-${OS}.inc" ]]; then
        found=1
        source "${DIR}/p-${component}-${OS}.inc";
        local failed=0
        try_execute "${action}"_"${component}" || failed=1
        [[ "$failed" -eq 0 ]] && return 0
    fi

    # Execute package specific action:
    if [[ -f "${DIR}/p-${component}.inc" ]]; then
        found=1
        source "${DIR}/p-${component}.inc";
        local failed=0
        try_execute "${action}"_"${component}" || failed=1
        [[ "$failed" -eq 0 ]] && return 0
    fi

    # Found an action file but something didn't work
    [[ "${found}" -eq 1 ]] && return 1

    # No action found
    return 2
}

##
# Returns a list that is the result of either an array or function call
get_list(){
  local list_name=$1
  local result=("")

  if [[ "$(type -t "${list_name}")" == "function" ]]; then
    mapfile -t result <<< "$("${list_name}")"
  elif [[ -n ${!list_name+x} ]]; then
    list_array="${list_name}[@]"
    result=("${!list_array}")
  fi

  for i in "${result[@]}"; do
    [[ -z "${i}" ]] && continue
    echo "${i}"
  done

  return 0
}

##
# Check if a lists exist
##
check_list(){
  local list_name=$1
  if [[ "$(type -t "${list_name}")" == "function" ]]; then
    return 0
  fi
  local list_array="${list_name}[@]"
  if [[ ${!list_array+x} ]]; then
    return 0
  fi
  echo "Variable or function \"${list_name}\" not found"
  exit 1
}

validate_required_variables() {
    local component="$1"
    local type="$2"
    local variables
    check_list required_variables_"${component}"
    mapfile -t variables <<< "$(get_list "${type}"_"${component}")"

    if [[ -n "${variables[*]}" ]]; then
        # Check if variables are defined
        for v in "${variables[@]}"; do
            if [[ -z ${!v+x} ]]; then
                echo "${v} Required but unset"
                exit 1
            fi
        done
    fi

    try_execute_if_exists required_variables_check_"${component}"
}

get_all_dependencies_rec() {
  local component="$1"
  local type="$2"
  local depending_components
  # Get the list of dependencies
  mapfile -t depending_components <<< "$(get_list "${type}"_dependency_"${component}")"
  for v in "${depending_components[@]}"; do
    [[ -z "${v}" ]] && continue
    get_all_dependencies_rec "$v" "$type"
  done
  echo "$component"
}

get_all_dependencies() {
  local component="$1"
  local type="$2"
  local depending_components=("")
  # Get the list of dependencies
  mapfile -t depending_components <<< "$(get_list "${type}"_dependency_"${component}")"
  local final_list=("${depending_components[@]}")
  for v in "${depending_components[@]}"; do
    [[ -z "${v}" ]] && continue
    local deps_list=("")
    local failed=0
    mapfile -t deps_list <<< "$(get_all_dependencies_rec "$v" "$type")"
    [[ "${failed}" -eq 1 ]] && continue

    # Make sure items are unique and keep the order of occurence
    for di in "${deps_list[@]}"; do
      local found=0
      for f in "${final_list[@]}"; do
        [[ -z "$f" ]] && continue
        [[ "$di" == "$f" ]] && found=1 && break
      done
      [[ "$found" -eq 0 ]] && final_list+=("${di}")
    done
  done

  for v in "${final_list[@]}"; do
    echo "${v}"
  done
}

gather_dependencies_rec() {
  local component="$1"
  local type="$2"
  local action="$3"

  load_component "${component}" "v" || { echo "Loading component failed ${component}"; return 1; }
  validate_required_variables "${component}" "required_variables"

  # Get the list of dependencies
  local variables
  check_list "${type}"_dependency_"${component}"
  mapfile -t variables <<< "$(get_list "${type}"_dependency_"${component}")"
  for v in "${variables[@]}"; do
    [[ -z "${v}" ]] && continue
    gather_dependencies_rec "$v" "${type}" "${action}"
  done

  try_execute_if_exists "${action}"_"${component}"
}

gather_dependencies() {
  local component="$1"
  local dependency_type="$2"
  local action="$3"

  local depending_components
  # Get the list of dependencies
  check_list "${dependency_type}"_dependency_"${component}"
  mapfile -t depending_components <<< "$(get_list "${dependency_type}"_dependency_"${component}")"

  for v in "${depending_components[@]}"; do
    [[ -z "${v}" ]] && continue
    gather_dependencies_rec "$v" "${dependency_type}" "${action}"
  done
  try_execute_if_exists "${action}"_"${component}"
}

check_module() {
  local module="$1"
  # Validate module if different functions or arrays exist
  (
    load_component "${module}" "v" || return 1
    list_or_functions=(
      required_variables
      artifact_dependency
    )
    for item in "${list_or_functions[@]}"; do
      check_list "${item}_${module}"
    done

  ) || { echo "Module $1 not valid"; return 1; }

  # Validate build instructions
  (
    validate_component "${module}" "p" || { echo "No build instructions found for ${module}"; return 0; }
    load_component "${module}" "p"

    list_or_functions=(
      setup_build_variables
      download
      build
      install
      is_installed
      setup_artifact_variables
      get_build_artifacts
      get_docker_config_id
    )
    for item in "${list_or_functions[@]}"; do
      check_list "${item}_${module}"
    done

  ) || { echo "Build instructions for ${module} are not valid"; return 1; }

}

##
## Install the specified component
install_component() {
  local component="$1"

  # Load general component information
  load_component "${component}" "v" || { echo "Loading component failed ${component}"; return 1; }

  # Get the list of dependencies
  local depending_artifact_components
  check_list artifact_dependency_"${component}"
  mapfile -t depending_artifact_components <<< "$(get_list artifact_dependency_"${component}")"

  # Make sure an artifact is available for the depending component
  for v in "${depending_artifact_components[@]}"; do
    [[ -z "${v}" ]] && continue
    install_component "${v}"
  done

  # Setup general component variables
  validate_required_variables "${component}" "required_variables"

  # Handle dependencies of required artifacts
  gather_dependencies "${component}" "artifact" "setup_variables"

  # Check if the artifact is installed already
  execution_action is_installed "${component}" && execution_action setup_artifact_variables "${component}" && validate_required_variables "${component}"  "export_variables" && { echo "Already installed ${component}"; return 0; }

  # Install package if available
  execution_action install_binary_artifact "${component}" && execution_action setup_artifact_variables "${component}" && validate_required_variables "${component}"  "export_variables" && { echo "Package installed ${component}"; return 0 ;}

  # Validate general build variables
  validate_build_variables

  # Check if there is build information, if not - this is a meta package, return
  validate_component "${component}" "p" || return 0

  # Load build description
  load_component "${component}" "p" || { echo "Loading component failed ${component}"; return 1; }

  # Setup build variables
  setup_build_variables_"${component}"

  # Check and install runtime-dependencies if needed
  execution_action "install_runtime_dependencies" "${component}" || true

  # Validate configuration
  try_execute_if_exists validate_build_config_"${component}"

  # Install build dependencies
  if [[ "${INSTALL_SYSTEM_DEPS}" == "1" ]]; then
    execution_action "install_build_dependencies" "${component}" || {
      echo "Could not install build dependencies for ${component}";
      return 1;
    }
  fi

  # Install build source
  try_execute_if_exists download_"${component}"

  # Build the package
  try_execute_if_exists build_"${component}"

  # Install to its final destination
  try_execute_if_exists install_"${component}"
}


main() {
  local NAME
  NAME=$(basename "${0}")
  local COMPONENTS=()
  local BUILD_DOCKER=0
  local INSTALL_SYSTEM_DEPS=0
  local KEEP_DOCKERFILE=0
  local CHECK_MODULE=0
  local PUSH_DOCKER_DEPS=0
  local CREATE_FINAL_IMAGE=0

  for i in "$@" ;do
  case $i in
    --debug)
      set -x
    ;;
    --docker)
    BUILD_DOCKER=1
    ;;
    --keep-dockerfile)
      KEEP_DOCKERFILE=1
    ;;
    --push-docker-deps)
      PUSH_DOCKER_DEPS=1
    ;;
    --create-final-image)
      CREATE_FINAL_IMAGE=1
    ;;
    --install-system-deps)
      INSTALL_SYSTEM_DEPS=1
    ;;
    --help)
      show_help "${NAME}"
      exit 0
    ;;
    --check-os)
      # Checks the operating sytem used
      check_os
      exit 0
    ;;
    --check-module)
      # Check the validity of the module
      CHECK_MODULE=1
    ;;
    -*)
      echo "${NAME}: unknown argument: $i"
      show_help "${NAME}"
      exit 1
    ;;
    *)
      COMPONENTS+=("$i")
    ;;
  esac
  done

  if [[ ${#COMPONENTS[@]} -eq 0 ]]; then
    show_help "${NAME}"
    exit 1
  fi


  if [[ "${CHECK_MODULE}" -eq 1 ]]; then
    for m in "${COMPONENTS[@]}"; do
      check_module "${m}"
    done
    exit 0
  fi

  # Validate selected components: check if they have a v-"component".inc file
  for c in "${COMPONENTS[@]}"; do
    validate_component "$c" "v"
  done

  if [[ "${BUILD_DOCKER}" == "1" ]]; then
    if [[ -z ${BASE_IMAGE+X} ]]; then
      echo "No BASE_IMAGE provided"
      exit 1
    fi
    if [[ -z ${REPOSITORY+X} ]]; then
      echo "No REPOSITORY provided"
      exit 1
    fi
    
    # Gather information about the base image
    check_docker_os "${BASE_IMAGE}"

    # Install components
    for c in "${COMPONENTS[@]}"; do
      build_docker "$c" 0 || { echo "Building docker image ${c} failed"; return 1; }
    done
  else
    # General Setup
    check_os

    # Install components
    for c in "${COMPONENTS[@]}"; do
      install_component "$c"
      try_execute_if_exists check_export_variables_"$c"
    done
  fi
}

main "$@"
