#! /bin/ksh
#
# usage: mktapers.sh <taper>
#
# where taper has the form <arrowheadattr>_<dirattr>.fmt
# where .fmt gives the desired output format.

if (( $# == 0 ))
then
  echo "mktapers: missing taper name argument"
  exit 1
fi

TGT=$1                  # ah_dir.fmt
TAPER=${1%.*}           # ah_dir
FMT=${1#$TAPER.}        # fmt
F=$TAPER.gv             # ah_dir.gv
AH=${TAPER%_*}          # ah
DIR=${TAPER#${AH}_}  # dir

    exec 3> $F

    echo "digraph G { rankdir=LR ranksep=1 " >&3
    echo "  edge [style=tapered penwidth=7 arrowtail=none ]" >&3
    echo "  node[ style=filled label=\"\" ]" >&3
    echo "  a -> b [ dir=$DIR arrowhead=$AH ]" >&3
    echo "}" >&3

    exec 3>&-

    dot -T$FMT $F > $TGT
    rm -f $F


