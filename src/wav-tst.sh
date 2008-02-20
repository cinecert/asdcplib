#!/bin/sh

# WAV tests

${BUILD_DIR}/asdcp-test${EXEEXT} -c ${TEST_FILES}/write_test_wav.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}.wav
if [ $? -ne 0 ]; then
    exit 1
fi

${BUILD_DIR}/asdcp-test${EXEEXT} -x ${TEST_FILES}/extract ${TEST_FILES}/write_test_wav.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
cmp ${TEST_FILES}/${TEST_FILE_PREFIX}.wav ${TEST_FILES}/extract_1.wav
if [ $? -ne 0 ]; then
    exit 1
fi
