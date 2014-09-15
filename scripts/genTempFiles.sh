#!/bin/bash

if [ -z "$1" ] ; then
	echo "No directory given"
	exit 1
fi

mkdir -p $1
for i in `seq 1 100`; do
	mkdir -p $1/dir_foo_$i
	touch $1/tmp_foo_$i
done    
