#
# $Id$
# Copyright (c) 2007-8 CineCert, LLC. All rights reserved.
#
#
#
#
#
#
#
#
#


ARCH = win32

SRCDIR=..\src

!ifndef WITH_OPENSSL
!error "OpenSSL is needed! Specify it with WITH_OPENSSL=<OpenSSL directory>"
!endif

KUMU_OBJS = KM_fileio.obj KM_log.obj KM_prng.obj KM_util.obj KM_xml.obj
ASDCP_OBJS = MPEG2_Parser.obj MPEG.obj JP2K_Codestream_Parser.obj \
	JP2K_Sequence_Parser.obj JP2K.obj PCM_Parser.obj Wav.obj \
	TimedText_Parser.obj KLV.obj Dict.obj MXFTypes.obj MXF.obj \
	Index.obj Metadata.obj AS_DCP.obj AS_DCP_MXF.obj AS_DCP_AES.obj \
	h__Reader.obj h__Writer.obj AS_DCP_MPEG2.obj AS_DCP_JP2K.obj \
	AS_DCP_PCM.obj AS_DCP_TimedText.obj PCMParserList.obj \
	MDD.obj

CXXFLAGS1 = /nologo /W3 /GR /EHsc /DWIN32 /DKM_WIN32 /D_CONSOLE /I. /I$(SRCDIR) /DASDCP_PLATFORM=\"win32\" \
	/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /DPACKAGE_VERSION=\"1.3.19a\" \
	/I$(WITH_OPENSSL)\inc32

LIB_EXE = lib.exe
LIBFLAGS1 = /NOLOGO /LIBPATH:$(WITH_OPENSSL)\out32dll

LINK = link.exe
LINKFLAGS1 = /NOLOGO /SUBSYSTEM:console /MACHINE:I386 /LIBPATH:. /DEBUG


!ifdef DEBUG
CXXFLAGS2 = $(CXXFLAGS1) /MTd /DDEBUG /D_DEBUG /Od /RTC1 /ZI
LINKFLAGS = $(LINKFLAGS1) /DEBUG
!else
CXXFLAGS2 = $(CXXFLAGS1) /MT /DNDEBUG /D_NDEBUG /O2
LINKFLAGS = $(LINKFLAGS1)
!endif

!ifndef WITH_OPENSSL
!error "OpenSSL is needed!"
!endif

!IFDEF WITH_XERCES
!ifdef WITH_XML_PARSER
!ERROR "Cannot include both Expat and Xerces-C++!"
!endif

XERCES_DIR = $(WITH_XERCES)
CPPFLAGS = $(CXXFLAGS2) /DHAVE_XERCES_C=1 /I"$(XERCES_DIR)"\include
LIBFLAGS = $(LIBFLAGS1) /LIBPATH:"$(XERCES_DIR)"\lib
!ELSEIFDEF WITH_XML_PARSER
CPPFLAGS = $(CXXFLAGS2) /DASDCP_USE_EXPAT /I"$(WITH_XML_PARSER)"\Source\lib
!IFDEF DEBUG
LIBFLAGS = $(LIBFLAGS1) /LIBPATH:"$(WITH_XML_PARSER)"\Source\win32\bin\debug
!ELSE
LIBFLAGS = $(LIBFLAGS1) /LIBPATH:"$(WITH_XML_PARSER)"\Source\win32\bin\release
!ENDIF
!ELSE
CPPFLAGS = $(CXXFLAGS2)
LIBFLAGS = $(LIBFLAGS1)
!ENDIF


all: libkumu.lib kmfilegen.exe kmrandgen.exe kmuuidgen.exe asdcp-test.exe blackwave.exe klvwalk.exe wavesplit.exe

libkumu.lib : $(KUMU_OBJS)
!IFDEF WITH_XERCES
!IFDEF DEBUG
	$(LIB_EXE) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib xerces-c_2D.lib
!ELSE
	$(LIB_EXE) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib xerces-c_2.lib
!ENDIF
!ELSEIFDEF WITH_XML_PARSER
	$(LIB_EXE) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib libexpatMT.lib
!ELSE
	$(LIB_EXE) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib
!ENDIF 

libasdcp.lib: libkumu.lib $(ASDCP_OBJS)
	$(LIB_EXE) $(LIBFLAGS) /OUT:libasdcp.lib libkumu.lib $(ASDCP_OBJS)

blackwave.exe: libasdcp.lib blackwave.obj
	$(LINK) $(LINKFLAGS) /OUT:blackwave.exe blackwave.obj libasdcp.lib Advapi32.lib

wavesplit.exe: libasdcp.lib wavesplit.obj
	$(LINK) $(LINKFLAGS) /OUT:wavesplit.exe wavesplit.obj libasdcp.lib Advapi32.lib

kmuuidgen.exe: libkumu.lib kmuuidgen.obj
	$(LINK) $(LINKFLAGS) /OUT:kmuuidgen.exe kmuuidgen.obj libkumu.lib Advapi32.lib

kmrandgen.exe: libkumu.lib kmrandgen.obj
	$(LINK) $(LINKFLAGS) /OUT:kmrandgen.exe kmrandgen.obj libkumu.lib Advapi32.lib

kmfilegen.exe: libkumu.lib kmfilegen.obj
	$(LINK) $(LINKFLAGS) /OUT:kmfilegen.exe kmfilegen.obj libkumu.lib Advapi32.lib

klvwalk.exe: libasdcp.lib klvwalk.obj
	$(LINK) $(LINKFLAGS) /OUT:klvwalk.exe klvwalk.obj libasdcp.lib Advapi32.lib

asdcp-test.exe: libasdcp.lib asdcp-test.obj
	$(LINK) $(LINKFLAGS) /OUT:asdcp-test.exe asdcp-test.obj libasdcp.lib Advapi32.lib
