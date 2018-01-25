#! /bin/ksh
#
# usage: mksplines.sh <spline>
#
# where spline has the form spline_<splinesattr>.fmt
# where .fmt gives the desired output format.

if (( $# == 0 ))
then
  echo "mksplines: missing splines name argument"
  exit 1
fi

TGT=$1                  # spline_kind.fmt
SPLINE=${1%.*}          # spline_kind
FMT=${1#$SPLINE.}       # fmt
KIND=${SPLINE#spline_}  # kind

sfdp -T$FMT -Gviewport=300,300,1,40 -Goverlap=false -Gsplines=$KIND ../../graphs/undirected/ngk10_4.gv > $TGT

