/*
Copyright (c), Bjoern Stresing, Patrick Bichiou, Wolfgang Ruppel,
John Hurst, Pierre-Anthony Lemieux

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

/** 
 * Reader and writer classes for IAB Track Files, as defined in SMPTE ST 2067-201
 * 
 * @file
 */

#ifndef AS_02_IAB_h__
#define AS_02_IAB_h__

#include "AS_02.h"
#include "Metadata.h"

namespace AS_02 {

  namespace IAB {

    /** 
     * Writes IAB Track Files as specified in ST SMPTE 2067-201
     * 
     */
    class MXFWriter {
      class h__Writer;
      ASDCP::mem_ptr<h__Writer> m_Writer;
      ui64_t m_ClipStart;

      void Reset();

      ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

    public:
      MXFWriter();
      virtual ~MXFWriter();

      /**
       * Must be preceded by a succesful OpenWrite() call followed by zero or more WriteFrame() calls
       *
       * Warning: direct manipulation of MXF structures can interfere
       * with the normal operation of the wrapper.  Caveat emptor!
       *
       * @return Header of the Track File
       *
       * @throws std::runtime_error if the Track File is not open
       */
      virtual const ASDCP::MXF::OP1aHeader& OP1aHeader() const;

      /**
       * Must be preceded by a succesful OpenWrite() call followed by zero or more WriteFrame() calls
       * 
       * Warning: direct manipulation of MXF structures can interfere 
       * with the normal operation of the wrapper.  Caveat emptor!
       * 
       * @return RIP of the Track File
       * 
       * @throws std::runtime_error if the Track File is not open
       */
      virtual const ASDCP::MXF::RIP& RIP() const;

      /**
       * Creates and prepares an IAB Track File for writing.
       * 
       * Must be called following instantiation or after Finalize() call
       * 
       * @param filename Path to the file. The file must no exist.
       * @param Info MXF file metadata to be written.
       * @param sub IAB Soundfield Subdescritor items to be written. MCATagName, MCATagSymbol, MCALabelDictionaryID, MCALinkID are ignored.
       * @param conformsToSpecs Value of the ConformsToSpecifications preface item
       * @param edit_rate Frame rate of the IA Bitstream
       * @param sampling_rate Sampling rate of the audio essence within the IA Bitstream
       * 
       * @return RESULT_OK indicates that frames are ready to be written,
       * otherwise the reader is reset and the file is left is an undermined state.
       */
      Result_t OpenWrite(
        const std::string& filename,
        const ASDCP::WriterInfo& Info,
        const ASDCP::MXF::IABSoundfieldLabelSubDescriptor& sub,
        const std::vector<ASDCP::UL>& conformsToSpecs,
        const ASDCP::Rational& edit_rate,
        const ASDCP::Rational& sampling_rate = ASDCP::SampleRate_48k
      );

      /**
       * Writes a single frame.
       * 
       * Must be preceded by a succesful OpenWrite() call followed by zero or more WriteFrame() calls
       *
       *
       * @param frame Pointer to a complete IA Frame
       * @param sz Size in bytes of the IA Frame
       * @return RESULT_OK indicates that the frame is written and additional frames can be written, 
       * otherwise the reader is reset and the file is left is an undermined state.
       */
      Result_t WriteFrame(const ui8_t* frame, ui32_t sz);

      /**
       * Writes a single frame.
       * 
       * Must be preceded by a succesful OpenWrite() call followed by zero or more WriteFrame() calls
       *
       *
       * @param frame a complete IA Frame
       * @return RESULT_OK indicates that the frame is written and additional frames can be written, 
       * otherwise the reader is reset and the file is left is an undermined state.
       */
      Result_t WriteFrame(const ASDCP::FrameBuffer& frame);

      /**
       * Writes an XML text document to the MXF file as per RP 2057. If the
       * optional AESEncContext argument is present, the document is encrypted
       * prior to writing. Fails if the file is not open, is finalized, or an
       * operating system error occurs.
       */
      Result_t AddDmsGenericPartUtf8Text(const ASDCP::FrameBuffer& frame_buffer, ASDCP::AESEncContext* enc = 0, ASDCP::HMACContext* hmac = 0,
                                         const std::string& trackDescription = "Descriptive Track", const std::string& dataDescription = "");

      /**
       * Writes the Track File footer and closes the file.
       * 
       * Must be preceded by a succesful OpenWrite() call followed by zero or more WriteFrame() calls
       * 
       * @return RESULT_OK indicates that the frame is written and additional frames can be written, 
       * otherwise the reader is reset and the file is left is an undermined state.
       */
      Result_t Finalize();
    };

    /**
     * Reads IAB Track Files as specified in ST SMPTE 2067-201
     *
     */
    class MXFReader {
      class h__Reader;
      ASDCP::mem_ptr<h__Reader> m_Reader;

      ASDCP::FrameBuffer m_FrameBuffer;

      const Kumu::IFileReaderFactory& m_FileReaderFactory;

      void Reset();

      ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:

      /* typedefs*/

      typedef std::pair<size_t, const ui8_t*> Frame;

      /* methods */

      /**
       * Construct MXF Reader
       * .
       * @param fileReaderFactory Abstract interface that allows
       * to override asdcplib's file read access by a user implementation.
       * Notice that the factory object reference needs to remain valid
       * when performing OpenRead operation.
       */
      MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory);

      virtual ~MXFReader();

      /**
       * Warning: direct manipulation of MXF structures can interfere
       * with the normal operation of the wrapper.  Caveat emptor!
       *
       * @return Header of the Track File
       *
       * @throws std::runtime_error if the Track File is not open
       */
      virtual ASDCP::MXF::OP1aHeader& OP1aHeader() const;

      /**
       * Warning: direct manipulation of MXF structures can interfere
       * with the normal operation of the wrapper.  Caveat emptor!
       *
       * @return RIP of the Track File
       *
       * @throws std::runtime_error if the Track File is not open
       */
      virtual const ASDCP::MXF::RIP& RIP() const;

      /**
       * Creates and prepares an IAB Track File for reading.
       *
       * @param filename Path to the file. The file must no exist.
       *
       * @return RESULT_OK indicates that frames are ready to be read,
       * otherwise the reader is reset
       */
      Result_t OpenRead(const std::string& filename);

      /**
       * Closes the IAB Track File.
       *
       * @return RESULT_OK indicates that the Track File was successfully closed.
       */
      Result_t Close();

      /**
       * Fill a WriterInfo struct with the values from the Track File's header.
       *
       * @param  writer_info Struct to be filled
       * @return RESULT_OK indicates that writer_info was successfully filled.
       */
      Result_t FillWriterInfo(ASDCP::WriterInfo& writer_info) const;

      /**
       * Reads an IA Frame.
       * 
       * @param frame_number Index of the frame to be read. Must be in the range [0, GetFrameCount()).
       * @param frame Frame data. Must not be modified. Remains valid until the next call to ReadFrame().
       * @return RESULT_OK indicates that more frames are ready to be read,
       * otherwise the file is closed and the reader reset
       */
      Result_t ReadFrame(ui32_t frame_number, Frame& frame);

      /**
       * Reads an IA Frame.
       *
       * @param frame_number Index of the frame to be read. Must be in the range [0, GetFrameCount()).
       * @param frame Frame data. Must not be modified. Remains valid until the next call to ReadFrame().
       * @return RESULT_OK indicates that more frames are ready to be read,
       * otherwise the file is closed and the reader reset
       */
      Result_t ReadFrame(ui32_t frame_number, ASDCP::FrameBuffer& frame);

      /** Reads a Generic Stream Partition payload. Returns RESULT_INIT if the file is
       * not open, or RESULT_FORMAT if the SID is not present in the  RIP, or if the
       * actual partition at ByteOffset does not have a matching BodySID value.
       * Encryption is not currently supported.
       */
      Result_t ReadGenericStreamPartitionPayload(ui32_t SID, ASDCP::FrameBuffer& FrameBuf);

      /**
       * Returns the number of IA Frame in the Track File.
       *
       * @param frameCount Number of IA Frames
       * @return RESULT_OK unless the file is not open
       */
      Result_t GetFrameCount(ui32_t& frameCount) const;

      // Print debugging information to stream
      void     DumpHeaderMetadata(FILE* = 0) const;
      void     DumpIndex(FILE* = 0) const;
    };

  } //namespace IAB

} // namespace AS_02

#endif // AS_02_IAB_h__
