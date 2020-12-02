# TODO remove adding sudo package
# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the CI scripts)
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
