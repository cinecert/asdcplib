#!/bin/sh

# crypto JPEG 2000 stereoscopic tests

${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY} \
	-3 -c ${TEST_FILES}/write_crypt_test_jp2k.mxf \
	${TEST_FILES}/${TEST_FILE_PREFIX} ${TEST_FILES}/${TEST_FILE_PREFIX}
if [ $? -ne 0 ]; then
    exit 1
fi
${BUILD_DIR}/asdcp-test${EXEEXT} -i ${TEST_FILES}/write_crypt_test_jp2k.mxf
if [ $? -ne 0 ]; then
    exit 1
fi


(${BUILD_DIR}/asdcp-test${EXEEXT} -k ${CRYPT_KEY_B} \
	-3 -x ${TEST_FILES}/plaintext ${TEST_FILES}/write_crypt_test_jp2k.mxf; \
  if [ $? -eq 1 ]; then exit 0; fi; exit 1 )
${BUILD_DIR}/asdcp-test${EXEEXT} -m -k ${CRYPT_KEY} \
	-3 -x ${TEST_FILES}/plaintext/${JP2K_PREFIX} ${TEST_FILES}/write_crypt_test_jp2k.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
for file in `ls ../test/${TEST_FILE_PREFIX}`; do \
  echo "$file"; \
  cmp ../test/${TEST_FILE_PREFIX}/$file ../test/plaintext/$file; \
  if [ $? -ne 0 ]; then \
    exit 1; \
  fi; \
done
