#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([napali], [1.3.19.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/KM_error.h])
AC_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE([1.9 foreign])

# Checks for programs.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_SYS_LARGEFILE

CHECK_SSL
AX_LIB_EXPAT
#AM_PATH_PYTHON([2.3], , [:])
AZ_PYTHON_DEFAULT
AZ_PYTHON_WITH
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
