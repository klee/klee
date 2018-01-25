#! /bin/ksh
typeset -A desc fullname
name=

# Given the output specifications as stdin, read each one,
# storing the formats long name and description in the fullname
# and desc arrays, respectively, indexed by the format name.
# The initial line of an item has the format :name:long name 
OLDIFS="$IFS"
IFS=
while read line
do
  c=${line:0:1}
  if [[ $c == '#' ]]
  then continue
  elif [[ $c == ':' ]]
  then
    if [[ -n "$name" ]]
    then
      desc[$name]=$txt
      fullname[$name]=$tag
      txt=""
    fi
    line=${line#:}
    if [[ "$line" == *:* ]]
    then
      name=${line%%:*}
      tag=${line#$name:}
    else
      name=$line
      tag=""
    fi
  else
    txt="$txt${line}\n"
  fi
done
IFS="$OLDIFS"

if [[ -n "$name" ]]
then
  desc[$name]=$txt
  fullname[$name]=$tag
  txt=""
fi

#print ${!fullname[@]}
#print ${desc[@]}
#exit

set -s ${!desc[@]}

# Output a brief description of the formats as a table.
# This is followed by a list of the formats, with the long
# description of each.
# The formats are alphabetized on output.
# Note that an item may have multiple names, i.e., the first
# field may have the format name1/name2/name3.
# The output format <name> is given the anchor a:<name> in the
# table and d:<name> in the list.

print "<TABLE ALIGN=CENTER>"
print "<TR><TH>Command-line<BR>parameter</TH><TH>Format</TH></TR>"
for i
do
  print -n " <TR>";
  print -n "<TD ALIGN=CENTER>";
  first=yes
  for n in ${i//\// }
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n "<BR>"
    fi 
    printf "<A NAME=a:%s HREF=#d:%s>%s</A>\n" $n $n $n
  done
  print -n "</TD><TD>"
  print -n ${fullname[$i]};  print "</TD> </TR>";
done
print "</TABLE>"

print "<HR>"

#set -s ${!desc[@]}
print "<H2>Format Descriptions</H2>\n<DL>"
for i
do
  first=yes
  for n in ${i//\// }
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n ","
    fi 
    printf "<DT><A NAME=d:%s HREF=#a:%s><STRONG>%s</STRONG></A>\n" $n $n $n
  done
  print "<DD>${desc[$i]}"
done
print "</DL>\n<HR>"

exit 0
