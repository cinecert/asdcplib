I. INTRODUCTION
===============

Hello, and welcome to the Windows build instructions for asdcplib.


II. DEPENDENCIES
================

First, you'll need Microsoft's Visual Studio 2005 or later. A heavyweight edition isn't needed;
the Express edition works fine.

Next, you'll need an OpenSSL distribution. The absolute, bare minimum needed is 0.9.7. However,
if this will be used in conjunction with CineCert's other software, the later 0.9.8b
is needed. That said, the latest revision of OpenSSL as of this writing (0.9.8i) works great.
Extract and build in the directory of your choice.

For (optional) XML parsing support, you'll need to use Xerces-C++ 2.7 or 2.8 (3.0 isn't yet
supported) or Expat 2.0.1 (used in previous versions of asdcplib). If you'll be using this
software in conjuction with CineCert's other software, the use of Xerces-C++ is required. If
you need/desire XML parsing, extract the source package and build in a directory of your choice.


III. BUILDING
=============

Open a command prompt in which the VS build tools are available on the command line (e.g., the
"Visual Studio command prompt"). The nmake invocation follows this form:
C:\>nmake WITH_OPENSSL=<OpenSSL directory> [[WITH_XERCES=<Xerces directory>]|
	[WITH_XML_PARSER=<Expat directory>]] /f Makefile.mak

On our Windows development machine, the invocation with XML parsing by Xerces-C++ is as such:
C:\Program Files\asdcplib-1.3.20\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8i"
	WITH_XERCES="C:\Program Files\xerces-c_2_8_0-x86-windows-vc_8_0" /f Makefile.mak

With XML parsing by Expat:
C:\Program Files\asdcplib-1.3.20\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8i"
	WITH_XML_PARSER="C:\Program Files\Expat 2.0.1" /f Makefile.mak

Without XML parsing:
C:\Program Files\asdcplib-1.3.20\win32>nmake WITH_OPENSSL="c:\Program Files\openssl-0.9.8i"
	/f Makefile.mak


IV. CONCLUSION
==============

For answers to questions, please send a message to asdcplib@cinecert.com.
