#!/bin/sh

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
