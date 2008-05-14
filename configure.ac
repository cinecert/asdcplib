#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
# If you change this, you should update the libtool version number specified
# in src/Makefile.am. Read <http://www.gnu.org/software/libtool/manual.html#Versioning>
# for suggestions on doing so.
AC_INIT([asdcplib], [1.3.19], [asdcplib@cinecert.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/KM_error.h])
#AC_CONFIG_HEADER([src/config.h])
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
if test "$ac_cv_sys_largefile_CC" != no; then
   CXX=$CXX$ac_cv_sys_largefile_CC
fi

AX_LIB_OPENSSL([0.9.7])
AX_LIB_EXPAT
AX_LIB_XERCES
#AM_PATH_PYTHON([2.3], , [:])
AZ_PYTHON_DEFAULT
AZ_PYTHON_WITH
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
#AC_HEADER_DIRENT
#AC_HEADER_STDC
#AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_C_CONST
#AC_C_INLINE
#AC_TYPE_OFF_T
#AC_TYPE_SIZE_T
#AC_STRUCT_TM

# Checks for library functions.
#AC_FUNC_CLOSEDIR_VOID
#AC_FUNC_MALLOC
#AC_FUNC_MEMCMP
#AC_FUNC_STAT
#AC_CHECK_FUNCS([getcwd memset regcomp strchr strerror strrchr strstr strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
