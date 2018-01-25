#! /bin/ksh
typeset -A uses kinds dflts minvs flags descs namecnt
typeset -i J L
name=
use=
kind=
dflt=
minv=
flag=

# Filter taking a list of attribute descriptions as stdin
# and producing a summary table, followed by a list of
# attribute descriptions.

set -f   # Turn of globbing

# Map special HTML characters into their HTML representation.
# This can be escaped using '\'.
protect ()
{
  s=
  L=${#1}
  for (( J=0 ; J<L ; J++ ))
  do
    c=${1:J:1}
    case $c in
      '<' )
        s=${s}'&#60;'
        ;;
      '>' )
        s=${s}'&#62;'
        ;;
#      '&' )
#        s=${s}'&#38;'
#        ;;
      '\' )
        if (( J == L-1 ))
        then
          s=${s}'\'
        else
          (( J++ ))
          c=${1:J:1}
          s=${s}$c
        fi
        ;;
      * )
        s=${s}$c
        ;;
    esac
  done
  print -r -n -- $s
}

# The specification allows items with the same attribute name.
# If there is more than one, the later ones are indexed by
# <name>:<n>, where each gets a distinct positive n.
# This function takes the index name, extracts the real name
# and stores it in realname, and creates the anchor keys,
# which are n copies of 'a' and 'd'.
function setRealnameNKeys 
{
  if [[ $1 == *:* ]]
  then
    realname=${1%:*}
    J=${1##*:}
  else
    realname=$1
    J=0
  fi
  AKEY=a
  DKEY=d
  while (( J > 0 ))
  do
    AKEY=${AKEY}a
    DKEY=${DKEY}d
    (( J-- ))
  done
}

# Header has the format name:uses:kind[:dflt[:min]];  notes
function parseHdr 
{
# get flags to the right of ';'
  flag="${1##*;}"
  flag="${1##*;*( )}"
  if [[ -n $flag ]]
  then
    flag=${flag//,/ } # change optional commas to spaces
  fi

# strip off flags to the right of ';'
  opts="${1%;*}"

# get name: this has the format name1[/name2]* notes
  name=${opts%%:*}
  rest=${opts#*:}
  name=${name%% *} # strip off notes

# there may be duplicate names with separate entries.
# Keep count of name uses, and modify name with :i.
  J=${namecnt[$name]}
  namecnt[$name]=$(( J+1 ))
  if (( J > 0 )) 
  then
    name=${name}:$J
  fi
  
# get use string : this is a string formed of G,N,C,E
  use=${rest%%:*}
  rest=${rest#${use}:}

# get kind string: this has format kind1[/kind2]*
  kind=${rest%%:*}
  rest=${rest#${kind}}

# Remaining fields are optional.
# First, check for default value: this has format :dflt1[/dflt2]*
  if [[ -z "$rest" ]]
  then 
    dflt=
    minv=
    return
  fi
  rest=${rest#:}
# If rest contains a ':' not preceded by a space, we take this to
# mean that a minval field is present, and split rest at that ':'.
# Everything preceding the ':' becomes the dflt. 
# If there is no such colon (that is, there is no colon, or all
# colons are preceded by a space, we assume there is no minval, and
# use all of rest as dflt.
# This allows dflt to contain a ':' as long as it is preceded by a space.
  if [[ $rest == *[![:space:]]:* ]]
  then
    dflt=${rest%:*}
    rest=${rest#"${dflt}"}
  else
    dflt=${rest}
    rest=
  fi

# Check for minimum value: this has format :min1[/min2]*
  if [[ -z "$rest" ]]
  then 
    minv=
    return
  fi
  minv=${rest#:}
}

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
      descs[$name]=$txt
      txt=""
    fi
    line=${line#:}
    parseHdr $line
    uses[$name]="$use"
    kinds[$name]="$kind"
    dflts[$name]="$dflt"
    minvs[$name]="$minv"
    flags[$name]="$flag"
  else
    txt="$txt  ${line}\n"
  fi
done
IFS="$OLDIFS"

if [[ -n "$name" ]]
then
  descs[$name]=$txt
  txt=""
fi

#print ${!descs[@]}
#print ${uses[@]}
#print ${kinds[@]}
#print ${dflts[@]}
#print ${minvs[@]}
#print ${flags[@]}
#print ${descs[@]}
#exit

print "<TABLE ALIGN=CENTER>"
print "<TR><TH>Name</TH><TH><A HREF=#h:uses>Used By</A></TH><TH>Type</TH><TH ALIGN=CENTER>Default</TH><TH>Minimum</TH><TH>Notes</TH></TR>"

set -s ${!descs[@]}
for i
do
  print -n " <TR>"
  print -n "<TD>"
  setRealnameNKeys $i
  first=yes
  for n in ${realname//\// }
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n "<BR>"
    fi 
    printf "<A NAME=%s:%s HREF=#%s:%s>%s</A>\n" $AKEY $n $DKEY $n $n
  done
  print -n "</TD>"

  printf "<TD>%s</TD>" "${uses[$i]}"  

  print -n "<TD>";
  first=yes
  kind=${kinds[$i]}
  for n in ${kind//\// }
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n "<BR>"
    fi 
    case $n in
      double )
        print -n "double"
        ;;
      int )
        print -n "int"
        ;;
      string )
        print -n "string"
        ;;
      bool )
        printf "<A HREF=#k:bool>bool</A>\n"
        ;;
      * )
        printf "<A HREF=#k:%s>%s</A>\n" $n $n
        ;;
    esac
  done
  print -n "</TD>"

  print -n "<TD ALIGN=\"CENTER\">";
  first=yes
  dflt=${dflts[$i]}
  OLDIFS="$IFS"
  IFS='/'
  for n in ${dflt}
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n "<BR>"
    fi 
    n=$(protect $n)
    print -r -n -- $n
  done
  print -n "</TD>"
  IFS="$OLDIFS"

  print -n "<TD>";
  first=yes
  minv=${minvs[$i]}
  OLDIFS="$IFS"
  IFS='/'
  for n in ${minv}
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n "<BR>"
    fi 
    n=$(protect $n)
    print -n -- $n
  done
  IFS="$OLDIFS"
  print -n "</TD>"

  print -n "<TD>"
  flag=${flags[$i]}
  case $flag in
    "" )
      ;;
    "notdot" )
      print -n "not dot"
      ;;
    * )
      set -A farray $flag
      L=${#farray[@]}
      for (( J = L-1 ; J >= 0 ; J-- ))
      do
        n=${farray[$J]}
        if [[ $n == "ps" ]]
        then
          n=postscript
        elif [[ $n == "bitmap" ]]
        then
          n="bitmap output"
        fi
        print -n $n
        if (( J > 0 ))
        then
          print -n ', '
        fi
      done
      print -n " only"
      ;;
  esac
  print -n "</TD>"
  print " </TR>"
done

print "</TABLE>"

print "<HR>"

#set -s ${!descs[@]}
print "<H1>Attribute Descriptions</H1>\n<DL>"
for i
do
  setRealnameNKeys $i

  first=yes
  for n in ${realname//\// }
  do
    if [[ -n $first ]]
    then
      first=
    else
      print -n ","
    fi 
    printf "<DT><A NAME=%s:%s HREF=#%s:%s><STRONG>%s</STRONG></A>\n" $DKEY $n $AKEY $n $n
  done
  print "<DD>${descs[$i]}"
done
print "</DL>\n<HR>"

exit 0
