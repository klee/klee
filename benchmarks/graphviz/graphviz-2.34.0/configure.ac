# $Id$ $Revision$

# Process this file with autoconf to produce a configure script
# Try to use version available on FC8
AC_PREREQ(2.61)

dnl graphviz package version number, (as distinct from shared library version)
dnl For the minor number: odd => unstable series
dnl                       even => stable series
dnl For the micro number: 0 => in-progress development (from cvs)
dnl                       timestamp => tar-file snapshot or release
m4_define(graphviz_version_major, 2)
m4_define(graphviz_version_minor, 34)
#NB: the next line gets changed to a date/time string for development releases
m4_define(graphviz_version_micro, 0)

AC_INIT([graphviz],[graphviz_version_major.graphviz_version_minor.graphviz_version_micro],[http://www.graphviz.org/])

GRAPHVIZ_VERSION_MAJOR=graphviz_version_major()
GRAPHVIZ_VERSION_MINOR=graphviz_version_minor()
GRAPHVIZ_VERSION_MICRO=graphviz_version_micro()
AC_SUBST([GRAPHVIZ_VERSION_MAJOR])
AC_SUBST([GRAPHVIZ_VERSION_MINOR])
AC_SUBST([GRAPHVIZ_VERSION_MICRO])

VERSION_DATE=`date -u +%Y%m%d.%H%M`
AC_SUBST([VERSION_DATE])

#NB: "stable" on the next line gets changed to "development" when building a development release
GRAPHVIZ_COLLECTION=stable

GRAPHVIZ_SOURCES=graphviz/$GRAPHVIZ_COLLECTION/SOURCES
AC_SUBST([GRAPHVIZ_COLLECTION])
AC_SUBST([GRAPHVIZ_SOURCES])

dnl ===========================================================================

# libtool shared library version for plugins

# Increment if the interface has additions, changes, removals.
GVPLUGIN_CURRENT=6

# Increment any time the source changes; set to
# 0 if you increment CURRENT
GVPLUGIN_REVISION=0

# Increment if any interfaces have been added; set to 0
# if any interfaces have been removed. removal has
# precedence over adding, so set to 0 if both happened.
GVPLUGIN_AGE=0

GVPLUGIN_CONFIG_FILE="config$GVPLUGIN_CURRENT"
AC_DEFINE_UNQUOTED(GVPLUGIN_CONFIG_FILE,"$GVPLUGIN_CONFIG_FILE",[Filename for plugin configuration file.])
AC_SUBST([GVPLUGIN_CONFIG_FILE])

GVPLUGIN_VERSION_INFO="$GVPLUGIN_CURRENT:$GVPLUGIN_REVISION:$GVPLUGIN_AGE"
AC_SUBST([GVPLUGIN_VERSION_INFO])

dnl ===========================================================================


AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

dnl -----------------------------------
dnl Checks for system type

AC_CANONICAL_HOST

# This nonsense drives me crazy!!
case "${host_os}" in
  *irix* | *nonstopux* )
    GVPLUGIN_VERSION=`expr $GVPLUGIN_CURRENT + 1`
    ;;
  * )
    GVPLUGIN_VERSION=$GVPLUGIN_CURRENT
    ;;
esac
AC_DEFINE_UNQUOTED(GVPLUGIN_VERSION,$GVPLUGIN_VERSION,[Compatibility version number for plugins.])
AC_SUBST([GVPLUGIN_VERSION])

GRAPHVIZ_VERSION_BUILD=0
GRAPHVIZ_VERSION_REVISION=0
case "${host_os}" in
  *uwin* | *cygwin* | *mingw32* )
	# Windows version numbers
	# For the build number: months since Jan 2000, day of month from the timestamp
	# For the revision number: hour, minute from the timestamp
	if test $GRAPHVIZ_VERSION_MICRO != "0"; then
		GRAPHVIZ_VERSION_BUILD=`expr \( ${GRAPHVIZ_VERSION_MICRO:0:4} - 2000 \) \* 12 + ${GRAPHVIZ_VERSION_MICRO:4:2}`${GRAPHVIZ_VERSION_MICRO:6:2}
		GRAPHVIZ_VERSION_REVISION=${GRAPHVIZ_VERSION_MICRO:9:4}
	fi
	;;
esac
AC_SUBST([GRAPHVIZ_VERSION_BUILD])
AC_SUBST([GRAPHVIZ_VERSION_REVISION])

AC_ARG_VAR(LIBPOSTFIX, [subscript to lib, e.g. "/64" for "/usr/lib/64" on Solaris])

if test -z "$LIBPOSTFIX"; then
  case "${host_os}" in
    *linux* )
      case "${host_cpu}" in
        aarch64 | powerpc64 | s390x | x86_64 | sparc64 )
          LIBPOSTFIX="64"
          ;;
      esac
      ;;
    *solaris* )
      case "${host_cpu}" in
        x86_64 | sparc64 )
          LIBPOSTFIX="/64"
          ;;
      esac
      ;;
  esac
fi

UWIN=no
CYGWIN=no
MINGW32=no
DARWIN9=no
case "${host_os}" in
  *uwin* )
	UWIN=yes
	DEFAULT_FONTPATH="C:/WINDOWS/FONTS;C:/WINNT/Fonts;C:/winnt/fonts"
	CFLAGS="${CFLAGS} -DWIN32 -DMSWIN32"
	CXXFLAGS="${CXXFLAGS} -DWIN32 -DMSWIN32"
	PATHSEPARATOR=";"
        NO_POSTSCRIPT_ALIAS=1
	BROWSER="xdg-open"
	;;
  *cygwin* )
	CYGWIN=yes
	DEFAULT_FONTPATH="C:/WINDOWS/FONTS;C:/WINNT/Fonts;C:/winnt/fonts"
	PATHSEPARATOR=";"
        NO_POSTSCRIPT_ALIAS=1
	AC_DEFINE(NO_POSTSCRIPT_ALIAS,1,[Alias PostScript font names to URW type 1 fonts])
	BROWSER="xdg-open"
	;;
  *mingw32* )
	MINGW32=yes
	DEFAULT_FONTPATH="C:/WINDOWS/FONTS;C:/WINNT/Fonts;C:/winnt/fonts"
	PATHSEPARATOR=":"
        NO_POSTSCRIPT_ALIAS=1
	BROWSER="xdg-open"
	;;
  *darwin9* )
	DARWIN=yes
	AC_DEFINE_UNQUOTED(DARWIN,1,[Define for any Darwin-based OS.])
	DARWIN9=yes
	DEFAULT_FONTPATH="~/Library/Fonts:/Library/Fonts:/Network/Library/Fonts:/System/Library/Fonts"
	PATHSEPARATOR=":"
        NO_POSTSCRIPT_ALIAS=1
	AC_DEFINE_UNQUOTED(DARWIN_DYLIB,"$DARWIN_DYLIB",[Define for Darwin-style shared library names.])
	BROWSER="open"
	case "${host_os}" in
	*darwin9.4* | *darwin9.5* )
		EXTRA_SMYRNA_LDFLAGS="-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib"
		;;
	esac
	;;
  *darwin* )
	DARWIN=yes
	AC_DEFINE_UNQUOTED(DARWIN,1,[Define for any Darwin-based OS.])
	DEFAULT_FONTPATH="~/Library/Fonts:/Library/Fonts:/Network/Library/Fonts:/System/Library/Fonts"
	PATHSEPARATOR=":"
        NO_POSTSCRIPT_ALIAS=1
	BROWSER="open"
	AC_DEFINE_UNQUOTED(DARWIN_DYLIB,"$DARWIN_DYLIB",[Define for Darwin-style shared library names.])
	;;
  * ) # This sucks!  Whatever happened to standards for file system layout?   (Good job its no longer used unless fontconfig is unavailable.)
	DEFAULT_FONTPATH="/usr/share/fonts/sun/TrueType:/usr/share/fonts/TrueType:/usr/X11/lib/X11/fonts/TrueType:/usr/share/fonts/sun/Type1:/usr/openwin/lib/X11/fonts/Type1:/usr/share/fonts/X11/Type1:/usr/share/fonts/default/Type1:/usr/X11R6/lib/X11/fonts/TrueType:/usr/X11R6/lib/X11/fonts/truetype:/usr/X11R6/lib/X11/fonts/TTF:/usr/share/fonts/truetype:/usr/openwin/lib/X11/fonts/TrueType:/usr/X11R6/lib/X11/fonts/Type1:/usr/common/share/fonts/ttf";
	PATHSEPARATOR=":"
        NO_POSTSCRIPT_ALIAS=0
	BROWSER="xdg-open"
	;;
esac
AC_SUBST([CYGWIN])
AC_SUBST([MINGW32])
AC_SUBST([DARWIN])
AC_SUBST([DARWIN9])
AC_SUBST([EXTRA_SMYRNA_LDFLAGS])
AC_DEFINE_UNQUOTED(DEFAULT_FONTPATH,"$DEFAULT_FONTPATH",[Path to TrueType fonts.])
AC_DEFINE_UNQUOTED(PATHSEPARATOR,"$PATHSEPARATOR",[Path separator character.])
AC_DEFINE_UNQUOTED(NO_POSTSCRIPT_ALIAS,$NO_POSTSCRIPT_ALIAS,[Postscript fontnames.])
AM_CONDITIONAL(WITH_WIN32, [test "x$UWIN" = "xyes" -o "x$CYGWIN" = "xyes" -o "x$MINGW32" = "xyes"])
AM_CONDITIONAL(WITH_DARWIN9, [test "x$DARWIN9" = "xyes"])

DEFAULT_DPI=96
AC_DEFINE_UNQUOTED(DEFAULT_DPI,$DEFAULT_DPI,Default DPI.)
AC_DEFINE_UNQUOTED(BROWSER,"$BROWSER",[Command to open a browser on a URL])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Suppress verbose compile lines
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h lib/common/types.h])

# put BUILDDATE in its own file so that only targets that use it get rebuilt
echo "#define BUILDDATE \"$VERSION_DATE\"" > builddate.h

# only update ast_common.h (for GNU builds) from master if changed
if ! `cmp -s $srcdir/ast_common.h.in ast_common.h`; then
	cp $srcdir/ast_common.h.in ast_common.h;
fi

AC_PREFIX_DEFAULT([/usr/local])
if test "x${prefix}" = "xNONE"; then
	prefix=${ac_default_prefix}
	AC_SUBST([prefix])
fi

# automatically check for other dependencies in $prefix first
if test "x${prefix}" != "x/usr"; then
	CPPFLAGS="$CPPFLAGS -I${prefix}/include"
	LDFLAGS="$LDFLAGS -L${prefix}/lib"
	PKG_CONFIG_PATH="$prefix/lib/pkgconfig$PATHSEPARATOR$PKG_CONFIG_PATH"
	export PKG_CONFIG_PATH
fi

# Set of all available languages
# ALL_LINGUAS=""

dnl -----------------------------------
# Static/Shared binaries 

AC_ARG_ENABLE(static,
        [AS_HELP_STRING([--enable-static],[build static executable])])
if test "x$enable_static" = "xyes"; then
	AC_ENABLE_STATIC
        use_static="Yes"
else
	AC_DISABLE_STATIC
        use_static="No (disabled by default)"
fi
AM_CONDITIONAL(ENABLE_STATIC, [test "x$enable_static" = "xyes"])

AC_ARG_ENABLE(shared,
        [AS_HELP_STRING([--enable-shared],[build shared executable])])
if test "x$enable_shared" != "xno"; then
	AC_ENABLE_SHARED
        use_shared="Yes"
	case "${host_os}" in
  		*uwin* | *cygwin* | *mingw32* )
			AC_DEFINE_UNQUOTED(GVDLL,1, [Define for DLLs on Windows.])
		;;
	esac

else
        use_shared="No (disabled)"
	AC_DISABLE_SHARED
fi
AM_CONDITIONAL(ENABLE_SHARED, [test "x$enable_shared" = "xyes"])

dnl -----------------------------------
dnl checks for various programs

AC_SEARCH_LIBS([strerror],[cposix])
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

# prefer GNU variants over, say, Oracle's or Apples
AC_CHECK_PROGS(SED,gsed sed,false)
AC_CHECK_PROGS(SORT,gsort sort,false)

AC_CHECK_PROG(EGREP,egrep,egrep,false)
AC_CHECK_PROG(GROFF,groff,groff,false)
AC_CHECK_PROG(PS2PDF,ps2pdf,ps2pdf,false)
AC_CHECK_PROG(PS2PDF,pstopdf,pstopdf,false)

PKG_PROG_PKG_CONFIG

AC_ARG_WITH(tclsh,
  [AS_HELP_STRING([--with-tclsh=PROG],[use a specific tclsh])],
  TCLSH=$withval,)

if test "x$TCLSH" = "x"; then
  AC_PATH_PROGS(TCLSH,[tclsh8.6 tclsh8.5 tclsh8.4 tclsh8.3 tclsh])
#  if test "x$TCLSH" = "x"; then
#    AC_MSG_ERROR([Unable to find a tclsh. Tclsh is a required program for building graphviz, independent of wether tcl-based graphviz products are built])
#    use_tcl="No (tclsh unavailable)"
#  fi
fi

dnl -----------------------------------
dnl checks for compilers

AM_PROG_CC_C_O
#AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJC

AC_C_INLINE

# Workaround for native compilers
#  HP  : http://bugs.gnome.org/db/31/3163.html
#  DEC : Enable NaN/Inf

if test "${GCC}" != "yes" ; then
    AC_CHECK_PROGS(RC,rc,false)
    case "${build}" in
        *-*-hpux* )
	    CFLAGS="${CFLAGS} -Wp,-H30000"
            ;;
        *-dec-osf* )
            CFLAGS="${CFLAGS} -ieee"
            ;;
	*-apple-darwin* )
	    CFLAGS="${CFLAGS} -fno-common"
	    LDFLAGS="${LDFLAGS} -Wl,-headerpad_max_install_names"
	    ;;
	*-sgi-irix* )
	    ;;
    esac
else
    AC_CHECK_PROGS(RC,windres,false)
    case "${CC}" in
	*icc* )
	    # -Wall causes relocation errors with icc
	    CFLAGS="${CFLAGS} -wd269"
	    # icc doesn't like -pipe
	    CFLAGS=`echo ${CFLAGS} | sed 's/ -pipe//'`
	    # icc doesn't like -ffast-math
	    CFLAGS=`echo ${CFLAGS} | sed 's/ -ffast-math//'`
	    # icc doesn't like -O2
	    CFLAGS=`echo ${CFLAGS} | sed 's/ -O2/ -O0/'`
	    ;;
	* )
	    case "${build}" in
	    *-dec-osf* )
			CFLAGS="${CFLAGS} -mieee -Wall"
	        ;;
	    *alpha*-*-linux* )
            CFLAGS="${CFLAGS} -mieee -Wall"
            ;;
	    *-apple-darwin* )
	        CFLAGS="${CFLAGS} -fno-common -Wall"
	        LDFLAGS="${LDFLAGS} -Wl,-headerpad_max_install_names"
	        ;;
	    *-freebsd* )
	        CFLAGS="${CFLAGS} -Wstrict-prototypes -Wpointer-arith -Wall -ffast-math"
		    ;;
	    * )
	    # -Wno-unused-parameter only needed for bug in gcc3
	    # this test from: http://cvs.auriga.wearlab.de/cgi-bin/cvsweb.cgi/dillo/configure.in?rev=1.90;content-type=text%2Fplain;cvsroot=dillo
			if test "`$CC -v 2>&1 | $EGREP 'version 3'`" != ""; then
		        CFLAGS="${CFLAGS} -Wno-unused-parameter"
				# bug 1199
				case "${host_cpu}" in
				sparc* )
		        	CFLAGS="${CFLAGS} -DUSE_MAXFLOAT"
					;;
				esac
		    fi
	        CFLAGS="${CFLAGS} -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wall -ffast-math"
	        ;;
	    esac
	    ;;
    esac
fi
# -ffast-math is incompatible with FP error trapping
#     ref: https://bugzilla.redhat.com/bugzilla/show_bug.cgi?id=175793
if `echo ${CFLAGS} | $EGREP ffast-math >/dev/null`; then
    AC_DEFINE_UNQUOTED(NO_FPERR,1,[Define if no fpu error exception handling is required.])
fi

AM_CONDITIONAL(RC_IS_RC, [test "x$RC" = "xrc"])
AM_CONDITIONAL(RC_IS_WINDRES, [test "x$RC" = "xwindres"])

dnl -----------------------------------
dnl Check for various typedefs and provide substitutes if they do not exist.
                                                                                
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T

dnl -----------------------------------
dnl Checks for header files

# AC_HEADER_STDC
AC_CHECK_HEADERS(stdarg.h stddef.h stddef.h stdlib.h stdint.h malloc.h \
	fcntl.h search.h getopt.h pthread.h values.h float.h limits.h termios.h \
	errno.h time.h unistd.h fenv.h string.h strings.h inttypes.h setjmp.h \
	sys/time.h sys/times.h sys/types.h sys/select.h fpu_control.h \
	sys/fpu.h sys/socket.h sys/stat.h sys/mman.h \
	sys/ioctl.h sys/inotify.h langinfo.h libintl.h crt_externs.h)
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_HEADER_STDBOOL

# Internationalization macros
# AM_GNU_GETTEXT

# Check for socklen_t
AC_MSG_CHECKING([for socklen_t])
AC_CACHE_VAL(ac_cv_type_socklen_t,[AC_EGREP_CPP(changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])socklen_t[^a-zA-Z_0-9]>>dnl
changequote([,]),[
    #include <sys/types.h>
    #include <sys/socket.h>
    #if STDC_HEADERS
    #include <stdlib.h>
    #include <stddef.h>
    #endif
    ], ac_cv_type_socklen_t=yes, ac_cv_type_socklen_t=no)])
AC_MSG_RESULT($ac_cv_type_socklen_t)
if test $ac_cv_type_socklen_t = no; then
    AC_DEFINE(socklen_t, unsigned, [Define to unsigned if socklet_t is missing])
fi

# -----------------------------------
# Test if compiler supports bool
AC_MSG_CHECKING([for bool])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #ifdef HAVE_STDBOOL_H>
    #include <stdbool.h>
    #endif
    ]], [[
    bool foo==true, bar=false;
    ]])],[
AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_BOOL, 1,[Define to 1 if compiler supports bool])
    ],[
AC_MSG_RESULT(no)])

# -----------------------------------
# Test for direct I/O
AC_MSG_CHECKING([for struct dioattr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>]], [[
struct dioattr dio;]])],[
AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_STRUCT_DIOATTR, 1,
[Define to 1 if you have struct dioattr])],[
AC_MSG_RESULT(no)])

dnl -----------------------------------
dnl Checks for -lm library

save_LIBS=$LIBS
AC_CHECK_LIB(m, main, [MATH_LIBS="-lm"])
AC_SUBST([MATH_LIBS])

# Check for functions that might need -lm
LIBS="$LIBS $MATH_LIBS"
AC_CHECK_FUNCS([feenableexcept fesetenv log2 sincos])

LIBS=$save_LIBS

dnl -----------------------------------
dnl INCLUDES and LIBS for libgen (provides basename function)

save_CPPFLAGS=$CPPFLAGS
save_LDFLAGS=$LDFLAGS
AC_CHECK_HEADER(libgen.h,
        AC_CHECK_LIB(gen,basename,
                [LIBGEN_LIBS="-lgen"
                AC_DEFINE_UNQUOTED(HAVE_LIBGEN,1,[Define if the LIBGEN library has the basename feature])]),
        AC_MSG_WARN(libgen.h header not available))
LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([LIBGEN_LIBS])

# -----------------------------------

# Checks for library functions
AC_FUNC_ALLOCA

AC_CHECK_FUNCS([lrand48 drand48 srand48 setmode setenv getenv \
	__freadable _sysconf getrusage strerror cbrt lsqrt vsnprintf \
	strtoul strtoll strtoull uname memset nl_langinfo pow sqrt \
	strchr strdup strerror strstr _NSGetEnviron])

AC_REPLACE_FUNCS([strcasecmp strncasecmp strcasestr])

dnl -----------------------------------
dnl libtool ltdl on-demand plugin loading

save_LIBS="$LIBS"
LT_INIT([dlopen])
LT_CONFIG_LTDL_DIR([libltdl])
LTDL_INIT

AC_ARG_ENABLE(ltdl,
  [AS_HELP_STRING([--enable-ltdl],[support on-demand plugin loading])])
if test "x$enable_ltdl" != "xno"; then
  AC_DEFINE(ENABLE_LTDL,1,[Define if you want on-demand plugin loading])
  use_ltdl="Yes"
  # The lt_dladvise_init symbol was added with libtool-2.2
  if test "x$with_included_ltdl" != "xyes"; then
    save_CFLAGS="$CFLAGS"
    save_LDFLAGS="$LDFLAGS"
    CFLAGS="$CFLAGS $LTDLINCL"
    LDFLAGS="$LDFLAGS $LIBLTDL"
    AC_CHECK_LIB([ltdl], [lt_dladvise_init], [],
	[AC_MSG_ERROR([installed libltdl is too old])])
    LDFLAGS="$save_LDFLAGS"
    CFLAGS="$save_CFLAGS"
  else
    if test "x$DARWIN9" = "xyes"; then
      LIBLTDL_LDFLAGS="-Wl,-unexported_symbol,_lt_*"
    fi
  fi
else
  use_ltdl="No (disabled)"
fi
AM_CONDITIONAL(ENABLE_LTDL, [test "x$enable_ltdl" != "xno"])
AC_SUBST([INCLTDL])
AC_SUBST([LIBLTDL])
AC_SUBST([LIBLTDL_LDFLAGS])
LIBS="$save_LIBS"


# Checks for libraries.
#AC_CHECK_LIB([ICE], [main])
#AC_CHECK_LIB([SM], [main])
#AC_CHECK_LIB([Xaw], [main])
#AC_CHECK_LIB([Xpm], [main])
#AC_CHECK_LIB([art_lgpl_2], [main])
#AC_CHECK_LIB([cairo], [main])
#AC_CHECK_LIB([dl], [main])
#AC_CHECK_LIB([fl], [main])
#AC_CHECK_LIB([intl], [main])
#AC_CHECK_LIB([m], [main])
#AC_CHECK_LIB([perl], [main])
#AC_CHECK_LIB([popt], [main])
#AC_CHECK_LIB([xml2], [main])
#AC_CHECK_LIB([z], [main])

dnl -----------------------------------
dnl ElectricFence support for debugging

AC_ARG_WITH(efence,
   [AS_HELP_STRING([--with-efence=no],[use efence for debugging memory use])],
   [],[with_efence=no])
if test "x$with_efence" = "xyes"; then
	LIBS="$LIBS -lefence"
fi

dnl -----------------------------------
dnl Checks for X header files.

AC_PATH_XTRA
if test "x$no_x" = "xyes"; then
  use_xlib="No (disabled or unavailable)"
  AC_MSG_WARN([X11 not available.])
  X_SUBDIRS=""
else
  use_xlib="Yes"
#no x11.pc on centos4
#  PKG_CHECK_MODULES(X11, [x11])
  PKG_CHECK_MODULES(XRENDER, [xrender],HAVE_XRENDER="yes" ,HAVE_XRENDER="no")
fi
AM_CONDITIONAL(WITH_X, [test "x$use_xlib" = "xYes"])
AM_CONDITIONAL(WITH_XRENDER, [test "x$,HAVE_XRENDER" = "xyes"])

dnl -----------------------------------
dnl check for SWIG - needed for script-language bindings

AC_ARG_ENABLE(swig,
  [AS_HELP_STRING([--enable-swig=yes],[swig-generated language bindings])],
  [],[enable_swig=yes])

if test "x$enable_swig" != "xyes"; then
  use_swig="No (disabled)"
else
  AC_CHECK_PROG(SWIG,swig,swig)
  if test "x$SWIG" = "x"; then
    use_swig="No (swig not available)"
  else
    SWIG_VERSION=`$SWIG -version 2>&1 | $EGREP Version | cut -d ' ' -f 3`
    SWIG_VERSION_MAJOR=`echo $SWIG_VERSION | cut -d '.' -f 1`
    SWIG_VERSION_MINOR=`echo $SWIG_VERSION | cut -d '.' -f 2`
    if test $SWIG_VERSION_MAJOR -lt 2; then
      if test $SWIG_VERSION_MAJOR -lt 1; then
	SWIG=
      else
        if test $SWIG_VERSION_MINOR -lt 3; then
	  SWIG=
        fi
      fi
    fi
    if test "x$SWIG" = "x"; then
      AC_MSG_WARN([The version of swig is too old.])
      use_swig="No (swig too old)"
    else
      use_swig="Yes"
    fi
fi
fi
AM_CONDITIONAL(WITH_SWIG, [test "x$SWIG" != "x"])

dnl -----------------------------------
dnl INCLUDES and LIBS for C#

AC_ARG_ENABLE(sharp,
  [AS_HELP_STRING([--enable-sharp=yes],[C# language bindings])],
  [],[enable_sharp=yes])

if test "x$enable_sharp" != "xyes"; then
  use_sharp="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_sharp="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-csharp *- Generate'` = 0; then
      use_sharp="No (swig does not support -csharp option)"
    else
      AC_CHECK_PROG(MCS,mcs,mcs)
      if test "x$MCS" = "x"; then
        use_sharp="No (mcs not available)"
      else
        use_sharp="Yes"
        SHARP_INCLUDES=
        SHARP_LIBS=
        AC_SUBST([SHARP_INCLUDES])
        AC_SUBST([SHARP_LIBS])
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_SHARP, [test "x$use_sharp" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GO

AC_ARG_ENABLE(go,
  [AS_HELP_STRING([--enable-go=no],[go language bindings])],
  [], [enable_go=no])

if test "x$enable_go" != "xyes"; then
  use_go="No (disabled by default - experimental)"
else
  if test "x$use_swig" != "xYes"; then
    use_go="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-go* *- Generate'` = 0 ; then
      use_go="No (swig does not support -go option)"
    else
      AC_CHECK_PROG(GO,6g,8g)
      if test "x$GO" = "x"; then
        use_go="No (GO compiler not available)"
      else
        GO_INCLUDES=
#        GO_INSTALL_DIR="/usr/lib${LIBPOSTFIX}/go/modules"
#        GO_INSTALL_DATADIR="/usr/share/go"
        GO_LIBS=
#        save_CPPFLAGS=$CPPFLAGS
#        CPPFLAGS="$CPPFLAGS $GO_INCLUDES"
#        AC_CHECK_HEADER(go.h,,[
#          use_go="No (missing header)"
#          GO=
#        ])
#        CPPFLAGS=$save_CPPFLAGS
	if test "x$GO" != "x"; then
	  use_go="Yes"
#          AC_SUBST([GO_INSTALL_DATADIR])
#          AC_SUBST([GO_INSTALL_DIR])
          AC_SUBST([GO_INCLUDES])
          AC_SUBST([GO_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_GO, [test "x$use_go" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GUILE

AC_ARG_ENABLE(guile,
  [AS_HELP_STRING([--enable-guile=yes],[guile language bindings])],
  [],[enable_guile=yes])

if test "x$enable_guile" != "xyes"; then
  use_guile="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_guile="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-guile *- Generate'` = 0; then
      use_guile="No (swig does not support -guile option)"
    else
      AC_CHECK_PROGS(GUILE,guile2 guile1.8 guile)
      if test "x$GUILE" = "x"; then
        use_guile="No (guile not available)"
      else
        [GUILE_VERSION=`$GUILE --version | sed -n '1 s/^.* \+\([0-9\.]\+\)$/\1/ p'`]
        GUILE_VERSION_MAJOR=`echo $GUILE_VERSION | cut -d '.' -f 1`
        GUILE_VERSION_MINOR=`echo $GUILE_VERSION | cut -d '.' -f 2`
        if test 0$GUILE_VERSION_MAJOR -lt 2; then
          if test 0$GUILE_VERSION_MAJOR -lt 1; then
            GUILE=
          else
            if test 0$GUILE_VERSION_MINOR -lt 4; then
	      GUILE=
            fi
          fi
        fi
        if test "x$GUILE" = "x"; then
	  use_guile="No (guile is too old)"
        else
	  GUILE_INCLUDES=$(guile-config compile)
	  GUILE_LIBS=$(guile-config link)
	  # don't die if PKG_CHECK_MODULES not available - el4, el5
          PKG_CHECK_MODULES([GUILE],
		[guile-2.0 >= "$GUILE_VERSION_MAJOR.$GUILE_VERSION_MINOR"],
		GUILE_INCLUDES="$GUILE_CFLAGS",
          	[
			PKG_CHECK_MODULES([GUILE],
			[guile-1.8 >= "$GUILE_VERSION_MAJOR.$GUILE_VERSION_MINOR"],
			GUILE_INCLUDES="$GUILE_CFLAGS",
			false)
		])
          ac_save_CFLAGS="$CFLAGS"
          ac_save_CPPFLAGS="$save_CPPFLAGS"
          CFLAGS="$CFLAGS $GUILE_INCLUDES"
          CPPFLAGS="$CPPFLAGS $GUILE_INCLUDES"
          AC_CHECK_HEADER(libguile.h,,[
            use_guile="No (guile header missing)"
            GUILE=
          ])
          CFLAGS="$ac_save_CFLAGS"
          CPPFLAGS="$ac_save_CPPFLAGS"
          if test "x$GUILE" != "x"; then
	    use_guile="Yes"
            AC_SUBST([GUILE_INCLUDES])
            AC_SUBST([GUILE_LIBS])
          fi
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_GUILE, [test "x$use_guile" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for IO   (disbled until supported by swig)

AC_ARG_ENABLE(io,
  [AS_HELP_STRING([--enable-io=no],[io language bindings])],
  [], [enable_io=no])

if test "x$enable_io" != "xyes"; then
  use_io="No (disabled by default - no swig support yet)"
else
  if test "x$use_swig" != "xYes"; then
    use_io="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-io *- Generate'` = 0; then
      use_io="No (swig does not support -io option)"
    else
      AC_CHECK_PROG(IO,io,io)
      if test "x$IO" = "x"; then
        use_io="No (io not available)"
      else
        use_io="Yes"
        IO_INCLUDES=
        IO_LIBS=
        AC_SUBST([IO_INCLUDES])
        AC_SUBST([IO_LIBS])
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_IO, [test "x$use_io" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for JAVA

AC_ARG_ENABLE(java,
  [AS_HELP_STRING([--enable-java=yes],[java language bindings])],
  [], [enable_java=yes])

if test "x$enable_java" != "xyes"; then
  use_java="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_java="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-java *- Generate'` = 0; then
      use_java="No (swig does not support -java option)"
    else
      AC_CHECK_PROG(JAVA,java,java)
      if test "x$JAVA" = "x"; then
        use_java="No (java not available)"
      else
        for try_java_include in \
          /System/Library/Frameworks/JavaVM.framework/Headers \
          /usr/lib${LIBPOSTFIX}/jvm/java/include \
          /usr/lib${LIBPOSTFIX}/jvm/java/include/linux \
          /usr/lib${LIBPOSTFIX}/jvm/default-java/include \
          /usr/lib${LIBPOSTFIX}/jvm/default-java/include/linux \
          /usr/lib/jvm/java/include \
          /usr/lib/jvm/java/include/linux \
          /usr/lib/jvm/default-java/include \
          /usr/lib/jvm/default-java/include/linux \
          /etc/java-config-2/current-system-vm/include \
          /etc/java-config-2/current-system-vm/include/linux \
          /etc/java-config-2/current-system-vm/include/freebsd
        do
          if test -d "$try_java_include" ; then
            JAVA_INCLUDES="$JAVA_INCLUDES -I${try_java_include}"
          fi
        done
        JAVA_LIBS=
        AC_ARG_WITH(javaincludedir,
           [AS_HELP_STRING([--with-javaincludedir=DIR],[use JAVA includes from DIR])],
          [JAVA_INCLUDES="-I$withval"])
        AC_ARG_WITH(javalibdir,
          [AS_HELP_STRING([--with-javalibdir=DIR],[use JAVA libraries from DIR])],
          [JAVA_LIBS="$withval"])
	save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $JAVA_INCLUDES"
        AC_CHECK_HEADER(jni.h,,[
            use_java="No (missing header)"
  	    JAVA=
        ])
        CPPFLAGS="$save_CPPFLAGS"
        if test "x$JAVA" != "x"; then
          use_java="Yes"
	  AC_SUBST([JAVA_INCLUDES])
          AC_SUBST([JAVA_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_JAVA, [test "x$use_java" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for LUA

AC_ARG_ENABLE(lua,
  [AS_HELP_STRING([--enable-lua=yes],[lua language bindings])],
  [], [enable_lua=yes])

if test "x$enable_lua" != "xyes"; then
  use_lua="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_lua="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-lua *- Generate'` = 0; then
      use_lua="No (swig does not support -lua option)"
    else
      AC_CHECK_PROG(LUA,lua,lua)
      if test "x$LUA" = "x"; then
        use_lua="No (lua not available)"
      else
        ac_save_CPPFLAGS="$CPPFLAGS"
        ac_save_CFLAGS="$CFLAGS"
        ac_save_LDFLAGS="$LDFLAGS"
    
        AC_CHECK_PROG(LUA,lua,lua)
    
        LUA_INCLUDES=""
        LUA_LIBS=""
        LUA_INSTALL_DIR=""
    
        if test -n "$lua_inc"; then
           CFLAGS="$CFLAGS -I$lua_inc"
           CPPFLAGS="$CPPFLAGS -I$lua_inc"
        fi
        if test -n "$lua_lib"; then
           LDFLAGS="$LDFLAGS -L$lua_lib"
        fi
    
        if test "x$LUA_INCLUDES" = "x" -a "x$LUA_LIBS" = "x" ; then
            PKGCONFIG=""
            AC_CHECK_PROGS(PKGCONFIG, pkg-config)
            if test "x$PKGCONFIG" != "x"; then
	        AC_MSG_CHECKING(for Lua headers and libraries with pkg-config)
	        echo
	        for l in "$lua_suffix" "" "52" "5.2" "51" "5.1" "50" "5.0" ; do
	            pkgconfig_lua_found=`$PKGCONFIG --exists lua$l 2>/dev/null`
	            if test "x$?" = "x0" ; then
		        LUA_VERSION=`$PKGCONFIG --modversion lua$l`
                        LUA_INCLUDES="$LUA_CFLAGS "`$PKGCONFIG --cflags lua$l`
                        LUA_LIBS="$LUA_LFLAGS "`$PKGCONFIG --libs lua$l`
		        LUA_INSTALL_DIR="/usr/lib$LIBPOSTFIX/lua/`$PKGCONFIG --variable=V lua`"
    
		        pkgconfig_lualib_found=`$PKGCONFIG --exists lualib$l 2>/dev/null`
		        if test  "x$?" = "x0"; then
		           LUA_INCLUDES="$LUA_CFLAGS "`$PKGCONFIG --cflags lualib$l`
		           LUA_LIBS="$LUA_LFLAGS "`$PKGCONFIG --libs lualib$l`
		        fi
                        break
                    fi
                done
            fi
        fi
    
        if test "x$LUA_INCLUDES" = "x" -o "x$LUA_LIBS" = "x" ; then
            LUACONFIG=""
            AC_CHECK_PROGS(LUACONFIG, lua-config lua-config52 lua-config5.2 lua-config51 lua-config5.1 lua-config50 lua-config5.0)
            if test "x$LUACONFIG" != "x" ; then
                AC_MSG_CHECKING(for Lua headers and libraries with lua-config)
	        echo
                LUA_INCLUDES=`$LUACONFIG --include`
                LUA_LIBS=`$LUACONFIG --libs`
	        LUA_VERSION="5.0.x"
	        LUA_INSTALL_DIR="/usr/lib$LIBPOSTFIX/lua/5.0"
            fi
        fi
    
        if test "x$LUA_INCLUDES" = "x" -o "x$LUA_LIBS" = "x" ; then
            AC_MSG_CHECKING(for Lua headers and libraries)
	    echo
            AC_CHECK_HEADER(lua.h,ac_found_lua_header="yes",ac_found_lua_header="no")
            AC_CHECK_HEADER(lualib.h,ac_found_liblua_header="yes",ac_found_liblua_header="no")
            if test "x$ac_found_lua_header" = "xyes" -a "x$ac_found_liblua_header" = "xyes"; then
                LUA_INCLUDES="$CFLAGS"
            fi
            for l in "$lua_suffix" "" "52" "5.2" "51" "5.1" "50" "5.0" ; do
                AC_CHECK_LIB(lua$l,lua_call,ac_found_lua_lib="yes",ac_found_lua_lib="no")
                if test "x$ac_found_lua_lib" = "xyes" ; then
	            LUA_VERSION=">=5.1.0"
    
	            LUA_LIBS="$LDFLAGS -llua$l -lm"
    
		    ac2_save_LDFLAGS="$LDFLAGS"
		    LDFLAGS="$LDFLAGS -llua$l -lm"
    
		    if echo "$host_os" | grep "^linux" 1>/dev/null 2>&1 ; then
		       LDFLAGS="$LDFLAGS -ldl"
		    fi
    
		    AC_CHECK_LIB(lualib$l,luaL_openlib,ac_found_liblua_lib="yes",ac_found_liblua_lib="no")
		    if test "x$ac_found_liblua_lib" = "xyes" ; then
		       LUA_VERSION="5.0.x"
    
		       LUA_LIBS="$LUA_LFLAGS -llualib$l"
    
		       if echo "$host_os" | grep "^linux" 1>/dev/null 2>&1 ; then
		          LUA_LIBS="$LUA_LFLAGS -ldl"
		       fi
		    fi
    
		    LDFLAGS="$ac2_save_LDFLAGS"
		    break
	        fi
            done
        fi
    
        AC_MSG_CHECKING(for Lua compiling and linking)
        LUA_TEST=`LT=luatest.c ; echo "#include <lua.h>" > $LT;  echo "#include <lualib.h>" >> $LT; echo "int main() { return 0; }" >> $LT ; $CC -Wall $LT -o $LT.out $LUA_INCLUDES $LUA_LIBS $CFLAGS $LDFLAGS 1>/dev/null 2>&1 ; echo $?; rm -f $LT $LT.out 1>/dev/null 2>&1`
        if test "x$LUA_TEST" != "x0" ; then
          AC_MSG_RESULT(no)
          AC_MSG_WARN([
*** Lua (>=5.0) headers and/or libraries could not be found in your system.
*** Try to install liblua, liblualib and liblua-dev with your software package manager.
*** Graphviz will be built without Lua support.])
          use_lua="No (lua compiling/linking failed)"
        else
	  use_lua="Yes"
          AC_SUBST([LUA_INCLUDES])
          AC_SUBST([LUA_LIBS])
          AC_SUBST([LUA_INSTALL_DIR])
          # Not needed for Graphviz: AC_DEFINE(PLUGIN_LUA)
        fi
        CFLAGS="$ac_save_CFLAGS"
        CPPFLAGS="$ac_save_CPPFLAGS"
        LDFLAGS="$ac_save_LDFLAGS"
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_LUA, [test "x$use_lua" = "xYes"])


dnl -----------------------------------
dnl INCLUDES and LIBS for OCAML

AC_ARG_ENABLE(ocaml,
  [AS_HELP_STRING([--enable-ocaml=yes],[ocaml language bindings])],
  [], [enable_ocaml=yes])

if test "x$enable_ocaml" != "xyes"; then
  use_ocaml="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_ocaml="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-ocaml *- Generate'` = 0; then
      use_ocaml="No (swig does not support -ocaml option)"
    else
      AC_CHECK_PROG(OCAML,ocaml,ocaml)
      if test "x$OCAML" = "x"; then
        use_ocaml="No (ocaml not available)"
      else
        AC_CHECK_PROG(OCAML_OPT,ocamlopt,ocamlopt)
        OCAML_INCLUDES=-I/usr/lib$LIBPOSTFIX/ocaml
        OCAML_LIBS=-L/usr/lib$LIBPOSTFIX/ocaml
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $OCAML_INCLUDES"
        AC_CHECK_HEADER(caml/mlvalues.h,[
	  use_ocaml="Yes"
          AC_SUBST([OCAML_INCLUDES])
          AC_SUBST([OCAML_LIBS])
	],[
	  use_ocaml="No (missing header)"
          AC_MSG_WARN([Unable to find header caml/mlvalues.h. The OCAML packages will not be built])
        ])
        CPPFLAGS=$save_CPPFLAGS
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_OCAML, [test "x$use_ocaml" = "xYes"])
AM_CONDITIONAL(WITH_OCAMLOPT, [test "x$OCAML_OPT" != "x"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PERL

AC_ARG_ENABLE(perl,
  [AS_HELP_STRING([--enable-perl=yes],[perl language bindings])],
  [], [enable_perl=yes])

if test "x$enable_perl" != "xyes"; then
  use_perl="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_perl="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-perl *- Generate'` = 0; then
      use_perl="No (swig does not support -perl option)"
    else
      AC_CHECK_PROG(PERL,perl,perl)
      if test "x$PERL" = "x"; then
        use_perl="No (perl not available)"
      else
        if test $PERL -e 'exit !($] <= 5.004)' > /dev/null 2>&1 ; then
	  use_perl="No (perl is too old)"
	  PERL=
	else
          PERL_INCLUDES="`$PERL $srcdir/config/config_perl.pl PERL_INCLUDES`"
          PERL_LIBS="`$PERL $srcdir/config/config_perl.pl PERL_LIBS`"
          PERL_INSTALL_DIR="`$PERL $srcdir/config/config_perl.pl PERL_INSTALL_DIR`"
	  save_CPPFLAGS=$CPPFLAGS
	  CPPFLAGS="$CPPFLAGS $PERL_INCLUDES"
          AC_CHECK_HEADER(EXTERN.h,,[
            use_perl="No (missing header)"
  	    PERL=
          ])
#	  save_LDFLAGS=$LDFLAGS
#	  LDFLAGS="$LDFLAGS $PERL_LIBS"
#          AC_CHECK_LIB(perl,main,,[
#            use_perl="No (missing library)"
#            PERL=
#          ])
#	  LDFLAGS=$save_LDFLAGS
	  CPPFLAGS=$save_CPPFLAGS
	fi
        if test "x$PERL" != "x"; then
          use_perl="Yes"
	  AC_SUBST([PERL_INCLUDES])
          AC_SUBST([PERL_LIBS])
	  AC_SUBST([PERL_INSTALL_DIR])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PERL, [test "x$use_perl" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PHP

AC_ARG_ENABLE(php,
  [AS_HELP_STRING([--enable-php=yes],[php language bindings])],
  [], [enable_php=yes])

if test "x$enable_php" != "xyes"; then
  use_php="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_php="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-php5* *- Generate'` = 0 ; then
      use_php="No (swig does not support -php or -php5 option)"
    else
      AC_CHECK_PROGS(PHP,php5 php)
      if test "x$PHP" = "x"; then
        use_php="No (php not available)"
      else
        if test -d /usr/include/php5; then
          PHP_INCLUDES="-I/usr/include/php5 -I/usr/include/php5/main -I/usr/include/php5/Zend -I/usr/include/php5/TSRM"
        else
          PHP_INCLUDES="-I/usr/include/php -I/usr/include/php/main -I/usr/include/php/Zend -I/usr/include/php/TSRM"
        fi
        PHP_INSTALL_DIR="/usr/lib${LIBPOSTFIX}/php/modules"
        PHP_INSTALL_DATADIR="/usr/share/php"
        PHP_LIBS=
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PHP_INCLUDES"
        AC_CHECK_HEADER(php.h,,[
          use_php="No (missing header)"
          PHP=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PHP" != "x"; then
	  use_php="Yes"
          AC_SUBST([PHP_INSTALL_DATADIR])
          AC_SUBST([PHP_INSTALL_DIR])
          AC_SUBST([PHP_INCLUDES])
          AC_SUBST([PHP_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PHP, [test "x$use_php" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON

AC_ARG_ENABLE(python,
  [AS_HELP_STRING([--enable-python=yes],[python language bindings])],
  [], [enable_python=yes])

if test "x$enable_python" != "xyes"; then
  use_python="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_python="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON,python,python)
      if test "x$PYTHON" = "x"; then
        use_python="No (python not available)"
      else
        PYTHON_VERSION=`$PYTHON -c "import sys; print '%d.%d' % (sys.version_info[[0:2]])"`
        if test "x$PYTHON_VERSION" = "x"; then
          PYTHON=
        else
          PYTHON_VERSION_MAJOR=`echo $PYTHON_VERSION | cut -d '.' -f 1`
          PYTHON_VERSION_MINOR=`echo $PYTHON_VERSION | cut -d '.' -f 2`
          PYTHON_VERSION_SHORT=$PYTHON_VERSION_MAJOR.$PYTHON_VERSION_MINOR
          if test $PYTHON_VERSION_MAJOR -lt 3; then
            if test $PYTHON_VERSION_MAJOR -lt 2; then
	      PYTHON=
            else
              if test $PYTHON_VERSION_MINOR -lt 3; then
	        PYTHON=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON" = "x"; then
	use_python="No (python is too old)"
      else
        PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`
        PYTHON_INCLUDES=-I$PYTHON_PREFIX/include/python$PYTHON_VERSION_SHORT
        PYTHON_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON_INSTALL_DIR="`$PYTHON $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python="No (missing header)"
          PYTHON=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON" != "x"; then
	  use_python="Yes"
          AC_SUBST([PYTHON_INSTALL_DIR])
          AC_SUBST([PYTHON_INCLUDES])
          AC_SUBST([PYTHON_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON, [test "x$use_python" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON23

AC_ARG_ENABLE(python23,
  [AS_HELP_STRING([--enable-python23=no],[python23 language bindings])],
  [], [enable_python23=no])

if test "x$enable_python23" != "xyes"; then
  use_python23="No (disabled by default - for multiversion installs)"
else
  if test "x$use_swig" != "xYes"; then
    use_python23="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python23="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON23,python2.3,python2.3)
      if test "x$PYTHON23" = "x"; then
        use_python23="No (python23 not available)"
      else
        PYTHON23_VERSION=`$PYTHON23 -V 2>&1 | $EGREP Python | cut -d ' ' -f 2`
        if test "x$PYTHON23_VERSION" = "x"; then
          PYTHON23=
        else
          PYTHON23_VERSION_MAJOR=`echo $PYTHON23_VERSION | cut -d '.' -f 1`
          PYTHON23_VERSION_MINOR=`echo $PYTHON23_VERSION | cut -d '.' -f 2`
          PYTHON23_VERSION_SHORT=$PYTHON23_VERSION_MAJOR.$PYTHON23_VERSION_MINOR
          if test $PYTHON23_VERSION_MAJOR -lt 3; then
            if test $PYTHON23_VERSION_MAJOR -lt 2; then
	      PYTHON23=
            else
              if test $PYTHON23_VERSION_MINOR -lt 3; then
	        PYTHON23=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON23" = "x"; then
	use_python23="No (python2.3 is too old)"
      else
        PYTHON23_PREFIX=`$PYTHON23 -c "import sys; print sys.prefix"`
        PYTHON23_INCLUDES=-I$PYTHON23_PREFIX/include/python$PYTHON23_VERSION_SHORT
        PYTHON23_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON23_INSTALL_DIR="`$PYTHON23 $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON23_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python23="No (missing header)"
          PYTHON23=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON23" != "x"; then
	  use_python23="Yes"
          AC_SUBST([PYTHON23_INSTALL_DIR])
          AC_SUBST([PYTHON23_INCLUDES])
          AC_SUBST([PYTHON23_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON23, [test "x$use_python23" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON24

AC_ARG_ENABLE(python24,
  [AS_HELP_STRING([--enable-python24=no],[python24 language bindings])],
  [], [enable_python24=no])

if test "x$enable_python24" != "xyes"; then
  use_python24="No (disabled by default - for multiversion installs)"
else
  if test "x$use_swig" != "xYes"; then
    use_python24="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python24="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON24,python2.4,python2.4)
      if test "x$PYTHON24" = "x"; then
        use_python24="No (python24 not available)"
      else
        PYTHON24_VERSION=`$PYTHON24 -V 2>&1 | $EGREP Python | cut -d ' ' -f 2`
        if test "x$PYTHON24_VERSION" = "x"; then
          PYTHON24=
        else
          PYTHON24_VERSION_MAJOR=`echo $PYTHON24_VERSION | cut -d '.' -f 1`
          PYTHON24_VERSION_MINOR=`echo $PYTHON24_VERSION | cut -d '.' -f 2`
          PYTHON24_VERSION_SHORT=$PYTHON24_VERSION_MAJOR.$PYTHON24_VERSION_MINOR
          if test $PYTHON24_VERSION_MAJOR -lt 3; then
            if test $PYTHON24_VERSION_MAJOR -lt 2; then
	      PYTHON24=
            else
              if test $PYTHON24_VERSION_MINOR -lt 4; then
	        PYTHON24=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON24" = "x"; then
	use_python24="No (python2.4 is too old)"
      else
        PYTHON24_PREFIX=`$PYTHON24 -c "import sys; print sys.prefix"`
        PYTHON24_INCLUDES=-I$PYTHON24_PREFIX/include/python$PYTHON24_VERSION_SHORT
        PYTHON24_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON24_INSTALL_DIR="`$PYTHON24 $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON24_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python24="No (missing header)"
          PYTHON24=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON24" != "x"; then
	  use_python24="Yes"
          AC_SUBST([PYTHON24_INSTALL_DIR])
          AC_SUBST([PYTHON24_INCLUDES])
          AC_SUBST([PYTHON24_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON24, [test "x$use_python24" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON25

AC_ARG_ENABLE(python25,
  [AS_HELP_STRING([--enable-python25=no],[python25 language bindings])],
  [], [enable_python25=no])

if test "x$enable_python25" != "xyes"; then
  use_python25="No (disabled by default - for multiversion installs)"
else
  if test "x$use_swig" != "xYes"; then
    use_python25="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python25="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON25,python2.5,python2.5)
      if test "x$PYTHON25" = "x"; then
        use_python25="No (python25 not available)"
      else
        PYTHON25_VERSION=`$PYTHON25 -V 2>&1 | $EGREP Python | cut -d ' ' -f 2`
        if test "x$PYTHON25_VERSION" = "x"; then
          PYTHON25=
        else
          PYTHON25_VERSION_MAJOR=`echo $PYTHON25_VERSION | cut -d '.' -f 1`
          PYTHON25_VERSION_MINOR=`echo $PYTHON25_VERSION | cut -d '.' -f 2`
          PYTHON25_VERSION_SHORT=$PYTHON25_VERSION_MAJOR.$PYTHON25_VERSION_MINOR
          if test $PYTHON25_VERSION_MAJOR -lt 3; then
            if test $PYTHON25_VERSION_MAJOR -lt 2; then
	      PYTHON25=
            else
              if test $PYTHON25_VERSION_MINOR -lt 5; then
	        PYTHON25=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON25" = "x"; then
	use_python25="No (python2.5 is too old)"
      else
        PYTHON25_PREFIX=`$PYTHON25 -c "import sys; print sys.prefix"`
        PYTHON25_INCLUDES=-I$PYTHON25_PREFIX/include/python$PYTHON25_VERSION_SHORT
        PYTHON24_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON25_INSTALL_DIR="`$PYTHON25 $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON25_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python25="No (missing header)"
          PYTHON25=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON25" != "x"; then
	  use_python25="Yes"
          AC_SUBST([PYTHON25_INSTALL_DIR])
          AC_SUBST([PYTHON25_INCLUDES])
          AC_SUBST([PYTHON25_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON25, [test "x$use_python25" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON26

AC_ARG_ENABLE(python26,
  [AS_HELP_STRING([--enable-python26=no],[python26 language bindings])],
  [], [enable_python26=no])

if test "x$enable_python26" != "xyes"; then
  use_python26="No (disabled by default - for multiversion installs)"
else
  if test "x$use_swig" != "xYes"; then
    use_python26="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python26="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON26,python2.6,python2.6)
      if test "x$PYTHON26" = "x"; then
        use_python26="No (python26 not available)"
      else
        PYTHON26_VERSION=`$PYTHON26 -V 2>&1 | $EGREP Python | cut -d ' ' -f 2`
        if test "x$PYTHON26_VERSION" = "x"; then
          PYTHON26=
        else
          PYTHON26_VERSION_MAJOR=`echo $PYTHON26_VERSION | cut -d '.' -f 1`
          PYTHON26_VERSION_MINOR=`echo $PYTHON26_VERSION | cut -d '.' -f 2`
          PYTHON26_VERSION_SHORT=$PYTHON26_VERSION_MAJOR.$PYTHON26_VERSION_MINOR
          if test $PYTHON26_VERSION_MAJOR -lt 3; then
            if test $PYTHON26_VERSION_MAJOR -lt 2; then
	      PYTHON26=
            else
              if test $PYTHON26_VERSION_MINOR -lt 5; then
	        PYTHON26=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON26" = "x"; then
	use_python26="No (python2.6 is too old)"
      else
        PYTHON26_PREFIX=`$PYTHON26 -c "import sys; print sys.prefix"`
        PYTHON26_INCLUDES=-I$PYTHON26_PREFIX/include/python$PYTHON26_VERSION_SHORT
        PYTHON24_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON26_INSTALL_DIR="`$PYTHON26 $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON26_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python26="No (missing header)"
          PYTHON26=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON26" != "x"; then
	  use_python26="Yes"
          AC_SUBST([PYTHON26_INSTALL_DIR])
          AC_SUBST([PYTHON26_INCLUDES])
          AC_SUBST([PYTHON26_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON26, [test "x$use_python26" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PYTHON27

AC_ARG_ENABLE(python27,
  [AS_HELP_STRING([--enable-python27=no],[python27 language bindings])],
  [], [enable_python27=no])

if test "x$enable_python27" != "xyes"; then
  use_python27="No (disabled by default - for multiversion installs)"
else
  if test "x$use_swig" != "xYes"; then
    use_python27="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-python *- Generate'` = 0; then
      use_python27="No (swig does not support -python option)"
    else
      AC_CHECK_PROG(PYTHON27,python2.7,python2.7)
      if test "x$PYTHON27" = "x"; then
        use_python27="No (python27 not available)"
      else
        PYTHON27_VERSION=`$PYTHON27 -V 2>&1 | $EGREP Python | cut -d ' ' -f 2`
        if test "x$PYTHON27_VERSION" = "x"; then
          PYTHON27=
        else
          PYTHON27_VERSION_MAJOR=`echo $PYTHON27_VERSION | cut -d '.' -f 1`
          PYTHON27_VERSION_MINOR=`echo $PYTHON27_VERSION | cut -d '.' -f 2`
          PYTHON27_VERSION_SHORT=$PYTHON27_VERSION_MAJOR.$PYTHON27_VERSION_MINOR
          if test $PYTHON27_VERSION_MAJOR -lt 3; then
            if test $PYTHON27_VERSION_MAJOR -lt 2; then
	      PYTHON27=
            else
              if test $PYTHON27_VERSION_MINOR -lt 5; then
	        PYTHON27=
              fi
            fi
          fi
        fi
      fi
      if test "x$PYTHON27" = "x"; then
	use_python27="No (python2.7 is too old)"
      else
        PYTHON27_PREFIX=`$PYTHON27 -c "import sys; print sys.prefix"`
        PYTHON27_INCLUDES=-I$PYTHON27_PREFIX/include/python$PYTHON27_VERSION_SHORT
        PYTHON24_LIBS="-lpython$PYTHON_VERSION_SHORT"
        PYTHON27_INSTALL_DIR="`$PYTHON27 $srcdir/config/config_python.py archsitelib`"
        save_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS $PYTHON27_INCLUDES"
        AC_CHECK_HEADER(Python.h,,[
          use_python27="No (missing header)"
          PYTHON27=
        ])
        CPPFLAGS=$save_CPPFLAGS
	if test "x$PYTHON27" != "x"; then
	  use_python27="Yes"
          AC_SUBST([PYTHON27_INSTALL_DIR])
          AC_SUBST([PYTHON27_INCLUDES])
          AC_SUBST([PYTHON27_LIBS])
        fi
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_PYTHON27, [test "x$use_python27" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for R

AC_ARG_ENABLE(r,
  [AS_HELP_STRING([--enable-r=yes],[R language bindings])],
  [], [enable_r=yes])

if test "x$enable_r" != "xyes"; then
  use_r="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_r="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-r *- Generate'` = 0; then
      use_r="No (swig does not support -r option)"
    else
      PKG_CHECK_MODULES(R, [libR],[
	use_r="Yes"
	AC_SUBST([R_CFLAGS])
	AC_SUBST([R_LIBS])
      ],[
	use_r="No (R not available)"
      ])
    fi
  fi
fi
AM_CONDITIONAL(WITH_R, [test "x$use_r" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for RUBY

AC_ARG_ENABLE(ruby,
  [AS_HELP_STRING([--enable-ruby=yes],[ruby language bindings])],
  [], [enable_ruby=yes])

if test "x$enable_ruby" != "xyes"; then
  use_ruby="No (disabled)"
else
  if test "x$use_swig" != "xYes"; then
    use_ruby="No (swig not available)"
  else
    if test `$SWIG -help 2>&1 | $EGREP -c '\-ruby *- Generate'` = 0; then
      use_ruby="No (swig does not support -ruby option)"
    else
      PKG_CHECK_MODULES([RUBY], [ruby],[
	AC_CHECK_PROG(RUBY,ruby,ruby)
        if test "x$RUBY" = "x"; then
          use_ruby="No (ruby not available)"
        else
          RUBY_INSTALL_DIR="`$RUBY $srcdir/config/config_ruby.rb vendorarchdir`"
	fi
      ],[
        PKG_CHECK_MODULES([RUBY], [ruby-1.9],[
	  AC_CHECK_PROG(RUBY,ruby,ruby)
          if test "x$RUBY" = "x"; then
            use_ruby="No (ruby not available)"
          else
            RUBY_INSTALL_DIR="`$RUBY $srcdir/config/config_ruby.rb vendorarchdir`"
	  fi
        ],[
          AC_CHECK_PROG(RUBY,ruby,ruby)
          if test "x$RUBY" = "x"; then
	    use_ruby="No (ruby not available)"
          else
            RUBY_CFLAGS="-I`$RUBY $srcdir/config/config_ruby.rb archdir`"
	    # hack for powerpc-darwin8 (10.4)
	    if test "x$DARWIN" = "xyes"; then
	      RUBY_CFLAGS=`echo $RUBY_CFLAGS | sed 's/powerpc/universal/'`
	    fi
            RUBY_LIBS="-L`$RUBY $srcdir/config/config_ruby.rb lib` `$RUBY -rrbconfig -e \"puts Config::CONFIG[['LIBRUBYARG_SHARED']]\"`"
            RUBY_INSTALL_DIR="`$RUBY $srcdir/config/config_ruby.rb vendorarchdir`"
	    if test "x$RUBY_INSTALL_DIR" = "x"; then
	  	  RUBY_INSTALL_DIR="`$RUBY $srcdir/config/config_ruby.rb sitearchdir`"
	    fi
            save_CPPFLAGS=$CPPFLAGS
            CPPFLAGS="$CPPFLAGS $RUBY_CFLAGS"
            AC_CHECK_HEADER(ruby.h,,[
              use_ruby="No (missing header)"
              RUBY=
            ])
            CPPFLAGS=$save_CPPFLAGS
          fi
        ])
      ])
      if test "x$RUBY" != "x"; then
	use_ruby="Yes"
        AC_SUBST([RUBY_CFLAGS])
        AC_SUBST([RUBY_LIBS])
        AC_SUBST([RUBY_INSTALL_DIR])
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_RUBY, [test "x$use_ruby" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for TCL

AC_ARG_ENABLE(tcl,
  [AS_HELP_STRING([--enable-tcl=yes],[tcl language bindings])],
  [], [enable_tcl=yes])

if test "x$enable_tcl" != "xyes"; then
  use_tcl="No (disabled)"
fi

if test "x$use_tcl" = "x"; then
  TCL_VERSION_FOUND=`echo 'puts [[info tclversion]]' | $TCLSH`

  # can't assume ksh on all architectures
  # TCLSH_EXEC_PREFIX=${TCLSH%%/bin/tclsh.*}
  # so use sed instead
  TCLSH_EXEC_PREFIX=`echo $TCLSH|sed -e 's%/bin/.*tclsh.*$%%'`

  if test -f ${TCLSH_EXEC_PREFIX}/include/tcl${TCL_VERSION_FOUND}/tcl.h; then
    TCL_INCLUDES="-I${TCLSH_EXEC_PREFIX}/include/tcl${TCL_VERSION_FOUND} -I${TCLSH_EXEC_PREFIX}/include/tcl${TCL_VERSION_FOUND}/generic"
  else
    if test "x${TCLSH_EXEC_PREFIX}" != "x/usr" -a -f ${TCLSH_EXEC_PREFIX}/include/tcl.h; then
      TCL_INCLUDES=-I${TCLSH_EXEC_PREFIX}/include
    fi
  fi
  if test "x$TCL_INCLUDES" != "x" ; then
    AC_MSG_RESULT([using tcl headers from $TCL_INCLUDES])
  fi
  AC_SUBST([TCL_INCLUDES])
  save_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $TCL_INCLUDES"
  AC_CHECK_HEADER(tcl.h,,[
    AC_MSG_WARN([Unable to find header tcl.h. The Tcl packages will not be built])
    use_tcl="No (missing header)"
  ])
  CPPFLAGS=$save_CPPFLAGS
fi

if test "x$use_tcl" = "x"; then
  # Some systems don't have Tcl.  Don't build
  # the Tcl products if we don't have the library.
  if test -f ${TCLSH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tcl${TCL_VERSION_FOUND}/tclConfig.sh; then
    TCLCONFIG=${TCLSH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tcl${TCL_VERSION_FOUND}/tclConfig.sh
  else
    if test -f ${TCLSH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tclConfig.sh; then
      TCLCONFIG=${TCLSH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tclConfig.sh
    else
      if test -f ${TCLSH_EXEC_PREFIX}/lib/tclConfig.sh; then
        TCLCONFIG=${TCLSH_EXEC_PREFIX}/lib/tclConfig.sh
      fi
    fi
  fi
  if test "x$TCLCONFIG" != "x"; then
    AC_MSG_RESULT([using $TCLCONFIG])
    file=${TCLCONFIG}
      . $file
  else
    AC_MSG_WARN([Unable to find tclConfig.sh. The Tcl packages will not be built])
    use_tcl="No (missing tclConfig.sh)"
  fi
  TCL_INSTALL_DIR="${TCLSH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tcl${TCL_VERSION_FOUND}"
fi

if test "x$use_tcl" = "x"; then
  # TCL STUBS support is required
  if test "x${TCL_SUPPORTS_STUBS}" = "x1"; then
    use_tcl="Yes"
    TCL_CFLAGS="${TCL_CFLAGS} -DUSE_TCL_STUBS"
    AC_DEFINE_UNQUOTED(HAVE_TCL,1,
	[Define if you have the tcl library])
  else
    AC_MSG_WARN([Tcl does not have STUBs support, perhaps it is too old?  The Tcl packages will not be built])
    use_tcl="No (no STUBs support)"
  fi
fi

AC_SUBST([TCL_INSTALL_DIR])

AM_CONDITIONAL(WITH_TCL, [test "x$use_tcl" = "xYes"])

#AC_SUBST([TCLSH_EXEC_PREFIX])
AC_SUBST([TCL_VERSION])
AC_SUBST([TCL_MAJOR_VERSION])
AC_SUBST([TCL_MINOR_VERSION])
AC_SUBST([TCL_PATCH_LEVEL])
#AC_SUBST([TCL_CC])
#AC_SUBST([TCL_CXX])
AC_SUBST([TCL_CFLAGS])
#AC_SUBST([TCL_CXXFLAGS])
AC_SUBST([TCL_DEFS])
#AC_SUBST([TCL_DBGX])
#AC_SUBST([TCL_CFLAGS_DEBUG])
#AC_SUBST([TCL_CFLAGS_OPTIMIZE])
#AC_SUBST([TCL_SHARED_BUILD])
#AC_SUBST([TCL_LIB_FILE])
#AC_SUBST([TCL_NEEDS_EXP_FILE])
#AC_SUBST([TCL_EXPORT_FILE_SUFFIX])
#AC_SUBST([TCL_NEEDS_EXP_FILE])
#AC_SUBST([TCL_EXPORT_FILE_SUFFIX])

# TCL modules use stubs and don't need any other tcl libraries
TCL_LIBS=
AC_SUBST([TCL_LIBS])

#AC_SUBST([TCL_PREFIX])
#AC_SUBST([TCL_EXEC_PREFIX])
#AC_SUBST([TCL_SHLIB_CFLAGS])
#AC_SUBST([TCL_SHLIB_CXXFLAGS])
#AC_SUBST([TCL_EXTRA_CFLAGS])
#AC_SUBST([TCL_SHLIB_LD])
#AC_SUBST([TCL_SHLIB_LD_LIBS])
AC_SUBST([TCL_SHLIB_SUFFIX])
#AC_SUBST([TCL_DL_LIBS])
#AC_SUBST([TCL_LD_FLAGS])
#AC_SUBST([TCL_LD_SEARCH_FLAGS])
#AC_SUBST([TCL_COMPAT_OBJS])
#AC_SUBST([TCL_RANLIB])
#AC_SUBST([TCL_LIB_FLAG])
#AC_SUBST([TCL_BUILD_LIB_SPEC])
#AC_SUBST([TCL_LIB_SPEC])
#AC_SUBST([TCL_LIB_VERSIONS_OK])
#AC_SUBST([TCL_SHARED_LIB_SUFFIX])
#AC_SUBST([TCL_UNSHARED_LIB_SUFFIX])
AC_SUBST([TCL_SRC_DIR])
#AC_SUBST([TCL_PACKAGE_PATH])
#AC_SUBST([TCL_SUPPORTS_STUBS])
#AC_SUBST([TCL_STUB_LIB_FILE])
#AC_SUBST([TCL_STUB_LIB_FLAG])
#AC_SUBST([TCL_BUILD_STUB_LIB_SPEC])
AC_SUBST([TCL_STUB_LIB_SPEC])
#AC_SUBST([TCL_BUILD_STUB_LIB_PATH])
#AC_SUBST([TCL_STUB_LIB_PATH])

dnl -----------------------------------
dnl INCLUDES and LIBS for TK

if test "x$use_tcl" != "xYes"; then
    AC_MSG_WARN([No Tcl. The Tk packages will not be built either])
    use_tk="No (no tcl)"
fi

if test "x$use_tk" = "x"; then
    AC_ARG_WITH(wish,
        [AS_HELP_STRING([--with-wish=PROG],[use a specific wish])],
        WISH=$withval,)
    if test "x$WISH" = "x"; then
        AC_PATH_PROGS(WISH,[wish8.6 wish8.5 wish8.4 wish8.3 wish])
        if test "x$WISH" = "x"; then
            AC_MSG_WARN([Unable to find a wish. The Tk packages will not be built])
            use_tk="No (wish not found)"
    fi
    fi
fi

if test "x$use_tk" = "x"; then
    #TK_VERSION_FOUND=`echo 'puts [[info tkversion]]' | $WISH`
    #
    # No such var as tkversion.  Use tclversion instead.
    TK_VERSION_FOUND=${TCL_VERSION_FOUND}
    
    # can't assume ksh on all architectures
    # WISH_EXEC_PREFIX=${WISH%%/bin/tclsh.*}
    # so use sed instead
    WISH_EXEC_PREFIX=`echo $WISH|sed -e 's%/bin/.*wish.*$%%'`

    if test -f ${WISH_EXEC_PREFIX}/include/tk${TK_VERSION_FOUND}/tk.h; then
	TK_INCLUDES="-I${WISH_EXEC_PREFIX}/include/tk${TK_VERSION_FOUND} -I${WISH_EXEC_PREFIX}/include/tk${TK_VERSION_FOUND}/generic -I${WISH_EXEC_PREFIX}/include/tk${TK_VERSION_FOUND}/unix"
    else
        if test "x${WISH_EXEC_PREFIX}" != "x/usr" -a -f ${WISH_EXEC_PREFIX}/include/tk.h; then
	    TK_INCLUDES=-I${WISH_EXEC_PREFIX}/include
        fi
    fi
    if test "x$TK_INCLUDES" != "x" ; then
	AC_MSG_RESULT([using tk headers from $TK_INCLUDES])
    fi
    AC_SUBST([TK_INCLUDES])
    save_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $TCL_INCLUDES $TK_INCLUDES $X_CFLAGS"
    AC_CHECK_HEADER(tk.h,,[
        AC_MSG_WARN([Unable to find header tk.h. The Tk packages will not be built])
        use_tk="No (missing header)"])
    AC_CHECK_HEADERS(tk.h tkInt.h)
    CPPFLAGS=$save_CPPFLAGS
fi

if test "x$use_tk" = "x"; then
    # Some systems don't have Tcl.  Don't build
    # Some systems have Tcl, but not Tk.  Don't build
    # the Tk products if we don't have the library.
    if test -f ${WISH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tk${TK_VERSION_FOUND}/tkConfig.sh; then
        TKCONFIG=${WISH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tk${TK_VERSION_FOUND}/tkConfig.sh
    else
        if test -f ${WISH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tkConfig.sh; then
	    TKCONFIG=${WISH_EXEC_PREFIX}/lib${LIBPOSTFIX}/tkConfig.sh
	else
            if test -f ${WISH_EXEC_PREFIX}/lib/tkConfig.sh; then
	        TKCONFIG=${WISH_EXEC_PREFIX}/lib/tkConfig.sh
            fi
        fi
    fi
    if test "x$TKCONFIG" != "x"; then
        AC_MSG_RESULT([using $TKCONFIG])
        file=${TKCONFIG}
        . $file
    else
        AC_MSG_WARN([Unable to find tkConfig.sh. The Tk packages will not be built])
        use_tk="No (missing tkConfig.sh)"
    fi
fi
                                                                                
if test "x$use_tk" = "x"; then
    use_tk="Yes"
    TK_SUBDIRS="tkstubs tkspline"
else
    TK_SUBDIRS=""
fi
AM_CONDITIONAL(WITH_TK, [test "x$use_tk" = "xYes"])

# NB. No TK_SUPPORTS_STUBS set by tkConfig.sh
if test "${TCL_SUPPORTS_STUBS}" = "1"; then
    TK_CFLAGS="${TK_CFLAGS} -DUSE_TK_STUBS"
fi

AC_SUBST([TK_VERSION])
AC_SUBST([TK_MAJOR_VERSION])
AC_SUBST([TK_MINOR_VERSION])
AC_SUBST([TK_PATCH_LEVEL])
AC_SUBST([TK_DEFS])
#AC_SUBST([TK_SHARED_BUILD])
#AC_SUBST([TK_LIB_FILE])
#AC_SUBST([TK_LIBS])
#AC_SUBST([TK_PREFIX])
#AC_SUBST([TK_EXEC_PREFIX])
AC_SUBST([TK_XINCLUDES])
#AC_SUBST([TK_XLIBSW])
#AC_SUBST([TK_BUILD_LIB_SPEC])
AC_SUBST([TK_LIB_SPEC])
AC_SUBST([TK_SRC_DIR])
AC_SUBST([TK_CFLAGS])
#AC_SUBST([TK_SUPPORTS_STUBS])
#AC_SUBST([TK_STUB_LIB_FILE])
#AC_SUBST([TK_STUB_LIB_FLAG])
#AC_SUBST([TK_BUILD_STUB_LIB_SPEC])
AC_SUBST([TK_STUB_LIB_SPEC])
#AC_SUBST([TK_BUILD_STUB_LIB_PATH])
#AC_SUBST([TK_STUB_LIB_PATH])

dnl -----------------------------------
dnl Support for generic "extra" search paths for includes and libraries

AC_ARG_WITH(extraincludedir,
  [AS_HELP_STRING([--with-extraincludedir=DIR],[use extra includes from DIR])],
  [CPPFLAGS="$CPPFLAGS -I$withval"])

AC_ARG_WITH(extralibdir,
  [AS_HELP_STRING([--with-extralibdir=DIR],[use extra libraries from DIR])],
  [LDFLAGS="$LDFLAGS -L$withval"])

dnl -----------------------------------
dnl ...borrow test for -lsocket -lnsl from tk8.3.3/unix/configure.in

save_LIBS=$LIBS
LIBS=
tk_checkBoth=0
AC_CHECK_FUNC(connect, tk_checkSocket=0, tk_checkSocket=1)
if test "$tk_checkSocket" = 1; then
  AC_CHECK_LIB(socket, main, LIBS="$LIBS -lsocket", tk_checkBoth=1)
fi
if test "$tk_checkBoth" = 1; then
  tk_oldLibs=$LIBS
  LIBS="$LIBS -lsocket -lnsl"
  AC_CHECK_FUNC(accept, tk_checkNsl=0, [LIBS=$tk_oldLibs])
fi  
AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, main, [LIBS="$LIBS -lnsl"])])
SOCKET_LIBS=$LIBS
AC_SUBST([SOCKET_LIBS])
LIBS=$save_LIBS

dnl -----------------------------------
dnl INCLUDES and LIBS for EXPAT

AC_ARG_WITH(expat,
  [AS_HELP_STRING([--with-expat=yes],[use expat])],
  [],[with_expat=yes])

if test "x$with_expat" != "xyes"; then
  use_expat="No (disabled)"
else

  save_CPPFLAGS=$CPPFLAGS
  save_LDFLAGS=$LDFLAGS
  AC_PATH_PROG(EXPAT_CONFIG,expat-config)
  if test -n "$EXPAT_CONFIG"; then
	EXPAT_INCLUDES=`$EXPAT_CONFIG --cflags`
	EXPAT_PREFIX=`$EXPAT_CONFIG --prefix`
	if test "${EXPAT_PREFIX}/lib" != "/usr/lib" ; then
		EXPAT_LIBS="-L${EXPAT_PREFIX}/lib"
	fi
  else         
	EXPAT_INCLUDES=""
	EXPAT_LIBS=""
  fi
  AC_ARG_WITH(expatincludedir,
    [AS_HELP_STRING([--with-expatincludedir=DIR],[use EXPAT includes from DIR])],
    [EXPAT_INCLUDES="-I$withval"])
  CPPFLAGS="$CPPFLAGS $EXPAT_INCLUDES"
  EXPAT_LIBDIR="/usr/lib$LIBPOSTFIX"
  AC_ARG_WITH(expatlibdir,
    [AS_HELP_STRING([--with-expatlibdir=DIR],[use EXPAT libraries from DIR])],
    [EXPAT_LIBDIR="$withval"])
  if test "$EXPAT_LIBDIR" != "/usr/lib"; then
	  EXPAT_LIBS="-L$EXPAT_LIBDIR"
	  LDFLAGS="$LDFLAGS $EXPAT_LIBS"
  fi
  AC_CHECK_HEADERS([expat.h],
    [],
    [use_expat="No (missing header)"
     AC_MSG_WARN(Optional expat library not available - missing headers)],
    [])
  AC_CHECK_LIB(expat,main,
    [EXPAT_LIBS="$EXPAT_LIBS -lexpat"],
    [use_expat="No (missing library)"
     AC_MSG_WARN(Optional expat library not available)],
    [])
  LDFLAGS=$save_LDFLAGS
  CPPFLAGS=$save_CPPFLAGS
fi
if test "x$use_expat" = "x"; then
  use_expat="Yes"
  AC_DEFINE_UNQUOTED(HAVE_EXPAT,1,
    [Define if you have the expat library])
  AC_SUBST([EXPAT_INCLUDES])
  AC_SUBST([EXPAT_LIBS])
fi
  
dnl -----------------------------------
dnl INCLUDES and LIBS for DEVIL

AC_ARG_WITH(devil,
  [AS_HELP_STRING([--with-devil=yes],[DevIL plugin])],
  [],[with_devil=yes])

if test "x$with_devil" != "xyes"; then
  use_devil="No (disabled)"
else

  AC_ARG_WITH(devilincludedir,
    [AS_HELP_STRING([--with-devilincludedir=DIR],[use DevIL includes from DIR])],
    [DEVIL_INCLUDES="-I$withval"])
  AC_ARG_WITH(devillibdir,
    [AS_HELP_STRING([--with-devillibdir=DIR],[use DevIL libraries from DIR])],
    [DEVIL_LIBS="-L$withval"])

  save_CPPFLAGS=$CPPFLAGS
  save_LDFLAGS=$LDFLAGS
  CPPFLAGS="$CPPFLAGS $DEVIL_INCLUDES"
  LDFLAGS="$LDFLAGS $DEVIL_LIBS"
  AC_CHECK_HEADERS([IL/il.h],
    [],
    [use_devil="No (missing header)"
     AC_MSG_WARN(Optional DevIL library not available - missing headers)],
    [])
  AC_CHECK_LIB(IL,main,
    [DEVIL_LIBS="$DEVIL_LIBS -lIL -lILU"],
    [use_devil="No (missing library)"
     AC_MSG_WARN(Optional DevIL library not available)],
    [])
  LDFLAGS=$save_LDFLAGS
  CPPFLAGS=$save_CPPFLAGS
fi
if test "x$use_devil" = "x"; then
  use_devil="Yes"
  AC_DEFINE_UNQUOTED(HAVE_DEVIL,1,
    [Define if you have the DevIL library])
  AC_SUBST([DEVIL_INCLUDES])
  AC_SUBST([DEVIL_LIBS])
fi

AM_CONDITIONAL(WITH_DEVIL, [test "x$use_devil" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for XPM
#
# we don't want an X11 dependency in libgd
# so we set HAVE_LIBXPMFORLEFTY instead of the usual HAVE_LIBXPM
#

AC_ARG_WITH(Xpmincludedir,
  [AS_HELP_STRING([--with-Xpmincludedir=DIR],[use Xpm includes from DIR])],
  [XPM_INCLUDES="-I$withval"])
AC_ARG_WITH(Xpmlibdir,
  [AS_HELP_STRING([--with-Xpmlibdir=DIR],[use Xpm libraries from DIR])],
  [XPM_LIBS="-L$withval"])

save_CPPFLAGS=$CPPFLAGS
save_LDFLAGS=$LDFLAGS
CPPFLAGS="$CPPFLAGS $XPM_INCLUDES $X_CFLAGS"
LDFLAGS="$LDFLAGS $XPM_LIBS $X_LIBS $X_EXTRA_LIBS $X_PRE_LIBS $SOCKET_LIBS"
AC_CHECK_LIB(Xpm,main,
  [XPM_LIBS="$XPM_LIBS -lXpm"
   AC_DEFINE_UNQUOTED(HAVE_LIBXPMFORLEFTY,1,[Define if you have the XPM library])],
   AC_MSG_WARN(Optional Xpm library not available), [-lXmu -lXt -lX11 -lXext $SOCKET_LIBS])
LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([XPM_INCLUDES])
AC_SUBST([XPM_LIBS])

dnl -----------------------------------
dnl INCLUDES and LIBS for XAW

AC_ARG_WITH(Xawincludedir,
  [AS_HELP_STRING([--with-Xawincludedir=DIR],[use Xaw includes from DIR])],
  [XAW_INCLUDES="-I$withval"])
AC_ARG_WITH(Xawlibdir,
  [AS_HELP_STRING([--with-Xawlibdir=DIR],[use Xaw libraries from DIR])],
  [XAW_LIBS="-L$withval"])
save_CPPFLAGS=$CPPFLAGS
save_LDFLAGS=$LDFLAGS
CPPFLAGS="$CPPFLAGS $XAW_INCLUDES $X_CFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS $XAW_LIBS $XPM_LIBS $X_EXTRA_LIBS $X_PRE_LIBS $SOCKET_LIBS"
AC_CHECK_HEADERS([X11/Intrinsic.h])
use_lefty="Yes"
AC_CHECK_HEADERS([X11/Xaw/Text.h],
		 [],
		 [with_xaw=no
	          use_lefty="No (missing Xaw headers)"
		  AC_MSG_WARN(Optional Xaw library not available - missing headers)],
		 [#if HAVE_X11_INTRINSIC_H
		  # include <X11/Intrinsic.h>
		  # endif
		  ])
if test "x$with_xaw" != "xno"; then
AC_CHECK_LIB(Xaw,main,
	     [XAW_LIBS="$X_LIBS $XAW_LIBS -lXaw"],
	     [with_xaw=no
	      use_lefty="No (missing Xaw library)"
	      AC_MSG_WARN(Optional Xaw library not available)],
	      [$X_LIBS -lXmu -lXt -lX11 -lXext $SOCKET_LIBS])
fi
LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([XAW_INCLUDES])
AC_SUBST([XAW_LIBS])
AM_CONDITIONAL(WITH_XAW, [test "x$with_xaw" != "xno"])
if test "x$with_xaw" = "xno"; then
    if test "x$no_x" != "xyes"; then
	AC_MSG_WARN(Lefty cannot be built)
    fi
fi

dnl -----------------------------------
dnl INCLUDES and LIBS for Z

AC_ARG_WITH(zincludedir,
  [AS_HELP_STRING([--with-zincludedir=DIR],[use Z includes from DIR])],
  [Z_INCLUDES="-I$withval"])
AC_ARG_WITH(zlibdir,
  [AS_HELP_STRING([--with-zlibdir=DIR],[use Z libraries from DIR])],
  [Z_LIBS="-L$withval"])

save_CPPFLAGS=$CPPFLAGS
save_LDFLAGS=$LDFLAGS
CPPFLAGS="$CPPFLAGS $Z_INCLUDES"
LDFLAGS="$LDFLAGS $Z_LIBS"
AC_CHECK_HEADER(zlib.h,
	[AC_CHECK_LIB(z,main,
		[Z_LIBS="$Z_LIBS -lz"
		AC_DEFINE_UNQUOTED(HAVE_LIBZ,1,[Define if you have the Z library])],
			AC_MSG_WARN(Optional z library not available))],
	AC_MSG_WARN(Optional z library not available - no zlib.h))
	LDFLAGS="$LDFLAGS $Z_LIBS"
        AC_CHECK_FUNCS(deflateBound)
LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([Z_INCLUDES])
AC_SUBST([Z_LIBS])

dnl -----------------------------------
dnl INCLUDES and LIBS for WEBP
                                                                                
AC_ARG_WITH(webp,
  [AS_HELP_STRING([--with-webp=no],[webp library])],
  [], [with_webp=no])

if test "x$with_webp" != "xyes"; then
  use_webp="No (disabled by default - experimental)"
else
  PKG_CHECK_MODULES(WEBP, [libwebp],[
    use_webp="Yes"
    AC_DEFINE_UNQUOTED(HAVE_WEBP,1,
      [Define if you have the webp library])
    AC_SUBST([WEBP_CFLAGS])
    AC_SUBST([WEBP_LIBS])
  ],[
    use_webp="No (webp library not available)"
  ])
fi
AM_CONDITIONAL(WITH_WEBP, [test "x$use_webp" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for POPPLER
                                                                                
AC_ARG_WITH(poppler,
  [AS_HELP_STRING([--with-poppler=yes],[poppler library])],
  [], [with_poppler=yes])

if test "x$with_poppler" != "xyes"; then
  use_poppler="No (disabled)"
else
  PKG_CHECK_MODULES(POPPLER, [poppler-glib],[
    use_poppler="Yes"
    AC_DEFINE_UNQUOTED(HAVE_POPPLER,1,
      [Define if you have the poppler library])
    AC_SUBST([POPPLER_CFLAGS])
    AC_SUBST([POPPLER_LIBS])
  ],[
    use_poppler="No (poppler library not available)"
  ])
fi
AM_CONDITIONAL(WITH_POPPLER, [test "x$use_poppler" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for RSVG
                                                                                
AC_ARG_WITH(rsvg,
  [AS_HELP_STRING([--with-rsvg=yes],[rsvg library])],
  [], [with_rsvg=yes])

if test "x$with_rsvg" != "xyes"; then
  use_rsvg="No (disabled)"
else
  PKG_CHECK_MODULES(RSVG, [librsvg-2.0],[
    use_rsvg="Yes"
    AC_DEFINE_UNQUOTED(HAVE_RSVG,1,
      [Define if you have the rsvg library])
    AC_SUBST([RSVG_CFLAGS])
    AC_SUBST([RSVG_LIBS])
  ],[
    use_rsvg="No (rsvg library not available)"
  ])
fi
AM_CONDITIONAL(WITH_RSVG, [test "x$use_rsvg" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GHOSTSCRIPT
                                                                                
AC_ARG_WITH(ghostscript,
  [AS_HELP_STRING([--with-ghostscript=yes],[ghostscript library])],
  [], [with_ghostscript=yes])

if test "x$with_ghostscript" != "xyes"; then
  use_ghostscript="No (disabled)"
else
  if test "x$HAVE_XRENDER" != "xyes"; then
    use_ghostscript="No (missing Xrender)"
  else
      AC_CHECK_HEADER(ghostscript/iapi.h, GS_CFLAGS="",use_ghostscript="No (missing headers)")
      if test "x$use_ghostscript" = "x"; then
        AC_CHECK_LIB(gs, main, GS_LIBS="-lgs", use_ghostscript="No (missing lib)")
        if test "x$use_ghostscript" = "x"; then
          use_ghostscript="Yes"
          AC_DEFINE_UNQUOTED(HAVE_GS,1,
            [Define if you have the gs library])
          AC_SUBST([GS_CFLAGS])
          AC_SUBST([GS_LIBS])
        fi
     fi
  fi
fi
AM_CONDITIONAL(WITH_GS, [test "x$use_ghostscript" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for VISIO
                                                                                
AC_ARG_WITH(visio,
  [AS_HELP_STRING([--with-visio=no],[visio library])],
  [], [with_visio=no])

if test "x$with_visio" != "xyes"; then
  use_visio="No (disabled by default - experimental)"
else
#  AC_CHECK_HEADER(visio/visio.h, VISIO_CFLAGS="",use_visio="No (missing headers)")
#  if test "x$use_visio" = "x"; then
#    AC_CHECK_LIB(visio, main, VISIO_LIBS="-lgs", use_visio="No (missing lib)")
#    if test "x$use_visio" = "x"; then
      use_visio="Yes"
      AC_DEFINE_UNQUOTED(HAVE_VISIO,1,
        [Define if you have the visio library])
      AC_SUBST([VISIO_CFLAGS])
      AC_SUBST([VISIO_LIBS])
#    fi
#  fi
fi
AM_CONDITIONAL(WITH_VISIO, [test "x$use_visio" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PANGO, CAIRO et.al.
                                                                                
AC_ARG_WITH(pangocairo,
  [AS_HELP_STRING([--with-pangocairo=yes],[pangocairo library])],
  [], [with_pangocairo=yes])

if test "x$with_pangocairo" != "xyes"; then
  use_pangocairo="No (disabled)"
else
  # require at least the version available on centos5 (fc6 has 1.14.10)
  PKG_CHECK_MODULES(PANGOCAIRO, [pangocairo >= 1.14.9],[
    use_pangocairo="Yes"
    AC_DEFINE_UNQUOTED(HAVE_PANGOCAIRO,1,
      [Define if you have the pangocairo library])
    AC_SUBST([PANGOCAIRO_CFLAGS])
    AC_SUBST([PANGOCAIRO_LIBS])
  ],[
    use_pangocairo="No (pangocairo library not available)"
  ])
fi
AM_CONDITIONAL(WITH_PANGOCAIRO, [test "x$use_pangocairo" = "xYes"])

# see if pango uses a fontconfig/freetype2 backend
if test "x$use_pangocairo" = "xYes"; then
    save_LIBS=$LIBS
    PKG_CHECK_MODULES(PANGOFT2, [pangoft2])
    LIBS="$LIBS $PANGOCAIRO_LIBS $PANGOFT2_LIBS"
    AC_CHECK_FUNCS([pango_fc_font_lock_face pango_fc_font_unlock_face])
    LIBS=$save_LIBS
fi

dnl -----------------------------------
dnl INCLUDES and LIBS for LASI
                                                                                
AC_ARG_WITH(lasi,
  [AS_HELP_STRING([--with-lasi=yes],[lasi library])],
  [], [with_lasi=yes])

if test "x$with_lasi" != "xyes"; then
  use_lasi="No (disabled)"
else
  if test "x$use_pangocairo" != "xYes"; then
    use_lasi="No (missing pangocairo support)"
  else
    PKG_CHECK_MODULES(LASI, [lasi],[
      use_lasi="Yes"
      AC_DEFINE_UNQUOTED(HAVE_LASI,1,
        [Define if you have the lasi library])
      AC_SUBST([LASI_CFLAGS])
      AC_SUBST([LASI_LIBS])
    ],[
      use_lasi="No (lasi library not available)"
    ])
  fi
fi
AM_CONDITIONAL(WITH_LASI, [test "x$use_lasi" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GLITZ
                                                                                
AC_ARG_WITH(glitz,
  [AS_HELP_STRING([--with-glitz=no],[glitz library])],
  [], [with_glitz=no])

if test "x$with_glitz" != "xyes"; then
  use_glitz="No (disabled by default - incomplete)"
else
  PKG_CHECK_MODULES(GLITZ, [glitz],[
    use_glitz="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GLITZ,1,
      [Define if you have the glitz library])
    AC_SUBST([GLITZ_CFLAGS])
    AC_SUBST([GLITZ_LIBS])
  ],[
    use_glitz="No (glitz library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GLITZ, [test "x$use_glitz" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for FREETYPE2
                                                                                
AC_ARG_WITH(freetype2,
  [AS_HELP_STRING([--with-freetype2=yes],[freetype2 library])],
  [], [with_freetype2=yes])

if test "x$with_freetype2" != "xyes"; then
  use_freetype="No (disabled)"
else
  PKG_CHECK_MODULES(FREETYPE2, [freetype2],[
    use_freetype="Yes"
  ],[
    AC_MSG_WARN(pkg-config did not find a freetype2.pc.  Looking for freetype-config.)
  ])
fi

# pkgconfig for freetype2 did not exist in redhat[78]
if test "x$use_freetype" = "x"; then
    if test "x$FREETYPE_DIR" = "xyes"; then
	AC_PATH_PROG(FREETYPE_CONFIG,freetype-config)
    else
	AC_PATH_PROG(FREETYPE_CONFIG,freetype-config,,[$FREETYPE_DIR/bin:$PATH])
    fi

    if test -n "$FREETYPE_CONFIG"; then
        if test "x$FREETYPE_DIR" != "xyes"; then
	    FREETYPE2_CFLAGS="`$FREETYPE_CONFIG --cflags` -I$FREETYPE_DIR/include"
	else
	    FREETYPE2_CFLAGS=`$FREETYPE_CONFIG --cflags`
	fi
	FREETYPE2_LIBS=`$FREETYPE_CONFIG --libs`
	use_freetype="Yes"
    else
        AC_MSG_WARN(freetype library not available)
	use_freetype="No (missing freetype-config)"
    fi
fi

if test "x$use_freetype" = "xYes"; then
  AC_DEFINE_UNQUOTED(HAVE_FREETYPE2,1,[Define if you have the freetype2 library])
  AC_SUBST([FREETYPE2_CFLAGS])
  AC_SUBST([FREETYPE2_LIBS])
fi
AM_CONDITIONAL(WITH_FREETYPE2, [test "x$ue_freetype" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for FONTCONFIG
                                                                                
AC_ARG_WITH(fontconfig,
  [AS_HELP_STRING([--with-fontconfig=yes],[use fontconfig library])],
  [], [with_fontconfig=yes])

if test "x$with_fontconfig" != "xyes"; then
  use_fontconfig="No (disabled)"
else
  PKG_CHECK_MODULES(FONTCONFIG, [fontconfig],[
    use_fontconfig="Yes"
  ],[
    AC_MSG_WARN(pkg-config did not find a fontconfig.pc.  Looking for fontconfig-config.)
  ])
fi

# just in case pkgconfig for fontconfig does not exist
if test "x$use_fontconfig" = "x"; then
    if test "x$FONTCONFIG_DIR" = "xyes"; then
	AC_PATH_PROG(FONTCONFIG_CONFIG,fontconfig-config)
    else
	AC_PATH_PROG(FONTCONFIG_CONFIG,fontconfig-config,,[$FONTCONFIG_DIR/bin:$PATH])
    fi

    if test -n "$FONTCONFIG_CONFIG"; then
        if test "x$FONTCONFIG_DIR" != "xyes"; then
	    FONTCONFIG_CFLAGS="`$FONTCONFIG_CONFIG --cflags` -I$FONTCONFIG_DIR/include"
	else
	    FONTCONFIG_CFLAGS=`$FONTCONFIG_CONFIG --cflags`
	fi
	FONTCONFIG_LIBS=`$FONTCONFIG_CONFIG --libs`
	use_fontconfig="Yes"
    else
	use_fontconfig="No (missing fontconfig-config)"
        AC_MSG_WARN(fontconfig library not available)
    fi
fi

if test "x$use_fontconfig" = "xYes"; then
  AC_DEFINE_UNQUOTED(HAVE_FONTCONFIG,1,[Define if you have the fontconfig library])
  AC_SUBST([FONTCONFIG_CFLAGS])
  AC_SUBST([FONTCONFIG_LIBS])
fi
AM_CONDITIONAL(WITH_FONTCONFIG, [test "x$use_fontconfig" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GDK_PIXBUF.

AC_ARG_WITH(gdk-pixbuf,
  [AS_HELP_STRING([--with-gdk-pixbuf=yes],[gdk-pixbuf library])],
  [], [with_gdk_pixbuf=yes])

if test "x$with_gdk_pixbuf" != "xyes"; then
  use_gdk_pixbuf="No (disabled)"
else
  PKG_CHECK_MODULES(GDK_PIXBUF, [gdk-pixbuf-2.0],[
    use_gdk_pixbuf="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GDK_PIXBUF,1,
      [Define if you have the gdk_pixbuf library])
    AC_SUBST([GDK_PIXBUF_CFLAGS])
    AC_SUBST([GDK_PIXBUF_LIBS])
  ],[
    use_gdk_pixbuf="No (gdk_pixbuf library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GDK_PIXBUF, [test "x$use_gdk_pixbuf" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GTK.

AC_ARG_WITH(gtk,
  [AS_HELP_STRING([--with-gtk=yes],[gtk+ library])],
  [], [with_gtk=yes])

if test "x$with_gtk" != "xyes"; then
  use_gtk="No (disabled)"
else
  PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.7.0],[
    use_gtk="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GTK,1,
      [Define if you have the gtk library])
    AC_SUBST([GTK_CFLAGS])
    AC_SUBST([GTK_LIBS])
  ],[
    use_gtk="No (gtk library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GTK, [test "x$use_gtk" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GTKGL.

AC_ARG_WITH(gtkgl,
  [AS_HELP_STRING([--with-gtkgl=yes],[gtkgl library])],
  [], [with_gtkgl=yes])

if test "x$with_gtkgl" != "xyes"; then
  use_gtkgl="No (disabled)"
else
  PKG_CHECK_MODULES(GTKGL, [gtkgl-2.0],[
    use_gtkgl="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GTKGL,1,
      [Define if you have the gtkgl library])
    AC_SUBST([GTKGL_CFLAGS])
    AC_SUBST([GTKGL_LIBS])
  ],[
    use_gtkgl="No (gtkgl library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GTKGL, [test "x$use_gtkgl" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GTKGLEXT.

AC_ARG_WITH(gtkglext,
  [AS_HELP_STRING([--with-gtkglext=yes],[gtkglext library])],
  [], [with_gtkglext=yes])

if test "x$with_gtkglext" != "xyes"; then
  use_gtkglext="No (disabled)"
else
  PKG_CHECK_MODULES(GTKGLEXT, [gtkglext-1.0],[
    use_gtkglext="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GTKGLEXT,1,
      [Define if you have the gtkglext library])
    AC_SUBST([GTKGLEXT_CFLAGS])
    AC_SUBST([GTKGLEXT_LIBS])
  ],[
    use_gtkglext="No (gtkglext library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GTKGLEXT, [test "x$use_gtkglext" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GTS.

AC_ARG_WITH(gts,
  [AS_HELP_STRING([--with-gts=yes],[gts library])],
  [], [with_gts=yes])

if test "x$with_gts" != "xyes"; then
  use_gts="No (disabled)"
else
  PKG_CHECK_MODULES(GTS, [gts],[
    use_gts="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GTS,1,
      [Define if you have the gts library])
    AC_SUBST([GTS_CFLAGS])
    AC_SUBST([GTS_LIBS])
  ],[
    use_gts="No (gts library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GTS, [test "x$use_gts" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for ANN.

AC_ARG_WITH(ann,
  [AS_HELP_STRING([--with-ann=yes],[ANN library])],
  [], [with_ann=yes])

if test "x$with_ann" != "xyes"; then
  use_ann="No (disabled)"
else
  PKG_CHECK_MODULES(ANN, [ann],[
    use_ann="Yes"
    AC_DEFINE_UNQUOTED(HAVE_ANN,1,
      [Define if you have the ann library])
    AC_SUBST([ANN_CFLAGS])
    AC_SUBST([ANN_LIBS])
  ],[
    use_ann="No (no ann.pc found)"
  ])
  if test "$use_ann" != "Yes"; then
    if test -f /usr/lib$LIBPOSTFIX/libANN.so; then
      AC_DEFINE_UNQUOTED(HAVE_ANN,1,
        [Define if you have the ann library])
      ANN_LIBS=-lANN
      AC_SUBST([ANN_LIBS])
      use_ann="Yes (but no ann.pc found)"
    else
      if test -f /usr/lib$LIBPOSTFIX/libann.so; then
        AC_DEFINE_UNQUOTED(HAVE_ANN,1,
          [Define if you have the ann library])
        ANN_LIBS=-lann
        AC_SUBST([ANN_LIBS])
        use_ann="Yes (but no ann.pc found)"
      else
        use_ann="No (ANN library not available)"
      fi
    fi
  fi
fi
AM_CONDITIONAL(WITH_ANN, [test x"${use_ann%% *}" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GLADE.

AC_ARG_WITH(glade,
  [AS_HELP_STRING([--with-glade=yes],[glade library])],
  [], [with_glade=yes])

if test "x$with_glade" != "xyes"; then
  use_glade="No (disabled)"
else
  PKG_CHECK_MODULES(GLADE, [libglade-2.0],[
    use_glade="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GLADE,1,
      [Define if you have the glade library])
    AC_SUBST([GLADE_CFLAGS])
    AC_SUBST([GLADE_LIBS])
  ],[
    use_glade="No (glade library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GLADE, [test "x$use_glade" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for MING (SWF renderer)

AC_ARG_WITH(ming,
  [AS_HELP_STRING([--with-ming=no],[ming library (plugin for -Tswf)])],
  [], [with_ming=no])

if test "x$with_ming" != "xyes"; then
  use_ming="No (disabled by default - incomplete)"
else
  PKG_CHECK_MODULES(MING, [libming >= 0.4],[
    use_ming="Yes"
    AC_DEFINE_UNQUOTED(HAVE_MING,1,
      [Define if you have the ming library for SWF support])
    AC_SUBST([MING_CFLAGS])
    AC_SUBST([MING_LIBS])
  ],[
    use_ming="No (ming library not available)"
  ])
fi
AM_CONDITIONAL(WITH_MING, [test "x$use_ming" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for QT

AC_ARG_WITH(qt,
  [AS_HELP_STRING([--with-qt=yes],[Qt features])],
  [], [with_qt=yes])

if test "x$with_qt" != "xyes"; then
  use_qt="No (disabled)"
else

  AC_CHECK_PROGS(QMAKE,qmake-qt4 qmake-qt3 qmake,false)
  if test "$QMAKE" != "false"; then
    PKG_CHECK_MODULES(QTCORE, [QtCore],[
      PKG_CHECK_MODULES(QTGUI, [QtGui],[
	use_qt="Yes"
      ],[
        use_qt="No (QtGui not available)"
      ])
    ],[
      use_qt="No (QtCore not available)"
    ])
  else
    use_qt="No (qmake not found)"
  fi
fi
AM_CONDITIONAL(WITH_QT, [test "x$use_qt" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for QUARTZ 

AC_ARG_WITH(quartz,
  [AS_HELP_STRING([--with-quartz=no],[Quartz framework (Mac OS X)])],
  [], [with_quartz=no])

if test "x$with_quartz" != "xyes"; then
  use_quartz="No (disabled by default - Mac only)"
else
  if test -d "/System/Library/Frameworks/ApplicationServices.framework"; then
    use_quartz="Yes"
    AC_DEFINE_UNQUOTED(HAVE_QUARTZ,1,[Define if you have the Quartz framework for Mac OS X])
    QUARTZ_CFLAGS=''
    QUARTZ_LIBS='-framework ApplicationServices'
    AC_SUBST([QUARTZ_CFLAGS])
    AC_SUBST([QUARTZ_LIBS])
  else
    use_quartz="No (missing ApplicationServices.framework)"
  fi
fi
AM_CONDITIONAL(WITH_QUARTZ, [test "x$use_quartz" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for PLATFORMSDK

AC_ARG_WITH(platformsdkincludedir,
  [AS_HELP_STRING([--with-platformsdkincludedir=DIR],[use Platform SDK (Windows) includes from DIR])],
  [PLATFORMSDKINCLUDE=$withval; PLATFORMSDKINCLUDE_ESCAPED=${PLATFORMSDKINCLUDE//'\\'/\\\\}; PLATFORMSDKINCLUDE_ESCAPED=${PLATFORMSDKINCLUDE_ESCAPED// /\\ }])

AC_ARG_WITH(platformsdklibdir,
  [AS_HELP_STRING([--with-platformsdklibdir=DIR],[use Platform SDK (Windows) libraries from DIR])],
  [PLATFORMSDKLIB=$withval])

AC_SUBST([PLATFORMSDKINCLUDE])
AC_SUBST([PLATFORMSDKINCLUDE_ESCAPED])
AC_SUBST([PLATFORMSDKLIB]) 

dnl -----------------------------------
dnl INCLUDES and LIBS for GDI+ 

AC_ARG_WITH(gdiplus,
  [AS_HELP_STRING([--with-gdiplus=no],[GDI+ framework (Windows)])],
  [], [with_gdiplus=no])

if test "x$with_gdiplus" != "xyes"; then
  use_gdiplus="No (disabled by default - Windows only)"
else
  if test -f "$PLATFORMSDKINCLUDE\GdiPlus.h" -a -f "$PLATFORMSDKLIB\GdiPlus.lib"; then
    use_gdiplus="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GDIPLUS,1,[Define if you have the GDI+ framework for Windows])
    GDIPLUS_HEADS=`cd "$PLATFORMSDKINCLUDE" && echo GdiPlus*.h`
    GDIPLUS_CFLAGS=''
    GDIPLUS_LIBS=''
    AC_SUBST([GDIPLUS_HEADS])
    AC_SUBST([GDIPLUS_CFLAGS])
    AC_SUBST([GDIPLUS_LIBS])
  else
    use_gdiplus="No (missing headers)"
    AC_MSG_WARN(--with-gdiplus requires valid --with-platformsdkincludedir and --with-platformsdklibdir.)
  fi
fi
AM_CONDITIONAL(WITH_GDIPLUS, [test "x$use_gdiplus" = "xYes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GD
                                                                                
AC_ARG_WITH(libgd,
  [AS_HELP_STRING([--with-libgd=yes],[use gd library])],
  [], [with_libgd=yes])

if test "x$with_libgd" != "xyes"; then
  use_gd="No (disabled)"
fi

if test "x$use_gd" = "x"; then
      AC_PATH_PROG(GDLIB_CONFIG,gdlib-config)
      if test -n "$GDLIB_CONFIG"; then
        GD_INCLUDES=`$GDLIB_CONFIG --includes`
        GD_LDFLAGS=`$GDLIB_CONFIG --ldflags | sed 's/-xarch=[^ ]* //'`
        GD_LIBS=`$GDLIB_CONFIG --libs`
        GD_MAJORVERSION=`$GDLIB_CONFIG --majorversion`
        GD_MINORVERSION=`$GDLIB_CONFIG --minorversion`
        GD_REVISION=`$GDLIB_CONFIG --revision`
        for f in `$GDLIB_CONFIG --features` ; do
	    if test "$f" = "GD_PNG"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_PNG,1,[Define if the GD library supports PNG])
	    fi
	    if test "$f" = "GD_JPEG"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_JPEG,1,[Define if the GD library supports JPEG])
	    fi
	    if test "$f" = "GD_XPM"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_XPM,1,[Define if the GD library supports XPM])
	    fi
	    if test "$f" = "GD_FONTCONFIG"; then
		HAVE_GD_FONTCONFIG=1
                AC_DEFINE_UNQUOTED(HAVE_GD_FONTCONFIG,1,[Define if the GD library supports FONTCONFIG])
	    fi
	    if test "$f" = "GD_FREETYPE"; then
		HAVE_GD_FREETYPE=1
                AC_DEFINE_UNQUOTED(HAVE_GD_FREETYPE,1,[Define if the GD library supports FREETYPE])
	    fi
	    if test "$f" = "GD_GIF"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_GIF,1,[Define if the GD library supports GIF])
	    fi
	    if test "$f" = "GD_GIFANIM"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_GIFANIM,1,[Define if the GD library supports GIFANIM])
	    fi
	    if test "$f" = "GD_OPENPOLYGON"; then
                AC_DEFINE_UNQUOTED(HAVE_GD_OPENPOLYGON,1,[Define if the GD library supports OPENPOLYGON])
	    fi
        done

	if test "x$HAVE_GD_FONTCONFIG" = "x"; then
            AC_MSG_WARN(Your libgd was not built with freetype support. This may result in problems displaying fonts.)
	fi
	if test "x$HAVE_GD_FREETYPE" = "x"; then
	    AC_MSG_WARN(Your libgd was not built with fontconfig support. This may result in problems resolving fonts.)
        fi

        # GD version check >= 2.0.33  -- centos-5.8 has gd-2.0.33
        REQ_GD_MAJORVERSION=2
        REQ_GD_MINORVERSION=0
        REQ_GD_REVISION=33
        if test $GD_MAJORVERSION -lt $REQ_GD_MAJORVERSION; then
            use_gd="No (major version too old)"
        else
            if test $GD_MAJORVERSION -eq $REQ_GD_MAJORVERSION; then
                if test $GD_MINORVERSION -lt $REQ_GD_MINORVERSION; then
                    use_gd="No (minor version too old)"
	        else
                    if test $GD_MINORVERSION -eq $REQ_GD_MINORVERSION; then
		        if test $GD_REVISION -lt $REQ_GD_REVISION; then
                            use_gd="No (revision too old)"
		        fi
	            fi
	        fi
            fi
        fi
        if test "x$use_gd" != "x"; then
            AC_MSG_WARN(GD library version < $REQ_GD_MAJORVERSION.$REQ_GD_MINORVERSION.$REQ_GD_REVISION)
        fi
      else
            AC_MSG_WARN(GD gdlib-config not found)
      fi
  # prevent explicit use of -I/usr/include as it breaks mingw cross-compiles
  if test "x$GD_INCLUDES" = "x-I/usr/include"; then
     GD_INCLUDES="";
  fi
  # prevent explicit use of -L/usr/lib or -L/usr/lib64, unnecessary clutter
  if test "x$GD_LDFLAGS" = "x-L/usr/lib"; then
     GD_LDFLAGS="";
  fi
  if test "x$GD_LDFLAGS" = "x-L/usr/lib64"; then
     GD_LDFLAGS="";
  fi

  if test "x$use_gd" = "x"; then
    if test "x$with_libgd" = "xyes"; then
    # see if we can use the external gd lib
      save_CPPFLAGS=$CPPFLAGS
      save_LDFLAGS=$LDFLAGS
      AC_ARG_WITH(gdincludedir,
        [AS_HELP_STRING([--with-gdincludedir=DIR],[use GD includes from DIR])],
        GD_INCLUDES="-I$withval")
      CPPFLAGS="$CPPFLAGS $GD_INCLUDES"
      AC_ARG_WITH(gdlibdir,
        [AS_HELP_STRING([--with-gdlibdir=DIR],[use GD libraries from DIR])],
        [GD_LDFLAGS="-L$withval"])
      GD_LIBS="$GD_LDFLAGS -lgd $GD_LIBS"
      LDFLAGS="$LDFLAGS $GD_LIBS"
      AC_CHECK_HEADER(gd.h,,[
        AC_MSG_WARN(Optional GD library not available - no gd.h)
        use_gd="No (gd headers not found)"
	with_libgd="no"
      ])
      if test "x$use_gd" = "x"; then
        AC_CHECK_LIB(gd,main,[
          AC_DEFINE_UNQUOTED(HAVE_LIBGD,1,[Define if you have the GD library])
	  use_gd="Yes"
        ],[
          AC_MSG_WARN(Optional GD library not available)
          use_gd="No (library not found)"
	  with_libgd="no"
        ], $GD_LIBS)
      fi
      LDFLAGS=$save_LDFLAGS
      CPPFLAGS=$save_CPPFLAGS
    fi
  fi
  AC_SUBST([GD_INCLUDES])
  AC_SUBST([GD_LIBS])
fi
AM_CONDITIONAL(WITH_LIBGD, [test "x$with_libgd" = "xyes"])

dnl -----------------------------------
dnl INCLUDES and LIBS for GLUT

AC_ARG_WITH(glut,
  [AS_HELP_STRING([--with-glut=yes],[GLUT library])],
  [], [with_glut=yes])

if test "x$with_glut" != "xyes"; then
  use_glut="No (disabled)"
else
  save_CPPFLAGS=$CPPFLAGS
  save_LDFLAGS=$LDFLAGS
  AC_ARG_WITH(glutincludedir,
    [AS_HELP_STRING([--with-glutincludedir=DIR],[use GLUT includes from DIR])],
    [GLUT_CFLAGS="-I$withval"])
  CPPFLAGS="$CPPFLAGS $GLUT_CFLAGS"
  AC_ARG_WITH(glutlibdir,
    [AS_HELP_STRING([--with-glutlibdir=DIR],[use GLUT libraries from DIR])],
    [GLUT_LIBS="-L$withval"])
  LDFLAGS="$LDFLAGS $GLUT_LIBS"
  
  AC_CHECK_HEADER(GL/glut.h,
    AC_CHECK_LIB(glut,main,
      [GLUT_LIBS="$GLUT_LIBS -lglut"
      use_glut="Yes"
      AC_DEFINE_UNQUOTED(HAVE_GLUT,1,[Define if you have the GLUT library])],
      use_glut="No (missing libglut)"
      AC_MSG_WARN(Optional glut library not available),
      []),
    use_glut="No (missing GL/glut.h)"
    AC_MSG_WARN(Optional glut library not available - no GL/glut.h))
fi

LDFLAGS=$save_LDFLAGS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([GLUT_CFLAGS])
AC_SUBST([GLUT_LIBS])

# -----------------------------------
# tclInt.h
if test -f "$TCL_SRC_DIR/generic/tclInt.h"; then
TCLINT_INCLUDES="-I$TCL_SRC_DIR/generic"
else
if test -f "/usr/include/tclInt.h"; then
TCLINT_INCLUDES=""
else
if test -f "/usr/local/include/tclInt.h"; then
TCLINT_INCLUDES="-I/usr/local/include"
else
TCLINT_INCLUDES='-I$(top_srcdir)/tclpkg/tclstubs'
fi
fi
fi
AC_SUBST([TCLINT_INCLUDES])

# -----------------------------------
# tkInt.h
if test -f "$TK_SRC_DIR/generic/tkInt.h"; then
TKINT_INCLUDES="-I$TK_SRC_DIR/generic -I$TK_SRC_DIR/unix"
else
if test -f "/usr/include/tkInt.h"; then
TKINT_INCLUDES=""
else
if test -f "/usr/local/include/tkInt.h"; then
TKINT_INCLUDES="-I/usr/local/include"
else
TKINT_INCLUDES='-I$(top_srcdir)/tclpkg/tkstubs'
fi
fi
fi
AC_SUBST([TKINT_INCLUDES])

# ------------------------------------
# triangle.[ch]

if test -f "lib/sfdpgen/triangle.c"; then
if test -f "lib/sfdpgen/triangle.h"; then
    AC_DEFINE_UNQUOTED(HAVE_TRIANGLE,1,[Define if triangle.[ch] are available.])
fi
fi

# ----------------------------------
# tcl/tk pkgIndex.tcl generation

TCL_PKGINDEX=""
TCL_PKGINDEX_GD=""
TCL_PKGINDEX_SWIG=""
TK_PKGINDEX=""
if test "$use_tcl" = "Yes"; then
    TCL_PKGINDEX="tcldot/pkgIndex.tcl tclpathplan/pkgIndex.tcl"
    if test "x$with_libgd" = "xyes"; then
	TCL_PKGINDEX_GD="gdtclft/pkgIndex.tcl"
    fi
    if test "x$SWIG" != "x"; then
	TCL_PKGINDEX_SWIG="gv/pkgIndex.tcl"
    fi
    if test "$use_tk" = "Yes"; then
        TK_PKGINDEX="tkspline/pkgIndex.tcl"
    fi
fi
AC_SUBST([TCL_PKGINDEX])
AC_SUBST([TCL_PKGINDEX_GD])
AC_SUBST([TCL_PKGINDEX_SWIG])
AC_SUBST([TK_PKGINDEX])

dnl -----------------------------------
dnl SFDP
 
AC_ARG_WITH(sfdp,
  [AS_HELP_STRING([--with-sfdp=yes],[sfdp layout engine])],
  [], [with_sfdp=yes])

if test "x$with_sfdp" != "xyes"; then
  use_sfdp="No (disabled)"
else
  use_sfdp="Yes"
  AC_DEFINE_UNQUOTED(SFDP,1,[Define if you want SFDP])
fi
AM_CONDITIONAL(WITH_SFDP, [test "x$use_sfdp" = "xYes"])

dnl -----------------------------------
dnl SMYRNA 

AC_ARG_WITH(smyrna,
  [AS_HELP_STRING([--with-smyrna=no],[SMYRNA large graph viewer])],
  [], [with_smyrna=no])

if test "x$with_smyrna" != "xyes"; then
  use_smyrna="No (disabled by default - experimental)"
else
  if test "x$use_gtk" != "xYes"; then
     AC_MSG_WARN(SMYRNA requires GTK)
     requires=" gtk+"
  fi
  if test "x$use_gtkglext" != "xYes"; then
     AC_MSG_WARN(SMYRNA requires GTKGLEXT)
     requires="$requires gtkglext"
  fi
  if test "x$use_glade" != "xYes"; then
     AC_MSG_WARN(SMYRNA requires GLADE)
     requires="$requires glade"
  fi
  if test "x$use_gts" != "xYes"; then
     AC_MSG_WARN(SMYRNA requires GTS)
     requires="$requires gts"
  fi
  if test "x$use_glut" != "xYes"; then
     AC_MSG_WARN(SMYRNA requires GLUT)
     requires="$requires glut"
  fi
  if test "x$requires" != "x"; then
     use_smyrna="No (requires:$requires)"
  else
     use_smyrna="Yes"
     AC_DEFINE_UNQUOTED(SMYRNA,1,[Define if you want SMYRNA])
  fi
fi
AM_CONDITIONAL(WITH_SMYRNA, [test "x$use_smyrna" = "xYes"])

dnl -----------------------------------
dnl CGRAPH 

AC_ARG_WITH(cgraph,
  [AS_HELP_STRING([--with-cgraph=yes],[Use CGRAPH library])],
  [], [with_cgraph=yes])

if test "x$with_cgraph" != "xyes"; then
  use_cgraph="No (disabled)"
  use_graph="Yes"
else
  use_cgraph="Yes"
  use_graph="No (disabled deprecated)"
  AC_DEFINE_UNQUOTED(WITH_CGRAPH,1,[Define if you want CGRAPH])
fi
AM_CONDITIONAL(WITH_CGRAPH, [test "x$use_cgraph" = "xYes"])

dnl -----------------------------------
dnl ORTHO 

AC_ARG_WITH(ortho,
  [AS_HELP_STRING([--with-ortho=yes],[ORTHO features in neato layout engine])],
  [], [with_ortho=yes])

if test "x$with_ortho" != "xyes"; then
  use_ortho="No (disabled)"
else
  use_ortho="Yes"
  AC_DEFINE_UNQUOTED(ORTHO,1,[Define if you want ORTHO])
fi
AM_CONDITIONAL(WITH_ORTHO, [test "x$use_ortho" = "xYes"])

dnl -----------------------------------
dnl DIGCOLA 

AC_ARG_WITH(digcola,
  [AS_HELP_STRING([--with-digcola=yes],[DIGCOLA features in neato layout engine])],
  [], [with_digcola=yes])

if test "x$with_digcola" != "xyes"; then
  use_digcola="No (disabled)"
else
  use_digcola="Yes"
  AC_DEFINE_UNQUOTED(DIGCOLA,1,[Define if you want DIGCOLA])
fi

dnl -----------------------------------
dnl IPSEPCOLA   /* disabled by default due to C++ issues on some platforms */
AC_ARG_WITH(ipsepcola,
  [AS_HELP_STRING([--with-ipsepcola=no],[IPSEPCOLA features in neato layout engine])],
  [], [with_ipsepcola=no])

if test "x$with_ipsepcola" != "xyes"; then
  use_ipsepcola="No (disabled by default - C++ portability issues)"
else
  if test "x$use_digcola" != "xYes"; then
    use_ipsepcola="No (requires: digcola)"
  else
    use_ipsepcola="Yes"
    AC_DEFINE_UNQUOTED(IPSEPCOLA,1,[Define if you want IPSEPCOLA])
    IPSEPCOLA_INCLUDES='-I$(top_srcdir)/lib/vpsc'
    IPSEPCOLA_LIBS='$(top_builddir)/lib/vpsc/libvpsc_C.la -lstdc++'
    AC_SUBST([IPSEPCOLA_INCLUDES])
    AC_SUBST([IPSEPCOLA_LIBS])
  fi
fi
AM_CONDITIONAL(WITH_IPSEPCOLA, [test "x$use_ipsepcola" = "xYes"])

# -----------------------------------
AC_MSG_CHECKING(if FILE struct contains _cnt)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
	]], [[FILE *f;
	int i;
	i=f->_cnt;
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_FILE_CNT,1,Define if FILE structure provides _cnt)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if FILE struct contains _r)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
	]], [[FILE *f;
	int i;
	i=f->_r;
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_FILE_R,1,Define if FILE structure provides _r)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if FILE struct contains _next)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
	]], [[FILE *f;
	int i;
	i=f->_next;
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_FILE_NEXT,1,Define if FILE structure provides _next)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if FILE struct contains _IO_read_end)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
	]], [[FILE *f;
	int i;
	i=f->_IO_read_end;
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_FILE_IO_READ_END,1,Define if FILE structure provides _IO_read_end)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if errno externs are declared)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
	#include <errno.h>
	]], [[if ( errno < sys_nerr ) return sys_errlist[errno];
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_ERRNO_DECL,1,Define if errno externs are declared)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if getopt externs are declared)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>
	#include <stdio.h>
	#if HAVE_UNISTD_H
	#include <unistd.h>
	#endif
	]], [[if ( optopt ) exit(0); else exit(1);
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_GETOPT_DECL,1,Define if getopt externs are declared)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if intptr_t is declared)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>
	#if HAVE_STDINT_H
	#include <stdint.h>
	#endif
	#if HAVE_INTTYPES_H
	#include <inttypes.h>
	#endif
	#if HAVE_UNISTD_H
	#include <unistd.h>
	#endif
	]], [[intptr_t  abc;
	]])],[AC_MSG_RESULT(yes)
	AC_DEFINE_UNQUOTED(HAVE_INTPTR_T,1,Define if intptr_t is declared)
	],[AC_MSG_RESULT(no)
	])

# -----------------------------------

AC_MSG_CHECKING(if assert needs __eprintf)
cat > conftest.$ac_ext <<EOF
#include "confdefs.h"
#include "assert.h"
int main(int argc, char **argv) {
assert(argc);
; return 0; }
EOF
${CC-cc} -c $CFLAGS $CPPFLAGS conftest.$ac_ext 1>&5
if test "x`nm conftest.o |$EGREP __eprintf`" != "x"; then
    AC_MSG_RESULT(yes)
    CFLAGS="$CFLAGS -DNDEBUG"
else
    AC_MSG_RESULT(no)
fi
rm -f conftest*

# -----------------------------------------------------------------------
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([int])
# -----------------------------------------------------------------------

# Generate Makefiles
AC_CONFIG_FILES(Makefile
	debian/changelog
	doc/Makefile
	doc/info/Makefile
	doc/schema/Makefile
	contrib/Makefile
	contrib/prune/Makefile
	contrib/diffimg/Makefile
	graphs/Makefile
	graphs/directed/Makefile
	graphs/undirected/Makefile
	lib/Makefile
	lib/cdt/Makefile
	lib/cdt/libcdt.pc
	lib/graph/Makefile
	lib/graph/libgraph.pc
	lib/cgraph/Makefile
	lib/cgraph/libcgraph.pc
	lib/rbtree/Makefile
	lib/pathplan/Makefile
	lib/pathplan/libpathplan.pc
	lib/ast/Makefile
	lib/sfio/Makefile
	lib/sfio/Sfio_f/Makefile
	lib/vmalloc/Makefile
	lib/dotgen/Makefile
	lib/dotgen2/Makefile
	lib/neatogen/Makefile
	lib/fdpgen/Makefile
	lib/sparse/Makefile
	lib/mingle/Makefile
	lib/label/Makefile
	lib/sfdpgen/Makefile
	lib/osage/Makefile
	lib/gvpr/Makefile
	lib/gvpr/libgvpr.pc
	lib/circogen/Makefile
	lib/twopigen/Makefile
	lib/patchwork/Makefile
	lib/pack/Makefile
	lib/ortho/Makefile
	lib/expr/Makefile
	lib/expr/libexpr.pc
	lib/common/Makefile
	lib/ingraphs/Makefile
	lib/vpsc/Makefile
	lib/gvc/Makefile
	lib/gvc/libgvc.pc
	lib/xdot/Makefile
	lib/xdot/libxdot.pc
	lib/topfish/Makefile
	lib/glcomp/Makefile
	macosx/Info.plist
	macosx/build/graphviz.pmdoc/01local.xml
	macosx/build/graphviz.pmdoc/02graphviz.xml
	windows/build/graphviz.wxs
	windows/Properties/AssemblyInfo.cs
	plugin/Makefile
	plugin/core/Makefile
	plugin/devil/Makefile
	plugin/gd/Makefile
	plugin/gdk_pixbuf/Makefile
	plugin/gdiplus/Makefile
	plugin/glitz/Makefile
	plugin/gs/Makefile
	plugin/gtk/Makefile
	plugin/lasi/Makefile
	plugin/ming/Makefile
	plugin/pango/Makefile
	plugin/poppler/Makefile
	plugin/quartz/Makefile
	plugin/rsvg/Makefile
	plugin/visio/Makefile
	plugin/webp/Makefile
	plugin/xlib/Makefile
	plugin/dot_layout/Makefile
	plugin/neato_layout/Makefile
	cmd/Makefile
	cmd/dot/Makefile
	cmd/tools/Makefile
	cmd/gvpr/Makefile
	cmd/gvpr/lib/Makefile
	cmd/lefty/ws/x11/libfilereq/Makefile
	cmd/lefty/ws/x11/Makefile
	cmd/lefty/ws/mswin32/Makefile
	cmd/lefty/ws/Makefile
	cmd/lefty/os/unix/Makefile
	cmd/lefty/os/mswin32/Makefile
	cmd/lefty/os/Makefile
	cmd/lefty/dot2l/Makefile
	cmd/lefty/examples/Makefile
	cmd/lefty/Makefile
	cmd/lneato/Makefile
	cmd/dotty/Makefile
	cmd/smyrna/Makefile
	cmd/gvmap/Makefile
	cmd/mingle/Makefile
	cmd/gvedit/Makefile
	cmd/gvedit/gvedit.pro
	cmd/gvedit/ui/Makefile
	cmd/gvedit/images/Makefile
	tclpkg/Makefile
	tclpkg/tclstubs/Makefile
	tclpkg/tkstubs/Makefile
	tclpkg/tclhandle/Makefile
	tclpkg/gdtclft/Makefile
	tclpkg/gdtclft/demo/Makefile
	tclpkg/tcldot/Makefile
	tclpkg/tcldot/demo/Makefile
	tclpkg/tclpathplan/Makefile
	tclpkg/tclpathplan/demo/Makefile
	tclpkg/tclpathplan/demo/pathplan_data/Makefile
	tclpkg/tkspline/Makefile
	tclpkg/tkspline/demo/Makefile
	tclpkg/gv/Makefile
	tclpkg/gv/demo/Makefile
	tclpkg/gv/META.gv
	rtest/Makefile
	share/Makefile
	share/examples/Makefile
	share/gui/Makefile
	graphviz.spec
	Doxyfile
	)

AC_OUTPUT

dnl ===========================================================================

if test "x$use_tcl" != "xYes"; then
  use_tcldot="No (tcl not available)"
  use_gdtclft="No (tcl not available)"
  use_tclpathplan="No (tcl not available)"
  use_gv_tcl="No (tcl not available)"
else
  use_tcldot="Yes"
  use_tclpathplan="Yes"
  if test "x$use_swig" != "xYes"; then
    use_gv_tcl="No (swig not available)"
  else
    use_gv_tcl="Yes"
  fi
  if test "x$use_gd" != "xYes"; then
    if test "x$use_gd" = "xYes (internal)"; then
        use_gdtclft="Yes"
    else
        use_gdtclft="No (gd not available)"
    fi
  else
    use_gdtclft="Yes"
  fi
fi

if test "x$use_tk" != "xYes"; then
  use_tkspline="No (tk not available)"
else
  use_tkspline="Yes"
fi

echo ""
echo "----------------------------------------------------------------"
echo ""
echo "graphviz-$VERSION will be compiled with the following:"
echo ""
echo "options:"
echo "  graph:         $use_graph"
echo "  cgraph:        $use_cgraph"
echo "  digcola:       $use_digcola"
echo "  expat:         $use_expat"
echo "  fontconfig:    $use_fontconfig"
echo "  freetype:      $use_freetype"
echo "  glut:          $use_glut"
echo "  ann:           $use_ann"
echo "  gts:           $use_gts"
echo "  ipsepcola:     $use_ipsepcola"
echo "  ltdl:          $use_ltdl"
echo "  ortho:         $use_ortho"
echo "  sfdp:          $use_sfdp"
echo "  shared:        $use_shared"
echo "  static:        $use_static"
echo "  qt:            $use_qt"
echo "  x:             $use_xlib"
echo ""
echo "commands:"
echo "  dot:           Yes (always enabled)"
echo "  neato:         Yes (always enabled)"
echo "  fdp:           Yes (always enabled)"
echo "  circo:         Yes (always enabled)"
echo "  twopi:         Yes (always enabled)"
echo "  gvpr:          Yes (always enabled)"
echo "  gvmap:         Yes (always enabled)"
echo "  lefty:         $use_lefty"
echo "  smyrna:        $use_smyrna"
echo "  gvedit:        $use_qt"
echo ""
echo "plugin libraries:"
echo "  dot_layout:    Yes (always enabled)"
echo "  neato_layout:  Yes (always enabled)"
echo "  core:          Yes (always enabled)"
echo "  devil:         $use_devil"
echo "  gd:            $use_gd"
echo "  gdiplus:       $use_gdiplus"
echo "  gdk_pixbuf:    $use_gdk_pixbuf"
echo "  ghostscript:   $use_ghostscript"
echo "  glitz:         $use_glitz"
echo "  gtk:           $use_gtk"
echo "  lasi:          $use_lasi"
echo "  ming:          $use_ming"
echo "  pangocairo:    $use_pangocairo"
echo "  poppler:       $use_poppler"
echo "  quartz:        $use_quartz"
echo "  rsvg:          $use_rsvg"
echo "  visio:         $use_visio"
echo "  webp:          $use_webp"
echo "  xlib:          $use_xlib"
echo ""
echo "language extensions:"
echo "  gv_sharp:      $use_sharp"
echo "  gv_go:         $use_go"
echo "  gv_guile:      $use_guile"
echo "  gv_io:         $use_io"
echo "  gv_java:       $use_java"
echo "  gv_lua:        $use_lua"
echo "  gv_ocaml:      $use_ocaml"
echo "  gv_perl:       $use_perl"
echo "  gv_php:        $use_php"
echo "  gv_python:     $use_python"
echo "  gv_python23:   $use_python23"
echo "  gv_python24:   $use_python24"
echo "  gv_python25:   $use_python25"
echo "  gv_python26:   $use_python26"
echo "  gv_python27:   $use_python27"
echo "  gv_R:          $use_r"
echo "  gv_ruby:       $use_ruby"
echo "  gv_tcl:        $use_gv_tcl"
echo ""
echo "  tcldot:        $use_tcldot"
echo "  tclpathplan:   $use_tclpathplan"
echo "  gdtclft:       $use_gdtclft"
echo "  tkspline:      $use_tkspline"

