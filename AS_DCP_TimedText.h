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
#include <list>


#ifndef _AS_DCP_SUBTITLE_H_
#define _AS_DCP_SUBTITLE_H_


namespace ASDCP {

  //
  namespace TimedText
    {
      enum MIMEType_t { MT_BIN, MT_PNG, MT_OPENTYPE };

      struct TimedTextResourceDescriptor
      {
	byte_t      ResourceID[UUIDlen];
	MIMEType_t  Type;

        TimedTextResourceDescriptor() : Type(MT_BIN) {}
      };

      typedef std::list<TimedTextResourceDescriptor> ResourceList_t;

      struct TimedTextDescriptor
      {
	Rational       EditRate;                // 
	ui32_t         ContainerDuration;
	byte_t         AssetID[UUIDlen];
	std::string    NamespaceName; // NULL-terminated string
	std::string    EncodingName;
	ResourceList_t ResourceList;

      TimedTextDescriptor() : ContainerDuration(0), EncodingName("UTF-8") {} // D-Cinema format is always UTF-8
      };

      // Print debugging information to stream (stderr default)
      void   DescriptorDump(const TimedTextDescriptor&, FILE* = 0);

      //
      class FrameBuffer : public ASDCP::FrameBuffer
      {
	ASDCP_NO_COPY_CONSTRUCT(FrameBuffer); // TODO: should have copy construct

      protected:
	byte_t      m_AssetID[UUIDlen];
	std::string m_MIMEType;

      public:
	FrameBuffer() { memset(m_AssetID, 0, UUIDlen); }
	FrameBuffer(ui32_t size) { Capacity(size); memset(m_AssetID, 0, UUIDlen); }
	virtual ~FrameBuffer() {}
        
	inline const byte_t* AssetID() const { return m_AssetID; }
	inline void          AssetID(const byte_t* buf) { memcpy(m_AssetID, buf, UUIDlen); }
	inline const char*   MIMEType() const { return m_MIMEType.c_str(); }
	inline void          MIMEType(const std::string& s) { m_MIMEType = s; }

	// Print debugging information to stream (stderr default)
	void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
      };

      //
      class DCSubtitleParser
	{
	  class h__SubtitleParser;
	  mem_ptr<h__SubtitleParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(DCSubtitleParser);

	public:
	  DCSubtitleParser();
	  virtual ~DCSubtitleParser();

	  // Opens the XML file for reading, parse data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const char* filename) const;

	  // Fill a TimedTextDescriptor struct with the values from the file's contents.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillDescriptor(TimedTextDescriptor&) const;

	  // Reads the complete Timed Text Resource into the given string.
	  Result_t ReadTimedTextResource(std::string&) const;

	  // Reads the Ancillary Resource having the given ID. Fails if the buffer
	  // is too small or the resource does not exist.
	  Result_t ReadAncillaryResource(const byte_t* uuid, FrameBuffer&) const;
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

	  // Writes the Timed-Text Resource to the MXF file. The file must be UTF-8
	  // encoded. If the optional AESEncContext argument is present, the essence
	  // is encrypted prior to writing. Fails if the file is not open, is finalized,
	  // or an operating system error occurs.
	  // This method may only be called once, and it must be called before any
	  // call to WriteAncillaryResource(). RESULT_STATE will be returned if these
	  // conditions are not met.
	  Result_t WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* = 0, HMACContext* = 0);

	  // Writes an Ancillary Resource to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs. RESULT_STATE will be returned if the method is called before
	  // WriteTimedTextResource()
	  Result_t WriteAncillaryResource(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

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

	  // Reads the complete Timed Text Resource into the given string. Fails if the resource
	  // is encrypted and AESDecContext is NULL (use the following method to retrieve the
	  // raw ciphertet block).
	  Result_t ReadTimedTextResource(std::string&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Reads the complete Timed Text Resource from the MXF file. If the optional AESEncContext
	  // argument is present, the resource is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadTimedTextResource(FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Reads the timed-text resource having the given UUID from the MXF file. If the
	  // optional AESEncContext argument is present, the resource is decrypted after
	  // reading. If the MXF file is encrypted and the AESDecContext argument is NULL,
	  // the frame buffer will contain the ciphertext frame data. If the HMACContext
	  // argument is not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadAncillaryResource(const byte_t* uuid, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

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
