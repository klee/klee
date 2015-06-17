FROM ubuntu:14.04
MAINTAINER Dan Liew <daniel.liew@imperial.ac.uk>

# FIXME: Docker doesn't currently offer a way to
# squash the layers from within a Dockerfile so
# the resulting image is unnecessarily large!

ENV LLVM_VERSION=3.4 \
    STP_VERSION=UPSTREAM \
    DISABLE_ASSERTIONS=0 \
    ENABLE_OPTIMIZED=1 \
    KLEE_UCLIBC=1 \
    KLEE_SRC=/home/klee/klee_src \
    COVERAGE=0 \
    BUILD_DIR=/home/klee/klee_build

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
    pip3 install -U lit tabulate && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 50

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
ADD configure \
    LICENSE.TXT \
    Makefile \
    Makefile.* \
    README.md \
    TODO.txt \
    ${KLEE_SRC}/
ADD .travis ${KLEE_SRC}/.travis/
ADD autoconf ${KLEE_SRC}/autoconf/
ADD docs ${KLEE_SRC}/docs/
ADD include ${KLEE_SRC}/include/
ADD lib ${KLEE_SRC}/lib/
ADD runtime ${KLEE_SRC}/runtime/
ADD scripts ${KLEE_SRC}/scripts/
ADD test ${KLEE_SRC}/test/
ADD tools ${KLEE_SRC}/tools/
ADD unittests ${KLEE_SRC}/unittests/
ADD utils ${KLEE_SRC}/utils/

# Set klee user to be owner
RUN sudo chown --recursive klee: ${KLEE_SRC}

# Create build directory
RUN mkdir -p ${BUILD_DIR}

# Build STP (use TravisCI script)
RUN cd ${BUILD_DIR} && mkdir stp && cd stp && ${KLEE_SRC}/.travis/stp.sh

# Install testing utils (use TravisCI script)
RUN cd ${BUILD_DIR} && mkdir testing-utils && cd testing-utils && \
    ${KLEE_SRC}/.travis/testing-utils.sh

# FIXME: This is a nasty hack so KLEE's configure and build finds
# LLVM's headers file, libraries and tools
RUN sudo mkdir -p /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin && \
    sudo ln -s /usr/bin/llvm-config /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/llvm-config && \
    sudo ln -s /usr/bin/llvm-dis /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/llvm-dis && \
    sudo ln -s /usr/bin/llvm-as /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/llvm-as && \
    sudo ln -s /usr/bin/llvm-link /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/llvm-link && \
    sudo ln -s /usr/bin/llvm-ar /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/llvm-ar && \
    sudo ln -s /usr/bin/opt /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/opt && \
    sudo ln -s /usr/bin/lli /usr/lib/llvm-${LLVM_VERSION}/build/Release/bin/lli && \
    sudo mkdir -p /usr/lib/llvm-${LLVM_VERSION}/build/include && \
    sudo ln -s /usr/include/llvm-${LLVM_VERSION}/llvm /usr/lib/llvm-${LLVM_VERSION}/build/include/llvm && \
    sudo ln -s /usr/include/llvm-c-${LLVM_VERSION}/llvm-c /usr/lib/llvm-${LLVM_VERSION}/build/include/llvm-c && \
    for static_lib in /usr/lib/llvm-3.4/lib/*.a ; do sudo ln -s ${static_lib} /usr/lib/`basename ${static_lib}`; done

# FIXME: This is **really gross**. The Official Ubuntu LLVM packages don't ship
# with ``FileCheck`` or the ``not`` tools so we have to hack building these
# into KLEE's build system in order for the tests to pass
RUN cd ${KLEE_SRC}/tools && \
    for tool in FileCheck not; do \
        svn export \
        http://llvm.org/svn/llvm-project/llvm/branches/release_34/utils/${tool} ${tool} ; \
        sed -i 's/^USEDLIBS.*$/LINK_COMPONENTS = support/' ${tool}/Makefile; \
    done && \
    sed -i '0,/^PARALLEL_DIRS/a PARALLEL_DIRS += FileCheck not' Makefile

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

# Add KLEE binary directory to PATH
RUN echo 'export PATH=$PATH:'${BUILD_DIR}'/klee/Release+Asserts/bin' >> /home/klee/.bashrc
