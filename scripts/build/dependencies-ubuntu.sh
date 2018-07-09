#!/bin/bash
# Installs ubuntu dependencies
set -e

# Update packages
apt update

# Install essential build tools
apt -y --no-install-recommends install build-essential software-properties-common wget

# Add repository for additional compilers
add-apt-repository ppa:ubuntu-toolchain-r/test -y
add-apt-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial main"
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key| apt-key add -
apt update -y

#Install essential dependencies
apt -y --no-install-recommends install \
  binutils \
  bison \
  cmake \
  curl \
  flex \
  git \
  groff-base \
  libboost-program-options-dev \
  libncurses-dev \
  ninja-build \
  patch \
  perl \
	python \
  python3-dev \
  python3-pip \
	python3-setuptools \
  subversion \
	sudo \
  unzip \
  wget \
  zlib1g-dev
apt clean

# Install lit for testing
pip3 install wheel && pip3 install lit
