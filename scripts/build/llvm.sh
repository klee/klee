#!/bin/bash -x
set -ev
STAGE="$1"

DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

if [[ "${LLVM_VERSION_SHORT}" == "" ]]; then
  echo "LLVM_VERSION_SHORT not set"
  exit 1
fi

if [[ "${BASE}" == "" ]]; then
  echo "BASE not set"
  exit 1
fi

# Install packages if possible
if [[ "${PACKAGED}x" == "1x" ]]; then
  apt update
  apt install -y llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev
  apt install -y clang-${LLVM_VERSION}
  exit 0
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  LLVM_BASE="${BASE}/llvm-${LLVM_VERSION_SHORT}"
  # Checkout LLVM code
  svn co http://llvm.org/svn/llvm-project/llvm/branches/release_${LLVM_VERSION_SHORT} "${LLVM_BASE}"
  cd "${LLVM_BASE}/tools"
  svn co http://llvm.org/svn/llvm-project/cfe/branches/release_${LLVM_VERSION_SHORT} clang
  cd "${LLVM_BASE}/projects"
  svn co http://llvm.org/svn/llvm-project/compiler-rt/branches/release_${LLVM_VERSION_SHORT} compiler-rt

  if [[ ${LLVM_VERSION_SHORT} -gt 37 ]]; then
    cd "${LLVM_BASE}/projects"
    svn co http://llvm.org/svn/llvm-project/libcxx/branches/release_${LLVM_VERSION_SHORT} libcxx
    cd "${LLVM_BASE}/projects"
    svn co http://llvm.org/svn/llvm-project/libcxxabi/branches/release_${LLVM_VERSION_SHORT} libcxxabi
  fi

  # Apply existing patches if needed
  if [ -f "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch" ]; then
     cd "${LLVM_BASE}"
     patch -p0 -i "${DIR}/patches/llvm${LLVM_VERSION_SHORT}.patch"
  fi
fi


# For memory sanitizer, we have a multi-stage build process
if [[ "${SANITIZER_BUILD}" == "memory" ]]; then
   if [[ ${LLVM_VERSION_SHORT} -le 37 ]]; then
     echo "Memory sanitizer builds for <= LLVM 3.7 are not supported"
     exit 1
   fi
   # Build uninstrumented compiler
   mkdir "${SANITIZER_LLVM_UNINSTRUMENTED}"
   cd "${SANITIZER_LLVM_UNINSTRUMENTED}"
   cmake -GNinja -DCMAKE_BUILD_TYPE=Release "${LLVM_BASE}"
   ninja

   # Build instrumented libc/libc++
   mkdir "${SANITIZER_LLVM_LIBCXX}"
   cd "${SANITIZER_LLVM_LIBCXX}"
   cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DLLVM_USE_SANITIZER=MemoryWithOrigins \
    ${SANITIZER_CMAKE_C_COMPILER} \
    ${SANITIZER_CMAKE_CXX_COMPILER} \
    "${LLVM_BASE}"
   ninja cxx cxxabi

   # Build instrumented clang
   mkdir "${LLVM_BUILD}"
   cd "${LLVM_BUILD}"
   cmake -GNinja \
      ${SANITIZER_CMAKE_C_COMPILER} \
      ${SANITIZER_CMAKE_CXX_COMPILER} \
      -DCMAKE_C_FLAGS="$SANITIZER_C_FLAGS" \
      -DCMAKE_CXX_FLAGS="$SANITIZER_CXX_FLAGS" \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_USE_SANITIZER=MemoryWithOrigins \
      -DLLVM_ENABLE_LIBCXX=ON \
      -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_LD_FLAGS" \
      -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL}" \
      "${LLVM_BASE}"
  # Build clang as a dependency and install all needed packages
  ninja clang
  ninja install-clang install-llvm-config install-llvm-objdump \
      install-llvm-link install-llvm-ar install-llvm-nm install-llvm-dis \
      install-clang-headers install-llvm-as installhdrs install-LLVMX86Disassembler \
      install-LLVMX86AsmParser install-LLVMX86CodeGen install-LLVMSelectionDAG \
      install-LLVMAsmPrinter install-LLVMX86Desc install-LLVMMCDisassembler \
      install-LLVMX86Info install-LLVMX86AsmPrinter install-LLVMX86Utils \
      install-LLVMMCJIT install-LLVMExecutionEngine install-LLVMRuntimeDyld \
      install-LLVMipo install-LLVMVectorize install-LLVMLinker install-LLVMIRReader \
      install-LLVMAsmParser install-LLVMCodeGen install-LLVMTarget install-LLVMScalarOpts \
      install-LLVMInstCombine install-LLVMInstrumentation install-LLVMProfileData \
      install-LLVMObject install-LLVMMCParser install-LLVMTransformUtils install-LLVMMC \
      install-LLVMAnalysis install-LLVMBitWriter install-LLVMBitReader install-LLVMCore \
      install-llvm-symbolizer install-LLVMSupport install-lli not FileCheck
  cp "${LLVM_BUILD}/bin/FileCheck" "${LLVM_INSTALL}/bin/"
  cp "${LLVM_BUILD}/bin/not" "${LLVM_INSTALL}/bin/"
  exit 0
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
  # Configure; build; and install
  mkdir -p "${LLVM_BUILD}"
  cd "${LLVM_BUILD}"

  # Skip building if already finished
  if [[ -e "${LLVM_BUILD}/.build_finished" ]]; then
    exit 0
  fi


  # Configure LLVM
  if [[ ${LLVM_VERSION_SHORT} -le 37 ]]; then
    CONFIG=(--enable-jit --prefix="${LLVM_INSTALL}")
    if [[ "${ENABLE_OPTIMIZED}" == "1" ]]; then
      CONFIG+=(--enable-optimized)
    else
      CONFIG+=(--disable-optimized)
    fi

    if [[ "${DISABLE_ASSERTIONS}" == "1" ]]; then
      CONFIG+=(--disable-assertions)
    else
      CONFIG+=(--enable-assertions)
    fi

    if [[ "${ENABLE_DEBUG}" == "1" ]]; then
      CONFIG+=(--enable-debug-runtime --enable-debug-symbols)
    else
      CONFIG+=(--disable-debug-symbols)
    fi
    CC=${CC} CXX=${CXX} CFLAGS="${LLVM_CFLAGS}" CXXFLAGS="${LLVM_CXXFLAGS}" LDFLAGS="${LLVM_LDFLAGS}" "${LLVM_BASE}/configure" "${CONFIG[@]}"
  else
    CONFIG=(-DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL}")
    # cmake build
    if [[ "${ENABLE_OPTIMIZED}" == "1" && "${ENABLE_DEBUG}" != "1" ]]; then
      CONFIG+=(-DCMAKE_BUILD_TYPE=Release)
    fi
    if [[ "${ENABLE_OPTIMIZED}" == "1" && "${ENABLE_DEBUG}" == "1" ]]; then
      CONFIG+=(-DCMAKE_BUILD_TYPE=RelWithDebInfo)
    fi
    if [[ "${ENABLE_OPTIMIZED}" != "1" && "${ENABLE_DEBUG}" == "1" ]]; then
      CONFIG+=(-DCMAKE_BUILD_TYPE=Debug)
    fi

    if [[ "${DISABLE_ASSERTIONS}" == "1" ]]; then
      CONFIG+=(-DLLVM_ENABLE_ASSERTIONS=Off)
    else
      CONFIG+=(-DLLVM_ENABLE_ASSERTIONS=On)
    fi

    if [[ ! -z "${LLVM_CFLAGS}" ]] ; then
      CONFIG+=(-DCMAKE_C_FLAGS="$LLVM_CFLAGS")
    fi

    if [[ ! -z "${LLVM_CXXFLAGS}" ]] ; then
      CONFIG+=(-DCMAKE_CXX_FLAGS="$LLVM_CXXFLAGS")
    fi

    if [[ ! -z "${LLVM_LDFLAGS}" ]]; then
       LDFLAGS="${LLVM_LDFLAGS}"
    fi

    cmake "${CONFIG[@]}" "${LLVM_BASE}"
  fi

  make -j$(nproc)
  make install

  touch "${LLVM_BUILD}/.build_finished"

  cp "${LLVM_BUILD_BIN}/FileCheck" "${LLVM_INSTALL}/bin/"
  cp "${LLVM_BUILD_BIN}/not" "${LLVM_INSTALL}/bin/"

  if [[ "${KEEP_BUILD}x" != "1x" ]]; then
    rm -rf "${LLVM_BUILD}"
  fi

  if [[ "${KEEP_SRC}x" != "1x" ]]; then
    rm -rf "${LLVM_BASE}"
  fi
elif [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
  # We use our own local cache if possible
  set +e
  brew install "$HOME"/Library/Caches/Homebrew/llvm\@${LLVM_VERSION}*.bottle.tar.gz
  RES=$?
  set -ev
  if [ ${RES} -ne 0 ]; then
    # This might build the llvm package use a time out to avoid dumping log information
    brew install -v --build-bottle "llvm@${LLVM_VERSION}"
    # Now bottle the brew
    brew bottle llvm\@${LLVM_VERSION}
    # Not make sure it's cached
    cp llvm\@${LLVM_VERSION}*.bottle.tar.gz $HOME/Library/Caches/Homebrew/
  fi
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi
