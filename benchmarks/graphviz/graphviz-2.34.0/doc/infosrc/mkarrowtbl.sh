#! /bin/ksh
#
# Create an HTML table using the argument images as items.
#
# usage: mkarrowtbl.sh arrow*
#
# where arrow starts with *_ and has a suffix indicating the
# desired output format.

typeset -i I CNT=0
typeset -i LCNT=0
CURRA="xxxx"
 
function closeLine
{
  echo "  </TR>"
  echo "  <TR ALIGN=CENTER>"
  for (( I=0;I<CNT;I++ ))
  do
    echo "    <TD>${AS[$I]}"
  done
  echo "  </TR>"
}

echo "<CENTER>"
echo "<TABLE>"

while (( $# > 0 ))
do
  ARROW=${1%.*}        # abc_arrowname
  FMT=${1#$ARROW.}     # fmt
  A=${ARROW#*_}        # arrowname
# Arrows with the same suffix (ignoring .fmt) are put on the same line.
# That is, if the right side of $A is not $CURRA, we start a new line.
  if [[ $A != *$CURRA ]]
  then
    if (( LCNT > 0 ))
    then
      closeLine
      (( CNT=0 ))
    fi
    echo "  <TR ALIGN=CENTER>"
    CURRA=$A
    (( LCNT++ ))
  fi
  AS[$CNT]=$A
  (( CNT++ ))
  echo "    <TD><IMG SRC=\"$1\">"
  shift
done

closeLine

echo "</TABLE>"
echo "</CENTER>"
echo "</BODY>"
echo "</HTML>"

