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
	/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /RTC1 /DPACKAGE_VERSION=\"win32\" \
	/MTd /Od /ZI /DDEBUG /OPT:NOREF /D_DEBUG /I$(OPENSSL_DIR)\inc32
CPPFLAGS = $(CXXFLAGS)

LIB = lib.exe
LIBFLAGS = /NOLOGO /LIBPATH:$(OPENSSL_DIR)\out32dll

LINK = link.exe
LINKFLAGS = /NOLOGO /SUBSYSTEM:console /MACHINE:I386 /LIBPATH:. /DEBUG

all: blackwave.exe

libkumu.lib : $(KUMU_OBJS)
	$(LIB) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib

libasdcp.lib: $(ASDCP_OBJS)
	$(LIB) $(LIBFLAGS) /OUT:libasdcp.lib $(ASDCP_OBJS) libeay32.lib

blackwave.exe: blackwave.obj libasdcp.lib libkumu.lib
	$(LINK) $(LINKFLAGS) /OUT:blackwave.exe blackwave.obj libkumu.lib libasdcp.lib Advapi32.lib
