#!/bin/bash -x
# Make sure we exit if there is a failure
set -e
: ${SOLVERS?"Solvers must be specified"}

SOLVER_LIST=$(echo "${SOLVERS}" | sed 's/:/ /')

for solver in ${SOLVER_LIST}; do
  echo "Getting solver ${solver}"
  case ${solver} in
  STP)
    echo "STP"
    mkdir stp
    cd stp
    CC=${STP_CC:-${CC}} CXX=${STP_CXX:-${CXX}} ${KLEE_SRC}/.travis/stp.sh
    cd ../
    ;;
  Z3)
    echo "Z3"
    mkdir z3
    cd z3
    CC=${Z3_CC:-${CC}} CXX=${Z3_CXX:-${CXX}} ${KLEE_SRC}/.travis/z3.sh
    cd ../
    ;;
  metaSMT)
    echo "metaSMT"
    CC=${METASMT_CC:-${CC}} CXX=${METASMT_CXX:-${CXX}} ${KLEE_SRC}/.travis/metaSMT.sh
    ;;
  *)
    echo "Unknown solver ${solver}"
    exit 1
  esac
done
