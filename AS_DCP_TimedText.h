/*
Copyright (c) 2003-2006, John Hurst
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
/*! \file    AS_DCP_Subtitle.h
    \version $Id$       
    \brief   experimental AS-DCP subtitle

    Implements Draft S429-5
*/

#include <AS_DCP.h>


#ifndef _AS_DCP_SUBTITLE_H_
#define _AS_DCP_SUBTITLE_H_


namespace ASDCP {

  //
  namespace TimedText
    {
      struct TimedTextDescriptor
      {
	std::string NamespaceName;
	ui32_t      ResourceCount;
      };

      // Print debugging information to stream (stderr default)
      void   DescriptorDump(const TimedTextDescriptor&, FILE* = 0);

      //
      class FrameBuffer : public ASDCP::FrameBuffer
	{
	  ASDCP_NO_COPY_CONSTRUCT(FrameBuffer); // TODO: should have copy construct

	protected:
	  std::string m_MIMEType;
	  byte_t      m_AssetID[UUIDlen];

	public:
	  FrameBuffer() { memset(m_AssetID, 0, UUIDlen); }
	  FrameBuffer(ui32_t size) { Capacity(size); memset(m_AssetID, 0, UUIDlen); }
	  virtual ~FrameBuffer() {}
	
	  const char*   MIMEType() { return m_MIMEType.c_str(); }
	  void          MIMEType(const char* s) { m_MIMEType = s; }
	  const byte_t* AssetID() { return m_AssetID; }
	  void          AssetID(const byte_t* buf) { memcpy(m_AssetID, buf, UUIDlen); }

	  // Print debugging information to stream (stderr default)
	  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	};

      //
      // NOTE: The ReadFrame() and WriteFrame() methods below do not stricly handle "frames".
      // They are named for continuity with other AS-DCP lib modules. The methods actually
      // handle complete assets, such as XML documents, PNG images and fonts.
      //

      // An object which opens and reads a SMPTE 428-7 (or T.I. CineCanvas (TM)) subtitle file.
      // The call to OpenRead() reads metadata from the file and populates an internal TimedTextDescriptor
      // object. The first call to ReadFrame() returns the XML document. Each subsequent call to
      // ReadFrame() reads exactly one resource file (if any) referenced by the XML into the given
      // FrameBuffer object.
      class DCSubtitleParser
	{
	  class h__DCSubtitleParser;
	  mem_ptr<h__DCSubtitleParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(DCSubtitleParser);

	public:
	  DCSubtitleParser();
	  virtual ~DCSubtitleParser();

	  // Opens the XML file for reading, parses enough data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const char* filename) const;

	  // Fill a TimedTextDescriptor struct with the values from the file's contents.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillDescriptor(TimedTextDescriptor&) const;

	  // Reset the contents of the internal TimedTextDescriptor
	  Result_t Reset() const;

	  // Reads the next subtitle resource in the list taken from the XML file.
	  // Fails if the buffer is too small or the file list has been exhausted.
	  // The XML file itself is returned in the first call.
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

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const char* filename, const WriterInfo&,
			     const TimedTextDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a timed-text resource to the MXF file. If the optional AESEncContext
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

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const char* filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a TimedTextDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillDescriptor(TimedTextDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a timed-text resource from the MXF file. If the optional AESEncContext
	  // argument is present, the resource is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace TimedText



} // namespace ASDCP


#endif // _AS_DCP_SUBTITLE_H_


//
// end AS_DCP_Subtitle.h
//
