# Build information for KLEE
required_variables_klee=(
  "COVERAGE"
  "USE_TCMALLOC"
)

required_variables_check_klee() {
  check_bool "COVERAGE"
  check_bool "USE_TCMALLOC"
}

# On which artifacts does KLEE depend on
artifact_dependency_klee(){
  local dependencies=("llvm" "solvers" "gtest")
  if [[ "${OS}" != "osx" ]]; then
    dependencies+=(uclibc)
  fi

  if [[ "${USE_TCMALLOC}" -eq 1 ]]; then
    dependencies+=("tcmalloc")
  fi

  for d in "${dependencies[@]}"; do
    echo "${d}"
  done
}

runtime_artifact_dependency_klee(){
  local dependencies=("")
  if [[ "${OS}" != "osx" ]]; then
    echo "uclibc"
  fi
  dependencies+=("llvm" "solvers" "gtest")

  if [[ "${USE_TCMALLOC}" -eq 1 ]]; then
    dependencies+=("tcmalloc")
  fi

  for d in "${dependencies[@]}"; do
    echo "${d}"
  done
}
