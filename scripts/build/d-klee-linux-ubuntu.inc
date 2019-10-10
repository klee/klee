# TODO remove adding sudo package
# Create ``klee`` user for container with password ``klee``.
# and give it password-less sudo access (temporarily so we can use the TravisCI scripts)
RUN apt update && apt -y --no-install-recommends install sudo file && \
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
ENV LD_LIBRARY_PATH /home/klee/klee_build/lib/

# Add KLEE binary directory to PATH
RUN /bin/bash -c 'ln -s ${BASE}/klee_src /home/klee/ && ln -s ${BASE}/klee_build* /home/klee/klee_build && echo "export PATH=\"$PATH:$(cd /tmp/llvm-*install*/bin && pwd):/home/klee/klee_build/bin\"" >> /home/klee/.bashrc'

# TODO Remove when STP is fixed
RUN /bin/bash -c 'echo "export LD_LIBRARY_PATH=$(cd ${BASE}/metaSMT-*-deps/stp-git-basic/lib/ && pwd):$LD_LIBRARY_PATH" >> /home/klee/.bashrc'
