get_docker_config_id_clang() {
  if [[ "${LLVM_VERSION_SHORT}" -ge "11" ]]; then
    echo ""
    return 0;
  else
    return 1;
  fi
}
