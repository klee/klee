#!/bin/sh

# $1 = .la file in build tree (doesn't need to be installed)
# $2 = Name of extension
# $3 = Version of extension

lib=`sed -n "/dlname/s/^[^']*'\([^ ']*\).*$/\1/p" $1`
if [ -z "$lib" ]
then
    libBaseName=`basename $1 .la`
    case `uname` in
        CYGWIN*) lib="${libBaseName}.dll"   ;;
        Darwin*) lib="${libBaseName}.dylib" ;;
        HP-UX*)  lib="${libBaseName}.sl"    ;;
        *)       lib="${libBaseName}.so"    ;;
    esac
fi

echo "package ifneeded $2 $3 \"" >pkgIndex.tcl
case "$1" in
  *tk* )
    echo "	package require Tk 8.3" >>pkgIndex.tcl
    ;;
esac
echo "	load [file join \$dir $lib] $2\"" >>pkgIndex.tcl
