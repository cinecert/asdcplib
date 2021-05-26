/*
Copyright (c) 2003-2018, John Hurst,
Copyright (c) 2020-2021, Christian Minuth, Thomas Richter Fraunhofer IIS,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    AS_DCP.h
    \version $Id$
    \brief   AS-DCP library, public interface

The asdcplib library is a set of file access objects that offer simplified
access to files conforming to the standards published by the SMPTE
D-Cinema Technology Committee 21DC. The file format, labeled AS-DCP,
is described in a series of documents which includes but may not
be be limited to:

 o SMPTE ST 429-2:2011 DCP Operational Constraints
 o SMPTE ST 429-3:2006 Sound and Picture Track File
 o SMPTE ST 429-4:2006 MXF JPEG 2000 Application
 o SMPTE ST 429-5:2009 Timed Text Track File
 o SMPTE ST 429-6:2006 MXF Track File Essence Encryption
 o SMPTE ST 429-10:2008 Stereoscopic Picture Track File
 o SMPTE ST 429-14:2008 Aux Data Track File
 o SMPTE ST 330:2004 - UMID
 o SMPTE ST 336:2001 - KLV
 o SMPTE ST 377:2004 - MXF (old version, required)
 o SMPTE ST 377-1:2011 - MXF
 o SMPTE ST 377-4:2012 - MXF Multichannel Audio Labeling Framework
 o SMPTE ST 390:2011 - MXF OP-Atom
 o SMPTE ST 379-1:2009 - MXF Generic Container (GC)
 o SMPTE ST 381-1:2005 - MPEG2 picture in GC
 o SMPTE ST 422:2006 - JPEG 2000 picture in GC
 o SMPTE ST 382:2007 - WAV/PCM sound in GC
 o IETF RFC 2104 - HMAC/SHA1
 o NIST FIPS 197 - AES (Rijndael) (via OpenSSL)

 o MXF Interop Track File Specification
 o MXF Interop Track File Essence Encryption Specification
 o MXF Interop Operational Constraints Specification
 - Note: the MXF Interop documents are not formally published.
   Contact the asdcplib support address to get copies.

The following use cases are supported by the library:

 o Write essence to a plaintext or ciphertext AS-DCP file:
 o Read essence from a plaintext or ciphertext AS-DCP file:
     JPEG XS codestreams

 o Read header metadata from an AS-DCP file

This project depends upon the following libraries:
 - OpenSSL http://www.openssl.org/
 - Expat http://expat.sourceforge.net/  or
     Xerces-C http://xerces.apache.org/xerces-c/
   An XML library is not needed if you don't need support for SMPTE ST 429-5:2009.
*/

#ifndef _AS_DCP_JXS_H_
#define _AS_DCP_JXS_H_

#include <KM_error.h>
#include <KM_fileio.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <iosfwd>
#include <string>
#include <cstring>
#include <list>
#include "AS_DCP.h"
#include "Metadata.h"

//--------------------------------------------------------------------------------
// All library components are defined in the namespace ASDCP
//
namespace ASDCP {
  //
  //---------------------------------------------------------------------------------
  //
  namespace JXS 
  {
	  const ui32_t MaxComponents = 4; // ISO 21122-1 Annex A.2 up to 8 components
	  const ui32_t MaxHorizontalLevels = 15;
	  const ui32_t MaxVerticalLevels = 2;

#pragma pack(1)
	  struct ImageComponent_t  // Essentially, a lookalike of the CDT marker, just with less bit-packing
	  {
		  ui8_t Bc;  // Bitdepth (literal, not -1)
		  ui8_t Sx;
		  ui8_t Sy;  // Subsampling factors, horizontal and vertically. Bit-packed in the marker.
	  };
#pragma pack()

	  struct PictureDescriptor
	  {
		  Rational       EditRate;
		  ui32_t         ContainerDuration;
		  Rational       SampleRate;
		  ui32_t         StoredWidth;
		  ui32_t         StoredHeight;
		  Rational       AspectRatio;
		  ui16_t         Ppih; // Profile, copy from the PIH marker
		  ui16_t         Plev; // Level and sublevel, copy from the PIH marker
		  ui16_t         Wf;  // Frame width, copy from the PIH marker
		  ui16_t         Hf;  // Frame height, copy from the PIH marker
		  ui16_t         Hsl; // slice height, copy from the PIH marker
		  ui16_t         Cw;  // column width, or 0 for no columns, copy from the PIH marker
		  ui8_t          Nc;  // number of components, copy from the PIH marker
	          ui32_t         MaximumBitRate; // bit rate in MB/sec, or 0 if not known
	          ui8_t          Primaries; // Color primaries as defined by CICP
	          ui8_t          TransferCurve; // Transfer curve as defined by CICP
	          ui8_t          Matrix; // Transform matrix, as defined by CICP
	          bool           fullRange; // If true, no head and toe region
		  ImageComponent_t ImageComponents[MaxComponents]; // These are copies from the CDT (component table)
	  };
	  // Print debugging information to std::ostream
	  std::ostream& operator << (std::ostream& strm, const PictureDescriptor& pdesc);
	  // Print debugging information to stream (stderr default)
	  void   PictureDescriptorDump(const PictureDescriptor&, FILE* = 0);

	  //
	  class FrameBuffer : public ASDCP::FrameBuffer
	  {
	  public:
		  FrameBuffer() {}
		  FrameBuffer(ui32_t size) { Capacity(size); }
		  virtual ~FrameBuffer() {}

		  // Print debugging information to stream (stderr default)
		  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	  };


	  // An object which opens and reads a JPEG XS codestream file.  The file is expected
	  // to contain exactly one complete frame of picture essence as an unwrapped (raw)
	  // ISO/IEC 21122 codestream.
	  class CodestreamParser
	  {
		  class h__CodestreamParser;
		  mem_ptr<h__CodestreamParser> m_Parser;
		  ASDCP_NO_COPY_CONSTRUCT(CodestreamParser);

	  public:
		  CodestreamParser();
		  virtual ~CodestreamParser();

		  // Opens a file for reading, parses enough data to provide a complete
			  // set of stream metadata for the MXFWriter below.
		  // The frame buffer's PlaintextOffset parameter will be set to the first
		  // byte of the data segment. Set this value to zero if you want
		  // encrypted headers.
		  Result_t OpenReadFrame(const std::string& filename, FrameBuffer&) const;

		  // Fill a PictureDescriptor struct with the values from the file's codestream.
		  // Returns RESULT_INIT if the file is not open.
		  Result_t FillPictureDescriptor(PictureDescriptor&) const;
	  };

	  // Parses the data in the frame buffer to fill in the picture descriptor. Copies
	  // the offset of the image data into start_of_data. Returns error if the parser fails.
	  Result_t ParseMetadataIntoDesc(const FrameBuffer&, PictureDescriptor&, byte_t* start_of_data = 0);

	  // An object which reads a sequence of files containing JPEG XS pictures.
	  class SequenceParser
	  {
		  class h__SequenceParser;
		  mem_ptr<h__SequenceParser> m_Parser;
		  ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

	  public:
		  SequenceParser();
		  virtual ~SequenceParser();

		  // Opens a directory for reading.  The directory is expected to contain one or
		  // more files, each containing the codestream for exactly one picture. The
		  // files must be named such that the frames are in temporal order when sorted
		  // alphabetically by filename. The parser will automatically parse enough data
		  // from the first file to provide a complete set of stream metadata for the
		  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
		  // parser will check the metadata for each codestream and fail if a
		  // mismatch is detected.
		  Result_t OpenRead(const std::string& filename) const;

		  // Opens a file sequence for reading.  The sequence is expected to contain one or
		  // more filenames, each naming a file containing the codestream for exactly one
		  // picture. The parser will automatically parse enough data
		  // from the first file to provide a complete set of stream metadata for the
		  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
		  // parser will check the metadata for each codestream and fail if a
		  // mismatch is detected.
		  Result_t OpenRead(const std::list<std::string>& file_list) const;

		  // Fill a PictureDescriptor struct with the values from the first file's codestream.
		  // Returns RESULT_INIT if the directory is not open.
		  Result_t FillPictureDescriptor(PictureDescriptor&) const;

		  // Rewind the directory to the beginning.
		  Result_t Reset() const;

		  // Reads the next sequential frame in the directory and places it in the
		  // frame buffer. Fails if the buffer is too small or the direcdtory
		  // contains no more files.
		  // The frame buffer's PlaintextOffset parameter will be set to the first
		  // byte of the data segment. Set this value to zero if you want
		  // encrypted headers.
		  Result_t ReadFrame(FrameBuffer&) const;
	  };


	  //
	  class MXFWriter
	  {
		  class h__Writer;
		  mem_ptr<h__Writer> m_Writer;
		  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	  public:
		  MXFWriter();
		  virtual ~MXFWriter();

		  // Warning: direct manipulation of MXF structures can interfere
		  // with the normal operation of the wrapper.  Caveat emptor!
		  virtual MXF::OP1aHeader& OP1aHeader();
		  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
		  virtual MXF::RIP& RIP();

		  // Open the file for writing. The file must not exist. Returns error if
		  // the operation cannot be completed or if nonsensical data is discovered
		  // in the essence descriptor.
		  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			  const PictureDescriptor&, ui32_t HeaderSize = 16384);

		  // Writes a frame of essence to the MXF file. If the optional AESEncContext
		  // argument is present, the essence is encrypted prior to writing.
		  // Fails if the file is not open, is finalized, or an operating system
		  // error occurs.
		  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

		  // Closes the MXF file, writing the index and revised header.
		  Result_t Finalize();
	  };

	  //
	  class MXFReader
	  {
		  class h__Reader;
		  mem_ptr<h__Reader> m_Reader;
		  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	  public:
		  MXFReader();
		  virtual ~MXFReader();

		  // Warning: direct manipulation of MXF structures can interfere
		  // with the normal operation of the wrapper.  Caveat emptor!
		  virtual MXF::OP1aHeader& OP1aHeader();
		  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
		  virtual MXF::RIP& RIP();

		  // Open the file for reading. The file must exist. Returns error if the
		  // operation cannot be completed.
		  Result_t OpenRead(const std::string& filename) const;

		  // Returns RESULT_INIT if the file is not open.
		  Result_t Close() const;

		  // Fill an AudioDescriptor struct with the values from the file's header.
		  // Returns RESULT_INIT if the file is not open.
		  Result_t FillPictureDescriptor(PictureDescriptor&) const;

		  // Fill a WriterInfo struct with the values from the file's header.
		  // Returns RESULT_INIT if the file is not open.
		  Result_t FillWriterInfo(WriterInfo&) const;

		  // Compute the necessary frame buffer size in bytes for CBR
		  // codestreamss.
		  Result_t CalcFrameBufferSize(ui64_t &size) const;
	  
		  // Reads a frame of essence from the MXF file. If the optional AESEncContext
		  // argument is present, the essence is decrypted after reading. If the MXF
		  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
		  // will contain the ciphertext frame data. If the HMACContext argument is
		  // not NULL, the HMAC will be calculated (if the file supports it).
		  // Returns RESULT_INIT if the file is not open, failure if the frame number is
		  // out of range, or if optional decrypt or HAMC operations fail.
		  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

		  // Using the index table read from the footer partition, lookup the frame number
		  // and return the offset into the file at which to read that frame of essence.
		  // Returns RESULT_INIT if the file is not open, and RESULT_FRAME if the frame number is
		  // out of range.
		  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

		  // Print debugging information to stream
		  void     DumpHeaderMetadata(FILE* = 0) const;
		  void     DumpIndex(FILE* = 0) const;
	  };
  }
  Result_t MD_to_JXS_PDesc(const ASDCP::MXF::GenericPictureEssenceDescriptor&  EssenceDescriptor,
			   const ASDCP::MXF::JPEGXSPictureSubDescriptor& EssenceSubDescriptor,
			   const ASDCP::Rational& EditRate, const ASDCP::Rational& SampleRate,
			   ASDCP::JXS::PictureDescriptor& PDesc);

  Result_t JXS_PDesc_to_MD(const JXS::PictureDescriptor& PDesc,
			   const ASDCP::Dictionary& dict,
			   ASDCP::MXF::GenericPictureEssenceDescriptor& EssenceDescriptor,
			   ASDCP::MXF::JPEGXSPictureSubDescriptor& EssenceSubDescriptor);
};

///
#endif
