#! /bin/ksh
#
# usage: mkarrows.sh [-s] <arrow>
#
# where arrow starts with *_ and has a suffix indicating the
# desired output format.

SHAPE=

while getopts ":s" c; do
  case "$c" in
  s)
    STYLE="shape=point style=invis"
    ;;
  esac
done
shift OPTIND-1

if (( $# == 0 ))
then
  echo "genarrows: missing arrow name argument"
  exit 1
fi

TGT=$1               # abc_arrowname.fmt
ARROW=${1%.*}        # abc_arrowname
FMT=${1#$ARROW.}     # fmt
A=${ARROW#*_}        # arrowname
F=$ARROW.dot         # abc_arrowname.dot

#for s in $(cat $INFILE)
#do
#  if [[ a_$s.gif -ot genarrows ]]
#  then
#    F=$s.dot
    exec 3> $F

    echo "digraph G {" >&3
    echo "  rankdir=LR;" >&3
    echo "  T [shape=point];" >&3
    echo "  H [label=\"\" $STYLE];" >&3
    echo "  T -> H [arrowhead=$A];" >&3
    echo "}" >&3

    exec 3>&-

    dot -Tgif $F > $TGT
    rm -f $F
#  fi
#done


