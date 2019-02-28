install_build_dependencies_llvm() { 
	apt update -y

	dependencies=(
		build-essential

		autoconf
		automake
		groff
		gcc
		g++
		python-dev
		make
		subversion # Check out code
		zlib1g-dev
	)

	#Install essential dependencies
	apt -y --no-install-recommends install "${dependencies[@]}"
}

install_binary_artifact_llvm() {
  apt update -y
  
  dependencies=(
    "clang-${LLVM_VERSION}"
  )

  #Install essential dependencies
  apt -y --no-install-recommends install "${dependencies[@]}"
}

# Check if the binary artifact is installed
is_installed_llvm() {
	local lc=""
	# Check if llvm-config with the right version exists	
	lc=$(which "llvm-config-${LLVM_VERSION}")
	
	local rtti
	rtti="$(${lc} --has-rtti)"
	local assertion
	assertion="$(${lc} --assertion-mode)"
	local build_mode
	build_mode="$(${lc} --build-mode)"
	
	# Check requested mode with mode of the found item	
	[[ $(to_bool "${REQUIRES_RTTI}") -eq $(to_bool "${rtti}") ]] || return 1
	[[ $(to_bool "${DISABLE_ASSERTIONS}") -ne $(to_bool "${assertion}") ]] || return 1
}