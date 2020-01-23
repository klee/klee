FROM klee/llvm:60_O_D_A_ubuntu_bionic-20200112 as llvm_base
FROM klee/gtest:1.7.0_ubuntu_bionic-20200112 as gtest_base
FROM klee/uclibc:klee_uclibc_v1.2_60_ubuntu_bionic-20200112 as uclibc_base
FROM klee/tcmalloc:2.7_ubuntu_bionic-20200112 as tcmalloc_base
FROM klee/stp:2.3.3_ubuntu_bionic-20200112 as stp_base
FROM klee/z3:4.8.4_ubuntu_bionic-20200112 as z3_base
FROM klee/libcxx:60_ubuntu_bionic-20200112 as libcxx_base
FROM llvm_base as intermediate
COPY --from=gtest_base /tmp /tmp/
COPY --from=uclibc_base /tmp /tmp/
COPY --from=tcmalloc_base /tmp /tmp/
COPY --from=stp_base /tmp /tmp/
COPY --from=z3_base /tmp /tmp/
COPY --from=libcxx_base /tmp /tmp/
ENV COVERAGE=0
ENV USE_TCMALLOC=1
ENV BASE=/tmp
ENV LLVM_VERSION=6.0
ENV ENABLE_OPTIMIZED=1
ENV ENABLE_DEBUG=1
ENV DISABLE_ASSERTIONS=0
ENV REQUIRES_RTTI=0
ENV SOLVERS=STP:Z3
ENV GTEST_VERSION=1.7.0
ENV UCLIBC_VERSION=klee_uclibc_v1.2
ENV LLVM_VERSION=6.0
ENV TCMALLOC_VERSION=2.7
ENV SANITIZER_BUILD=
ENV LLVM_VERSION=6.0
ENV STP_VERSION=2.3.3
ENV MINISAT_VERSION=master
ENV Z3_VERSION=4.8.4
ENV USE_LIBCXX=1
ENV KLEE_RUNTIME_BUILD="Debug+Asserts"
LABEL maintainer="KLEE Developers"


# TODO remove adding sudo package
# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the TravisCI scripts)
RUN apt update && DEBIAN_FRONTEND=noninteractive apt -y --no-install-recommends install sudo emacs-nox vim-nox file python3-dateutil && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m klee && \
    echo klee:klee | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo 'klee  ALL=(root) NOPASSWD: ALL' >> /etc/sudoers

# Copy across source files needed for build
COPY --chown=klee:klee . /tmp/klee_src/

# Build and set klee user to be owner
RUN /tmp/klee_src/scripts/build/build.sh --debug --install-system-deps klee && chown -R klee:klee /tmp/klee_build* && pip3 install flask wllvm && \
    rm -rf /var/lib/apt/lists/*

ENV PATH="$PATH:/tmp/llvm-60-install_O_D_A/bin:/home/klee/klee_build/bin"
ENV BASE=/tmp

USER klee
WORKDIR /home/klee
ENV LD_LIBRARY_PATH /home/klee/klee_build/lib/

# Add KLEE binary directory to PATH
RUN /bin/bash -c 'ln -s ${BASE}/klee_src /home/klee/ && ln -s ${BASE}/klee_build* /home/klee/klee_build' 

# TODO Remove when STP is fixed
RUN /bin/bash -c 'echo "export LD_LIBRARY_PATH=$(cd ${BASE}/metaSMT-*-deps/stp-git-basic/lib/ && pwd):$LD_LIBRARY_PATH" >> /home/klee/.bashrc'
