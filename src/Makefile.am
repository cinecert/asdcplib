## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I/var/local/include -I/usr/local/include
AM_LDFLAGS = -L/var/local/lib -L/usr/local/lib

# list of all the header files that should be installed
include_HEADERS = KM_error.h KM_fileio.h KM_log.h KM_memio.h KM_mutex.h \
 KM_platform.h KM_prng.h KM_util.h KM_xml.h AS_DCP.h
extra_HDRS = S12MTimecode.h MDD.h Metadata.h KLV.h MXFTypes.h MXF.h Wav.h \
 PCMParserList.h

# list of the libraries to build and install
lib_LTLIBRARIES = libkumu.la libasdcp.la
# sources for a library
libkumu_la_SOURCES = KM_error.h KM_fileio.cpp KM_fileio.h KM_log.cpp KM_log.h \
 KM_memio.h KM_mutex.h KM_platform.h KM_prng.cpp KM_prng.h KM_util.cpp \
 KM_util.h KM_xml.cpp KM_xml.h
# linker flags (*not* including libraries to link against) for a library
libkumu_la_LDFLAGS = -version-info 3:19:0
# sources for a library that don't get added to a distribution
nodist_libasdcp_la_SOURCES = Metadata_h.tt2 Metadata_cpp.tt2 \
 mxfgen.pl MXF_def.pl ullist.pl ULList.xml dict.xml DMS_Crypto.xml
libasdcp_la_SOURCES = MPEG2_Parser.cpp MPEG.cpp JP2K_Codestream_Parser.cpp \
 JP2K_Sequence_Parser.cpp JP2K.cpp PCM_Parser.cpp Wav.cpp \
 TimedText_Parser.cpp KLV.cpp Dict.cpp MXFTypes.cpp MXF.cpp \
 Index.cpp Metadata.cpp AS_DCP.cpp AS_DCP_MXF.cpp AS_DCP_AES.cpp \
 h__Reader.cpp h__Writer.cpp AS_DCP_MPEG2.cpp AS_DCP_JP2K.cpp \
 AS_DCP_PCM.cpp AS_DCP_TimedText.cpp PCMParserList.cpp \
 Wav.h WavFileWriter.h MXF.h Metadata.h \
 JP2K.h AS_DCP.h AS_DCP_internal.h KLV.h MPEG.h MXFTypes.h MDD.h \
 PCMParserList.h S12MTimecode.h MDD.cpp
libasdcp_la_LDFLAGS = -version-info 3:19:0
# additional libraries to link against for a library
libasdcp_la_LIBADD = libkumu.la
libasdcp_la_CPPFLAGS=-DASDCP_PLATFORM=\"@host@\"
# Python extension
if PYTHON_USE
lib_LTLIBRARIES += libpyasdcp.la
nodist_libpyasdcp_la_SOURCES = asdcp_python.cpp asdcp_python.h asdcp_python_misc.cpp \
 asdcp_python_reader.cpp asdcp_python_writer.cpp asdcp_wrappers.h \
 kumu_python.cpp kumu_python.h
libpyasdcp_la_CPPFLAGS = @PYTHON_CPPFLAGS@
libpyasdcp_la_LDFLAGS = @PYTHON_LSPEC@
libpyasdcp_la_LIBADD = libkumu.la libasdcp.la
pyexecdir = @PYTHON_EXECDIR@
#pyexec_includedir = $(PYTHON_PREFIX)/include
pyexec_LTLIBRARIES = kumu.la asdcp.la
nodist_kumu_la_SOURCES = pykumu.cpp kumu_python.h
kumu_la_CPPFLAGS = @PYTHON_CPPFLAGS@
kumu_la_LDFLAGS = @PYTHON_LSPEC@ -avoid-version -module
kumu_la_LIBADD = libpyasdcp.la

nodist_asdcp_la_SOURCES = pyasdcp.cpp asdcp_python.h
asdcp_la_CPPFLAGS = @PYTHON_CPPFLAGS@
asdcp_la_LDFLAGS = @PYTHON_LSPEC@ -avoid-version -module
asdcp_la_LIBADD = libpyasdcp.la
#pyexec_include_HEADERS = kumu_python.h asdcp_python.h
endif

# list of programs to be built and installed
bin_PROGRAMS = asdcp-test blackwave klvwalk kmfilegen kmrandgen \
 kmuuidgen wavesplit

# sources for a program
asdcp_test_SOURCES = asdcp-test.cpp
# list of libraries to link against for a program
asdcp_test_LDADD = libasdcp.la
kmfilegen_SOURCES = kmfilegen.cpp
kmfilegen_LDADD = libasdcp.la
kmrandgen_SOURCES = kmrandgen.cpp
kmrandgen_LDADD = libasdcp.la
kmuuidgen_SOURCES = kmuuidgen.cpp
kmuuidgen_LDADD = libasdcp.la
blackwave_SOURCES = blackwave.cpp
blackwave_LDADD = libasdcp.la
klvwalk_SOURCES = klvwalk.cpp
klvwalk_LDADD = libasdcp.la
wavesplit_SOURCES = wavesplit.cpp
wavesplit_LDADD = libasdcp.la

# list of programs that need to be compiled for use in test suite
check_PROGRAMS = asdcp-mem-test path-test jp2k-test S429-5-cgi \
	fips-186-rng-test asdcp-version

# sources for a test program
asdcp_mem_test_SOURCES = asdcp-mem-test.cpp
# list of libraries to link against for a test program
asdcp_mem_test_LDADD = libasdcp.la
path_test_SOURCES = path-test.cpp
path_test_LDADD = libkumu.la
jp2k_test_SOURCES = jp2k-test.cpp
jp2k_test_LDADD = libasdcp.la
S429_5_cgi_SOURCES = S429-5-cgi.cpp
S429_5_cgi_LDADD = libasdcp.la
fips_186_rng_test_SOURCES = fips-186-rng-test.cpp
fips_186_rng_test_LDADD = libasdcp.la
asdcp_version_SOURCES = asdcp-version.cpp
asdcp_version_LDADD = libkumu.la


# list of test scripts to execute during "make check"
TESTS = rng-tst.sh gen-tst.sh \
	jp2k-tst.sh jp2k-crypt-tst.sh jp2k-stereo-tst.sh jp2k-stereo-crypt-tst.sh \
	wav-tst.sh wav-crypt-tst.sh mpeg-tst.sh mpeg-crypt-tst.sh
# environment variables to pass to above tests
TESTS_ENVIRONMENT = BUILD_DIR="." TEST_FILES=../tests TEST_FILE_PREFIX=DCPd1-M1 \
	CRYPT_KEY=70e0de21c98fbd455ad5b8042edb41a6 CRYPT_KEY_B=aa2d05475d568cd52cb3415e65cba76f \
	JP2K_PREFIX=MM_2k_XYZ_

# files to include in the distribution that automake doesn't automatically include
EXTRA_DIST = fips-186-test-harness.pl $(TESTS)


# source files that are themselves built
BUILT_SOURCES = Metadata.h Metadata.cpp MDD.h MDD.cpp
# files to be removed with "make maintainer-clean"
MAINTAINERCLEANFILES = Metadata.h Metadata.cpp MDD.h MDD.cpp

# build commands for built sources
# dependencies: $(srcdir)/ullist.pl $(srcdir)/ULList.xml $(srcdir)/dict.xml $(srcdir)/DMS_Crypto.xml
MDD.h:
	$(srcdir)/ullist.pl $(srcdir)/ULList.xml $(srcdir)/dict.xml $(srcdir)/DMS_Crypto.xml > MDD.h
# dependencies: $(srcdir)/ullist.pl $(srcdir)/ULList.xml $(srcdir)/dict.xml $(srcdir)/DMS_Crypto.xml
MDD.cpp:
	$(srcdir)/ullist.pl -s $(srcdir)/ULList.xml $(srcdir)/dict.xml $(srcdir)/DMS_Crypto.xml > MDD.cpp
# dependencies: $(srcdir)/Metadata_h.tt2 $(srcdir)/mxfgen.pl $(srcdir)/MXF_def.pl
Metadata.h:
	$(srcdir)/mxfgen.pl $(srcdir)/MXF_def.pl $(srcdir)/Metadata_h.tt2 > Metadata.h
# dependencies:  Metadata.h $(srcdir)/Metadata_cpp.tt2
Metadata.cpp:
	$(srcdir)/mxfgen.pl $(srcdir)/MXF_def.pl $(srcdir)/Metadata_cpp.tt2 > Metadata.cpp

extraHDRS_INSTALL = $(INSTALL_HEADER)
install-extra: $(extra_HDRS)
	@$(NORMAL_INSTALL)
	test -z "$(includedir)" || $(MKDIR_P) "$(DESTDIR)$(includedir)"
	@list='$(extra_HDRS)'; for p in $$list; do \
		if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
		f=$(am__strip_dir) \
		echo " $(extraHDRS_INSTALL) '$$d$$p' '$(DESTDIR)$(includedir)/$$f'"; \
		$(extraHDRS_INSTALL) "$$d$$p" "$(DESTDIR)$(includedir)/$$f"; \
	done
