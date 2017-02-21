#!/bin/bash -x

# Get Boost, Z3, libgmp
sudo apt-get -y install libboost1.55-dev libz3 libz3-dev libgmp-dev

# Clone
git clone git://github.com/hoangmle/metaSMT.git
cd metaSMT
## FIXME: define and use an environment variable METASMT_VERSION instead, when a proper version of metaSMT is available
git checkout 1f9e399
git clone git://github.com/agra-uni-bremen/dependencies.git
cd dependencies
git checkout 45bc658
cd ..

source ${KLEE_SRC}/.travis/sanitizer_flags.sh
if [ "X${IS_SANITIZED_BUILD}" != "X0" ]; then
  echo "Error: Requested Sanitized build but sanitized build of metaSMT is not implemented"
  exit 1
fi

# Bootstrap
export BOOST_ROOT=/usr
sudo cp dependencies/Z3-2.19/Z3Config.cmake /usr # this is a hack
./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off --build stp-git-basic --build boolector-2.2.0 --build minisat-git --build lingeling-ayv-86bf266-140429 -DZ3_DIR=/usr
sudo cp deps/boolector-2.2.0/lib/* /usr/lib/              #
sudo cp deps/lingeling-ayv-86bf266-140429/lib/* /usr/lib/ #
sudo cp deps/minisat-git/lib/* /usr/lib/                  # hack
sudo cp deps/stp-git-basic/lib/* /usr/lib/                #

# Build
make -C build install
