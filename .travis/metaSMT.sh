#!/bin/bash -x

# Get Boost, Z3, libgmp
sudo apt-get -y install libboost1.55-dev libz3 libz3-dev libgmp-dev

# Clone
git clone git://github.com/hoangmle/metaSMT.git
cd metaSMT
git clone git://github.com/agra-uni-bremen/dependencies.git

# Bootstrap
export BOOST_ROOT=/usr
sudo cp dependencies/Z3-2.19/Z3Config.cmake /usr # this is a hack
./bootstrap.sh -d deps -m RELEASE build -DmetaSMT_ENABLE_TESTS=off --build stp-git-basic --build boolector-1.5.118 --build minisat-git -DZ3_DIR=/usr
sudo cp deps/boolector-1.5.118/lib/* /usr/lib/ #
sudo cp deps/minisat-git/lib/* /usr/lib/       # hack
sudo cp deps/stp-git-basic/lib/* /usr/lib/     #

# Build
make -C build install
