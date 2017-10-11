#!/bin/bash -x

set -e

: ${METASMT_VERSION?"METASMT_VERSION not specified"}

# Get Z3, libgmp, gperf (required by yices2)
sudo apt-get -y install libz3 libz3-dev libgmp-dev gperf

# Get Boost
if [ "X${METASMT_BOOST_VERSION}" != "X" ]; then
  # Taken from boost/hana/.travis.yml
  BOOST_URL="http://sourceforge.net/projects/boost/files/boost/${METASMT_BOOST_VERSION}/boost_${METASMT_BOOST_VERSION//\./_}.tar.gz"
  BOOST_DIR=`pwd`/boost-${METASMT_BOOST_VERSION}
  mkdir -p ${BOOST_DIR}
  wget --quiet -O - ${BOOST_URL} | tar --strip-components=1 -xz -C ${BOOST_DIR}
  sudo ln -s ${BOOST_DIR}/boost /usr/include/boost
else
  sudo apt-get -y install libboost1.55-dev
fi

# Get CVC4
wget http://www.informatik.uni-bremen.de/agra/systemc-verification/media/snapshots/cvc4-594301e.tar.gz
tar -xf cvc4-594301e.tar.gz
sudo mv cvc4 /usr

# Clone
git clone -b ${METASMT_VERSION} --single-branch --depth 1 https://github.com/hoangmle/metaSMT.git
cd metaSMT
git submodule update --init

source ${KLEE_SRC}/.travis/sanitizer_flags.sh
if [ "X${IS_SANITIZED_BUILD}" != "X0" ]; then
  echo "Error: Requested Sanitized build but sanitized build of metaSMT is not implemented"
  exit 1
fi

# Bootstrap
export BOOST_ROOT=/usr
sudo cp dependencies/Z3-2.19/Z3Config.cmake /usr # this is a hack
./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off --build stp-git-basic --build boolector-2.2.0 --build minisat-git --build lingeling-ayv-86bf266-140429 --build yices-2.5.1 -DZ3_DIR=/usr -DCVC4_DIR=/usr/cvc4
sudo cp deps/boolector-2.2.0/lib/* /usr/lib/              #
sudo cp deps/lingeling-ayv-86bf266-140429/lib/* /usr/lib/ #
sudo cp deps/minisat-git/lib/* /usr/lib/                  # hack
sudo cp -r deps/stp-git-basic/lib/lib* /usr/lib/          #
sudo cp deps/yices-2.5.1/lib/* /usr/lib/                  #

# Build
make -C build install
