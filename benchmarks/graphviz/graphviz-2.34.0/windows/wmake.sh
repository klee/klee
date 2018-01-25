# ksh script for building graphviz
#
# Set the following variable to the root location of MS VC++ on your machine 
# = Program Files/Microsoft Visual Studio

export PATH=/msdev/VC98/bin:/msdev/Common/MsDev98/bin:$PATH
export LIB='C:\progra~1\micros~3\vc98\lib'
export INCLUDE='C:\progra~1\micros~3\vc98\include'

ROOT=/C/graphvizCVS/builddaemon
GVIZ=$ROOT/graphviz-win
LFILE=$ROOT/LOG
VERSION=Release
typeset -i RV=0

while [[ $# > 0 ]]
do
  case $1 in
  -d )
    VERSION=Debug
    ;;
  -* )
    echo "wmake: unknown flag $1 - ignored"
    ;;
  * )
    LFILE=$1
    ;;
  esac
  shift
done

# libs to be built
LIBS=(cdt graph agraph gd pathplan common gvc pack neatogen dotgen twopigen circogen fdpgen ingraphs)

# commands to be built
CMDS=(dot lefty/gfx lefty dotty lneato)

# gui's to be built
GUIS=(cmd/lefty/gfx cmd/lefty dotty lneato)

# tools to be built
TOOLS=(Acyclic ccomps gvcolor gc nop sccmap tred unflatten gxl2dot dijkstra bcomps gvpack)

function doComp
{
  f=$1
  echo "###################" >> $LFILE
  echo "compiling $f" >> $LFILE
  nmake -nologo -f $f.mak CFG="$f - Win32 $VERSION" >> $LFILE 2>&1
  RV=$((RV | $?))
}

function mkDir
{
  if [[ ! -d $1 ]]
  then
    mkdir $1
  fi
}

cd $GVIZ

# process libs
cd lib
mkDir lib
mkDir lib/Release
mkDir lib/Debug
for d in ${LIBS[@]}
do
  cd $d
  doComp $d
  cd ..
done
cd ..

# process plugins
cd plugin
doComp plugin
cd ..

# process commands
cd cmd
for d in ${CMDS[@]}
do
  cd $d
  p=${d##*/}
  doComp $p
  cd $GVIZ/cmd
done
cd ..

# process tools
cd cmd/tools
for d in ${TOOLS[@]}
do
  doComp $d
done
cd $GVIZ

exit $RV
