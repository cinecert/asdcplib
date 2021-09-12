/*
Copyright (c) 2011-2018, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
Copyright (c) 2020-2021, Thomas Richter Fraunhofer IIS,
John Hurst

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
/*! \file    AS_02.h
    \version $Id$       
    \brief   AS-02 library, public interface

This module implements the JPEG XS specific wrapping of IMF

 o Write essence to a plaintext or ciphertext AS-02 file:
     JPEG XS codestreams

 o Read essence from a plaintext or ciphertext AS-02 file:
     JPEG XS codestreams

 o Read header metadata from an AS-02 file

NOTE: ciphertext support for clip-wrapped PCM is not yet complete.
*/

#ifndef _AS_02_JXS_H_
#define _AS_02_JXS_H_

#include "Metadata.h"
#include "AS_02.h"
#include "JXS.h"

namespace AS_02
{
  using Kumu::Result_t;

  namespace JXS
  {
	  //
	  class MXFWriter
	  {
		  class h__Writer;
		  ASDCP::mem_ptr<h__Writer> m_Writer;
		  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	  public:
		  MXFWriter();
		  virtual ~MXFWriter();

		  // Warning: direct manipulation of MXF structures can interfere
		  // with the normal operation of the wrapper.  Caveat emptor!
		  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
		  virtual ASDCP::MXF::RIP& RIP();

		  // Open the file for writing. The file must not exist. Returns error if
		  // the operation cannot be completed or if nonsensical data is discovered
		  // in the essence descriptor.
		  Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo&,
                          ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
                          ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_sub_descriptor,
			  const ASDCP::Rational& edit_rate, const ui32_t& header_size = 16384,
			  const IndexStrategy_t& strategy = IS_FOLLOW, const ui32_t& partition_space = 10);

		  // Writes a frame of essence to the MXF file. If the optional AESEncContext
		  // argument is present, the essence is encrypted prior to writing.
		  // Fails if the file is not open, is finalized, or an operating system
		  // error occurs.
		  Result_t WriteFrame(const ASDCP::JXS::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

		  // Closes the MXF file, writing the index and revised header.
		  Result_t Finalize();
	  };

	  //
	  class MXFReader
	  {
		  class h__Reader;
		  ASDCP::mem_ptr<h__Reader> m_Reader;
		  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	  public:
		  MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory);
		  virtual ~MXFReader();

		  // Warning: direct manipulation of MXF structures can interfere
		  // with the normal operation of the wrapper.  Caveat emptor!
		  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
		  virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
		  virtual ASDCP::MXF::RIP& RIP();

		  // Open the file for reading. The file must exist. Returns error if the
		  // operation cannot be completed.
		  Result_t OpenRead(const std::string& filename) const;

		  // Returns RESULT_INIT if the file is not open.
		  Result_t Close() const;

		  // Fill a WriterInfo struct with the values from the file's header.
		  // Returns RESULT_INIT if the file is not open.
		  Result_t FillWriterInfo(ASDCP::WriterInfo&) const;
		  
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
		  Result_t ReadFrame(ui32_t frame_number, ASDCP::JXS::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

		  // Print debugging information to stream
		  void     DumpHeaderMetadata(FILE* = 0) const;
		  void     DumpIndex(FILE* = 0) const;
	  };

  } //namespace JXS
}

///
#endif
