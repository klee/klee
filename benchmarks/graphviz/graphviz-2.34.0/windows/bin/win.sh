
HOME=/C/graphvizCVS/builddaemon
GVIZ_HOME=$HOME/graphviz-win                # directory for source and build
PACKAGE_HOME=package                  # directory for output package
PACKAGEDFILE=$PACKAGE_HOME/graphviz.exe     # name of output package
THIRDPARTY_HOME=$HOME/third-party           # add-on libraries
TCLSH=/C/tcl/bin/tclsh.exe                  # tcl 
INPKG=graphviz-win.tgz                      # input CVS package
#WISE=/C/wisein~1/wise32.exe                 # = /Wise InstallMaker
WISE=/C/progra~1/wisein~2/wise32.exe        # = Wise InstallMaker
SCP=/C/progra~1/putty/pscp.exe        # scp
SOURCEM=www.graphviz.org
SOURCEID=ellson@www.graphviz.org
SOURCE=/home/ellson/www.graphviz.org/pub/graphviz
#NSOURCE=www.graphviz.org:/home/ellson/www.graphviz.org/pub/graphviz
SOURCEFILE=$SOURCE/CURRENT/$INPKG
#NSOURCEFILE=$NSOURCE/CURRENT/$INPKG
DESTDIR=$SOURCE/CURRENT
#NDESTDIR=$NSOURCE/CURRENT

OPTION=Release
WISEFLAG=0
VERSION=$(date +%K)
RELEASE=0

# scp data
#SCPCOMMAND=scp/pscp.exe
#KEYFILE=scp/priv.PPK
#REMOTEFILE=/var/www/html/pub/graphviz/graphviz-current.tar.gz
#$SCPCOMMAND -i $KEYFILE -l kp www.graphviz.org:$REMOTEFILE graphviz.tar.gz

# wget data
#INPKG=${URL##*/}
#wget -q $URL

function setVersion {
  if [[ -f $GVIZ_HOME/config.h ]]
  then
    VERSION=$(grep 'VERSION' $GVIZ_HOME/config.h)
    VERSION=${VERSION#*VERSION \"}
    VERSION=${VERSION%\"*}
  fi
}

# Copy file from CVS server. We use the tcl script get.tcl.
# Other possibilities are to use scp or wget.
function getFile
{
#  $TCLSH get.tcl $INPKG
#  scp -q $SOURCEFILE . >> $LFILE 2>&1
#  echo ssh graphviz. scp -q $NSOURCEFILE . >> $LFILE 2>&1
#  ssh soohan scp -q $NSOURCEFILE .
#  echo rcp raptor:graphviz-win.tgz . >> $LFILE 2>&1
#  rcp raptor:graphviz-win.tgz . >> $LFILE 2>&1
#  echo ssh $SOURCEID "cat $SOURCEFILE" >> $LFILE
#  ssh $SOURCEID "cat $SOURCEFILE" > $INPKG 2>> $LFILE
   echo $SCP -q $SOURCEM:$SOURCEFILE . >> $LFILE 2>&1
   $SCP -q $SOURCEM:$SOURCEFILE . >> $LFILE 2>&1
  
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to get source"
  fi
}

# Copy file back to CVS server
function putFile
{
#  $TCLSH tclscript.tcl $1 >> $LFILE 2>&1
#  scp -q $1 $DESTDIR >> $LFILE 2>&1 &
#  PID=$(ps -e | grep scp | awk '{print $1 }')
#  sleep 10
#  kill $PID
#  BASE=$(basename $1)
#  echo rcp $1 raptor:. >> $LFILE 2>&1
#  rcp $1 raptor:. >> $LFILE 2>&1
#  echo ssh raptor scp -q $BASE $SOURCEM:$DESTDIR  >> $LFILE 2>&1
#  ssh raptor scp -q $BASE $SOURCEM:$DESTDIR  >> $LFILE 2>&1
#  ssh raptor rm $BASE >> $LFILE 2>&1
#  echo "cat $1 | ssh $SOURCEID cat - $DESTDIR" >> $LFILE 
#  cat $1 | ssh $SOURCEID "cat - > $DESTDIR/$BASE" 
  echo "$SCP -q $1 $SOURCEM:$DESTDIR" >> $LFILE 2>&1  
  $SCP -q $1 $SOURCEM:$DESTDIR  
  if [[ $? != 0 ]]
  then
    ErrorEx "failure to put $1"
  fi
}

LFILE=$HOME/build.log    # log file
GET=1        # get from cvs machine and create tree
BUILD=1      # build all of the software
INSTALL=1    # install the software locally
PACKAGE=1    # make main package and copy all software to cvs machine
TEST=1       # run regression tests
CLEANUP=0    # cleanup

while [[ $# > 0 ]]
do
  case $1 in
  -B )       # no build
	BUILD=0
    ;;
  +B )       # build only
	INSTALL=0
	PACKAGE=0
	GET=0
	TEST=0
	CLEANUP=0
    ;;
  -G )       # no get
	GET=0
    ;;
  +G )       # get only
	INSTALL=0
	PACKAGE=0
	BUILD=0
	TEST=0
	CLEANUP=0
    ;;
  -I )       # no install
	INSTALL=0
    ;;
  +I )       # install only
	GET=0
	PACKAGE=0
	BUILD=0
	TEST=0
	CLEANUP=0
    ;;
  -P )       # no packaging
	PACKAGE=0
    ;;
  +P )       # packaging only
	GET=0
	INSTALL=0
	BUILD=0
	TEST=0
	CLEANUP=0
    ;;
  -R )       # build official release
    shift
	RELEASE=$1
    INPKG=graphviz-win-${RELEASE}.tar.gz
    SOURCEFILE=$SOURCE/ARCHIVE/$INPKG
    DESTDIR=$SOURCE/ARCHIVE
    ;;
  -T )       # no test
	TEST=0
    ;;
  -C )       # no cleanup
	CLEANUP=0
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

rm -f $LFILE

function ErrorEx
{
	rcp build.log erg@raptor:src/gviz/build.log.$VERSION
	rsh erg@raptor "echo $1 | mail erg"
	exit 1
}

function Get
{
	cd $HOME
	rm -f $PACKAGEDFILE

	# cleanup old version
  	echo "Removing old version" >> $LFILE
  	rm -rf $GVIZ_HOME

	# download the zip file from webserver
	echo "Building graphviz : "$(date) >> $LFILE
	echo "downloading the current source package $SOURCEFILE" >> $LFILE
	getFile

	# open the package
	#gunzip < graphviz.tar.gz | tar xf -
	echo "unpacking the package" >> $LFILE
	#pax -rf $INPKG >> $LFILE 2>&1
	echo "(gunzip < $INPKG | tar xf - )" >> $LFILE 2>&1
	(gunzip < $INPKG | tar xf - ) >> $LFILE 2>&1
	if [[ $? != 0 ]]
	then
  		ErrorEx "failure in unwrapping"
	fi

	# overlay third party libraries
	echo "overlay third-party software" >> $LFILE
	cp -r $THIRDPARTY_HOME $GVIZ_HOME/third-party
}

function Build 
{
	# invoke the winmake script
	echo "invoking wmake.sh script" >> $LFILE
	cd $GVIZ_HOME
	wmake.sh $LFILE
	RV=$?
	cd $HOME
	if [[ $RV != 0 ]]
	then
		ErrorEx "windows build failure"
	fi
}

function finstall
{
  SRC=$SRCDIR/$1
  if [[ -n "$2" ]]
  then
    DST=$DSTDIR/$2
  else
    DST=$DSTDIR/$1
  fi

  if [[ -f $SRC ]]
  then
    echo "copying $SRC" >> $LFILE
    cp $SRC $DST
  else
    WISEFLAG=1
    echo "copying $SRC failed ..." >> $LFILE
  fi
}

function cdInstall
{
	SRCDIR=$GVIZ_HOME/cmd/$1/Release
    finstall $1.exe $2
}

CMDS=(dot lefty dotty lneato)
TOOLS=(Acyclic ccomps gvcolor gc nop sccmap tred unflatten gxl2dot dijkstra bcomps gvpack)

function Install 
{
	# copy binaries to package directory
	echo "copying binaries to package directory" >> $LFILE
	
	DSTDIR=$PACKAGE_HOME/source/bin
    for f in ${CMDS[@]}
    do
      cdInstall $f
    done
	cdInstall dot neato.exe
	cdInstall dot twopi.exe
	cdInstall dot fdp.exe
	cdInstall dot circo.exe
	
	SRCDIR=$GVIZ_HOME/cmd/tools/Release
    for f in ${TOOLS[@]}
    do
      finstall $f.exe
    done
	finstall gxl2dot.exe dot2gxl.exe
	
	SRCDIR=$GVIZ_HOME/cmd/gvui
	finstall GVUI.exe
	
	SRCDIR=$GVIZ_HOME/cmd/dotty
	finstall dotty.lefty
	finstall dotty_draw.lefty
	finstall dotty_edit.lefty
	finstall dotty_layout.lefty
	finstall dotty_ui.lefty
	
	# install libraries
	echo "copying libraries" >> $LFILE
	LIBS=(ingraphs agraph cdt circogen common dotgen fdpgen gd graph gvc neatogen pack pathplan plugin twopigen)
	if [[ ! -d $PACKAGE_HOME/source/lib ]]
	then
	  mkdir $PACKAGE_HOME/source/lib
	fi
	SRCDIR=$GVIZ_HOME/lib/lib/Release
	DSTDIR=$PACKAGE_HOME/source/lib
	for d in ${LIBS[@]}
	do
	  finstall $d.lib
	done
}

function Package 
{
	# add copying of docs and graph files 
	echo "copying docs" >> $LFILE
	DOCS=(dotguide.pdf  dottyguide.pdf  leftyguide.pdf  neatoguide.pdf)
	if [[ ! -d $PACKAGE_HOME/source/doc ]]
	then
	  mkdir $PACKAGE_HOME/source/doc
	fi
	SRCDIR=$GVIZ_HOME/doc
	DSTDIR=$PACKAGE_HOME/source/doc
	for d in ${DOCS[@]}
	do
	  finstall $d
	done
	echo "copying man pages" >> $LFILE
	if [[ ! -d $PACKAGE_HOME/source/doc/man ]]
	then
	  mkdir $PACKAGE_HOME/source/doc/man
	fi
	cp $GVIZ_HOME/cmd/dot/dot.pdf  $PACKAGE_HOME/source/doc/man
	cp $GVIZ_HOME/cmd/dot/dot.pdf  $PACKAGE_HOME/source/doc/man/neato.pdf
	cp $GVIZ_HOME/cmd/dot/dot.pdf  $PACKAGE_HOME/source/doc/man/twopi.pdf
	cp $GVIZ_HOME/cmd/dot/dot.pdf  $PACKAGE_HOME/source/doc/man/circo.pdf
	cp $GVIZ_HOME/cmd/dot/dot.pdf  $PACKAGE_HOME/source/doc/man/fdp.pdf
	cp $GVIZ_HOME/cmd/tools/*.pdf  $PACKAGE_HOME/source/doc/man
	cp $GVIZ_HOME/cmd/tools/gxl2dot.pdf  $PACKAGE_HOME/source/doc/man/dot2gxl.pdf

	cp -r $GVIZ_HOME/graphs $PACKAGE_HOME/source
	
	# install headers
	echo "copying include files" >> $LFILE
	DSTDIR=$PACKAGE_HOME/source/include
	if [[ ! -d $PACKAGE_HOME/source/include ]]
	then
	  mkdir $PACKAGE_HOME/source/include
	fi
	SRCDIR=$GVIZ_HOME/lib/agraph
        finstall agraph.h
	SRCDIR=$GVIZ_HOME/lib/cdt
        finstall cdt.h 
	SRCDIR=$GVIZ_HOME/lib/common
        finstall types.h 
        finstall geom.h 
        finstall color.h 
        finstall textpara.h 
        finstall usershape.h 
	SRCDIR=$GVIZ_HOME/lib/gvc
        finstall gvc.h 
        finstall gvcext.h 
        finstall gvcjob.h 
        finstall gvcommon.h 
        finstall gvplugin.h 
        finstall gvplugin_layout.h 
        finstall gvplugin_render.h 
        finstall gvplugin_textlayout.h 
        finstall gvplugin_loadimage.h 
	SRCDIR=$GVIZ_HOME/lib/pack
        finstall pack.h
	SRCDIR=$GVIZ_HOME/lib/graph
        finstall graph.h 
	SRCDIR=$GVIZ_HOME/lib/pathplan
        finstall pathgeom.h 

	# Create tgz package
	TGZFILE=graphviz-win-${VERSION}.bin.tar.gz
	if [[ $WISEFLAG == 0 ]]
	then
	  echo "creating tgz package" >> $LFILE
      cd package
	  pax -w -x tar -s/source/graphviz-${VERSION}/ source | gzip > $TGZFILE
	  if [[ -f $TGZFILE ]]
	  then
	    echo "SUCCESS : tgz file created" >> $LFILE
	    echo "uploading the tgz file $TGZFILE" >> $LFILE
	    chmod 644 $TGZFILE
	    putFile $TGZFILE
	    rm $TGZFILE
	  else
	    echo "FAILED : could not create tgz file" >> $LFILE
        ErrorEx "windows install failure"
	  fi
      cd ..
	fi

	# Create wise package
	# Assume graphviz.wse has instructions for making package
	if [[ $WISEFLAG == 0 ]]
	then
	  echo "invoking wise installation script" >> $LFILE
	  rm -f package/*.exe
	  $WISE /c package/graphviz.wse
	fi
	
	# now copy it to the webserver
	UPLOADFILE=package/graphviz-${VERSION}.exe
	
	if [[ -f $PACKAGEDFILE ]]
	then
	  echo "SUCCESS : installation package created" >> $LFILE
	  mv $PACKAGEDFILE $UPLOADFILE    # attach version info
	
	  echo "uploading the package file $UPLOADFILE" >> $LFILE
	  chmod 755 $UPLOADFILE
	  putFile $UPLOADFILE
	else
	  echo "FAILED : could not create installation package" >> $LFILE
      ErrorEx "windows install failure"
	fi
	# copy commands separately
#	DSTDIR=$PACKAGE_HOME/source/bin
#	echo "uploading commands" >> $LFILE
#	putFile $DSTDIR/dot.exe
#	putFile $DSTDIR/neato.exe
#	putFile $DSTDIR/twopi.exe
#	putFile $DSTDIR/circo.exe
#	putFile $DSTDIR/fdp.exe


}

function Test {
	echo --------- TESTING COMMANDS ---------- >> $LFILE
	cd rtest
	ARCH=windows doit -d >> $LFILE 2>&1
	cd ..
	echo --------- END TESTING ---------- >> $LFILE
}
	
function Cleanup {
	# do some cleanup here
	echo "cleaning up the build directory" >> $LFILE

	rm $HOME/$INPKG
	rm $PACKAGE_HOME/source/bin/*.exe
	rm $PACKAGE_HOME/source/bin/*.lefty
	rm -r $PACKAGE_HOME/source/graphs
	rm -r $PACKAGE_HOME/source/doc
	rm $LOG
}

function PutLog {
	LOG=graphviz-windows-buildlog-${VERSION}.txt
	echo "uploading the log $LOG file" >> $LFILE
	cp $LFILE $LOG
	putFile $LOG
}

if [[ $GET == 1 ]]
then
  Get
fi
setVersion
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
fi
if [[ $TEST == 1 ]]
then
  Test
fi
if [[ $PACKAGE == 1 ]]
then
  PutLog
fi
if [[ $CLEANUP == 1 ]]
then
  Cleanup
fi

exit 0
