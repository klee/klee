#!/bin/bash

set -e
set -u

cd mytests

for file in *.cpp; do
    klee $file
done