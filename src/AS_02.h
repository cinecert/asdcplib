/*
Copyright (c) 2011-2012, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst
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

This module implements MXF AS-02 is a set of file access objects that
offer simplified access to files conforming to the standards published
by the SMPTE Media and Packaging Technology Committee 35PM. The file
format, labeled IMF Essence Component (AKA "AS-02" for historical
reasons), is described in the following document:

 o SMPTE 2067-5:201X (draft at this time) IMF Essence Component

The following use cases are supported by the module:

 o Write essence to a plaintext or ciphertext AS-02 file:
     JPEG 2000 codestreams
     PCM audio streams

 o Read essence from a plaintext or ciphertext AS-02 file:
     JPEG 2000 codestreams
     PCM audio streams

 o Read header metadata from an AS-02 file
*/

#ifndef _AS_02_H_
#define _AS_02_H_

#include "AS_DCP.h"
#include "MXF.h"


namespace ASDCP {
  namespace MXF {
    // #include<Metadata.h> to use this
    class OPAtomHeader;
  };
};

namespace AS_02
{
  using Kumu::Result_t;

  namespace MXF {

    //
    class OP1aIndexBodyPartion : public ASDCP::MXF::Partition
    {
      ASDCP::MXF::IndexTableSegment*  m_CurrentSegment;
      ASDCP::FrameBuffer  m_Buffer;
      ASDCP::MXF::Rational m_EditRate;
      ui32_t				klv_fill_size;

      ASDCP_NO_COPY_CONSTRUCT(OP1aIndexBodyPartion);

    public:
      const ASDCP::Dictionary*&  m_Dict;
      Kumu::fpos_t        m_ECOffset;
      ASDCP::IPrimerLookup*      m_Lookup;

      ui32_t             m_BytesPerEditUnit;
      ui32_t             m_FramesWritten;
      
      OP1aIndexBodyPartion(const ASDCP::Dictionary*&);
      virtual ~OP1aIndexBodyPartion();
      virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToFile(Kumu::FileWriter& Writer, ASDCP::UL& PartitionLabel);
      virtual void     Dump(FILE* = 0);

      virtual bool Lookup(ui32_t frame_num, ASDCP::MXF::IndexTableSegment::IndexEntry&,ui32_t&) const;
      virtual void PushIndexEntry(const ASDCP::MXF::IndexTableSegment::IndexEntry&);
      virtual void SetIndexParamsCBR(ASDCP::IPrimerLookup* lookup, ui32_t size, const ASDCP::MXF::Rational& Rate);
      virtual void SetIndexParamsVBR(ASDCP::IPrimerLookup* lookup, const ASDCP::MXF::Rational& Rate, Kumu::fpos_t offset);

      //new
      virtual ui64_t Duration();
      virtual Result_t FillWriteToFile(Kumu::FileWriter& Writer, ui32_t numberOfIndexEntries);

      //new for PCM
      virtual void PCMSetIndexParamsCBR(ui32_t sizeFirst, ui32_t sizeOthers);
      virtual void PCMIndexLookup(ui32_t frame_num, ASDCP::MXF::IndexTableSegment::IndexEntry& Entry) const;
      
    };

    //
    class OP1aIndexFooter : public ASDCP::MXF::Partition
    {
      ASDCP::MXF::IndexTableSegment*  m_CurrentSegment;
      ASDCP::FrameBuffer  m_Buffer;
      ui32_t              m_BytesPerEditUnit;
      ASDCP::MXF::Rational m_EditRate;
      ui32_t              m_BodySID;
      ASDCP_NO_COPY_CONSTRUCT(OP1aIndexFooter);

    public:
      const ASDCP::Dictionary*&   m_Dict;
      Kumu::fpos_t        m_ECOffset;
      ASDCP::IPrimerLookup*      m_Lookup;

      OP1aIndexFooter(const ASDCP::Dictionary*&);
      virtual ~OP1aIndexFooter();
      virtual Result_t InitFromFile(const Kumu::FileReader& Reader);
      virtual Result_t InitFromPartitionBuffer(const byte_t* p, ui32_t l);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToFile(Kumu::FileWriter& Writer, ui64_t duration);
      virtual void     Dump(FILE* = 0);

      virtual Result_t Lookup(ui32_t frame_num, ASDCP::MXF::IndexTableSegment::IndexEntry&) const;
      virtual void     PushIndexEntry(const ASDCP::MXF::IndexTableSegment::IndexEntry&);
      virtual void     SetIndexParamsCBR(ASDCP::IPrimerLookup* lookup, ui32_t size, const ASDCP::MXF::Rational& Rate);
      virtual void     SetIndexParamsVBR(ASDCP::IPrimerLookup* lookup, const ASDCP::MXF::Rational& Rate, Kumu::fpos_t offset);
    };

  } // namespace MXF


  //---------------------------------------------------------------------------------
  // Accessors in the MXFReader and MXFWriter classes below return these types to
  // provide direct access to MXF metadata structures declared in MXF.h and Metadata.h

  // See ST 2067-5 Sec. x.y.z
  enum IndexStrategy_t
  {
    IS_LEAD,
    IS_FOLLOW,
    IS_FILE_SPECIFIC,
  };
 
  namespace JP2K
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
      virtual ASDCP::MXF::OPAtomHeader& OPAtomHeader();

      // Open the file for writing. The file must not exist. Returns error if
      // the operation cannot be completed or if nonsensical data is discovered
      // in the essence descriptor.
      Result_t OpenWrite(const char* filename, const ASDCP::WriterInfo&,
			 const ASDCP::JP2K::PictureDescriptor&,
			 const IndexStrategy_t& Strategy = IS_FOLLOW,
			 const ui32_t& PartitionSpace = 60, /* seconds per partition */
			 const ui32_t& HeaderSize = 16384);
      
      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const ASDCP::JP2K::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

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
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OPAtomHeader& OPAtomHeader();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed.
      Result_t OpenRead(const char* filename) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill an AudioDescriptor struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillPictureDescriptor(ASDCP::JP2K::PictureDescriptor&) const;

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
      Result_t ReadFrame(ui32_t frame_number, ASDCP::JP2K::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
    
  } //namespace JP2K
  

  //---------------------------------------------------------------------------------
  //
  namespace PCM
  {
    // see AS_DCP.h for related data types

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
      virtual ASDCP::MXF::OPAtomHeader& OPAtomHeader();

      // Open the file for writing. The file must not exist. Returns error if
      // the operation cannot be completed or if nonsensical data is discovered
      // in the essence descriptor.
      Result_t OpenWrite(const char* filename, const ASDCP::WriterInfo&,
				const ASDCP::PCM::AudioDescriptor&, ui32_t HeaderSize = 16384);

      // Writes a frame of essence to the MXF file. If the optional AESEncContext
      // argument is present, the essence is encrypted prior to writing.
      // Fails if the file is not open, is finalized, or an operating system
      // error occurs.
      Result_t WriteFrame(const ASDCP::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);
      
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
      MXFReader();
      virtual ~MXFReader();

      // Warning: direct manipulation of MXF structures can interfere
      // with the normal operation of the wrapper.  Caveat emptor!
      virtual ASDCP::MXF::OPAtomHeader& OPAtomHeader();

      // Open the file for reading. The file must exist. Returns error if the
      // operation cannot be completed.
      Result_t OpenRead(const char* filename) const;

      // Returns RESULT_INIT if the file is not open.
      Result_t Close() const;

      // Fill an AudioDescriptor struct with the values from the file's header.
      // Returns RESULT_INIT if the file is not open.
      Result_t FillAudioDescriptor(ASDCP::PCM::AudioDescriptor&) const;

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
      Result_t ReadFrame(ui32_t frame_number, ASDCP::PCM::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;
      
      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };
  } // namespace PCM



} // namespace AS_02

#endif // _AS_02_H_

//
// end AS_02.h
//
