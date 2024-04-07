#!/bin/bash

set -e
set -u
DIR="$(cd "$(dirname "$0")" && pwd)"

cd klee_fork/mytests

for file in *.cpp; do
    klee $file
done