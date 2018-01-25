#! /bin/ksh
typeset -i N_PER_ROW=4
typeset -i cnt=0
list=

function doLine {
  echo "  <TR ALIGN=CENTER>"
  for s in $@
  do
    echo "    <TD><IMG SRC=$s.gif>"
  done
  echo "  </TR>"

  echo "  <TR ALIGN=CENTER>"
  for s in $@
  do
    echo "    <TD><A NAME=d:$s>$s</A>"
  done
  echo "  </TR>"
}

# Use the shapes name in shape list to create the
# contents of an HTML array.

for s in $(cat shapelist)
do
  list=$list" $s"
  cnt=$(( $cnt + 1 ))
  if [[ $cnt = $N_PER_ROW ]]
  then
    doLine $list
    list=
    cnt=0
  fi
done

if [[ -n "$list" ]]
then
    doLine $list
fi

exit 0
