# Meta-package for solvers
required_variables_solvers=(
  "SOLVERS"
)

required_variables_check_solvers() {
  local solvers
  IFS=":" read -r -a solvers <<< "${SOLVERS}"
  [[ -z "${solvers[*]}" ]] && { echo "SOLVERS can't be empty"; exit 1; }
  
  for solver in "${solvers[@]}"; do
    [[ -z "${solver}" ]] && continue
    solver="$(to_lower ${solver})"

    [[ "${solver}" == "z3" ]] && continue
    [[ "${solver}" == "stp" ]] && continue
    [[ "${solver}" == "metasmt" ]] && continue
    
    echo "Unknown solver: \"$solver\""
    exit 1
  done
}

# On which artifacts does SOLVERS depend on
artifact_dependency_solvers() {
  # Setup variables first to get selected solvers
  setup_variables_solvers
  for solver in "${SELECTED_SOLVERS[@]}"; do
    echo "${solver}"
  done
}

# On which artifacts does SOLVERS depend on
runtime_artifact_dependency_solvers() {
  # Setup variables first to get selected solvers
  setup_variables_solvers
  for solver in "${SELECTED_SOLVERS[@]}"; do
    echo "${solver}"
  done
}

# Setup general variables
setup_variables_solvers() {
  local solvers
  IFS=":" read -r -a solvers <<< "${SOLVERS}"

  SELECTED_SOLVERS=()
  for solver in "${solvers[@]}"; do
    [[ -z "${solver}" ]] && continue
    solver="$(to_lower ${solver})"
    [[ "${solver}" == "z3" ]] && SELECTED_SOLVERS+=("z3") && continue
    [[ "${solver}" == "stp" ]] && SELECTED_SOLVERS+=("stp") && continue
    [[ "${solver}" == "metasmt" ]] && SELECTED_SOLVERS+=("metasmt") && continue

    echo "Unknown solver: \"$solver\""
    exit 1
  done

  # Sort solvers
  SELECTED_SOLVERS=( $( for e in "${SELECTED_SOLVERS[@]}"; do echo "$e"; done | sort|uniq) )
# TODO Add specific suffix of each solver
  SOLVER_SUFFIX=$(IFS=_ ; echo "${SELECTED_SOLVERS[*]}")
}