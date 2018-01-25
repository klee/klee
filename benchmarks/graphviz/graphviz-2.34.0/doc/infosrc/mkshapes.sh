#! /bin/ksh
# For each name in shapelist, create a dot file using that
# shape, then produce a gif file as output.

for s in $(cat shapelist)
do
  F=$s.dot
  exec 3> $F
  echo "digraph G {" >&3
  if [[ $s == "plaintext" || $s == "none" ]]
  then
    echo " node [shape=$s];" >&3
  else
    echo " node [style=filled,shape=$s];" >&3
    echo " node [label=\"\"];" >&3
  fi
  if [[ $s == point ]]
  then
    echo " node [width=\"0.1\"];" >&3
  elif [[ $s == polygon ]]
  then
    echo " node [sides=7]; " >&3
  fi
  echo " $s;" >&3
  echo "}" >&3
  exec 3>&-

  dot -Tgif $s.dot > $s.gif
  rm -f $s.dot
done


