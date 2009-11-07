# ===========================================================================
#             ax_lib_openssl.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_OPENSSL([MINIMUM-VERSION])
#
# DESCRIPTION
#
#   This macro provides tests of availability of OpenSSL of a
#   particular version or newer. This macros checks for OpenSSL
#   headers and libraries and defines compilation flags.
#
#   Macro supports following options and their values:
#
#   1) Single-option usage:
#
#     --with-openssl - yes, no or path to OpenSSL installation prefix
#
#   This macro calls:
#
#     AC_SUBST(OPENSSL_CFLAGS)
#     AC_SUBST(OPENSSL_LDFLAGS)
#     AC_SUBST(OPENSSL_VERSION) - only if version requirement is used
#
#   And sets:
#
#     HAVE_OPENSSL
#
# Copyright (c) 2008-2009 CineCert, LLC
# All rights reserved.
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
#

AC_DEFUN([AX_LIB_OPENSSL],
[
    AC_ARG_WITH([openssl],
        AC_HELP_STRING([--with-openssl=@<:@ARG@:>@],
            [use OpenSSL from given prefix (ARG=path); check standard prefixes (ARG=yes); disable (ARG=no)]
        ),
        [
        if test "$withval" = "yes"; then
            if test -d /var/local/ssl/include ; then
                openssl_prefix=/var/local/ssl
            elif test -d /var/local/include/openssl ; then
                openssl_prefix=/var/local
            elif test -d /usr/local/ssl/include ; then
                openssl_prefix=/usr/local/ssl
            elif test -d /usr/lib/ssl/include ; then
                openssl_prefix=/usr/lib/ssl
            elif test -d /usr/include/openssl ; then
                openssl_prefix=/usr
            else
                openssl_prefix=""
            fi
            openssl_requested="yes"
        elif test -d "$withval"; then
            openssl_prefix="$withval"
            openssl_requested="yes"
        else
            openssl_prefix=""
            openssl_requested="no"
        fi
        ],
        [
        dnl Default behavior is implicit yes
        if test -d /var/local/ssl/include ; then
            openssl_prefix=/var/local/ssl
        elif test -d /var/local/include/openssl ; then
            openssl_prefix=/var/local
        elif test -d /usr/local/ssl/include ; then
            openssl_prefix=/usr/local/ssl
        elif test -d /usr/lib/ssl/include ; then
            openssl_prefix=/usr/lib/ssl
        elif test -d /usr/include/openssl ; then
            openssl_prefix=/usr
        else
            openssl_prefix=""
        fi
        ]
    )

    OPENSSL_CPPFLAGS=""
    OPENSSL_LDFLAGS=""
    OPENSSL_VERSION=""

    dnl
    dnl Collect include/lib paths and flags
    dnl
    run_openssl_test="no"

    if test -n "$openssl_prefix"; then
        openssl_include_dir="$openssl_prefix/include"
        openssl_ldflags="-L$openssl_prefix/lib64 -L$openssl_prefix/lib"
        run_openssl_test="yes"
    elif test "$openssl_requested" = "yes"; then
        if test -n "$openssl_include_dir" -a -n "$openssl_lib_flags"; then
            run_openssl_test="yes"
        fi
    else
        run_openssl_test="no"
    fi

    openssl_libs="-lssl -lcrypto"

    dnl
    dnl Check OpenSSL files
    dnl
    if test "$run_openssl_test" = "yes"; then

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -I$openssl_include_dir"

        saved_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $openssl_ldflags"

        saved_LIBS="$LIBS"
        LIBS="$openssl_libs $LIBS"

        dnl
        dnl Check OpenSSL headers
        dnl
        AC_MSG_CHECKING([for OpenSSL headers in $openssl_include_dir])

        AC_LANG_PUSH([C])
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM(
                [[
@%:@include <openssl/opensslv.h>
@%:@include <openssl/ssl.h>
@%:@include <openssl/crypto.h>
                ]],
                [[]]
            )],
            [
            OPENSSL_CPPFLAGS="-I$openssl_include_dir"
            openssl_header_found="yes"
            AC_MSG_RESULT([found])
            ],
            [
            openssl_header_found="no"
            AC_MSG_RESULT([not found])
            ]
        )
        AC_LANG_POP([C])

        dnl
        dnl Check OpenSSL libraries
        dnl
        if test "$openssl_header_found" = "yes"; then

            AC_MSG_CHECKING([for OpenSSL libraries])

            AC_LANG_PUSH([C])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM(
                    [[
@%:@include <openssl/opensslv.h>
@%:@include <openssl/ssl.h>
@%:@include <openssl/crypto.h>
#if (OPENSSL_VERSION_NUMBER < 0x0090700f)
#endif
                    ]],
                    [[
SSL_library_init();
SSLeay();
                    ]]
                )],
                [
                OPENSSL_LDFLAGS="$openssl_ldflags"
                OPENSSL_LIBS="$openssl_libs"
                openssl_lib_found="yes"
                AC_MSG_RESULT([found])
                ],
                [
                openssl_lib_found="no"
                AC_MSG_RESULT([not found])
                ]
            )
            AC_LANG_POP([C])
        fi

        CPPFLAGS="$saved_CPPFLAGS"
        LDFLAGS="$saved_LDFLAGS"
        LIBS="$saved_LIBS"
    fi

    AC_MSG_CHECKING([for OpenSSL])

    if test "$run_openssl_test" = "yes"; then
        if test "$openssl_header_found" = "yes" -a "$openssl_lib_found" = "yes"; then

            AC_SUBST([OPENSSL_CPPFLAGS])
            AC_SUBST([OPENSSL_LDFLAGS])
            AC_SUBST([OPENSSL_LIBS])

            HAVE_OPENSSL="yes"
        else
            HAVE_OPENSSL="no"
        fi

        AC_MSG_RESULT([$HAVE_OPENSSL])

        dnl
        dnl Check OpenSSL version
        dnl
        if test "$HAVE_OPENSSL" = "yes"; then

            openssl_version_req=ifelse([$1], [], [], [$1])

            if test  -n "$openssl_version_req"; then

                AC_MSG_CHECKING([if OpenSSL version is >= $openssl_version_req])

                if test -f "$openssl_include_dir/openssl/opensslv.h"; then

                    OPENSSL_VERSION=`grep OPENSSL_VERSION_TEXT $openssl_include_dir/openssl/opensslv.h \
                                    | grep -v fips | grep -v PTEXT | cut -f 2 | tr -d \"`
                    AC_SUBST([OPENSSL_VERSION])

                    dnl Decompose required version string and calculate numerical representation
                    openssl_version_req_major=`expr $openssl_version_req : '\([[0-9]]*\)'`
                    openssl_version_req_minor=`expr $openssl_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
                    openssl_version_req_revision=`expr $openssl_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
                    openssl_version_req_patch=`expr $openssl_version_req : '[[0-9]]*\.[[0-9]]*\.[[0-9]]*\([[a-z]]*\)'`
                    if test "x$openssl_version_req_revision" = "x"; then
                        openssl_version_req_revision="0"
                    fi
                    if test "x$openssl_version_req_patch" = "x"; then
                        openssl_version_req_patch="\`"
                    fi

                    openssl_version_req_number=`expr $openssl_version_req_major \* $((0x10000000)) \
                                               \+ $openssl_version_req_minor \* $((0x100000)) \
                                               \+ $openssl_version_req_revision \* $((0x1000)) \
                                               \+ $((1 + $(printf "%d" \'$openssl_version_req_patch) - $(printf "%d" \'a))) \* $((0x10)) \
                                               \+ $((0xf))`

                    dnl Calculate numerical representation of detected version
                    openssl_version_number=`expr $(($(grep OPENSSL_VERSION_NUMBER $openssl_include_dir/openssl/opensslv.h | cut -f 2 | tr -d L)))`

                    openssl_version_check=`expr $openssl_version_number \>\= $openssl_version_req_number`
                    if test "$openssl_version_check" = "1"; then
                        AC_MSG_RESULT([yes])
                    else
                        AC_MSG_RESULT([no])
                        AC_MSG_WARN([Found $OPENSSL_VERSION, which is older than required. Possible compilation failure.])
                    fi
                else
                    AC_MSG_RESULT([no])
                    AC_MSG_WARN([Missing header openssl/opensslv.h. Unable to determine OpenSSL version.])
                fi
            fi
        fi

    else
        HAVE_OPENSSL="no"
        AC_MSG_RESULT([$HAVE_OPENSSL])

        if test "$openssl_requested" = "yes"; then
            AC_MSG_WARN([OpenSSL support requested but headers or library not found. Specify valid prefix of OpenSSL using --with-openssl=@<:@DIR@:>@])
        fi
    fi
    if test "$HAVE_OPENSSL" = "yes"; then
        CPPFLAGS="$CPPFLAGS $OPENSSL_CPPFLAGS -DHAVE_SSL=1"
        LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS $OPENSSL_LIBS"
    fi
])
