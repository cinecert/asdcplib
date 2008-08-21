ARCH = win32

SRCDIR=..\src

OPENSSL_DIR = ..\..\openssl

KUMU_OBJS = KM_fileio.obj KM_log.obj KM_prng.obj KM_util.obj KM_xml.obj
ASDCP_OBJS = MPEG2_Parser.obj MPEG.obj JP2K_Codestream_Parser.obj \
	JP2K_Sequence_Parser.obj JP2K.obj PCM_Parser.obj Wav.obj \
	TimedText_Parser.obj KLV.obj Dict.obj MXFTypes.obj MXF.obj \
	Index.obj Metadata.obj AS_DCP.obj AS_DCP_MXF.obj AS_DCP_AES.obj \
	h__Reader.obj h__Writer.obj AS_DCP_MPEG2.obj AS_DCP_JP2K.obj \
	AS_DCP_PCM.obj AS_DCP_TimedText.obj PCMParserList.obj \
	MDD.obj


CXXFLAGS = /nologo /W3 /GR /EHsc /DWIN32 /DKM_WIN32 /D_CONSOLE /I. /DASDCP_PLATFORM=\"win32\" \
	/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /RTC1 /DPACKAGE_VERSION=\"1.3.19a\" \
	/MTd /Od /ZI /DDEBUG /D_DEBUG /I$(OPENSSL_DIR)\inc32
CPPFLAGS = $(CXXFLAGS)

LIB_EXE = lib.exe
LIBFLAGS = /NOLOGO /LIBPATH:$(OPENSSL_DIR)\out32dll

LINK = link.exe
LINKFLAGS = /NOLOGO /SUBSYSTEM:console /MACHINE:I386 /LIBPATH:. /DEBUG

all: kmfilegen.exe kmrandgen.exe kmuuidgen.exe asdcp-test.exe blackwave.exe klvwalk.exe wavesplit.exe

libkumu.lib : $(KUMU_OBJS)
	$(LIB_EXE) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib

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
