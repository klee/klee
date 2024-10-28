FROM ghcr.io/klee/llvm:130_O_D_A_ubuntu_jammy-20230126 as llvm_base
FROM ghcr.io/klee/gtest:1.11.0_ubuntu_jammy-20230126 as gtest_base
FROM ghcr.io/klee/uclibc:klee_uclibc_v1.3_130_ubuntu_jammy-20230126 as uclibc_base
FROM ghcr.io/klee/tcmalloc:2.9.1_ubuntu_jammy-20230126 as tcmalloc_base
FROM ghcr.io/klee/stp:2.3.3_ubuntu_jammy-20230126 as stp_base
FROM ghcr.io/klee/z3:4.8.15_ubuntu_jammy-20230126 as z3_base
FROM ghcr.io/klee/libcxx:130_ubuntu_jammy-20230126 as libcxx_base
FROM ghcr.io/klee/sqlite:3400100_ubuntu_jammy-20230126 as sqlite3_base
FROM llvm_base as intermediate
COPY --from=gtest_base /tmp /tmp/
COPY --from=uclibc_base /tmp /tmp/
COPY --from=tcmalloc_base /tmp /tmp/
COPY --from=stp_base /tmp /tmp/
COPY --from=z3_base /tmp /tmp/
COPY --from=libcxx_base /tmp /tmp/
COPY --from=sqlite3_base /tmp /tmp/
ENV COVERAGE=0
ENV USE_TCMALLOC=1
ENV BASE=/tmp
ENV LLVM_VERSION=13.0
ENV ENABLE_DOXYGEN=1
ENV ENABLE_OPTIMIZED=1
ENV ENABLE_DEBUG=1
ENV DISABLE_ASSERTIONS=0
ENV REQUIRES_RTTI=0
ENV SOLVERS=STP:Z3
ENV GTEST_VERSION=1.11.0
ENV UCLIBC_VERSION=klee_uclibc_v1.3
ENV TCMALLOC_VERSION=2.9.1
ENV SANITIZER_BUILD=
ENV STP_VERSION=2.3.3
ENV MINISAT_VERSION=master
ENV Z3_VERSION=4.8.15
ENV USE_LIBCXX=1
ENV KLEE_RUNTIME_BUILD="Debug+Asserts"
ENV SQLITE_VERSION=3400100
LABEL maintainer="KLEE Developers"

# TODO remove adding sudo package
# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the CI scripts)
RUN apt update && DEBIAN_FRONTEND=noninteractive apt -y --no-install-recommends install sudo less emacs-nox vim-nox file python3-dateutil && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m klee && \
    echo klee:klee | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo 'klee  ALL=(root) NOPASSWD: ALL' >> /etc/sudoers

# Copy across source files needed for build
COPY --chown=klee:klee . /tmp/klee_src/

USER klee
WORKDIR /home/klee
# Build and set klee user to be owner
RUN /tmp/klee_src/scripts/build/build.sh --debug --install-system-deps klee && pip3 install flask wllvm && \
    sudo rm -rf /var/lib/apt/lists/*


ENV PATH="$PATH:/tmp/llvm-130-install_O_D_A/bin:/home/klee/klee_build/bin:/home/klee/.local/bin"
ENV BASE=/tmp
# Add path to local LLVM installation - let system install precede local install
RUN /bin/bash -c 'echo "export \"PATH=$PATH:$(cd ${BASE}/llvm-*-install*/bin/ && pwd)\" >> /home/klee/.bashrc"'

# Add KLEE header files to system standard include folder
RUN sudo /bin/bash -c 'ln -s /tmp/klee_src/include/klee /usr/include/'

ENV LD_LIBRARY_PATH /home/klee/klee_build/lib/

# Add KLEE binary directory to PATH
RUN /bin/bash -c 'ln -s ${BASE}/klee_src /home/klee/ && ln -s ${BASE}/klee_build* /home/klee/klee_build'

# TODO Remove when STP is fixed
RUN /bin/bash -c 'echo "export LD_LIBRARY_PATH=$(cd ${BASE}/metaSMT-*-deps/stp-git-basic/lib/ && pwd):$LD_LIBRARY_PATH" >> /home/klee/.bashrc'