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
    ${KLEE_SRC}/.travis/stp.sh
    cd ../
    ;;
  Z3)
    echo "Z3"
    # Should we install libz3-dbg too?
    sudo apt-get -y install libz3 libz3-dev
    ;;
  metaSMT)
    echo "metaSMT"
    ${KLEE_SRC}/.travis/metaSMT.sh
    ;;
  *)
    echo "Unknown solver ${solver}"
    exit 1
  esac
done
