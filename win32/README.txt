I. INTRODUCTION
===============

Hello, and welcome to the Windows build instructions for asdcplib.


II. DEPENDENCIES
================

First, you'll need Microsoft's Visual Studio 2005 or later. A heavyweight edition isn't needed;
the freely downloadable Express edition works fine.

Next, you'll need an OpenSSL distribution. The absolute, bare minimum needed is 0.9.7. However,
if this will be used in conjunction with CineCert's other software, the later 0.9.8b
is needed. That said, the latest revision of OpenSSL as of this writing (0.9.8j) works great.
Extract and build in the directory of your choice.

For optional XML parsing support, you'll need to use Xerces-C++ 2.7 or 3.x, or Expat 2.0.1
(supported in previous versions of asdcplib). If you'll be using this software in conjuction with
CineCert's other software, the use of Xerces-C++ is required. As with OpenSSL above, if you
need/desire XML parsing, extract the source package and build in a directory of your choice.

Header files and libraries from the OpenSSL and XML packages must be available to the compiler
and linker. You may need to modify the makefile to make include files (/I...) and library files
(/LIBPATH...) available.


III. BUILDING
=============

There's a build option that changes the behavior of UUID generation. If ENABLE_RANDOM_UUID is
set at build, then mixed-case UUID generation will be enabled if (and only if) the environment
variable KM_USE_RANDOM_UUID is set during runtime.

Open a command prompt in which the VS build tools are available on the command line (e.g., the
"Visual Studio command prompt"). The nmake invocation follows this form:
C:\>nmake WITH_OPENSSL=<OpenSSL directory> [WITH_XERCES=<Xerces directory>|
	WITH_XML_PARSER=<Expat directory>] [ENABLE_RANDOM_UUID=1] /f Makefile.mak

On our Windows development machine, the invocation with XML parsing by Xerces-C++ is as such:
C:\Program Files\asdcplib\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8j"
	WITH_XERCES="C:\Program Files\xerces-c_2_8_0-x86-windows-vc_8_0" /f Makefile.mak

With XML parsing by Expat and random UUID generation enabled:
C:\Program Files\asdcplib\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8j"
	WITH_XML_PARSER="C:\Program Files\Expat 2.0.1" ENABLE_RANDOM_UUID=1 /f Makefile.mak

Without XML parsing:
C:\Program Files\asdcplib\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8j"
	/f Makefile.mak

Without XML parsing but with the AS-02 library and executables:
C:\Program Files\asdcplib\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8j"
	USE_AS_02=1 /f Makefile.mak

Want a 64-bit build? Change the following line in Makefile.mak:
    LINKFLAGS1 = /NOLOGO /SUBSYSTEM:console /MACHINE:I386 /LIBPATH:. /DEBUG
to
    LINKFLAGS1 = /NOLOGO /SUBSYSTEM:console /MACHINE:X64 /LIBPATH:. /DEBUG


IV. CONCLUSION
==============

For answers to questions, please send a message to <asdcplib@cinecert.com>.
