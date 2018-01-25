# Relies on pscp (/c/Program Files/putty/pscp)
#

PATH=$PATH:"/c/Program Files/putty:/msdev/vc/bin"
export ROOT=$PWD
export INSTALLROOT=$ROOT/local
GVIZ_HOME=$ROOT/gviz       
LFILE=$ROOT/build.log    # log file
SOURCEM=www.graphviz.org
SOURCE=/var/www/www.graphviz.org/pub/graphviz
INPKG=graphviz-working.tar.gz
SOURCEFILE=$SOURCE/CURRENT/$INPKG
DESTDIR=$SOURCE/CURRENT
VERSION=$(date +%K)
export GTKDIR=/c/gtk
typeset -l USER=${USER:-erg}
#WINDBG="erg"
WINDBG="erg arif ellson"
SFX=
SUBJECT="-s'Windows build failure'"
export nativepp=-1

function SetVersion 
{
  if [[ -f $GVIZ_HOME/config.h ]]
  then
    VERSION=$(grep 'e VERSION' $GVIZ_HOME/config.h)
    VERSION=${VERSION#*VERSION \"}
    VERSION=${VERSION%\"*}

    SFX=${VERSION#*.*.}
    SHORTVERSION=${VERSION%.$SFX}
  fi
}

function WarnEx
{
  ssh erg@penguin "echo $1 | mail erg $SUBJECT"
}  

function ErrorEx
{
  scp -q $LFILE $USER@penguin:/home/$USER/build.log.$VERSION
  ssh $USER@penguin "echo $1 | mail $WINDBG $SUBJECT"
  exit 1
}  

# Copy file to graphviz machine
function PutFile
{
  echo scp $1 $SOURCEM:$DESTDIR  >> $LFILE 2>&1
  scp -q  $1 $SOURCEM:$DESTDIR  >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to put $1"
  fi
}

function Get 
{
  echo scp $SOURCEM:$SOURCEFILE . >> $LFILE
  scp -q  $SOURCEM:$SOURCEFILE . >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to get source"
  fi 

     # Remove any old source directories 
  rm -rf graphviz-[1-9]*[0-9]
  echo "unpacking the package" >> $LFILE
  echo "(gunzip < $INPKG | tar xf - )" >> $LFILE 2>&1
  (gunzip < $INPKG | tar xf - ) >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
     ErrorEx "failure in unwrapping"
  fi

  echo rm -rf $GVIZ_HOME >> $LFILE
  rm -rf $GVIZ_HOME
  if [[ $? != 0 ]]
  then
     ls -ld $GVIZ_HOME >> errfile
     echo $USER >> errfile
     WarnEx "Warning: failure in removing old gviz directory"
     mv $GVIZ_HOME gviz$$
     if [[ $? != 0 ]]
     then
       ErrorEx "failure in removing old gviz directory"
     fi
  fi

  echo mv graphviz-[1-9]*[0-9] $GVIZ_HOME >> $LFILE
  mv graphviz-[1-9]*[0-9] $GVIZ_HOME
  if [[ ! -d $GVIZ_HOME ]]
  then
     ErrorEx "could not create $GVIZ_HOME"
  fi
  echo "Finish Get" >> $LFILE
}

function mkDir {
  if [[ ! -d $1 ]]
  then 
    echo mkdir $1 >> $LFILE 2>&1
    mkdir $1 >> $LFILE 2>&1
  fi 
}

THIRD_PARTY_DLL="freetype6.dll libexpat.dll png.dll jpeg.dll libexpatw.dll z.dll zlib1.dll" 
THIRD_PARTY="freetype.lib libexpat.lib libexpatw.lib expat.lib png.lib jpeg.lib expatw.lib z.lib" 

#Todo : 
# Do we need freetype[6].def?

# Prepare to build the software.
#   Initialize target tree
#   Add any missing software and libraries
#   Run configure
#   Make post-configure tweaks to source, Makefiles and config.h
function Setup 
{
  cd $ROOT
  echo rm -rf $INSTALLROOT >> $LFILE 2>&1
  rm -rf $INSTALLROOT >> $LFILE 2>&1

  mkDir $INSTALLROOT
  mkDir $INSTALLROOT/lib
  mkDir $INSTALLROOT/include
  mkDir $INSTALLROOT/bin
  mkDir $INSTALLROOT/doc
  mkDir $INSTALLROOT/doc/man

  # Overlay third-party libraries
#  cd $ROOT/third-party/include
#  cp -r * $INSTALLROOT/include
#  cp ltdl.h $INSTALLROOT/include
#  cd ../lib
#  cp libltdl* $INSTALLROOT/lib
#  cp $THIRD_PARTY_DLL $INSTALLROOT/bin
#  cp $THIRD_PARTY $INSTALLROOT/lib

  # Add regex code
  cd $ROOT/add-on
  cp regex_win32.c regex_win32.h $GVIZ_HOME/lib/gvc

  # Add ltdlt library if necessary
  if [[ $USE_DLL == 2 ]]
  then
    cp libltdl.a ltdl.lib  $INSTALLROOT/lib
    cp ltdl.dll  $INSTALLROOT/bin
  fi

  cd $GVIZ_HOME
  cp configure xxx
  sed '/PKG_CONFIG_PATH=/d' xxx > configure
  echo ../runconf.sh $CONFARG >> $LFILE 2>&1
  ../runconf.sh $CONFARG > conf.log 2>&1
  if [[ $? != "0" ]]
  then
    ErrorEx "configure failed"
  fi
  chmod 664 config.h

  # make source fixes for Windows
  if [[ ! -f lib/agraph/unistd.h ]]
  then
    > lib/agraph/unistd.h  # for scan.c
    > lib/cgraph/unistd.h  # for scan.c
    > lib/gd/unistd.h      # for fontconfig.h
    > plugin/pango/unistd.h      # for fontconfig.h
  fi
#  if grep _typ_ssize_t ast_common.h > /dev/null 
#  then
#    mv ast_common.h xx
#    sed '/#define _typ_ssize_t/d' xx > ast_common.h
#  fi

  if ! grep DISABLE_THREADS lib/gd/gd.h > /dev/null
  then
    cp lib/gd/gd.h xx
    sed 's/#ifndef WIN32/#define NONDLL 1\n#define DISABLE_THREADS 1\n#ifndef WIN32/' xx > lib/gd/gd.h
  fi
  if grep 'define NO_POSTSCRIPT_ALIAS' lib/gd/gdft.c > /dev/null
  then
    cp lib/gd/gdft.c xx
    sed 's/^.*define NO_POSTSCRIPT_ALIAS.*$/#define NO_POSTSCRIPT_ALIAS 1/' xx > lib/gd/gdft.c
  fi

  if [[ $USE_DLL != 0 ]]
  then
    echo "\n#define GVDLL 1" >> config.h
  fi
  if [[ $USE_DLL != 2 ]]
  then
    cp cmd/dot/Makefile xx
    sed '/dot -c/d' xx > cmd/dot/Makefile
  fi

  echo "Finish Setup" >> $LFILE
}

# To build libltdl:
#   cd libltdl
#   CC=ncc CFLAGS=-D__WINDOWS__ configure --prefix=/c/graphviz/local
#   make install
#   cd $INSTALLROOT/lib
#   mkDLL ltdl

function mkGvpr
{
  FILES="actions.o compile.o gvpr.o gprstate.o parse.o queue.o"

  for f in $FILES
  do
    make $f
    if [[ $? != 0 ]]
    then
      return 1
    fi 
  done

  echo "ncc -g -L$INSTALLROOT/lib -o gvpr.exe  actions.o compile.o gvpr.o gprstate.o parse.o queue.o ../../lib/ingraphs/.libs/libingraphs_C.a -lexpr -lagraph -lcdt"
  ncc -g -L$INSTALLROOT/lib -o gvpr.exe  actions.o compile.o gvpr.o gprstate.o parse.o queue.o ../../lib/ingraphs/.libs/libingraphs_C.a -lexpr -lagraph -lcdt
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  cp gvpr.exe $INSTALLROOT/bin
  return 0
}

function mkDot
{
  make dot.o
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  echo "ncc -g -o dot.exe dot.o  -L$INSTALLROOT/lib -L$GTKDIR/lib -lgvc -lgraph -lpathplan -lexpat -lz -lltdl" 
  ncc -g -o dot.exe dot.o  -L$INSTALLROOT/lib -L$GTKDIR/lib -lgvc -lgraph -lpathplan -lexpat -lz -lltdl
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  cp dot.exe $INSTALLROOT/bin
  return 0
}

function mkTool
{
  echo ncc ${1}.o ${3} ../../lib/ingraphs/.libs/libingraphs_C.a -L$INSTALLROOT/lib $2 -lcdt -o ${1}
  ncc -g ${1}.o ${3} ../../lib/ingraphs/.libs/libingraphs_C.a -L$INSTALLROOT/lib $2 -lcdt -o ${1}
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  cp ${1}.exe $INSTALLROOT/bin
  return 0
}

function mkTools
{
  TBASE="acyclic bcomps ccomps colxlate cvtgxl dijkstra dot2gxl gc gvcolor gxl2dot nop sccmap tred unflatten gvpack"

  for f in $TBASE
  do
    make ${f}.o
    if [[ $? != 0 ]]
    then
      return 1
    fi 
  done

  mkTool acyclic -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool bcomps -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool ccomps -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool dijkstra -lagraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool gc -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool gvcolor -lgraph colxlate.o
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool gxl2dot "-L$GTKDIR/lib -lagraph -lgraph -lexpat" "cvtgxl.o dot2gxl.o" 
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool nop -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool sccmap -lagraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool tred -lgraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool unflatten -lagraph
  if [[ $? != 0 ]]
  then
    return 1
  fi 
  mkTool gvpack "-lgvplugin_neato_layout -lgvc -lgraph"
  if [[ $? != 0 ]]
  then
    return 1
  fi 

  return 0
}

function Build 
{
  cd $GVIZ_HOME

  cd lib
  make install >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make lib"
  fi 

  cd ../plugin
  make install >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make plugins"
  fi 

  # Go to install directory and move libraries from graphviz
  # subdirectory to main lib directory
  cd $INSTALLROOT/lib/graphviz
  mv lib*.a ..
  cd ..
  rm -rf graphviz

  if [[ $USE_DLL != 0 ]]
  then
    mkDLL cdt
    mkDLL pathplan
    mkDLL graph -lcdt
    mkDLL agraph -lcdt
    cp $GVIZ_HOME/lib/expr/.libs/libexpr_C.a libexpr.a
    mkDLL expr -lcdt
    mkDLL gvc "-L$GTKDIR/lib -lpathplan -lgraph -lcdt -lexpat -lz -lltdl" 
    mkDLL gvplugin_core "-L$GTKDIR/lib -lgvc -lgraph -lcdt -lexpat -lz -lltdl"
    mkDLL gvplugin_dot_layout "-L$GTKDIR/lib -lgvc -lgraph -lpathplan -lcdt -lexpat -lz -lltdl"
    mkDLL gvplugin_neato_layout "-L$GTKDIR/lib -lgvc -lpathplan -lgraph -lcdt -lexpat -lz -lltdl"
    mkDLL gvplugin_gd "-L$GTKDIR/lib -lgvc -lpathplan -lgraph -lcdt -lpng -ljpeg -lfontconfig -lfreetype -lcairo -liconv -lexpat -lz -lltdl" 
#    mkDLL gvplugin_gdk_pixbuf "-L$GTKDIR/lib -lcairo -lgdk_pixbuf-2.0 -lltdl" 
    mkDLL gvplugin_pango "-L$GTKDIR/lib -lgvc -lfontconfig  -lfreetype  -ljpeg  -lpng  -lexpat  -lz -lcairo -lpango-1.0 -lpangocairo-1.0 -lgobject-2.0 -lgtk-win32-2.0 -lglib-2.0 -lgdk-win32-2.0 -latk-1.0 -lgdk_pixbuf-2.0"  
  fi 

  cd $GVIZ_HOME/cmd
  cd lefty
  make install >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make lefty"
  fi 

  cd ../lneato
  make install >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make lneato"
  fi 

  cd ../dotty
  make install >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make dotty"
  fi 

  cd ../gvpr
  if [[ $USE_DLL != 0 ]]
  then
    mkGvpr >> $LFILE 2>&1 
  else
    make install >> $LFILE 2>&1
  fi
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make gvpr"
  fi 

  cd ../tools
  if [[ $USE_DLL != 0 ]]
  then
    mkTools >> $LFILE 2>&1 
  else
    make install >> $LFILE 2>&1
  fi
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make tools"
  fi 

  cd ../dot
  if [[ $USE_DLL != 0 ]]
  then
    mkDot >> $LFILE 2>&1 
  else
    make install >> $LFILE 2>&1
  fi
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make dot"
  fi 

#  make install >> $LFILE 2>&1
#  if [[ $? != 0 ]]
#  then
#    ErrorEx "failure to make cmd"
#  fi 

}

# $1 is the library base name xxx
# create xxx.lib and xxx.dll, moving the latter to ../bin
function mkDLL
{
  LIB=$1
  > $LIB.ign
  echo "nld -L. -G -o $LIB.dll -Bwhole-archive lib$LIB.a -Bno-whole-archive $LIB.ign" $2 >> $LFILE 2>&1
  nld -L. -G -o $LIB.dll -Bwhole-archive lib$LIB.a -Bno-whole-archive $LIB.ign $2 >> $LFILE 2>&1
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to make dll $1"
  fi
  mv $LIB.dll ../bin
  rm $LIB.ign
  return 0
}

GTKPROGS="fc-cache fc-cat fc-list fc-match"

GTKDLLS="iconv intl jpeg62 libcairo-2 libexpat libfontconfig-1 \
libfreetype-6 libglib-2.0-0 libgmodule-2.0-0 libgobject-2.0-0 \
libpango-1.0-0 libpangocairo-1.0-0 libpangoft2-1.0-0 libpangowin32-1.0-0 \
libpng12 libxml2 zlib1 libgdk_pixbuf-2.0-0"

# Add additional software (DLLs, manuals, examples, etc.) to tree
function Install
{
# Add 3rd party programs and libraries
  cd $GTKDIR/bin
  echo "Copying 3rd party programs" >> $LFILE 2>&1   
  for l in $GTKPROGS
  do
    cp $l.exe $INSTALLROOT/bin
  done
  echo "Copying 3rd party libraries" >> $LFILE 2>&1   
  for l in $GTKDLLS
  do
    cp $l.dll $INSTALLROOT/bin
  done
  cp -r $GTKDIR/lib/pango $INSTALLROOT/lib
  cp -r $GTKDIR/etc $INSTALLROOT

  # For some reasson, fc-cache and fc-list are linked against
  # fontconfig.dll rather than libfontconfig-1.dll, so we make a copy.
  cd $INSTALLROOT/bin
  cp libfontconfig-1.dll fontconfig.dll
  
  # Add extra software
  cd $ROOT/add-on
  cp Uninstall.exe $INSTALLROOT
  cp fonts.conf $INSTALLROOT/etc/fonts
  cp props.txt GVedit.exe GVedit.html GVUI.exe Settings.ini $INSTALLROOT/bin
  cp comdlg32.ocx $INSTALLROOT/bin

  if [[ $USE_DLL == 0 ]]
  then
    cd $INSTALLROOT/bin
    if [[ -f dot_static.exe ]]
    then
      mv dot_static.exe dot.exe
    fi
    cd ..
  fi
  
# Create "soft" links. At present, hard links appear necessary for
# GVedit and GVUI. It is hoped this will change with the new GVUI.
  echo "Create soft links" >> $LFILE 2>&1   
  echo cd $INSTALLROOT/bin >> $LFILE 2>&1
  cd $INSTALLROOT/bin
  cp dot.exe neato.exe
  cp dot.exe twopi.exe
  cp dot.exe circo.exe
  cp dot.exe fdp.exe
  cp gxl2dot.exe dot2gxl.exe

  if [[ $USE_DLL == 2 ]]
  then
    ./dot -c >> $LFILE 2>&1   
    if [[ $? != 0 ]]
    then
      ErrorEx "failure to initialize dot"
    fi 
  fi
  
  cd $ROOT
  echo "Install extra files" >> $LFILE 2>&1   
  cp -r $GVIZ_HOME/graphs $INSTALLROOT
  cp -r $GVIZ_HOME/dot.demo $INSTALLROOT
  cp -r $GVIZ_HOME/contrib $INSTALLROOT
  cp -r $GVIZ_HOME/doc/*.pdf $INSTALLROOT/doc 

  echo "Install man pages" >> $LFILE 2>&1   
  cp $GVIZ_HOME/cmd/dot/dot.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/gvpr/gvpr.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/dotty/dotty.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/lefty/lefty.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/lneato/lneato.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/dot/dot.pdf $INSTALLROOT/doc/man/neato.pdf
  cp $GVIZ_HOME/cmd/dot/dot.pdf $INSTALLROOT/doc/man/twopi.pdf
  cp $GVIZ_HOME/cmd/dot/dot.pdf $INSTALLROOT/doc/man/circo.pdf
  cp $GVIZ_HOME/cmd/dot/dot.pdf $INSTALLROOT/doc/man/fdp.pdf
  cp $GVIZ_HOME/cmd/tools/*.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/cmd/tools/gxl2dot.pdf $INSTALLROOT/doc/man/dot2gxl.pdf
  cp $GVIZ_HOME/lib/agraph/agraph.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/lib/cdt/cdt.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/lib/graph/graph.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/lib/gvc/gvc.pdf $INSTALLROOT/doc/man
  cp $GVIZ_HOME/lib/pathplan/pathplan.pdf $INSTALLROOT/doc/man

  if [[ -d $INSTALLROOT/share ]]
  then
    echo "Install share software" >> $LFILE 2>&1   
    echo "cd $INSTALLROOT/share/graphviz" >> $LFILE 2>&1
    cd $INSTALLROOT/share/graphviz
    if [[ -d lefty ]]
    then
      mv lefty ../../lib
    fi

    cd ../..
    rm -rf share
  fi

  if [[ -d $INSTALLROOT/include/graphviz ]]
  then
    echo "Install include/graphviz files" >> $LFILE 2>&1   
    cd $INSTALLROOT
    mv include/graphviz ginclude
    rm -rf include
    mv ginclude include
  fi

    # Remove makefiles
  cd $INSTALLROOT
  rm -f $(find graphs -name Makefile* -print)
  rm -f $(find contrib -name Makefile -print)
  rm -f $(find contrib -name Makefile.in -print)

  echo "Finish Install" >> $LFILE 2>&1   
}

function PackageTar
{
  cd $ROOT

  if [[ -n "$RELEASE" ]]
  then
    localstr=local.$RELEASE
  else
    localstr=local
  fi

    # Create tgz package
  TGZFILE=graphviz-win-${VERSION}${STATIC}.bin.tar.gz
  echo "creating tgz $localstr" >> $LFILE
  pax -w -x tar -s/$localstr/graphviz-${VERSION}/ $localstr | gzip > $TGZFILE
  if [[ -f $TGZFILE ]]
  then
    echo "SUCCESS : tgz file created" >> $LFILE
    echo "uploading the tgz file $TGZFILE" >> $LFILE
    chmod 644 $TGZFILE
    PutFile $TGZFILE
#    rm $TGZFILE
  else
    echo "FAILED : could not create tgz file" >> $LFILE
    ErrorEx "windows install failure"
  fi
  cd ..
}
  
function Package
{
  cd $ROOT

  # new package
  rm -rf release
  mkDir release
  cd add-on
  cp Setup.exe Selfstbv.exe $ROOT/release
  sed "s/XXX/$SHORTVERSION/" Graphviz.ini > $ROOT/release/Graphviz.ini
  cd $ROOT
  ./gsetup.sh $ROOT "${RELEASE}" >> $LFILE 2>&1
 
  # now copy it to the webserver
  UPLOADFILE=graphviz-${VERSION}${STATIC}.exe
  PACKAGEDFILE=$ROOT/release/graphvizw.exe
  
  if [[ -f $PACKAGEDFILE ]]
  then
    echo "SUCCESS : installation package created" >> $LFILE
    mv $PACKAGEDFILE $UPLOADFILE    # attach version info
  
    echo "uploading the package file $UPLOADFILE" >> $LFILE
    chmod 755 $UPLOADFILE
    PutFile $UPLOADFILE
  else
    echo "FAILED : could not create installation package" >> $LFILE
      ErrorEx "windows install failure"
  fi
  
}

function PutLog 
{
  LOG=graphviz-windows-buildlog-${VERSION}.txt
  echo "uploading the log $LOG file" >> $LFILE
  cp $LFILE $LOG
  PutFile $LOG
  rm -f $LOG
}

SETOPTS=0
GET=0        # get from cvs machine and create tree
SETUP=0      # run configure and modify source
BUILD=0      # build all of the software
INSTALL=0    # install the software locally
PACKAGE=0    # make main package and copy all software to cvs machine
USE_DLL=2
CONFARG=-PL
STATIC=
RELEASE=

Usage='build [-CDLGSBIP] [-R<relno>] \n
 -C : core package \n
 -D : build DLLs \n
 -L : build using plugins \n
 -G : get software \n
 -S : setup software \n
 -B : build \n
 -I : install in local tree \n
 -P : create package and ship'

while getopts :XCDLGSBIPR: c
do
  case $c in
  D )       # build with dlls
    CONFARG=-P
    USE_DLL=1
    ;;
  L )       # use ltdl
    USE_DLL=2
    CONFARG=-PL
    ;;
  C )       # general static package
    STATIC=".static"
    SUBJECT="-s'Static windows build failure'"
    CONFARG=-P
    USE_DLL=0
    ;;
  G )       # get
    SETOPTS=1
    GET=1
    ;;
  S )       # setup
    SETOPTS=1
    SETUP=1
    ;;
  B )       # build
    SETOPTS=1
    BUILD=1
    ;;
  I )       # install
    SETOPTS=1
    INSTALL=1
    ;;
  P )       # package
    SETOPTS=1
    PACKAGE=1
    ;;
  R )       # build official release
    RELEASE=$OPTARG
    INPKG=graphviz-${RELEASE}.tar.gz
    SOURCEFILE=$SOURCE/ARCHIVE/$INPKG
    DESTDIR=$SOURCE/ARCHIVE
    INSTALLROOT=$ROOT/local.${RELEASE}
    GVIZ_HOME=$ROOT/gviz.${RELEASE}       
    ;;
  :)
    echo $OPTARG requires a value
    exit 2
    ;;
  \? )
    if [[ "$OPTARG" == '?' ]]
    then
      echo $Usage
      exit 0
    else
      echo "build: unknown flag $OPTARG - ignored"
    fi
    ;;
  esac
done

shift $((OPTIND-1))


if [[ $# > 0 ]]
then
  LFILE=$1
fi


# If no passes picked, do all
if [[ $SETOPTS == 0 ]]
then
  GET=1
  SETUP=1
  BUILD=1
  INSTALL=1
  PACKAGE=1
fi

cd $ROOT
rm -f $LFILE

if [[ $GET == 1 ]]
then
  Get
fi
if [[ $SETUP == 1 ]]
then
  Setup
fi
SetVersion
if [[ $BUILD == 1 ]]
then
  Build
fi
if [[ $INSTALL == 1 ]]
then
  Install
fi
if [[ $PACKAGE == 1 ]]
then
  Package
  PackageTar
#  PutLog
fi

exit 0

