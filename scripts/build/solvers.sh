#!/bin/bash -x
# Make sure we exit if there is a failure
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

: ${SOLVERS?"Solvers must be specified"}
SOLVER_LIST=$(echo "${SOLVERS}" | sed 's/:/ /g')

for solver in ${SOLVER_LIST}; do
  echo "Getting solver ${solver}"
  case ${solver} in
  STP)
    echo "STP"
    "${DIR}/solver-stp.sh"
    ;;
  Z3)
    echo "Z3"
    "${DIR}/solver-z3.sh"
    ;;
  metaSMT)
    echo "metaSMT"
    "${DIR}/solver-metasmt.sh"
    ;;
  *)
    echo "Unknown solver ${solver}"
    exit 1
  esac
done
