set -x
set -e
set -o
set -u

COMPILER=$1
FILENAME=$2
CURDIR=$3
TMPNAME=$4
KLEE=$5
INCLUDE_DIR=$6

$COMPILER $FILENAME -std=c++11 -I ${INCLUDE_DIR} -emit-llvm -O0 -c -g -nostdinc++ -o ${TMPNAME}.bc
rm -rf ${TMPNAME}.klee-out
$KLEE --output-dir=${TMPNAME}.klee-out --no-output --exit-on-error --libc=uclibc --use-libcxx ${TMPNAME}.bc 2>&1
