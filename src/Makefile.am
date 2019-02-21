## Makefile.am -- Process this file with automake to produce Makefile.in
#
# $Id$
#
# Copyright (c) 2007-2018 John Hurst. All rights reserved.
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

# Allow for configure's changes to this makefile
AM_CPPFLAGS =
AM_LDFLAGS =

if OPENBSD_HOST
AM_CPPFLAGS += -I/var/local/include -I/usr/local/include
AM_LDFLAGS += -L./.libs -L/var/local/lib -L/usr/local/lib
endif

if ENABLE_RANDOM_UUID
AM_CPPFLAGS += -DCONFIG_RANDOM_UUID
endif

AM_CPPFLAGS += -g

# list of all the header files that should be installed
include_HEADERS = \
	KM_error.h \
	KM_fileio.h \
	KM_log.h \
	KM_memio.h \
	KM_mutex.h \
	KM_platform.h \
	KM_prng.h \
	KM_util.h \
	KM_tai.h \
	KM_xml.h \
	AS_DCP.h

if DEV_HEADERS
include_HEADERS += \
	S12MTimecode.h \
	ST2095_PinkNoise.h \
	MDD.h \
	Metadata.h \
	KLV.h \
	JP2K.h \
	MXFTypes.h \
	MXF.h \
	Wav.h \
	PCMParserList.h \
	AtmosSyncChannel_Mixer.h \
	AtmosSyncChannel_Generator.h \
	PCMDataProviders.h \
	SyncCommon.h \
	SyncEncoder.h \
	UUIDInformation.h \
	WavFileWriter.h \
	AS_DCP_internal.h \
	AS_DCP_DCData_internal.h

endif

if USE_AS_02
include_HEADERS		+= AS_02.h
endif

if USE_PHDR
include_HEADERS		+= AS_02_PHDR.h
endif

# list of the libraries to build and install

lib_LTLIBRARIES = libkumu.la libasdcp.la

if USE_AS_02
lib_LTLIBRARIES		+= libas02.la
endif

if USE_PHDR
lib_LTLIBRARIES		+= libphdr.la
endif

# sources for kumu library
libkumu_la_SOURCES = KM_error.h KM_fileio.cpp KM_fileio.h KM_log.cpp KM_log.h \
		KM_memio.h KM_mutex.h KM_platform.h KM_prng.cpp KM_prng.h KM_util.cpp \
		KM_util.h KM_xml.cpp KM_xml.h KM_tai.h KM_tai.cpp

# linker flags (*not* including libraries to link against) for a library
libkumu_la_LDFLAGS = -release @VERSION@


# sources for asdcp library that don't get added to a distribution
nodist_libasdcp_la_SOURCES = Metadata_h.tt2 Metadata_cpp.tt2 \
		mxfgen.pl MXF_def.pl ullist.pl ULList.xml dict.xml DMS_Crypto.xml
# sources for asdcp library
libasdcp_la_SOURCES = MPEG2_Parser.cpp MPEG.cpp JP2K_Codestream_Parser.cpp \
	JP2K_Sequence_Parser.cpp JP2K.cpp PCM_Parser.cpp Wav.cpp \
	TimedText_Parser.cpp KLV.cpp Dict.cpp MXFTypes.cpp MXF.cpp \
	Index.cpp Metadata.cpp AS_DCP.cpp AS_DCP_MXF.cpp AS_DCP_AES.cpp \
	h__Reader.cpp h__Writer.cpp AS_DCP_MPEG2.cpp AS_DCP_JP2K.cpp \
	AS_DCP_PCM.cpp AS_DCP_TimedText.cpp PCMParserList.cpp \
	Wav.h WavFileWriter.h MXF.h Metadata.h \
	JP2K.h AS_DCP.h AS_DCP_internal.h KLV.h MPEG.h MXFTypes.h MDD.h \
	PCMParserList.h S12MTimecode.h MDD.cpp \
	AS_DCP_ATMOS.cpp  AS_DCP_DCData.cpp info.in \
	DCData_ByteStream_Parser.cpp DCData_Sequence_Parser.cpp \
	AtmosSyncChannel_Generator.cpp AtmosSyncChannel_Generator.h \
	AtmosSyncChannel_Mixer.cpp AtmosSyncChannel_Mixer.h \
	PCMDataProviders.cpp PCMDataProviders.h \
	SyncEncoder.c SyncEncoder.h SyncCommon.h CRC16.c CRC16.h \
	UUIDInformation.c UUIDInformation.h \
	ST2095_PinkNoise.cpp


libasdcp_la_LDFLAGS = -release @VERSION@
# additional libraries to link against for a library
libasdcp_la_LIBADD = libkumu.la
libasdcp_la_CPPFLAGS = -DASDCP_PLATFORM=\"@host@\"

if USE_AS_02
# sources for as-02 library
libas02_la_SOURCES	= \
	AS_02.h \
	AS_02_internal.h \
	ACES.h \
	AS_02_ACES.h \
	h__02_Reader.cpp \
	h__02_Writer.cpp \
	AS_02_JP2K.cpp \
	AS_02_PCM.cpp \
	AS_02_ISXD.cpp \
	ST2052_TextParser.cpp \
	AS_02_TimedText.cpp \
	ACES.cpp \
	ACES_Codestream_Parser.cpp \
	ACES_Sequence_Parser.cpp \
	AS_02_ACES.cpp

libas02_la_LDFLAGS = -release @VERSION@
libas02_la_LIBADD = libasdcp.la libkumu.la
libas02_la_CPPFLAGS = -DASDCP_PLATFORM=\"@host@\"
endif


if USE_PHDR
# sources for PHDR library
libphdr_la_SOURCES	= \
	AS_02_PHDR.h \
	PHDR_Sequence_Parser.cpp \
	AS_02_PHDR.cpp

libphdr_la_LDFLAGS = -release @VERSION@
libphdr_la_LIBADD = libasdcp.la libkumu.la
libphdr_la_CPPFLAGS = -DASDCP_PLATFORM=\"@host@\"
endif

# list of programs to be built and installed
bin_PROGRAMS = \
	asdcp-wrap \
	asdcp-unwrap \
	asdcp-util \
	asdcp-info \
	asdcp-test \
	j2c-test \
	blackwave \
	pinkwave \
	klvwalk \
	wavesplit \
	klvsplit \
	kmfilegen \
	kmrandgen \
	kmuuidgen

if USE_AS_02
bin_PROGRAMS += \
	as-02-wrap \
	as-02-unwrap \
	as-02-info
endif

if USE_PHDR
bin_PROGRAMS += \
	phdr-wrap \
	phdr-unwrap
endif

# sources and linkage for CLI utilities
asdcp_test_SOURCES = asdcp-test.cpp
asdcp_test_LDADD = libasdcp.la libkumu.la

asdcp_wrap_SOURCES = asdcp-wrap.cpp
asdcp_wrap_LDADD = libasdcp.la libkumu.la

asdcp_unwrap_SOURCES = asdcp-unwrap.cpp
asdcp_unwrap_LDADD = libasdcp.la libkumu.la

asdcp_util_SOURCES = asdcp-util.cpp
asdcp_util_LDADD = libasdcp.la libkumu.la

asdcp_info_SOURCES = asdcp-info.cpp
asdcp_info_LDADD = libasdcp.la libkumu.la

kmfilegen_SOURCES = kmfilegen.cpp
kmfilegen_LDADD = libkumu.la 

kmrandgen_SOURCES = kmrandgen.cpp
kmrandgen_LDADD = libkumu.la 

kmuuidgen_SOURCES = kmuuidgen.cpp
kmuuidgen_LDADD = libkumu.la 

blackwave_SOURCES = blackwave.cpp
blackwave_LDADD = libasdcp.la  libkumu.la

pinkwave_SOURCES = pinkwave.cpp
pinkwave_LDADD = libasdcp.la  libkumu.la

klvwalk_SOURCES = klvwalk.cpp
klvwalk_LDADD = libasdcp.la libkumu.la

wavesplit_SOURCES = wavesplit.cpp
wavesplit_LDADD = libasdcp.la libkumu.la

j2c_test_SOURCES = j2c-test.cpp
j2c_test_LDADD = libasdcp.la libkumu.la

klvsplit_SOURCES = klvsplit.cpp
klvsplit_LDADD = libasdcp.la libkumu.la

if USE_AS_02
as_02_wrap_SOURCES = as-02-wrap.cpp
as_02_wrap_LDADD = libas02.la libasdcp.la libkumu.la

as_02_unwrap_SOURCES = as-02-unwrap.cpp
as_02_unwrap_LDADD = libas02.la libasdcp.la libkumu.la

as_02_info_SOURCES = as-02-info.cpp
as_02_info_LDADD = libas02.la libasdcp.la libkumu.la
endif

if USE_PHDR
phdr_wrap_SOURCES = phdr-wrap.cpp
phdr_wrap_LDADD = libphdr.la libas02.la libasdcp.la libkumu.la

phdr_unwrap_SOURCES = phdr-unwrap.cpp
phdr_unwrap_LDADD = libphdr.la libas02.la libasdcp.la libkumu.la
endif

# list of programs that need to be compiled for use in test suite
check_PROGRAMS = asdcp-mem-test path-test \
	fips-186-rng-test asdcp-version
if DEV_HEADERS
check_PROGRAMS += tt-xform
endif

# sources for a test program
# list of libraries to link against for a test program
asdcp_mem_test_SOURCES = asdcp-mem-test.cpp
asdcp_mem_test_LDADD = libasdcp.la

path_test_SOURCES = path-test.cpp
path_test_LDADD = libkumu.la

fips_186_rng_test_SOURCES = fips-186-rng-test.cpp
fips_186_rng_test_LDADD = libasdcp.la

asdcp_version_SOURCES = asdcp-version.cpp
asdcp_version_LDADD = libkumu.la 

if DEV_HEADERS
nodist_tt_xform_SOURCES = tt-xform.cpp TimedText_Transform.h
tt_xform_LDADD = libasdcp.la
endif


# list of test scripts to execute during "make check"
TESTS = rng-tst.sh gen-tst.sh \
	jp2k-tst.sh jp2k-crypt-tst.sh jp2k-stereo-tst.sh jp2k-stereo-crypt-tst.sh \
	wav-tst.sh wav-crypt-tst.sh mpeg-tst.sh mpeg-crypt-tst.sh

# environment variables to pass to above tests
TESTS_ENVIRONMENT = BUILD_DIR="." TEST_FILES=../tests TEST_FILE_PREFIX=DCPd1-M1 \
	CRYPT_KEY=70e0de21c98fbd455ad5b8042edb41a6 CRYPT_KEY_B=aa2d05475d568cd52cb3415e65cba76f \
	JP2K_PREFIX=MM_2k_XYZ_

# files to include in the distribution that automake doesn't automatically include
EXTRA_DIST = fips-186-test-harness.pl $(TESTS) ../README.cmake ../CMakeLists.txt  CMakeLists.txt dirent_win.h
if !FREEDIST
if DEV_HEADERS
EXTRA_DIST += $(nodist_libasdcp_la_SOURCES) $(nodist_tt_xform_SOURCES)
endif
endif


#
# end Makefile.am
#
