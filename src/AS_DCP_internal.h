/*
Copyright (c) 2004-2006, John Hurst
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
/*! \file    AS_DCP_internal.h
    \version $Id$       
    \brief   AS-DCP library, non-public common elements
*/

#ifndef _AS_DCP_INTERNAL_H__
#define _AS_DCP_INTERNAL_H__

#include "AS_DCP_system.h"
#include "Metadata.h"
#include "hex_utils.h"

using namespace std;
using namespace ASDCP;
using namespace ASDCP::MXF;



namespace ASDCP
{
  // constant values used to calculate KLV and EKLV packet sizes
  static const ui32_t klv_key_size = 16;
  static const ui32_t klv_length_size = 4;

  static const ui32_t klv_cryptinfo_size =
    klv_length_size
    + UUIDlen /* ContextID */
    + klv_length_size
    + sizeof(ui64_t) /* PlaintextOffset */
    + klv_length_size
    + klv_key_size /* SourceKey */
    + klv_length_size
    + sizeof(ui64_t) /* SourceLength */
    + klv_length_size /* ESV length */ ;

  static const ui32_t klv_intpack_size =
    klv_length_size
    + UUIDlen /* TrackFileID */
    + klv_length_size
    + sizeof(ui64_t) /* SequenceNumber */
    + klv_length_size
    + 20; /* HMAC length*/

  // why this value? i dunno. it was peeled from mxflib.
  static const ui32_t HeaderPadding = 16384;
  
  const byte_t GCMulti_Data[16] =
  { 0x06, 0x0E, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x03,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x7F, 0x01, 0x00 };

  static const byte_t CipherAlgorithm_AES[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07,
    0x02, 0x09, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };

  static const byte_t MICAlgorithm_NONE[klv_key_size] = {0};
  static const byte_t MICAlgorithm_HMAC_SHA1[klv_key_size] = 
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07,
    0x02, 0x09, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };

#ifdef SMPTE_LABELS
  static byte_t OPAtom_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02,
    0x0d, 0x01, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
  static UL OPAtomUL(OPAtom_Data);
#else
  static byte_t OPAtom_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
    0x0d, 0x01, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };
  static UL OPAtomUL(OPAtom_Data);
#endif

  static const byte_t OP1a_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
    0x0d, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00 };
  static UL OP1aUL(OP1a_Data);
  
  // Essence element labels
  static const byte_t WAVEssenceUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
    0x0d, 0x01, 0x03, 0x01, 0x16, 0x01, 0x01, 0x00 };

  static const byte_t MPEGEssenceUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
    0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x05, 0x00 };

  static const byte_t JP2KEssenceUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
    0x0d, 0x01, 0x03, 0x01, 0x15, 0x01, 0x08, 0x01 };

#ifdef SMPTE_LABELS
  static const byte_t CryptEssenceUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x04, 0x01, 0x01,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x7e, 0x01, 0x00 };
#else
  static const byte_t CryptEssenceUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x02, 0x04, 0x01, 0x07,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x7e, 0x01, 0x00 };
#endif

  // Essence Container Labels
  static const byte_t WrappingUL_Data_PCM_24b_48k[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x06, 0x01, 0x00 };

  static const byte_t WrappingUL_Data_MPEG2_VES[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x02,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x04, 0x60, 0x01 };

  static const byte_t WrappingUL_Data_JPEG_2000[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x0c, 0x01, 0x00 };

  static const byte_t WrappingUL_Data_Crypt[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07,
    0x0d, 0x01, 0x03, 0x01, 0x02, 0x0b, 0x01, 0x00 };


  // the label for the Cryptographic Framework DM scheme
  static const byte_t CryptoFrameworkUL_Data[klv_key_size] =
  { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x07,
    0x0d, 0x01, 0x04, 0x01, 0x02, 0x01, 0x01, 0x00 };

  // the check value for EKLV packets
  // CHUKCHUKCHUKCHUK
  static const byte_t ESV_CheckValue[CBC_BLOCK_SIZE] =
  { 0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b,
    0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b };

  // labels used for FilePackages
  static std::string MPEG_PACKAGE_LABEL = "File Package: SMPTE 381M frame wrapping of MPEG2 video elementary stream";
  static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE XXXM frame wrapping of JPEG 2000 codestreams";
  static std::string PCM_PACKAGE_LABEL = "File Package: SMPTE 382M frame wrapping of wave audio";

  // GetMDObjectByPath() allows searching for metadata object by pathname
  // This character separates the path elements.
  static const char OBJECT_PATH_SEPARATOR = '.';

  //------------------------------------------------------------------------------------------
  //

  Result_t MD_to_MPEG2_VDesc(MXF::MPEG2VideoDescriptor*, MPEG2::VideoDescriptor&);
  Result_t MD_to_JP2K_PDesc(MXF::RGBAEssenceDescriptor*, JP2K::PictureDescriptor&);
  Result_t MD_to_PCM_ADesc(MXF::WaveAudioDescriptor*, PCM::AudioDescriptor&);
  Result_t MD_to_WriterInfo(MXF::Identification*, WriterInfo&);
  Result_t MD_to_CryptoInfo(MXF::CryptographicContext*, WriterInfo&);
  Result_t MPEG2_VDesc_to_MD(MPEG2::VideoDescriptor&, MXF::MPEG2VideoDescriptor*);
  Result_t JP2K_PDesc_to_MD(JP2K::PictureDescriptor&, MXF::RGBAEssenceDescriptor*);
  Result_t PCM_ADesc_to_MD(PCM::AudioDescriptor&, MXF::WaveAudioDescriptor*);
  Result_t EncryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESEncContext*);
  Result_t DecryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESDecContext*);

  // calculate size of encrypted essence with IV, CheckValue, and padding
  inline ui32_t
    calc_esv_length(ui32_t source_length, ui32_t plaintext_offset)
    {
      ui32_t ct_size = source_length - plaintext_offset;
      ui32_t diff = ct_size % CBC_BLOCK_SIZE;
      ui32_t block_size = ct_size - diff;
      return plaintext_offset + block_size + (CBC_BLOCK_SIZE * 3);
    }

  //
  class h__Reader
    {
      ASDCP_NO_COPY_CONSTRUCT(h__Reader);

    public:
      FileReader         m_File;
      OPAtomHeader       m_HeaderPart;
      Partition          m_BodyPart;
      OPAtomIndexFooter  m_FooterPart;
      ui64_t             m_EssenceStart;
      WriterInfo         m_Info;
      ASDCP::FrameBuffer m_CtFrameBuf;
      fpos_t             m_LastPosition;

      h__Reader();
      virtual ~h__Reader();

      Result_t InitInfo(WriterInfo& Info);
      Result_t OpenMXFRead(const char* filename);
      Result_t InitMXFIndex();
      Result_t ReadEKLVPacket(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			      const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);
      void     Close();
    };


  // state machine for mxf writer
  enum WriterState_t {
    ST_BEGIN,   // waiting for Open()
    ST_INIT,    // waiting for SetSourceStream()
    ST_READY,   // ready to write frames
    ST_RUNNING, // one or more frames written
    ST_FINAL,   // index written, file closed
  };

  // implementation of h__WriterState class Goto_* methods
#define Goto_body(s1,s2) if ( m_State != (s1) ) \
                           return RESULT_STATE; \
                         m_State = (s2); \
                         return RESULT_OK
  //
  class h__WriterState
    {
      ASDCP_NO_COPY_CONSTRUCT(h__WriterState);

    public:
      WriterState_t m_State;
      h__WriterState() : m_State(ST_BEGIN) {}
      ~h__WriterState() {}

      inline bool     Test_BEGIN()   { return m_State == ST_BEGIN; }
      inline bool     Test_INIT()    { return m_State == ST_INIT; }
      inline bool     Test_READY()   { return m_State == ST_READY;}
      inline bool     Test_RUNNING() { return m_State == ST_RUNNING; }
      inline bool     Test_FINAL()   { return m_State == ST_FINAL; }
      inline Result_t Goto_INIT()    { Goto_body(ST_BEGIN,   ST_INIT); }
      inline Result_t Goto_READY()   { Goto_body(ST_INIT,    ST_READY); }
      inline Result_t Goto_RUNNING() { Goto_body(ST_READY,   ST_RUNNING); }
      inline Result_t Goto_FINAL()   { Goto_body(ST_RUNNING, ST_FINAL); }
    };

  //
  class h__Writer
    {
      ASDCP_NO_COPY_CONSTRUCT(h__Writer);

    public:
      FileWriter         m_File;
      OPAtomHeader       m_HeaderPart;
      Partition          m_BodyPart;
      OPAtomIndexFooter  m_FooterPart;
      ui64_t             m_EssenceStart;

      MaterialPackage*   m_MaterialPackage;
      TimecodeComponent* m_MPTimecode;
      SourceClip*        m_MPClip;			//! Material Package SourceClip for each essence stream 

      SourcePackage*     m_FilePackage;
      TimecodeComponent* m_FPTimecode;
      SourceClip*        m_FPClip;			//! File Package SourceClip for each essence stream 

      FileDescriptor*    m_EssenceDescriptor;

      ui32_t             m_FramesWritten;
      ui64_t             m_StreamOffset;
      ASDCP::FrameBuffer m_CtFrameBuf;
      h__WriterState     m_State;
      WriterInfo         m_Info;

      h__Writer();
      virtual ~h__Writer();

      Result_t WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
			      const MXF::Rational& EditRate,
			      ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

      Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,
			       const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);

      Result_t WriteMXFFooter();

   };


  // helper class for calculating Integrity Packs, used by WriteEKLVPacket() below.
  //
  class IntegrityPack
    {
    public:
      byte_t Data[klv_intpack_size];
  
      IntegrityPack() {
	memset(Data, 0, klv_intpack_size);
      }

      ~IntegrityPack() {}
  
      Result_t CalcValues(const ASDCP::FrameBuffer&, byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
      Result_t TestValues(const ASDCP::FrameBuffer&, byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
    };

  //
  class KLVReader
    {
      byte_t m_Key[32];
      ui64_t m_Length;
      ui32_t m_BERLength;
      ui32_t m_HeaderLength;

      ASDCP_NO_COPY_CONSTRUCT(KLVReader);

    public:
      KLVReader() : m_Length(0), m_BERLength(0), m_HeaderLength(0) {}
      ~KLVReader() {}

      inline const byte_t* Key() { return m_Key; }
      inline const ui64_t  Length() { return m_Length; }
      inline const ui64_t  KLLength() { return m_BERLength + klv_key_size; }
      Result_t ReadKLFromFile(ASDCP::FileReader& Reader);
    };

} // namespace ASDCP

#endif // _AS_DCP_INTERNAL_H__


//
// end AS_DCP_internal.h
//
