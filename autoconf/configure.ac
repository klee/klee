dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([[KLEE]],[[1.3.0]],[[klee-dev@imperial.ac.uk]],[[klee-]],[[https://klee.github.io]])

dnl Identify where LLVM source tree is (this is patched by
dnl AutoRegen.sh)
LLVM_SRC_ROOT=XXX

dnl Tell autoconf that the auxilliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR($LLVM_SRC_ROOT/autoconf)

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT("","")

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["Makefile.config.in"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.config)

dnl Configure Doxygen file
AC_CONFIG_FILES([docs/doxygen.cfg])

dnl Configure project makefiles
dnl List every Makefile that exists within your source tree
AC_CONFIG_HEADERS([include/klee/Config/config.h])

AH_TOP([#ifndef KLEE_CONFIG_CONFIG_H
#define KLEE_CONFIG_CONFIG_H])
AH_BOTTOM([#endif])

dnl We need to check for the compiler up here to avoid anything else
dnl starting with a different one.
AC_PROG_CC(gcc clang)
AC_PROG_CXX(g++ clang++ )
AC_LANG([C++])
AC_SUBST(KLEE_HOST_C_COMPILER,$CC)
AC_SUBST(KLEE_HOST_CXX_COMPILER,$CXX)

dnl **************************************************************************
dnl Find the host

AC_CANONICAL_TARGET

dnl Determine the platform type and cache its value. This helps us configure
dnl the System library to the correct build platform.
AC_CACHE_CHECK([type of operating system we're going to host on],
               [klee_cv_os_type],
[case $host in
  *-*-linux*)
    host_supports_posix_runtime=yes ;;
  *)
    host_supports_posix_runtime=no ;;
esac])

dnl **************************************************************************
dnl Verify that we can find llvm

dnl --with-llvm is a shortcut for setting srcdir and objdir.
AC_ARG_WITH(llvm,
  AS_HELP_STRING([--with-llvm],
    [Location of LLVM Source and Object code]),,)

AC_MSG_CHECKING([llvm source dir])

if test X${with_llvm} != X; then
    dnl Verify that --with-llvm{src,obj} were not given.
    if test X${with_llvmsrc} != X; then
       AC_MSG_ERROR([--with-llvmsrc cannot be specified when using --with-llvm])
    fi   
    if test X${with_llvmobj} != X; then
       AC_MSG_ERROR([--with-llvmobj cannot be specified when using --with-llvm])
    fi   
    with_llvmsrc=$with_llvm
    with_llvmobj=$with_llvm
fi

dnl If one of with_llvmsrc or with_llvmobj was given, we must have both.
if (test X${with_llvmsrc} != X || test X${with_llvmobj} != X); then
    dnl Verify that with_llvmobj was given as well.
    if test X${with_llvmsrc} = X; then
       AC_MSG_ERROR([--with-llvmsrc must be specified when using --with-llvmobj])
    fi      
    if test X${with_llvmobj} = X; then
       AC_MSG_ERROR([--with-llvmobj must be specified when using --with-llvmsrc])
    fi      
else
    dnl Otherwise try and use llvm-config to find.
    llvm_version=`llvm-config --version`
    if test X${llvm_version} = X; then
       AC_MSG_ERROR([unable to find llvm, use --with-llvmsrc and --with-llvmobj])
    fi
    
    with_llvmsrc=`llvm-config --src-root`
    with_llvmobj=`llvm-config --obj-root`
fi

dnl Try to validate directories.
if test ! -f ${with_llvmsrc}/Makefile.rules; then
   AC_MSG_ERROR([invalid llvmsrc directory: ${with_llvmsrc}])
fi
if test ! -f ${with_llvmobj}/Makefile.config; then
   AC_MSG_ERROR([invalid llvmobj directory: ${with_llvmobj}])
fi

dnl Make the paths absolute.
llvm_src=`cd $with_llvmsrc 2> /dev/null; pwd`
llvm_obj=`cd $with_llvmobj 2> /dev/null; pwd`
   
AC_MSG_RESULT([$llvm_src])

dnl Report obj dir as well.
AC_MSG_CHECKING([llvm obj dir])
AC_MSG_RESULT([$llvm_obj])

AC_SUBST(LLVM_SRC,$llvm_src)
AC_SUBST(LLVM_OBJ,$llvm_obj)

dnl Determine LLVM version.
AC_MSG_CHECKING([llvm package version])
llvm_package_version=`grep PACKAGE_VERSION= $with_llvmsrc/configure | cut -d\' -f 2`
AC_MSG_RESULT([$llvm_package_version])

llvm_version_split=`python -c "import re; print('\t'.join(map(str, re.match('([[0-9]]+)[.]([[0-9]]+)(svn)?', \"$llvm_package_version\").groups())))"`

AC_MSG_CHECKING([llvm version major])
llvm_version_major=`echo "$llvm_version_split" | cut -f 1`
AC_MSG_RESULT([$llvm_version_major])

AC_MSG_CHECKING([llvm version minor])
llvm_version_minor=`echo "$llvm_version_split" | cut -f 2`
AC_MSG_RESULT([$llvm_version_minor])

AC_MSG_CHECKING([llvm is release version])
llvm_version_svn=`echo "$llvm_version_split" | cut -f 3`
if test "$llvm_version_svn" == "svn"; then
  llvm_is_release=0
else
  llvm_is_release=1
fi
AC_MSG_RESULT([$llvm_is_release])

AC_DEFINE_UNQUOTED(LLVM_VERSION_MAJOR, $llvm_version_major, [LLVM major version number])
AC_SUBST(LLVM_VERSION_MAJOR,$llvm_version_major)
AC_DEFINE_UNQUOTED(LLVM_VERSION_MINOR, $llvm_version_minor, [LLVM minor version number])
AC_SUBST(LLVM_VERSION_MINOR,$llvm_version_minor)
AC_DEFINE_UNQUOTED(LLVM_IS_RELEASE, $llvm_is_release, [LLVM version is release (instead of development)])
AC_SUBST(LLVM_IS_RELEASE,$llvm_is_release)

dnl LLVM <= 2.6 requires RTTI.
if test $llvm_version_major -eq 2 -a $llvm_version_minor -le 6 ; then
  requires_rtti=1
else
  requires_rtti=0
fi
AC_SUBST(REQUIRES_RTTI,$requires_rtti)

dnl Provide option to use C++11
AC_ARG_ENABLE([cxx11],[  --enable-cxx11          Build using C++11], [klee_use_cxx11=1], [klee_use_cxx11=0])

dnl LLVM >= 3.5 requires C++11
AC_MSG_CHECKING([if LLVM needs C++11])
if test '(' $llvm_version_major -eq 3 -a $llvm_version_minor -ge 5 ')' -o '(' $llvm_version_major -gt 3 ')' ; then
  klee_use_cxx11=1
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

dnl Check if the compiler supports C++11 (this check is taken from LLVM's configure.ac).
if test X${klee_use_cxx11} = X1; then
  klee_old_cxxflags="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -std=c++11"
  AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <atomic>
std::atomic<float> x(0.0f);
int main() { return (float)x; }
]])],
            [AC_MSG_RESULT([yes])],
            [AC_MSG_RESULT([no])
             AC_MSG_ERROR([C++11 not supported])])
  CXXFLAGS="$klee_old_cxxflags"
fi

AC_SUBST(KLEE_USE_CXX11,$klee_use_cxx11)


AC_ARG_WITH(llvm-build-mode,
  AS_HELP_STRING([--with-llvm-build-mode],
    [LLVM build mode (e.g. Debug or Release, default autodetect)]),,[with_llvm_build_mode=check])

AC_MSG_CHECKING([llvm build mode])

if test X${with_llvm_build_mode} = Xcheck ; then
  llvm_configs="`ls -1 $llvm_obj/*/bin/llvm-config 2>/dev/null | head -n 1`"
  if test -x "$llvm_configs" ; then
    llvm_build_mode="`$llvm_configs --build-mode`"
  else
    AC_MSG_ERROR([Could not autodetect build mode])
  fi
else
  llvm_configs="`echo $llvm_obj/*/bin/llvm-config`"
  if test -x "$llvm_obj/$with_llvm_build_mode/bin/llvm-config" ; then
    llvm_build_mode=$with_llvm_build_mode
  else
    AC_MSG_ERROR([Invalid build mode: $llvm_build_mode])
  fi
fi

AC_MSG_RESULT([$llvm_build_mode])
AC_SUBST(LLVM_BUILD_MODE,$llvm_build_mode)

dnl Check if we are building against a +Asserts LLVM, and enable the DEBUG_*
dnl macros if so (they use symbols only available in +Asserts builds).
AC_MSG_CHECKING([llvm has asserts enabled])
if test `expr "$llvm_build_mode" : ".*Asserts.*"` -ne 0; then
   llvm_has_asserts_enabled=1
   AC_DEFINE_UNQUOTED(ENABLE_KLEE_DEBUG, 1, [Enable KLEE DEBUG checks])
else
   llvm_has_asserts_enabled=0
fi
AC_MSG_RESULT([$llvm_has_asserts_enabled])

dnl **************************************************************************
dnl Detect a LLVM Bitcode compiler for building KLEE runtime library

dnl Check for clang built with llvm build
AC_MSG_CHECKING([LLVM Bitcode compiler])
klee_llvm_bc_c_compiler=""
klee_llvm_bc_cxx_compiler=""

AC_ARG_WITH([llvmcc],
            AS_HELP_STRING([--with-llvmcc],
                           [Set the path to the C LLVM bitcode compiler to use (Default: auto-detect). If set, --with-llvmcxx= must be set too.]
                          ),
            [],
            [with_llvmcc=none]
           )

AC_ARG_WITH([llvmcxx],
            AS_HELP_STRING([--with-llvmcxx],
                           [Set the path to the C++ LLVM bitcode compiler to use (Default: auto-detect). If set, --with-llvmcc= must be set too.]
                          ),
            [],
            [with_llvmcxx=none]
           )
if test \( "X$with_llvmcc" != Xnone -a "X$with_llvmcxx" = Xnone \) -o \( "X$with_llvmcxx" != Xnone -a "X$with_llvmcc" = Xnone \) ; then
    AC_MSG_ERROR([You must set both --with-llvmcc= and --with-llvmcxx= or set neither])
fi

if test X$with_llvmcc = Xnone ; then
    dnl Try to automatically find compiler
    
    dnl Try Clang inside the LLVM build
    if test -x "$llvm_obj/$llvm_build_mode/bin/clang" ; then
        AC_MSG_RESULT([Found clang in LLVM Build])
        klee_llvm_bc_c_compiler="$llvm_obj/$llvm_build_mode/bin/clang"

        if test -x "$llvm_obj/$llvm_build_mode/bin/clang++" ; then
            klee_llvm_bc_cxx_compiler="$llvm_obj/$llvm_build_mode/bin/clang++"
        else
            AC_MSG_ERROR([Found clang but could not find clang++])
        fi
    fi

    dnl Try llvm-gcc in PATH
    if test "X${klee_llvm_bc_c_compiler}" = X ; then
        AC_MSG_RESULT([]) # Force a new line
        AC_CHECK_PROG(llvm_gcc,llvm-gcc,FOUND,NOT_FOUND)
        if test ${llvm_gcc} = FOUND ; then
            klee_llvm_bc_c_compiler=`which llvm-gcc`

            AC_CHECK_PROG(llvm_gxx,llvm-g++,FOUND,NOT_FOUND)
            if test ${llvm_gxx} = FOUND; then
                klee_llvm_bc_cxx_compiler=`which llvm-g++`
            else
                AC_MSG_ERROR([Found llvm-gcc but could not find llvm-g++ in PATH])
            fi
        fi

    fi

    dnl Try clang in PATH
    if test "X${klee_llvm_bc_c_compiler}" = X ; then
        AC_MSG_RESULT([]) # Force a new line
        AC_CHECK_PROG(clang,clang,FOUND,NOT_FOUND)
        if test ${clang} = FOUND ; then
            klee_llvm_bc_c_compiler=`which clang`

            AC_CHECK_PROG(clang_cxx,clang++,FOUND,NOT_FOUND)
            if test ${clang_cxx} = FOUND; then
                klee_llvm_bc_cxx_compiler=`which clang++`
            else
                AC_MSG_ERROR([Found clang but could not find clang++ in PATH])
            fi
        fi

    fi

    if test X"${klee_llvm_bc_c_compiler}" = X ; then
        AC_MSG_ERROR([Could not find a C LLVM Bitcode compiler. Did you try building Clang in the LLVM Build directory or putting llvm-gcc or clang in your path?])
    fi

    if test X"${klee_llvm_bc_cxx_compiler}" = X ; then
        AC_MSG_ERROR([Could not find a C++ LLVM Bitcode compiler. Did you try building Clang in the LLVM Build directory or putting llvm-gcc or clang in your path?])
    fi

else
    dnl Use user supplied values
    klee_llvm_bc_c_compiler="$with_llvmcc"
    klee_llvm_bc_cxx_compiler="$with_llvmcxx"

    if test \! -x "${klee_llvm_bc_c_compiler}"; then
        AC_MSG_ERROR([--with-llvmcc= supplied compiler does not exist])
    fi

    if test \! -x "${klee_llvm_bc_cxx_compiler}"; then
        AC_MSG_ERROR([--with-llvmcxx= supplied compiler does not exist])
    fi
    AC_MSG_RESULT([Using user supplied LLVM bitcode compilers.])
fi

dnl Tell the user what we are going to try and use
AC_MSG_RESULT([Using C llvm compiler : $klee_llvm_bc_c_compiler])
AC_MSG_RESULT([Using C++ llvm compiler : $klee_llvm_bc_cxx_compiler])

dnl Test that the bitcode compiler works

dnl Function for checking bitcode compiler works
dnl $1 : compiler to invoke
dnl $2 : source code extension (e.g. cpp or c)
dnl $3 : Compiler string (e.g. CXX or C)
function klee_check_bc()
{
    AC_MSG_CHECKING([${3} LLVM Bitcode compiler works])
    dnl FIXME: write to tmp directory instead of binary build dir
    klee_bc_test_file="./.klee_llvm_bitcode_test.${2}"

    echo "int main() { return 0;}" > "${klee_bc_test_file}"
    "${1}" -emit-llvm -c "${klee_bc_test_file}" -o "${klee_bc_test_file}.bc"
    if test $? -ne 0 ; then
        AC_MSG_ERROR([Failed running ${3} LLVM Bitcode compiler])
    fi

    if test \! -e "${klee_bc_test_file}.bc"; then
        AC_MSG_ERROR([ ${3} LLVM Bitcode compiler did not produce any output])
    fi

    dnl Convert bitcode to human readable form as a hacky check
    dnl that the version of LLVM we are configuring with can
    dnl parse the LLVM bitcode produced by the detected compiler
    if test -x "$llvm_obj/$llvm_build_mode/bin/llvm-dis" ; then
        "$llvm_obj/$llvm_build_mode/bin/llvm-dis" -o "${klee_bc_test_file}.ll" "${klee_bc_test_file}.bc"

        if test $? -ne 0; then
            AC_MSG_ERROR([Failed converting LLVM Bitcode to LLVM assembly. Maybe your LLVM versions do not match?])
        fi

        if test -e "${klee_bc_test_file}.ll" ; then
            AC_MSG_RESULT([Success])
            rm "${klee_bc_test_file}" "${klee_bc_test_file}.bc" "${klee_bc_test_file}.ll"
        else
            rm "${klee_bc_test_file}" "${klee_bc_test_file}.bc" "${klee_bc_test_file}.ll"
            AC_MSG_ERROR([Failed converting LLVM Bitcode to LLVM assembly. Maybe your LLVM versions do not match?])
        fi
        
    else
        rm "${klee_bc_test_file}" "${klee_bc_test_file}.bc"
        AC_MSG_ERROR([Could not find llvm-dis])
    fi
}

dnl Invoke previously defined function to check the LLVM bitcode compilers
klee_check_bc "${klee_llvm_bc_c_compiler}" "c" "C"
klee_check_bc "${klee_llvm_bc_cxx_compiler}" "cpp" "CXX"

dnl Set variable for Makefile.config
AC_SUBST(KLEE_BITCODE_C_COMPILER,$klee_llvm_bc_c_compiler)
AC_SUBST(KLEE_BITCODE_CXX_COMPILER,$klee_llvm_bc_cxx_compiler)

dnl **************************************************************************
dnl User option to disable timestamping.

AC_ARG_ENABLE([timestamp],AS_HELP_STRING([--enable-timestamp],
	[Enable timestamping the source code while building. (default=disabled)]))

if test "x${enable_timestamp}" = "xyes" ; then
  AC_DEFINE(KLEE_ENABLE_TIMESTAMP,[1],[Enable time stamping the sources])
  AC_MSG_NOTICE([Source timestamping enabled.])
else
  AC_MSG_NOTICE([Source timestamping disabled.])
fi

dnl **************************************************************************
dnl User option to enable uClibc support.

AC_ARG_WITH(uclibc,
  AS_HELP_STRING([--with-uclibc],
                 [Enable use of the klee uclibc at the given path (klee-uclibc root directory or libc.a file]),,)

dnl Validate uclibc if given.

AC_MSG_CHECKING([uclibc])
if (test X${with_uclibc} != X); then
   if test  -d ${with_uclibc}; then
      dnl Support the legacy way of configuring with 
      dnl klee-uclibc, i.e. the root klee-uclibc
      dnl directory is passed as an argument.

      dnl Make the path absolute
      with_uclibc=`cd $with_uclibc 2> /dev/null; pwd`

      dnl create path to libc file
      KLEE_UCLIBC_BCA="${with_uclibc}/lib/libc.a"

      if test ! -e "${KLEE_UCLIBC_BCA}"; then
      AC_MSG_ERROR([Could not find file ${KLEE_UCLIBC_BCA}])
      fi
   elif test -f ${with_uclibc}; then
      dnl Support the new way of configuring klee-uclibc
      dnl i.e. the built bitcode archive is passed as the
      dnl argument.

      dnl Get absolute path to file
      _kud=`dirname ${with_uclibc}`
      _kud=`cd ${_kud}; pwd 2> /dev/null`
      _kuf=`basename ${with_uclibc}`

      KLEE_UCLIBC_BCA="${_kud}/${_kuf}" 
   else
      AC_MSG_ERROR([Could not detect klee-uclibc])
   fi

   dnl FIXME: Validate the libc.a file

   AC_MSG_RESULT([$KLEE_UCLIBC_BCA])
   AC_SUBST(ENABLE_UCLIBC,[[1]])
   AC_SUBST(KLEE_UCLIBC_BCA)
   AC_DEFINE(SUPPORT_KLEE_UCLIBC, [[1]], [klee-uclibc is supported])
else
   AC_MSG_RESULT([no])
   AC_SUBST(ENABLE_UCLIBC,[[0]])
fi


dnl **************************************************************************
dnl User option to enable the POSIX runtime

AC_ARG_ENABLE(posix-runtime,
              AS_HELP_STRING([--enable-posix-runtime],
                             [Enable the POSIX runtime]),
                             ,enableval=default)

AC_MSG_CHECKING([POSIX runtime])
if test ${enableval} = "default" ; then
  if test X${with_uclibc} != X; then
    enableval=$host_supports_posix_runtime
    if test ${enableval} = "yes"; then
      AC_MSG_RESULT([default (enabled)])
    else
      AC_MSG_RESULT([default (disabled, unsupported target)])
    fi
  else
    enableval="no"
    AC_MSG_RESULT([default (disabled, no uclibc)])
  fi
else
  if test ${enableval} = "yes" ; then
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
fi

if test ${enableval} = "yes" ; then
  AC_SUBST(ENABLE_POSIX_RUNTIME,[[1]])
else
  AC_SUBST(ENABLE_POSIX_RUNTIME,[[0]])
fi

dnl **************************************************************************
dnl User option to select runtime version

AC_ARG_WITH(runtime,
  AS_HELP_STRING([--with-runtime],
    [Select build configuration for runtime libraries (default [Release+Asserts])]),,
    withval=default)
if test X"${withval}" = Xdefault; then
   with_runtime=Release+Asserts
fi

AC_MSG_CHECKING([runtime configuration])
if test X${with_runtime} = XRelease; then
    AC_MSG_RESULT([Release])
    AC_SUBST(RUNTIME_ENABLE_OPTIMIZED,[[1]])
    AC_SUBST(RUNTIME_DISABLE_ASSERTIONS,[[1]])
    AC_SUBST(RUNTIME_DEBUG_SYMBOLS,[[]])
elif test X${with_runtime} = XRelease+Asserts; then
    AC_MSG_RESULT([Release+Asserts])
    AC_SUBST(RUNTIME_ENABLE_OPTIMIZED,[[1]])
    AC_SUBST(RUNTIME_DISABLE_ASSERTIONS,[[0]])
    AC_SUBST(RUNTIME_DEBUG_SYMBOLS,[[]])
elif test X${with_runtime} = XDebug; then
   AC_MSG_RESULT([Debug])
   AC_SUBST(RUNTIME_ENABLE_OPTIMIZED,[[0]])
   AC_SUBST(RUNTIME_DISABLE_ASSERTIONS,[[1]])
   AC_SUBST(RUNTIME_DEBUG_SYMBOLS,[[1]])
elif test X${with_runtime} = XDebug+Asserts; then
   AC_MSG_RESULT([Debug+Asserts])
   AC_SUBST(RUNTIME_ENABLE_OPTIMIZED,[[0]])
   AC_SUBST(RUNTIME_DISABLE_ASSERTIONS,[[0]])
   AC_SUBST(RUNTIME_DEBUG_SYMBOLS,[[1]])
else
   AC_MSG_ERROR([invalid configuration: ${with_runtime}])
fi

AC_DEFINE_UNQUOTED(RUNTIME_CONFIGURATION, "$with_runtime", [Configuration for runtime libraries])
AC_SUBST(RUNTIME_CONFIGURATION)

dnl **************************************************************************
dnl See if we should support __ctype_b_loc externals.

dnl FIXME: Do the proper test if we continue to need this.
case $host in
  *-*-linux*)
    AC_DEFINE_UNQUOTED(HAVE_CTYPE_EXTERNALS, 1, [Does the platform use __ctype_b_loc, etc.])
esac

dnl **************************************************************************
dnl Checks for header files.

dnl NOTE: This is mostly just to force autoconf to make CFLAGS defines
dnl for us.
AC_LANG_PUSH([C])

AC_CHECK_HEADERS([sys/acl.h])

AC_CHECK_LIB([cap], [cap_get_proc], [have_cap=yes], [have_cap=no])
if test "x${have_cap}" = xyes; then
  AC_CHECK_HEADERS([sys/capability.h], [], [have_cap=no])
fi
if test "x${have_cap}" = xno; then
  AC_MSG_WARN([Library cap and its header file sys/capability.h not found, disabling chroot \
capability checking support for klee-replay.])
fi

AC_CHECK_HEADERS([selinux/selinux.h],
        AC_SUBST(HAVE_SELINUX, 1),
        AC_SUBST(HAVE_SELINUX, 0))

if test "$HAVE_SELINUX" = "1"; then
   # Test what function signature we need to use for SELinux. The signatures
   # have changed between 2.2 and 2.3. In particular, the type of the "security
   # context" parameter was changed from char * to const char *, with this
   # patch: [PATCH] Get rid of security_context_t and fix const declarations.
   # [http://www.spinics.net/lists/selinux/msg14827.html]
   AC_CACHE_CHECK([for selinux security context type constness],
                  [klee_cv_sel_ctx_const],
   [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM([[
#include <selinux/selinux.h>
int setcon(char *context);]])],
     [klee_cv_sel_ctx_const=''],
     [klee_cv_sel_ctx_const='const'])])
   AC_DEFINE_UNQUOTED([KLEE_SELINUX_CTX_CONST], [$klee_cv_sel_ctx_const],
     [Define to empty or 'const' depending on how SELinux qualifies its
      security context parameters.])
fi

AC_LANG_POP([C])

dnl **************************************************************************
dnl Test for features
dnl **************************************************************************

AC_CHECK_HEADERS([malloc/malloc.h])
AC_CHECK_FUNCS([malloc_zone_statistics])
AC_SEARCH_LIBS(mallinfo,malloc,
               AC_DEFINE([HAVE_MALLINFO],[1],[Define if mallinfo() is available on this platform.]))

dnl **************************************************************************
dnl Test for tcmalloc
dnl **************************************************************************

AC_ARG_WITH([tcmalloc],
	    AS_HELP_STRING([--without-tcmalloc], [Ignore presence of tcmalloc and disable it (default=detect)]))

AS_IF([test "x$with_tcmalloc" != "xno"],
      AC_CHECK_HEADERS([gperftools/malloc_extension.h],
		       [have_tcmalloc=yes], [have_tcmalloc=no]),
      [have_tcmalloc=no])

AS_IF([test "x$have_tcmalloc" = "xyes"],
      [
	AC_SEARCH_LIBS(tc_malloc,tcmalloc_minimal,
		     [
		      AC_SUBST(HAVE_TCMALLOC, 1)
		      if test "${ac_cv_search_tc_malloc}" != "none required"; then
			 TCMALLOC_LIB=${ac_cv_search_tc_malloc}
			 AC_SUBST(TCMALLOC_LIB)
		      fi
		     ],
		     [
		      AC_MSG_WARN([Could not link with tcmalloc])
		      AC_SUBST(HAVE_TCMALLOC, 0)
		     ],)

       ],
      [AS_IF([test "x$with_tcmalloc" = "xyes"],
	     [AC_MSG_ERROR([tcmalloc requested but not found])],
	     [
	      AC_SUBST(HAVE_TCMALLOC, 0)
	      ])
])

dnl **************************************************************************
dnl Test for zlib
dnl **************************************************************************

AC_ARG_WITH([zlib],
        AS_HELP_STRING([--without-zlib], [Ignore presence of zlib and disable it (default=detect)]))

AS_IF([test "x$with_zlib" != "xno"],
      AC_CHECK_HEADERS([zlib.h],
               [have_zlib=yes], [have_zlib=no]),
      [have_zlib=no])

AS_IF([test "x$have_zlib" = "xyes"],
      [
    AC_SEARCH_LIBS(deflateEnd, z,
             [
              AC_SUBST(HAVE_ZLIB, 1)
              if test "${ac_cv_search_zlib}" != "none required"; then
             ZLIB_LIB=${ac_cv_search_zlib}
             AC_SUBST(ZLIB_LIB)
              fi
             ],
             [
              AC_MSG_WARN([Could not link with zlib])
              AC_SUBST(HAVE_ZLIB, 0)
             ],)

       ],
      [AS_IF([test "x$with_zlib" = "xyes"],
         [AC_MSG_ERROR([zlib requested but not found])],
         [
          AC_SUBST(HAVE_ZLIB, 0)
          ])
])

dnl **************************************************************************
dnl Find an install of STP
dnl **************************************************************************

ENABLE_STP=0
AC_ARG_WITH(stp,
  AS_HELP_STRING([--with-stp],
    [Location of STP installation directory]),[
      #Check for empty argument
      if test "X$withval" = X ; then
        AC_MSG_ERROR([<path> cannot be empty in --with-stp=<path>])
      fi
      stp_root=`(cd $withval && pwd) 2> /dev/null`

      #Check for bad path
      if test "X$stp_root" = X ; then
        AC_MSG_ERROR([Cannot access path $with_stp passed to --with-stp])
      fi

      STP_CFLAGS="-I$stp_root/include"
      STP_LDFLAGS="-L$stp_root/lib"
      ENABLE_STP=1
    ])

if test "X$ENABLE_STP" != X0; then
    old_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $STP_CFLAGS"
    AC_CHECK_HEADER(stp/c_interface.h,,
        [
            AC_MSG_ERROR([Unable to use stp/c_interface.h header])
        ])

    dnl we use [:] as a no-op so that AC_CHECK_LIB doesn't append -lstp
    dnl automatically which would break subsequent uses of AC_CHECK_LIB()
    STP_NEEDS_MINISAT=0
    AC_CHECK_LIB(stp, vc_setInterfaceFlags, [:], [
           STP_NEEDS_MINISAT=1; AC_MSG_RESULT([Could not link with libstp])
    ], "$STP_LDFLAGS")
    CPPFLAGS="$old_CPPFLAGS"

    dnl Try linking again with minisat if necessary
    if test "X$STP_NEEDS_MINISAT" != X0 ; then
      # Need to clear cached result
      unset ac_cv_lib_stp_vc_setInterfaceFlags

      AC_CHECK_LIB(stp,
               vc_setInterfaceFlags,[:], [
               AC_MSG_ERROR([Unable to link with libstp. Check config.log to see what went wrong])
      ], "$STP_LDFLAGS" "-lminisat" )

      STP_LDFLAGS="${STP_LDFLAGS} -lstp -lminisat"
    else
      STP_LDFLAGS="${STP_LDFLAGS} -lstp"
    fi
    AC_MSG_NOTICE([Using STP solver backend])
    CPPFLAGS="$old_CPPFLAGS"
    AC_DEFINE(ENABLE_STP, [1], [Using STP Solver backend])
else
    AC_MSG_NOTICE([Not using STP solver backend])
fi

AC_SUBST(ENABLE_STP)
AC_SUBST(STP_CFLAGS)
AC_SUBST(STP_LDFLAGS)

dnl **************************************************************************
dnl Find an install of Z3
dnl **************************************************************************

ENABLE_Z3=0
AC_ARG_WITH(z3,
  AS_HELP_STRING([--with-z3], [Location of Z3 installation directory]), [
    # Check for empty argument
    if test "X$withval" = X ; then
      AC_MSG_ERROR([<path> cannot be empty in --with-z3=<path>])
    fi
    z3_root=`(cd $withval && pwd) 2> /dev/null`

    #Check for bad path
    if test "X$z3_root" = X ; then
      AC_MSG_ERROR([Cannot access path $with_z3 passed to --with-z3])
    fi

    Z3_CFLAGS="-I$z3_root/include"
    Z3_LDFLAGS="-L$z3_root/lib"
    ENABLE_Z3=1
  ])

if test "X$ENABLE_Z3" != X0 ; then
    old_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $Z3_CFLAGS"
    old_CPPFLAGS="$CPPFLAGS"
    AC_CHECK_HEADER(z3.h,,
        [
            AC_MSG_ERROR([Unable to use z3.h header])
        ])
    CPPFLAGS="$old_CPPFLAGS"

    AC_CHECK_LIB(z3, Z3_mk_context,[:], [
           AC_MSG_ERROR([Unable to link against z3])
    ], "$Z3_LDFLAGS")

    # Test which function signature of ``Z3_get_error_msg()`` we need to use.
    # There's an API break between Z3 4.4.1 and the master branch
    AC_MSG_CHECKING([if Z3_get_error_msg() requires Z3_context])
    AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[#include "z3.h"]],
                       [[Z3_context c = Z3_mk_context(0); Z3_get_error_msg(c, Z3_OK);]])],
      [Z3_HAS_ERROR_MSG_CONTEXT=1],
      [Z3_HAS_ERROR_MSG_CONTEXT=0])

    if test "X$Z3_HAS_ERROR_MSG_CONTEXT" == X1; then
      AC_DEFINE(HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT, [1], [Z3 needs a Z3_context passed to Z3_get_error_msg()])
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
    fi

    Z3_LDFLAGS="${Z3_LDFLAGS} -lz3"
    AC_MSG_NOTICE([Using Z3 solver backend])
    CPPFLAGS="$old_CPPFLAGS"
    AC_DEFINE(ENABLE_Z3, [1], [Using Z3 Solver backend])
else
    AC_MSG_NOTICE([Not using Z3 solver backend])
fi

AC_SUBST(ENABLE_Z3)
AC_SUBST(Z3_CFLAGS)
AC_SUBST(Z3_LDFLAGS)


dnl **************************************************************************
dnl User option to enable metaSMT constraint solvers and to specify the
dnl the location of the metaSMT root directory
dnl **************************************************************************

AC_ARG_WITH(metasmt,
  AS_HELP_STRING([--with-metasmt],
    [Location of metaSMT installation directory]),,)
if test X$with_metasmt = X ; then
  ENABLE_METASMT=0
  AC_MSG_NOTICE([Not using MetaSMT solver backend])
else
  metasmt_root=`(cd $with_metasmt && pwd) 2> /dev/null`
  #Check for bad path
  if test "X$metasmt_root" = X ; then
    AC_MSG_ERROR([Cannot access path $with_metasmt passed to --with-metasmt])
  fi

  dnl AC_LANG(C++)
  old_CPPFLAGS="$CPPFLAGS"
  old_LDFLAGS="$LDFLAGS"
  old_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS -I$metasmt_root/include"
  LDFLAGS="$LDFLAGS -L$metasmt_root/lib"
  LIBS="$LIBS -lmetaSMT"

  AC_CHECK_HEADERS([$metasmt_root/include/metaSMT/DirectSolver_Context.hpp $metasmt_root/include/metaSMT/frontend/QF_BV.hpp],, [
         AC_MSG_ERROR([Unable to use $metasmt_root/include/metaSMT/DirectSolver_Context.hpp header])
  ])
  AC_MSG_CHECKING([for new_bitvector() in -lmetaSMT])
  AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([#include <metaSMT/frontend/QF_BV.hpp>], [metaSMT::logic::QF_BV::new_bitvector(3)])],
            [AC_MSG_RESULT([yes])],
            [AC_MSG_RESULT([no])
             AC_MSG_ERROR([Unable to link with libmetaSMT])])
  CPPFLAGS="$old_CPPFLAGS"
  LDFLAGS="$old_LDFLAGS"
  LIBS="$old_LIBS"

  AC_DEFINE(ENABLE_METASMT, 1, [Enable metaSMT API])
  AC_SUBST(METASMT_ROOT,$metasmt_root)
  ENABLE_METASMT=1
  AC_MSG_NOTICE([Using MetaSMT solver backend])
fi

AC_SUBST(ENABLE_METASMT)


dnl **************************************************************************
dnl User option to specify the default backend of metaSMT
dnl **************************************************************************

AC_ARG_WITH(metasmt-default-backend,
  AS_HELP_STRING([--with-metasmt-default-backend],
    [Default backend of metaSMT (btor|stp|z3, stp if unspecified)]),,)

if test "X$with_metasmt_default_backend" != X ; then
  if test "X$ENABLE_METASMT" != X1 ; then
    AC_MSG_ERROR([--with-metasmt-default-backend requires metaSMT to be enabled])
  fi
  if test "$with_metasmt_default_backend" == "btor" ; then
    METASMT_DEFAULT_BACKEND=BTOR
    AC_MSG_NOTICE([metaSMT uses Boolector as default backend])
  fi
  if test "$with_metasmt_default_backend" == "z3" ; then
    METASMT_DEFAULT_BACKEND=Z3
    AC_MSG_NOTICE([metaSMT uses Z3 as default backend])
  fi
  if test "$with_metasmt_default_backend" == "stp" ; then
    METASMT_DEFAULT_BACKEND=STP
    AC_MSG_NOTICE([metaSMT uses STP as default backend])
  fi
  if test "X$METASMT_DEFAULT_BACKEND" == X ; then
    METASMT_DEFAULT_BACKEND=STP
    AC_MSG_NOTICE([$with_metasmt_default_backend unsupported, metaSMT uses STP as default backend])
  fi
else
  if test "X$ENABLE_METASMT" == X1 ; then
    METASMT_DEFAULT_BACKEND=STP
    AC_MSG_NOTICE([No solver specified, metaSMT uses STP as default backend])
  fi  
fi

AC_SUBST(METASMT_DEFAULT_BACKEND)


dnl **************************************************************************
dnl Check at least one solver backend is enabled
dnl **************************************************************************

if test "X$ENABLE_STP$ENABLE_Z3$ENABLE_METASMT" == X000 ; then
    AC_MSG_ERROR([At least one solver backend must be enabled, try using --with-stp, --with-z3 or --with-metasmt])
fi


dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************

dnl Do special configuration of Makefiles
AC_CONFIG_MAKEFILE(Makefile)
AC_CONFIG_MAKEFILE(Makefile.common)
AC_CONFIG_MAKEFILE(lib/Makefile)
AC_CONFIG_MAKEFILE(runtime/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile)
AC_CONFIG_MAKEFILE(test/Makefile.tests)
AC_CONFIG_MAKEFILE(test/Concrete/Makefile)
AC_CONFIG_MAKEFILE(tools/Makefile)
AC_CONFIG_MAKEFILE(unittests/Makefile)

dnl This must be last
AC_OUTPUT
