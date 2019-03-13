# Build information for KLEE
required_variables_klee=(
  "COVERAGE"
  "USE_TCMALLOC"
  "USE_LIBCXX"
)

required_variables_check_klee() {
  check_bool "COVERAGE"
  check_bool "USE_TCMALLOC"
  check_bool "USE_LIBCXX"
}

# On which artifacts does KLEE depend on
artifact_dependency_klee(){
  local dependencies=("llvm" "solvers" "gtest")
  if [[ "${OS}" != "osx" ]]; then
    dependencies+=(uclibc)
  fi

  [[ "${USE_TCMALLOC}" -eq 1 ]] && dependencies+=("tcmalloc")
  [[ "${USE_LIBCXX}" -eq 1 ]] && dependencies+=("libcxx")

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

  [[ "${USE_TCMALLOC}" -eq 1 ]] && dependencies+=("tcmalloc")
  [[ "${USE_LIBCXX}" -eq 1 ]] && dependencies+=("libcxx")

  for d in "${dependencies[@]}"; do
    echo "${d}"
  done
}
