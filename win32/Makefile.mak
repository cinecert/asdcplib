SRCDIR=..\src

OPENSSL_DIR = ..\..\openssl

KUMU_OBJS = KM_fileio.obj KM_log.obj KM_prng.obj KM_util.obj KM_xml.obj

CXXFLAGS = /nologo /W3 /GR /EHsc /DWIN32 /DKM_WIN32 /D_CONSOLE /I. /DASDCP_PLATFORM=\"win32\" \
	/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS /RTC1 /DPACKAGE_VERSION=\"win32\" \
	/MTd /Od /ZI /DDEBUG /OPT:NOREF /D_DEBUG /I$(OPENSSL_DIR)\inc32
CPPFLAGS = $(CXXFLAGS)

LIB = lib.exe
LIBFLAGS = /NOLOGO /LIBPATH:$(OPENSSL_DIR)\out32dll

libkumu.lib : $(KUMU_OBJS)
	$(LIB) $(LIBFLAGS) /OUT:libkumu.lib $(KUMU_OBJS) libeay32.lib 

