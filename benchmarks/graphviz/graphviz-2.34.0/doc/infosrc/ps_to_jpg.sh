#! /bin/ksh

gs -g$(grep PageBoundingBox $1 | awk -f sz.awk) -sDEVICE=jpeg -sOutputFile=$2  -dNOPAUSE -dBATCH $1 > /dev/null
