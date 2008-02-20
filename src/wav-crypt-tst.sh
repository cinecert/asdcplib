#!/bin/sh

# crypto WAV tests

${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY} \
	-c ${TEST_FILES}/write_crypt_test_wav.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}.wav
if [ $? -ne 0 ]; then
    exit 1
fi
${BUILD_DIR}/asdcp-test${EXEEXT} -i ${TEST_FILES}/write_crypt_test_wav.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
${BUILD_DIR}/asdcp-test${EXEEXT} -p 23 -k ${CRYPT_KEY} \
	-c ${TEST_FILES}/write_crypt_test_2398_wav.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}.wav
if [ $? -ne 0 ]; then
    exit 1
fi
${BUILD_DIR}/asdcp-test${EXEEXT} -i ${TEST_FILES}/write_crypt_test_2398_wav.mxf
if [ $? -ne 0 ]; then
    exit 1
fi


( ${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY_B} \
	-x ${TEST_FILES}/plaintext ${TEST_FILES}/write_crypt_test_wav.mxf; \
  if [ $? -eq 1 ]; then exit 0; fi; exit 1 )
${BUILD_DIR}/asdcp-test${EXEEXT} -m -k ${CRYPT_KEY} \
	-x ${TEST_FILES}/plaintext ${TEST_FILES}/write_crypt_test_wav.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
cmp ${TEST_FILES}/${TEST_FILE_PREFIX}.wav ${TEST_FILES}/plaintext_1.wav
if [ $? -ne 0 ]; then
    exit 1
fi
( ${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY_B} \
	-x ${TEST_FILES}/plaintext_2398 ${TEST_FILES}/write_crypt_test_2398_wav.mxf; \
  if [ $? -eq 1 ]; then exit 0; fi; exit 1 )
${BUILD_DIR}/asdcp-test${EXEEXT} -m -k ${CRYPT_KEY} \
	-x ${TEST_FILES}/plaintext_2398 ${TEST_FILES}/write_crypt_test_2398_wav.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
