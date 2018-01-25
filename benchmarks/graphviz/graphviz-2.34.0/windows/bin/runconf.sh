export nativepp=-1
if [[ -z "$GTKDIR" ]]
then
  GTKDIR is undefined in runconf script
  exit 1
fi
if [[ -z "$INSTALLROOT" ]]
then
  INSTALLROOT is undefined in runconf script
  exit 1
fi

PANGOFLAGS=
LTDLFLAGS=--disable-ltdl 
SHAREFLAGS="--disable-shared --enable-static "
#SHAREFLAGS="--enable-shared --disable-static "
# Removed if pixbuf gets fixed
GDKPIXBUF=--without-gdk-pixbuf
export CPPFLAGS="-I$GTKDIR/include -I$GTKDIR/include/freetype2" 
export LDFLAGS="-L$GTKDIR/lib"
export CC=ncc 

while getopts :PLX c
do
  case $c in
   X )
    GDKPIXBUF=
    ;;
   P )
    export FONTCONFIG_CFLAGS=-I$GTKDIR/include/ 
    export FONTCONFIG_LIBS=-L$GTKDIR/lib 
    export FREETYPE2_CFLAGS=-IC:/gtk/include/freetype2
    export FREETYPE2_LIBS="-LC:/gtk/lib -lfreetype -lz"
    export PKG_CONFIG=$GTKDIR/bin/pkg-config 
    export PKG_CONFIG_PATH='C:/gtk/lib/pkgconfig' 
    export PANGOCAIRO_CFLAGS="-I$GTKDIR/include/cairo -I$GTKDIR/include/pango-1.0 -I$GTKDIR/include/glib-2.0 -I$GTKDIR/LIB/GLIB-2.0/INCLUDE"
    export PANGOCAIRO_LIBS="-L$GTKDIR/lib -lfontconfig  -lfreetype  -ljpeg  -lpng  -lexpat  -lz -lcairo -lpango-1.0 -lpangocairo-1.0 -lgobject-2.0 -lgtk-win32-2.0 -lglib-2.0 -lgdk-win32-2.0 -latk-1.0 -lgdk_pixbuf-2.0"  
    export GTK_CFLAGS="-I$GTKDIR/include -I$GTKDIR/INCLUDE/GTK-2.0 -I$GTKDIR/INCLUDE/CAIRO -I$GTKDIR/include/glib-2.0 -I$GTKDIR/LIB/GLIB-2.0/INCLUDE -I$GTKDIR/LIB/GLIB-2.0/INCLUDE -I$GTKDIR/INCLUDE/PANGO-1.0 -I$GTKDIR/LIB/GTK-2.0/INCLUDE -I$GTKDIR/INCLUDE/ATK-1.0"
    export GTK_LIBS=-L$GTKDIR/lib
    PANGOFLAGS="--with-fontconfig --with-fontconfiglibdir=/C/gtk/2.0/lib --with-fontconfigincludedir=$GTKDIR/include/ --with-pangocairo"
    ;;
   L )
    LTDLFLAGS="--enable-ltdl" 
    SHAREFLAGS="--enable-shared --disable-static "
    ;;
esac
done

echo ./configure -C --prefix=$INSTALLROOT $GDKPIXBUF --with-mylibgd --disable-swig --without-x  --without-tclsh --without-ipsepcola $SHAREFLAGS --with-freetype=$GTKDIR/lib $LTDLFLAGS $PANGOFLAGS
./configure -C --prefix=$INSTALLROOT $GDKPIXBUF --with-mylibgd --disable-swig --without-x  --without-tclsh --without-ipsepcola $SHAREFLAGS --with-freetype=$GTKDIR/lib $LTDLFLAGS $PANGOFLAGS
