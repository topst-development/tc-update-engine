#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TcUpdateEngine], [1.0.0], [Jun@telechips.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CONFIG_SRCDIR([src/update-engine.c])
AC_CONFIG_MACRO_DIR([m4])

#AC_PREFIX_DEFAULT(/usr)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h stdint.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

AC_ARG_ENABLE([standalone-app],
		AS_HELP_STRING([--enable-standalone-app], [Enable standalone-app]))
AS_IF([test "x$enable_standalone_app" = "xyes"], ENABLE_STANDALONE_APP=1, ENABLE_STANDALONE_APP=0)
AC_SUBST(HAVE_STANDALONE_APP)
AM_CONDITIONAL([HAVE_STANDALONE_APP], [test "x$ENABLE_STANDALONE_APP" = "x1"])

AC_ARG_ENABLE([803x-update],
		AS_HELP_STRING([--enable-803x-update], [Enable 803x update]))
AS_IF([test "x$enable_803x_update" = "xyes"], ENABLE_803X_UPDATE=1, ENABLE_803X_UPDATE=0)
AC_SUBST(HAVE_803X_UPDATE)
AM_CONDITIONAL([HAVE_803X_UPDATE], [test "x$ENABLE_803X_UPDATE" = "x1"])

AC_CONFIG_FILES([Makefile
				makelibrary/Makefile
				makeprocessor/Makefile])
AC_OUTPUT
