/*
Copyright (c) 2011-2016, John Hurst

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

/*! \file    AS_02_PHDR.h
  \version $Id$
  \brief   AS-02 library, JPEG 2000 P-HDR essence reader and writer implementation
*/


#ifndef _AS_02_PHDR_H_
#define _AS_02_PHDR_H_

#include "AS_02.h"

namespace AS_02
{

  namespace PHDR
  { 
    //
    class FrameBuffer : public ASDCP::JP2K::FrameBuffer
      {
      public:
	std::string OpaqueMetadata;

	FrameBuffer() {}
	FrameBuffer(ui32_t size) { Capacity(size); }
	virtual ~FrameBuffer() {}

	// Print debugging information to stream (stderr default)
	void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
      };

    // An object which reads a sequence of files containing
    // JPEG 2000 P-HDR pictures and metadata.
    class SequenceParser
    {
      class h__SequenceParser;
      Kumu::mem_ptr<h__SequenceParser> m_Parser;
      ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

    public:
      SequenceParser();
      virtual ~SequenceParser();

      // Opens a directory for reading.  The directory is expected to contain one or
      // more pairs of files, each containing the codestream for exactly one picture (.j2c)
      // and the corresponding P-HDR metadata (.xml). The files must be named such that the
      // frames are in temporal order when sorted alphabetically by filename. The parser
      // will automatically parse enough data from the first file to provide a complete set
      // of stream metadata for the MXFWriter below. The contents of the metadata files will
      // not be analyzed (i.e., the raw bytes will be passed in without scrutiny.) If the
      // "pedantic" parameter is given and is true, the J2C parser will check the JPEG 2000
      // metadata for each codestream and fail if a mismatch is detected.
      Result_t OpenRead(const std::string& filename, bool pedantic = false) const;

      // Opens a file sequence for reading.  The sequence is expected to contain one or
      // more pairs of filenames, each naming a file containing the codestream (.j2c) and the
      // corresponding P-HDR metadata (.xml) for exactly one picture. The parser will 
      // automatically parse enough data from the first file to provide a complete set of
      // stream metadata for the MXFWriter below.  If the "pedantic" parameter is given and
      // is true, the parser will check the metadata for each codestream and fail if a
      // mismatch is detected.
      Result_t OpenRead(const std::list<std::string>& file_list, bool pedantic = false) const;

      // Fill a PictureDescriptor struct with the values from the first file's codestream.
      // Returns RESULT_INIT if the directory is not open.
      Result_t FillPictureDescriptor(ASDCP::JP2K::PictureDescriptor&) const;

      // Rewind the directory to the beginning.
      Result_t Reset() const;

      // Reads the next sequential frame in the directory and places it in the frame buffer.
      // Fails if the buffer is too small or the direcdtory contains no more files. The frame
      // buffer's PlaintextOffset parameter will be set to the first byte of the data segment.
      // Set this value to zero if you want encrypted headers.
      Result_t ReadFrame(AS_02::PHDR::FrameBuffer&) const;
    };

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
			 ASDCP::MXF::FileDescriptor* essence_descriptor,
			 ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
			 const ASDCP::Rational& edit_rate, const ui32_t& header_size = 16384,
			 const IndexStrategy_t& strategy = IS_FOLLOW, const ui32_t& partition_space = 10);

      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const AS_02::PHDR::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

      // Closes the MXF file, writing the final index, the PHDR master metadata and the revised header.
      Result_t Finalize(const std::string& PHDR_master_metadata);
    };

    //
    class MXFReader
    {
      class h__Reader;
      ASDCP::mem_ptr<h__Reader> m_Reader;
      ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed. If master metadata is available it will
      // be placed into the string object passed as the second argument.
      Result_t OpenRead(const std::string& filename, std::string& PHDR_master_metadata) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill a WriterInfo struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

      // Reads a frame of essence from the MXF file. If the optional AESEncContext
      // argument is present, the essence is decrypted after reading. If the MXF
      // file is encrypted and the AESDecContext argument is NULL, the frame buffer
      // will contain the ciphertext frame data. If the HMACContext argument is
      // not NULL, the HMAC will be calculated (if the file supports it).
      // Returns RESULT_INIT if the file is not open, failure if the frame number is
      // out of range, or if optional decrypt or HAMC operations fail.
      Result_t ReadFrame(ui32_t frame_number, AS_02::PHDR::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
    
  } // end namespace PHDR

  namespace PIDM
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
			 const ASDCP::UL& data_essence_coding,
			 const ASDCP::Rational& edit_rate, const ui32_t& header_size = 16384,
			 const IndexStrategy_t& strategy = IS_FOLLOW, const ui32_t& partition_space = 10);

      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const ASDCP::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

      // Closes the MXF file, writing the index and revised header. No global metadata block is written.
      Result_t Finalize();

      // Closes the MXF file, writing the global metadata block and then final index and revised header.
      Result_t Finalize(const ASDCP::FrameBuffer& global_metadata);
    };

    //
    class MXFReader
    {
      class h__Reader;
      ASDCP::mem_ptr<h__Reader> m_Reader;
      ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
      virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
      virtual ASDCP::MXF::RIP& RIP();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed.
      Result_t OpenRead(const std::string& filename) const;

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed. If global metadata is available it will
      // be placed into the buffer object passed as the second argument.
      Result_t OpenRead(const std::string& filename, ASDCP::FrameBuffer& global_metadata) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill a WriterInfo struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

      // Reads a frame of essence from the MXF file. If the optional AESEncContext
      // argument is present, the essence is decrypted after reading. If the MXF
      // file is encrypted and the AESDecContext argument is NULL, the frame buffer
      // will contain the ciphertext frame data. If the HMACContext argument is
      // not NULL, the HMAC will be calculated (if the file supports it).
      // Returns RESULT_INIT if the file is not open, failure if the frame number is
      // out of range, or if optional decrypt or HAMC operations fail.
      Result_t ReadFrame(ui32_t frame_number, ASDCP::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
    
  }

} // end namespace AS_02

#endif // _AS_02_PHDR_H_

//
// end AS_02_PHDR.h
//
