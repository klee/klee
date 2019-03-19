FROM klee/llvm:60_O_D_A_ubuntu_xenial-20181005 as llvm_base
FROM klee/gtest:1.7.0_ubuntu_xenial-20181005 as gtest_base
FROM klee/uclibc:klee_uclibc_v1.2_60_ubuntu_xenial-20181005 as uclibc_base
FROM klee/tcmalloc:2.7_ubuntu_xenial-20181005 as tcmalloc_base
FROM klee/stp:2.3.3_ubuntu_xenial-20181005 as stp_base
FROM klee/z3:4.8.4_ubuntu_xenial-20181005 as z3_base
FROM klee/libcxx:60_ubuntu_xenial-20181005 as libcxx_base
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
COPY . /tmp/klee_src/
RUN /tmp/klee_src//scripts/build/build.sh --debug --install-system-deps klee
LABEL maintainer="KLEE Developers"
# TODO remove adding sudo package
# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the TravisCI scripts)
RUN apt update && apt -y --no-install-recommends install sudo && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m klee && \
    echo klee:klee | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo 'klee  ALL=(root) NOPASSWD: ALL' >> /etc/sudoers

ENV BASE=/tmp
# Copy across source files needed for build
# Set klee user to be owner
ADD --chown=klee:klee / ${BASE}/klee_src

USER klee
WORKDIR /home/klee

# Add KLEE binary directory to PATH
RUN /bin/bash -c 'ln -s ${BASE}/klee_src /home/klee/ && ln -s ${BASE}/klee_build* /home/klee/klee_build && echo "export PATH=\"$PATH:$(cd /tmp/llvm-*install*/bin && pwd):/home/klee/klee_build/bin\"" >> /home/klee/.bashrc'

# TODO Remove when STP is fixed
RUN /bin/bash -c 'echo "export LD_LIBRARY_PATH=$(cd ${BASE}/metaSMT-*-deps/stp-git-basic/lib/ && pwd)" >> /home/klee/.bashrc'
