#!/bin/sh
#
# $Id$
# Copyright (c) 2007-2009 John Hurst. All rights reserved.
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

# crypto MPEG tests

${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY} \
	-c ${TEST_FILES}/write_crypt_test_mpeg.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}.mpg
if [ $? -ne 0 ]; then
    exit 1
fi
${BUILD_DIR}/asdcp-test${EXEEXT} -i ${TEST_FILES}/write_crypt_test_mpeg.mxf
if [ $? -ne 0 ]; then
    exit 1
fi


(${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY_B} \
	-x ${TEST_FILES}/plaintext ${TEST_FILES}/write_crypt_test_mpeg.mxf; \
  if [ $? -eq 1 ]; then exit 0; fi; exit 1 )
${BUILD_DIR}/asdcp-test${EXEEXT} -m -k ${CRYPT_KEY} \
	-x ${TEST_FILES}/plaintext ${TEST_FILES}/write_crypt_test_mpeg.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
cmp ${TEST_FILES}/${TEST_FILE_PREFIX}.mpg ${TEST_FILES}/plaintext.ves
if [ $? -ne 0 ]; then
    exit 1
fi
