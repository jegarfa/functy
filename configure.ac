#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Functy], [0.38], [david@flypig.co.uk])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/functy.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_LANG(C++)

# Check for C++ compiler, thanks to mono project for implementatioon
# See https://github.com/mono/mono/blob/6ead72475fe42b148c01f2d63ea0d539c6d8f28b/configure.ac#L389
# AC_PROG_CXX helpfully sets CXX to g++ even if no c++ compiler is found so check
# GXX instead. See http://lists.gnu.org/archive/html/bug-autoconf/2002-04/msg00056.html
if test "x$CXX" = "xg++"; then
	if test "x$GXX" != "xyes"; then
		AC_MSG_ERROR([You need to install g++])
	fi
fi

# Checks for libraries.
PKG_CHECK_MODULES([FUNCTY], [gtk+-2.0 gdk-2.0 gtkglext-1.0 glee [libzip >= 0.11], libpng, libpulse-mainloop-glib, fftw3, ncursesw])
PKG_CHECK_MODULES([SYMBOLIC], [libsymbolic-1])
PKG_CHECK_MODULES([OPENVDB], [OpenEXR, tbb])
AC_CHECK_LIB([glut], [glutInit], [], [AC_MSG_ERROR([The glut library is missing.])])
#AC_CHECK_LIB([symbolic], [CreateInteger], [], [AC_MSG_ERROR([The symbolic library is missing.])])

OPENVDB_LIBS="$OPENVDB_LIBS -lopenvdb"
OPENVDB_CFLAGS="$OPENVDB_CFLAGS"
# See http://nerdland.net/2009/07/detecting-c-libraries-with-autotools/
#AC_MSG_CHECKING([for OpenVDB])
#AC_LANG(C++)
#SAVED_LIBS=$LIBS
#LIBS="$LIBS -lopenvdb"
#AC_LINK_IFELSE(
#	[AC_LANG_PROGRAM([[#include <openvdb/openvdb.h>]], [openvdb::initialize();])],
#	[AC_MSG_RESULT([yes]); SVX2VDB_LIBS="$SVX2VDB_LIBS -lopenvdb"] [HAVE_OPENVDB=1],
#	[AC_MSG_ERROR([The openvdb library is missing.])])
#LIBS=$LIBS

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/timeb.h])

AC_CHECK_HEADERS([GLee.h], [], AC_MSG_ERROR([Glee is missing.]))
AC_CHECK_HEADERS([boost/cstdint.hpp], [], AC_MSG_ERROR([Boost is missing.]))
AC_CHECK_HEADERS([openvdb/openvdb.h], [], AC_MSG_ERROR([OpenVDB is missing.]))

# Checks for compiler characteristics
AC_C_BIGENDIAN

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_SEARCH_LIBS([GLeeEnabled], [glee GLee], [], [])
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor ftime pow sqrt])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 assets/Makefile
                 assets/icons/Makefile
                 assets/shaders/Makefile])
AC_OUTPUT
