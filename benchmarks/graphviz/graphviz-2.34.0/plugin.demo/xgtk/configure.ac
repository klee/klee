#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([xgtk-plugin], [0.1], [ellson@research.att.com])
AC_CONFIG_SRCDIR([src/gvplugin_xgtk.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# Checks for libraries.

dnl -----------------------------------
dnl INCLUDES and LIBS for GVC.

AC_ARG_WITH(gvc,
  [AS_HELP_STRING([--with-gvc=yes],[gvc library])],
  [], [with_gvc=yes])

if test "x$with_gvc" != "xyes"; then
  use_gvc="No (disabled)"
else
  PKG_CHECK_MODULES(GVC, [libgvc],[
    use_gvc="Yes"
    AC_DEFINE_UNQUOTED(HAVE_GVC,1,
      [Define if you have the gvc library])
    AC_SUBST(GVC_CFLAGS)
    AC_SUBST(GVC_LIBS)
  ],[
    use_gvc="No (gvc library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GVC, [test "x$use_gvc" = "xYes"])

if test "x$use_gvc" = "xYes"; then
    GVPLUGIN_VERSION_INFO=`pkg-config libgvc --variable=plugins`:0:0
    AC_SUBST(GVPLUGIN_VERSION_INFO)
    GVPLUGIN_DIR=`pkg-config libgvc --variable=libdir`/graphviz
    AC_SUBST(GVPLUGIN_DIR)
fi

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
    AC_SUBST(GTK_CFLAGS)
    AC_SUBST(GTK_LIBS)
  ],[
    use_gtk="No (gtk library not available)"
  ])
fi
AM_CONDITIONAL(WITH_GTK, [test "x$use_gtk" = "xYes"])


# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([inttypes.h libintl.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile src/Makefile xgtk-plugin.spec])
AC_OUTPUT
