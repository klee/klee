#! /bin/sh

# autoregen doesn't run libtoolize with --ltdl on older systems, so force it

LIBTOOLIZE=libtoolize
if test -x /opt/local/bin/glibtoolize; then
    LIBTOOLIZE=/opt/local/bin/glibtoolize
fi
export LIBTOOLIZE

echo "autogen.sh: running: $LIBTOOLIZE --copy --force"
$LIBTOOLIZE --copy --force 

autoreconf -v --install --force || exit 1

# ensure config/depcomp exists even if still using automake-1.4
# otherwise "make dist" fails.
touch config/depcomp

# ensure COPYING is based on epl-v10.txt
#   epl-v10.txt was obtained from: http://www.eclipse.org/legal/epl-v10.html
#   by using lynx to print to .txt.
rm -f COPYING
cp epl-v10.txt COPYING

# don't use any old cache, but create a new one
rm -f config.cache
./configure -C "$@"
