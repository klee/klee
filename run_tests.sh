# Put this script inside <Build directory>/bin
# and .bc files of tests you want to run in <Build directory>/bin/test_bc

#!/bin/bash

mkdir -p results
mkdir -p results_noguide

rm -rf results/*
rm -rf results_noguide/*


for file in test_bc/*.bc; do
    timeout 3m ./klee --emit-all-errors --max-time=2min --output-dir=results/${file#*/} test_bc/${file#*/} >results/li_log 2>results/li_log
    mv results/li_log results/${file#*/}/li_log
done

for file in test_bc/*.bc; do
    timeout 3m ./klee --emit-all-errors --search=bfs --execution-mode=default --max-time=2min --output-dir=results_noguide/${file#*/} test_bc/${file#*/} >results_noguide/li_log 2>results_noguide/li_log
    mv results_noguide/li_log results_noguide/${file#*/}/li_log
done


for file in results/*; do
    pushd $file > /dev/null
    rm -f li_count
    touch li_count
    grep -rnw . -e '"n_offsets": 1' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 2' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 3' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 4' --exclude="li_count" >> li_count
    popd > /dev/null
done

for file in results_noguide/*; do
    pushd $file > /dev/null
    rm -f li_count
    touch li_count
    grep -rnw . -e '"n_offsets": 1' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 2' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 3' --exclude="li_count" >> li_count
    grep -rnw . -e '"n_offsets": 4' --exclude="li_count" >> li_count
    popd > /dev/null
done

