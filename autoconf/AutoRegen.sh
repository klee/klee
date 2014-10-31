#!/bin/sh

if ([ "$#" != 1 ] || 
    [ ! -d "$1" ] ||
    [ ! -d "$1/autoconf/m4" ]); then
    echo "usage: $0 <llvmsrc-dir>" 1>& 2
    exit 1
fi

llvm_src_root=$(cd $1; pwd)
llvm_m4=$llvm_src_root/autoconf/m4
die () {
	echo "$@" 1>&2
	exit 1
}
test -d autoconf && test -f autoconf/configure.ac && cd autoconf
test -f configure.ac || die "Can't find 'autoconf' dir; please cd into it first"
autoconf --version | egrep '2\.69' > /dev/null
if test $? -ne 0 ; then
  die "Your autoconf was not detected as being 2.69"
fi
# Patch LLVM_SRC_ROOT in configure.ac
sed -e "s#^LLVM_SRC_ROOT=.*#LLVM_SRC_ROOT=\"$llvm_src_root\"#" \
    configure.ac > configure.tmp.ac
echo "Regenerating aclocal.m4 with aclocal"
rm -f aclocal.m4
echo aclocal -I $llvm_m4 -I "$llvm_m4/.." || die "aclocal failed"
aclocal -I $llvm_m4 -I "$llvm_m4/.." || die "aclocal failed"
echo "Regenerating configure with autoconf 2.60"
echo autoconf --warnings=all -o ../configure configure.tmp.ac || die "autoconf failed"
autoconf --warnings=all -o ../configure configure.tmp.ac || die "autoconf failed"
cp ../configure ../configure.bak
sed -e "s#^LLVM_SRC_ROOT=.*#LLVM_SRC_ROOT=\".\"#" \
    ../configure.bak > ../configure
cd ..
echo "Regenerating config.h.in with autoheader"
autoheader --warnings=all \
    -I autoconf -I autoconf/m4 \
    autoconf/configure.tmp.ac || die "autoheader failed"
rm -f autoconf/configure.tmp.ac configure.bak
exit 0
