#! /bin/ksh
typeset -A desc
name=

# Read in each type description. Store the descriptions
# in the desc array, indexed by the typename.

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
      txt=""
    fi
    name=${line#:}
  else
    if [[ -z "$txt" ]]
    then
      txt="${line}\n"
    else
      txt="$txt  ${line}\n"
    fi
  fi
done
IFS="$OLDIFS"

if [[ -n "$name" ]]
then
  desc[$name]=$txt
  txt=""
fi

#print ${!desc[@]}
#print ${desc[@]}
#exit

# Output type descriptions as an HTML list

print "<H1>Attribute Type Descriptions</H1>"
print "The following list gives the legal strings corresponding to values of"
print "the given types."
print "The syntax for describing legal type strings is a mixture of literal strings,"
print "stdio encodings (e.g., <TT>%f</TT> for a double), and regular expressions."
print "For regular expressions, <TT>(...)*</TT> indicates 0 or more copies of the expression"
print "enclosed in the parentheses,  <TT>(...)+</TT> indicates 1 or more, and"
print "<TT>(...)?</TT> denotes 0 or 1 copy."
print "<DL>"

set -s ${!desc[@]}
for i
do
  printf "<DT><A NAME=k:%s><STRONG>%s</STRONG></A>\n" $i $i
  print "<DD>${desc[$i]}"
done

print "</DL>"
print "</BODY>\n</HTML>"

exit 0
