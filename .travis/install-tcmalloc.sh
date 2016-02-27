#!/bin/bash -x
set -ev

if [ ${USE_TCMALLOC:-0} -eq 1 ] ; then
	# Get tcmalloc release
	wget https://github.com/gperftools/gperftools/releases/download/gperftools-2.4/gperftools-2.4.tar.gz
	tar xfz gperftools-2.4.tar.gz
	cd gperftools-2.4
	./configure --disable-dependency-tracking --disable-cpu-profiler --disable-heap-checker --disable-debugalloc  --enable-minimal --prefix=/usr
	make
	sudo make install
fi
