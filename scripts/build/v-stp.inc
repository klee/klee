# Build information for STP solver
required_variables_stp=(
  "STP_VERSION"
  "MINISAT_VERSION"
)

# On which artifacts does STP depend on
artifact_dependency_stp=("sanitizer")

artifact_dependency_stp() {
  echo "sanitizer"
  if [[ "${SANITIZER_BUILD:-}" == "memory" ]]; then
    echo "llvm"
  fi
}