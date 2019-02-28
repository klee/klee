#!/bin/bash
# Build all the docker containers utilized, that would be
# also build by TravisCI.
#
# Every line starting with " - LLVM_VERSION=" is a single setup

set -e
#set -x
DIR="$(cd "$(dirname "$0")" && pwd)"

trim() {
    local var="$*"
    # remove leading whitespace characters
    var="${var#"${var%%[![:space:]]*}"}"
    # remove trailing whitespace characters
    var="${var%"${var##*[![:space:]]}"}"   
    echo -n "$var"
}

# Build dependencies only
isGlobal=0
isEnv=0
commons=()
experiments=()
while read -r line
do
    [[ "${line}" == "env:"* ]] && isEnv=1 && continue
    [[ "${line}" == "#stop"* ]] && isEnv=0 && break
 
    
    [[ "${isEnv}" -eq 0 ]] && continue
    
    line="$(trim "${line}")"

    # Ignore empty lines
    [[ -z "${line}" ]] && continue
    [[ "${line}" == "#"* ]] && continue 
    
    [[ "${line}" == "global:"* ]] && isGlobal=1 && continue
    [[ "${line}" == "matrix:"* ]] && isGlobal=0 && continue

    
    
    if [[ "${isGlobal}" -eq 1 ]]; then
      tmp="${line#*-}"
      IFS=: read -r key value <<< "${tmp}"
      key=$(trim "${key}")
      [[ "${key}" == "secure" ]] && continue
      [[ -z "${key}" ]] && continue
      commons+=("${key}=$(trim "${value}")")
      continue
    fi

    experiment=$(echo "$line"| grep "\- " | xargs -L 1 | cut -d "-" -f 2)
    if [[ "x$experiment" == "x" ]]; then
      continue
    fi
    experiments+=("${experiment}")
done < "${DIR}/../../.travis.yml"

for experiment in "${experiments[@]}"; do
  [[ -z "${experiment}" ]] && continue
  /bin/bash -c "${commons[*]} ${experiment} ${DIR}/build.sh klee --docker --push-docker-deps --debug --create-final-image"
done