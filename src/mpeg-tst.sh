#!/bin/sh

# MPEG tests

${BUILD_DIR}/asdcp-test${EXEEXT} -c ${TEST_FILES}/write_test_mpeg.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}.mpg
if [ $? -ne 0 ]; then
    exit 1
fi


${BUILD_DIR}/asdcp-test${EXEEXT} -x ${TEST_FILES}/extract ${TEST_FILES}/write_test_mpeg.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
cmp ${TEST_FILES}/${TEST_FILE_PREFIX}.mpg ${TEST_FILES}/extract.ves
if [ $? -ne 0 ]; then
    exit 1
fi


#${BUILD_DIR}/asdcp-test${EXEEXT} -G ${TEST_FILES}/write_test_mpeg.mxf
#if [ $? -ne 0 ]; then
#    exit 1
#fi
