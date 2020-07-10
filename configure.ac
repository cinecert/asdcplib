#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Copyright (c) 2007-2016 John Hurst. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ([2.59])
# If you change this, you should update the libtool version number specified
# in src/Makefile.am. Read <http://www.gnu.org/software/libtool/manual.html#Versioning>
# for suggestions on doing so.

# The version number consists of three segments: major, API minor, and
# implementation minor. Whenever a change is made to AS_DCP.h, the API minor
# version will increment. Changes made to the internal implementation will
# result in the incrementing of the implementation minor version.
#
# For example, if asdcplib version 1.0.0 were modified to accomodate changes
# in file format, and if no changes were made to AS_DCP.h, the new version would be
# 1.0.1. If changes were also required in AS_DCP.h, the new version would be 1.1.1.
AC_INIT([asdcplib], [2.10.38], [asdcplib@cinecert.com])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/KM_error.h])
#AC_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE([1.9 foreign])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
case $host_os in
       openbsd*) openbsd_host=true ;;
       *)        openbsd_host=false ;;
esac
AM_CONDITIONAL([OPENBSD_HOST], [test x$openbsd_host = xtrue])
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

AC_ARG_ENABLE([freedist],
     [  --enable-freedist    ensure source distribution is of BSD-licensed code],
     [case "${enableval}" in
       yes) freedist=true
       AC_MSG_WARN([BSD licensed ASDCPlib creation ENABLED.]) ;;
       no)  freedist=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-freedist]) ;;
     esac],[freedist=false])
     AM_CONDITIONAL([FREEDIST], [test x$freedist = xtrue])
AC_ARG_ENABLE([dev-headers],
     [  --enable-dev-headers    install development headers],
     [case "${enableval}" in
       yes) dev_headers=true ;;
       no)  dev_headers=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-dev-headers]) ;;
     esac],[dev_headers=false])
     AM_CONDITIONAL([DEV_HEADERS], [test x$dev_headers = xtrue])
AC_ARG_ENABLE([random-case-UUID],
     [  --enable-random-case-UUID    encode randomly-cased UUID values when the environment variable KM_USE_RANDOM_UUID is set],
     [case "${enableval}" in
       yes) random_case_UUID=true ;;
       no)  random_case_UUID=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-random-case-UUID]) ;;
     esac],[random_case_UUID=false])
     AM_CONDITIONAL([ENABLE_RANDOM_UUID], [test x$random_case_UUID = xtrue])
AC_ARG_ENABLE([as-02],
     [  --enable-as-02    enable support for SMPTE ST 2067-5 MXF files, A.K.A. AS-02],
     [case "${enableval}" in
       yes) as_02_use=true ;;
       no)  as_02_use=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-as-02]) ;;
     esac],[as_02_use=false])
     AM_CONDITIONAL([USE_AS_02], [test x$as_02_use = xtrue])
AC_ARG_ENABLE([phdr],
     [  --enable-phdr    enable support for Prototype for High Dyamic Range in AS-02],
     [case "${enableval}" in
       yes) phdr_use=true ;;
       no)  phdr_use=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-phdr]) ;;
     esac],[phdr_use=false])
     AM_CONDITIONAL([USE_PHDR], [test x$phdr_use = xtrue])

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
                 src/Makefile
		 win32/Makefile
		 win32/Makefile.mak:win32/Makefile32.wmk])
AC_OUTPUT
