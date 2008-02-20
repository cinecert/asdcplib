#!/bin/sh

# JPEG 2000 stereoscopic tests

${BUILD_DIR}/asdcp-test${EXEEXT} -3 -c ${TEST_FILES}/write_test_jp2k.mxf \
	${TEST_FILES}/${TEST_FILE_PREFIX} ${TEST_FILES}/${TEST_FILE_PREFIX}
if [ $? -ne 0 ]; then
    exit 1
fi


${BUILD_DIR}/asdcp-test${EXEEXT} -3 -x ${TEST_FILES}/extract/${JP2K_PREFIX} ${TEST_FILES}/write_test_jp2k.mxf
if [ $? -ne 0 ]; then
    exit 1
fi
for file in `ls ${TEST_FILES}/${TEST_FILE_PREFIX}`; do \
  echo "$file"; \
  cmp ${TEST_FILES}/${TEST_FILE_PREFIX}/$file ${TEST_FILES}/extract/$file; \
  if [ $? -ne 0 ]; then \
    exit 1; \
  fi; \
done
