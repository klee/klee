FROM ubuntu:14.04
MAINTAINER Dan Liew <daniel.liew@imperial.ac.uk>

# FIXME: Docker doesn't currently offer a way to
# squash the layers from within a Dockerfile so
# the resulting image is unnecessarily large!

ENV LLVM_VERSION=3.4 \
    SOLVERS=STP:Z3 \
    STP_VERSION=2.1.2 \
    DISABLE_ASSERTIONS=0 \
    ENABLE_OPTIMIZED=1 \
    KLEE_UCLIBC=klee_uclibc_v1.0.0 \
    KLEE_SRC=/home/klee/klee_src \
    COVERAGE=0 \
    BUILD_DIR=/home/klee/klee_build \
    ASAN_BUILD=0 \
    UBSAN_BUILD=0 \
    TRAVIS_OS_NAME=linux

RUN apt-get update && \
    apt-get -y --no-install-recommends install \
        clang-${LLVM_VERSION} \
        llvm-${LLVM_VERSION} \
        llvm-${LLVM_VERSION}-dev \
        llvm-${LLVM_VERSION}-runtime \
        llvm \
        libcap-dev \
        git \
        subversion \
        cmake \
        make \
        libboost-program-options-dev \
        python3 \
        python3-dev \
        python3-pip \
        perl \
        flex \
        bison \
        libncurses-dev \
        zlib1g-dev \
        patch \
        wget \
        unzip \
        binutils && \
    pip3 install -U lit tabulate wllvm && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 50 && \
    ( wget -O - http://download.opensuse.org/repositories/home:delcypher:z3/xUbuntu_14.04/Release.key | apt-key add - ) && \
    echo 'deb http://download.opensuse.org/repositories/home:/delcypher:/z3/xUbuntu_14.04/ /' >> /etc/apt/sources.list.d/z3.list && \
    apt-get update

# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the TravisCI scripts)
RUN useradd -m klee && \
    echo klee:klee | chpasswd && \
    cp /etc/sudoers /etc/sudoers.bak && \
    echo 'klee  ALL=(root) NOPASSWD: ALL' >> /etc/sudoers
USER klee
WORKDIR /home/klee

# Copy across source files needed for build
RUN mkdir ${KLEE_SRC}
ADD / ${KLEE_SRC}

# Set klee user to be owner
RUN sudo chown --recursive klee: ${KLEE_SRC}

# Create build directory
RUN mkdir -p ${BUILD_DIR}

# Build/Install SMT solvers (use TravisCI script)
RUN cd ${BUILD_DIR} && ${KLEE_SRC}/.travis/solvers.sh

# Install testing utils (use TravisCI script)
RUN cd ${BUILD_DIR} && mkdir test-utils && cd test-utils && \
    ${KLEE_SRC}/.travis/testing-utils.sh

# FIXME: The current TravisCI script expects clang-${LLVM_VERSION} to exist
RUN sudo ln -s /usr/bin/clang /usr/bin/clang-${LLVM_VERSION} && \
    sudo ln -s /usr/bin/clang++ /usr/bin/clang++-${LLVM_VERSION}

# Build KLEE (use TravisCI script)
RUN cd ${BUILD_DIR} && ${KLEE_SRC}/.travis/klee.sh

# Revoke password-less sudo and Set up sudo access for the ``klee`` user so it
# requires a password
USER root
RUN mv /etc/sudoers.bak /etc/sudoers && \
    echo 'klee  ALL=(root) ALL' >> /etc/sudoers
USER klee

# FIXME: Shouldn't we just invoke the `install` target? This will
# duplicate some files but the Docker image is already pretty bloated
# so this probably doesn't matter.
# Add KLEE binary directory to PATH
RUN echo 'export PATH=$PATH:'${BUILD_DIR}'/klee/bin' >> /home/klee/.bashrc

# Link klee to /usr/bin so that it can be used by docker run
USER root
RUN for executable in ${BUILD_DIR}/klee/bin/* ; do ln -s ${executable} /usr/bin/`basename ${executable}`; done
USER klee
