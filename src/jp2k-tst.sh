#!/bin/sh

# JPEG 2000 tests

mkdir -p ${TEST_FILES}/extract ${TEST_FILES}/plaintext

${BUILD_DIR}/asdcp-test${EXEEXT} -c ${TEST_FILES}/write_test_jp2k.mxf ${TEST_FILES}/${TEST_FILE_PREFIX}
if [ $? -ne 0 ]; then
    exit 1
fi


${BUILD_DIR}/asdcp-test${EXEEXT} -x ${TEST_FILES}/extract/${JP2K_PREFIX} ${TEST_FILES}/write_test_jp2k.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
for file in `ls ../test/${TEST_FILE_PREFIX}`; do \
  echo "$file"; \
  cmp ../test/${TEST_FILE_PREFIX}/$file ../test/extract/$file; \
  if [ $? -ne 0 ]; then \
    exit 1; \
  fi; \
done


#${BUILD_DIR}/jp2k-test${EXEEXT} ${TEST_FILES}/${TEST_FILE_PREFIX}/MM_2k_XYZ_000000.j2c
#if [ $? -ne 0 ]; then
#    exit 1
#fi
