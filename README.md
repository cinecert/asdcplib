
# AS-DCP Lib

## Introduction

The asdcplib library is an API and command-line tool set that offers
access to files conforming to the sound and picture track file formats
developed by the SMPTE Working Group DC28.20 (now TC 21DC).

Support has since been added for SMPTE ST 2067-5 "IMF Essence
Component", AKA "AS-02", which was published and is maintained by
SMPTE TC 35 PM.  The initial draft of this code was donated by
Fraunhofer IIS and was created by Robert Scheler and Heiko Sparenberg.
It carries additional copyright information which should be listed
whenever you link the AS-02 elements of the library. Please look at
the top of the AS-02 files to see this copyright information. 

Support for ST 20XX OpenEXR was developed and contributed by Bjoern
Stresing, Patrick Bichiou and Wolfgang Ruppel, supported by AMPAS.
It carries additional copyright information which should be listed
whenever you link the AS-02 elements of the library. Please look at
the top of the AS-02 files to see this copyright information.

AS-02 support is carried in separate object modules, so unless you
`#include <AS_02.h>` and link `libas-02.so` you are still using plain old
asdcp. 


## Documentation

This library is intended (but of course not limited) for use by
developers of commercial D-Cinema and IMF products and for commercial
mastering toolchains. The documentation is terse and sparse.

The API documentation is mostly in AS_DCP.h. and AS_02.h  Read those
files for a detailed description of the library's capabilities. Read
asdcp-*.cpp and as-02-*.cpp files for library usage examples. The
command-line utilities all respond to -h.

Also, of course, the various SMPTE and ISO standards that underly all
of this work should be well understood if you want to tinker with
anything, or, in some cases, understand what properties are required
in a particular supported use case (e.g., selecting audio channel labels.)


## CLI Programs

### Standard Utilities

`asdcp-test` - DEPRECATED  Writes, reads and verifies AS-DCP (MXF) track files.

`asdcp-wrap` - Writes AS-DCP (MXF) track files.

`asdcp-unwrap` - Extracts essence from AS-DCP (MXF) track files.

`asdcp-info` - Displays information about AS-DCP (MXF) track files.

`asdcp-util` - Calculates digests and generates random numbers and UUIDs.

`as-02-wrap` - Writes AS-02 Essence Component files.

`as-02-unwrap` - Extracts essence from AS-02 Essence Component files.

`kmfilegen` - Writes and verifies large files using a platform-independent format. Use it to test issues related to large files.

`kmuuidgen`, `kmrandgen` - generate UUID values and random numbers.

`wavesplit` - Splits a WAVE file into two or more output files. Used  to untangle incorrectly-paired DCDM sound files.

`blackwave` - Write a WAVE file of zeros.

`pinkwave` - Write a WAVE file of SMPTE ST 2095 pink noise.

`j2c-test` - Displays information about JP2K codestreams.

### PHDR

An experimental feature, Prototype for High Dynamic Range is a wrapper
for the IMF application that allows JPEG-2000 codestreams to be paired
with opaque blobs of metadata.  AS-02 support must be enabled to
build this feature, so --enable-as-02 must be enabled if
--enable-phdr is to be used.  The following executable programs will be
built:

`phdr-wrap` - Writes AS-02 PHDR Essence Component files.

`phdr-unwrap` - Extracts essence from AS-02 PHDR Essence Component files.


## Historical Notes

This work was originally funded by Digital Cinema Initiatives, LLC
(DCI). Subsequent efforts have been funded by Deluxe Laboratories,
Doremi Labs, CineCert LLC, Avica Technology and others.

The asdcplib project was originally exchanged by FTP. The project was
on [SourceForge](https://sourceforge.net/projects/asdcplib) between
2005 and 2008, when it moved to a release-only distribution via
[CineCert](https://www.cinecert.com/asdcplib/download). As of late
February 2019, its new home is on [github](https://github.com/cinecert/asdcplib).

In the eraliest days, the project depended upon the
[mxflib](http://sourceforge.net/projects/mxflib) project. Because of
its focus on covering the whole of the MXF specifications, mxflib is
considerably larger and more complex that what was required for the
AS-DCP application. For this reason I developed a dedicated MXF
implementation. Special thanks to Matt Beard and Oliver Morgan for
their great work and support.

Thanks also to the members of the SMPTE DC28.20 packaging ad-hoc group
and the members of the MXF Interop Initiative for their encouragement
and support. Special thanks to Jim Whittlesey and Howard Lukk at DCI
for proposing and supporting this project. 
