#! /bin/ksh
#
# usage: mkstyles.sh <style>
#
# where style has the form [nce]_<styleattr>.fmt
# where .fmt gives the desired output format.

if (( $# == 0 ))
then
  echo "mkstyles: missing style name argument"
  exit 1
fi

TGT=$1                  # x_style.fmt
STYLE=${1%.*}           # x_style
FMT=${1#$STYLE.}        # fmt
F=$STYLE.gv             # x_style.gv
OBJ=${STYLE%_*}         # x
STYLE=${STYLE#${OBJ}_}  # style

    exec 3> $F

    echo "digraph G { rankdir=LR" >&3
case "$OBJ" in
 c )
    echo "  fillcolor=greenyellow" >&3
    echo "  node[style=filled label=\"\"]" >&3
    if [[ $STYLE == striped ]]
    then
        echo "  fillcolor=\"greenyellow:yellow:lightblue\"" >&3
    fi
    echo "  subgraph cluster { style=$STYLE 0}" >&3
  ;;
 e )
    echo "  node[ style=filled label=\"\"]" >&3
    echo "  a -> b [ style=$STYLE ]" >&3
  ;;
 n )
    case "$STYLE" in
      rounded | diagonals )
        echo "  node[ shape=box]" >&3
        ;;
      striped )
        echo "  node[ shape=box fillcolor=\"greenyellow:yellow:lightblue\"]" >&3
        ;;
      wedged )
        echo "  node[ fillcolor=\"greenyellow:yellow:lightblue\"]" >&3
        ;;
    esac
    echo "  0 [style=$STYLE label=\"\"]" >&3
  ;;
esac
    echo "}" >&3

    exec 3>&-

    dot -T$FMT $F > $TGT
    rm -f $F


