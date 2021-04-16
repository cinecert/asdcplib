/*
Copyright (c) 2004-2016, John Hurst
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
/*! \file    AS_DCP_MXF.cpp
    \version $Id$
    \brief   AS-DCP library, misc classes and subroutines
*/

#include <KM_fileio.h>
#include <KM_xml.h>
#include "AS_DCP_internal.h"
#include "JP2K.h"
#include "JXS.h"
#include "ACES.h"
#include "MPEG.h"
#include "Wav.h"
#include "KM_util.h"
#include <iostream>
#include <iomanip>


//------------------------------------------------------------------------------------------
// misc subroutines


//
std::ostream&
ASDCP::operator << (std::ostream& strm, const WriterInfo& Info)
{
  char str_buf[40];

  strm << "       ProductUUID: " << UUID(Info.ProductUUID).EncodeHex(str_buf, 40) << std::endl;
  strm << "    ProductVersion: " << Info.ProductVersion << std::endl;
  strm << "       CompanyName: " << Info.CompanyName << std::endl;
  strm << "       ProductName: " << Info.ProductName << std::endl;
  strm << "  EncryptedEssence: " << (Info.EncryptedEssence ? "Yes" : "No") << std::endl;

  if ( Info.EncryptedEssence )
    {
      strm << "              HMAC: " << (Info.UsesHMAC ? "Yes" : "No") << std::endl;
      strm << "         ContextID: " << UUID(Info.ContextID).EncodeHex(str_buf, 40) << std::endl;
      strm << "CryptographicKeyID: " << UUID(Info.CryptographicKeyID).EncodeHex(str_buf, 40) << std::endl;
    }

  strm << "         AssetUUID: " << UUID(Info.AssetUUID).EncodeHex(str_buf, 40) << std::endl;
  strm << "    Label Set Type: " << (Info.LabelSetType == LS_MXF_SMPTE ? "SMPTE" :
				     (Info.LabelSetType == LS_MXF_INTEROP ? "MXF Interop" :
				      "Unknown")) << std::endl;
  return strm;
}

//
void
ASDCP::WriterInfoDump(const WriterInfo& Info, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  char str_buf[40];

  fprintf(stream,"       ProductUUID: %s\n", UUID(Info.ProductUUID).EncodeHex(str_buf, 40));
  fprintf(stream,"\
    ProductVersion: %s\n\
       CompanyName: %s\n\
       ProductName: %s\n\
  EncryptedEssence: %s\n",
	  Info.ProductVersion.c_str(),
	  Info.CompanyName.c_str(),
	  Info.ProductName.c_str(),
	  ( Info.EncryptedEssence ? "Yes" : "No" )
	  );

  if ( Info.EncryptedEssence )
    {
      fprintf(stream, "              HMAC: %s\n", ( Info.UsesHMAC ? "Yes" : "No"));
      fprintf(stream, "         ContextID: %s\n", UUID(Info.ContextID).EncodeHex(str_buf, 40));
      fprintf(stream, "CryptographicKeyID: %s\n", UUID(Info.CryptographicKeyID).EncodeHex(str_buf, 40));
    }

  fprintf(stream,"         AssetUUID: %s\n", UUID(Info.AssetUUID).EncodeHex(str_buf, 40));
  fprintf(stream,"    Label Set Type: %s\n", ( Info.LabelSetType == LS_MXF_SMPTE ? "SMPTE" :
					       ( Info.LabelSetType == LS_MXF_INTEROP ? "MXF Interop" :
						 "Unknown" ) ));
}

//
Result_t
ASDCP::MD_to_WriterInfo(Identification* InfoObj, WriterInfo& Info)
{
  ASDCP_TEST_NULL(InfoObj);
  char tmp_str[IdentBufferLen];

  Info.ProductName = "Unknown Product";
  Info.ProductVersion = "Unknown Version";
  Info.CompanyName = "Unknown Company";
  memset(Info.ProductUUID, 0, UUIDlen);

  InfoObj->ProductName.EncodeString(tmp_str, IdentBufferLen);
  if ( *tmp_str ) Info.ProductName = tmp_str;

  InfoObj->VersionString.EncodeString(tmp_str, IdentBufferLen);
  if ( *tmp_str ) Info.ProductVersion = tmp_str;

  InfoObj->CompanyName.EncodeString(tmp_str, IdentBufferLen);
  if ( *tmp_str ) Info.CompanyName = tmp_str;

  memcpy(Info.ProductUUID, InfoObj->ProductUID.Value(), UUIDlen);

  return RESULT_OK;
}


//
Result_t
ASDCP::MD_to_CryptoInfo(CryptographicContext* InfoObj, WriterInfo& Info, const Dictionary& Dict)
{
  ASDCP_TEST_NULL(InfoObj);

  Info.EncryptedEssence = true;
  memcpy(Info.ContextID, InfoObj->ContextID.Value(), UUIDlen);
  memcpy(Info.CryptographicKeyID, InfoObj->CryptographicKeyID.Value(), UUIDlen);

  UL MIC_SHA1(Dict.ul(MDD_MICAlgorithm_HMAC_SHA1));
  UL MIC_NONE(Dict.ul(MDD_MICAlgorithm_NONE));

  if ( InfoObj->MICAlgorithm == MIC_SHA1 )
    Info.UsesHMAC = true;

  else if ( InfoObj->MICAlgorithm == MIC_NONE )
    Info.UsesHMAC = false;

  else
    {
      DefaultLogSink().Error("Unexpected MICAlgorithm UL.\n");
      return RESULT_FORMAT;
    }

  return RESULT_OK;
}

//
//
ASDCP::Result_t
ASDCP::EssenceType(const std::string& filename, EssenceType_t& type)
{
  const Dictionary* m_Dict = &DefaultCompositeDict();
  InterchangeObject* md_object = 0;

  assert(m_Dict);
  Kumu::FileReader   Reader;
  OP1aHeader TestHeader(m_Dict);

  Result_t result = Reader.OpenRead(filename);

  if ( ASDCP_SUCCESS(result) )
    result = TestHeader.InitFromFile(Reader); // test UL and OP

  if ( ASDCP_SUCCESS(result) )
    {
      type = ESS_UNKNOWN;

      if ( TestHeader.OperationalPattern == UL(m_Dict->ul(MDD_OPAtom))
	   || TestHeader.OperationalPattern == UL(m_Dict->ul(MDD_MXFInterop_OPAtom)) )
	{
	  if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor))) )
	    {
	      if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(StereoscopicPictureSubDescriptor))) )
		{
		  type = ESS_JPEG_2000_S;
		}
	      else
		{
		  type = ESS_JPEG_2000;
		}
	    }
	  else if (ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(JPEGXSPictureSubDescriptor), &md_object)))
		{
		  type = ESS_JPEG_XS;
		}
	  else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &md_object)) )
	    {
	      assert(md_object);
	      if ( static_cast<ASDCP::MXF::WaveAudioDescriptor*>(md_object)->AudioSamplingRate == SampleRate_96k )
		{
		  type = ESS_PCM_24b_96k;
		}
	      else
		{
		  type = ESS_PCM_24b_48k;
		}
	    }
	  else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(MPEG2VideoDescriptor))) )
	    {
	      type = ESS_MPEG2_VES;
	    }
	  else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(TimedTextDescriptor))) )
	    {
	      type = ESS_TIMED_TEXT;
	    }
	  else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(DCDataDescriptor)))
		    || ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(PrivateDCDataDescriptor))) )
	    {
	      if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(DolbyAtmosSubDescriptor))) )
		{
		  type = ESS_DCDATA_DOLBY_ATMOS;
		}
	      else
		{
		  type = ESS_DCDATA_UNKNOWN;
		}
	    }
	}
      else if (  TestHeader.OperationalPattern == UL(m_Dict->ul(MDD_OP1a)) )
	{
    	  // ST 2065-5 Picture Descriptor does not have a mandatory SubDescriptor, check EssenceContainer instead
	  if (ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor))) )
	  {
	    MXF::RGBAEssenceDescriptor *rgba_descriptor = 0;
	    char buf[64];

	    if ASDCP_SUCCESS(TestHeader.GetMDObjectByType(m_Dict->ul(MDD_RGBAEssenceDescriptor), reinterpret_cast<MXF::InterchangeObject**>(&rgba_descriptor)))
	    {
	        if (rgba_descriptor->EssenceContainer == m_Dict->ul(MDD_MXFGCFrameWrappedACESPictures))
	            type = ESS_AS02_ACES;
	    }
	  }
	  if (type == ESS_UNKNOWN)
	  {

	    if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor))) )
	      {
	        type = ESS_AS02_JPEG_2000;
	      }
		else if (ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(JPEGXSPictureSubDescriptor))))
		{
			type = ESS_AS02_JPEG_XS;
		}
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &md_object)) )
	      {
	        assert(md_object);
	        if ( static_cast<ASDCP::MXF::WaveAudioDescriptor*>(md_object)->AudioSamplingRate == SampleRate_96k )
	          {
	            type = ESS_AS02_PCM_24b_96k;
	          }
	        else
	          {
	            type = ESS_AS02_PCM_24b_48k;
	          }
	      }
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(TimedTextDescriptor))) )
	      {
	        type = ESS_AS02_TIMED_TEXT;
	      }
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(PIMFDynamicMetadataDescriptor))) )
	      {
	        type = ESS_DCDATA_UNKNOWN;
	      }
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(ISXDDataEssenceDescriptor))) )
	      {
	        type = ESS_AS02_ISXD;
	      }
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(ACESPictureSubDescriptor))) )
	      {
	        type = ESS_AS02_ACES;
	      }
	    else if ( ASDCP_SUCCESS(TestHeader.GetMDObjectByType(OBJ_TYPE_ARGS(IABEssenceDescriptor))) )
	      {
	        type = ESS_AS02_IAB;
	      }
	  }
	}
      else
	{
	  DefaultLogSink().Error("Unsupported MXF Operational Pattern.\n");
	  return RESULT_FORMAT;
	}
    }

  return result;
}

//
static bool
string_is_xml(const ASDCP::FrameBuffer& buffer)
{
  return (strncmp((const char *)buffer.RoData(),             "<?xml", 5) == 0 ||
          strncmp((const char *)buffer.RoData(), "\xEF\xBB\xBF<?xml", 8) == 0); // Allow BOM
 }
 
 //

//
ASDCP::Result_t
ASDCP::RawEssenceType(const std::string& filename, EssenceType_t& type)
{
  type = ESS_UNKNOWN;
  ASDCP::FrameBuffer FB;
  Kumu::FileReader Reader;
  ASDCP::Wav::SimpleWaveHeader WavHeader;
  ASDCP::RF64::SimpleRF64Header RF64Header;
  ASDCP::AIFF::SimpleAIFFHeader AIFFHeader;
  Kumu::XMLElement TmpElement("Tmp");

  ui32_t data_offset;
  ui32_t read_count;
  Result_t result = FB.Capacity(Wav::MaxWavHeader); // using Wav max because everything else is much smaller

  if ( Kumu::PathIsFile(filename) )
    {
      result = Reader.OpenRead(filename);

      if ( ASDCP_SUCCESS(result) )
	{
	  result = Reader.Read(FB.Data(), FB.Capacity(), &read_count);
	  Reader.Close();
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  const byte_t* p = FB.RoData();
	  FB.Size(read_count);

	  ui32_t i = 0;
	  while ( p[i] == 0 ) i++;

	  if ( i > 1 && p[i] == 1 &&  (p[i+1] == ASDCP::MPEG2::SEQ_START || p[i+1] == ASDCP::MPEG2::PIC_START) )
	    {
	      type = ESS_MPEG2_VES;
	    }
	  else if ( memcmp(FB.RoData(), ASDCP::JP2K::Magic, sizeof(ASDCP::JP2K::Magic)) == 0 )
	    {
	      type = ESS_JPEG_2000;
	    }
	  else if (memcmp(FB.RoData(), ASDCP::JXS::Magic, sizeof(ASDCP::JXS::Magic)) == 0)
	  {
		  type = ESS_JPEG_XS;
	  }
	  else if(memcmp(FB.RoData(), AS_02::ACES::Magic, sizeof(AS_02::ACES::Magic)) == 0)
	    {
	      type = ESS_AS02_ACES;
	    }
	  else if ( std::string((const char*)FB.RoData() + 8, 4) == "WAVE" )
	    {
	      if ( std::string((const char*)FB.RoData(), 4) == "RIFF" )
		{
		  result = WavHeader.ReadFromBuffer(FB.RoData(), read_count, &data_offset);

		  if ( ASDCP_SUCCESS(result) )
		    {
		      switch ( WavHeader.samplespersec )
			{
			case 48000: type = ESS_PCM_24b_48k; break;
			case 96000: type = ESS_PCM_24b_96k; break;
			default:
			  DefaultLogSink().Error("Unexpected sample rate: %d\n", WavHeader.samplespersec);
			  result = RESULT_FORMAT;
			}
		    }
		}
	      else
		{
		  result = RF64Header.ReadFromBuffer(FB.RoData(), read_count, &data_offset);
		
		  if ( ASDCP_SUCCESS(result) )
		    {
		      switch ( RF64Header.samplespersec )
			{
			case 48000: type = ESS_PCM_24b_48k; break;
			case 96000: type = ESS_PCM_24b_96k; break;
			default:
			  DefaultLogSink().Error("Unexpected sample rate: %d\n", WavHeader.samplespersec);
			  result = RESULT_FORMAT;
			}
		    }
		}
	    }
	  else if ( ASDCP_SUCCESS(AIFFHeader.ReadFromBuffer(FB.RoData(), read_count, &data_offset)) )
	    {
	      type = ESS_PCM_24b_48k;
	    }
	  else if ( string_is_xml(FB) )
	    {
	      type = ESS_TIMED_TEXT;
	    }
	  else if ( ASDCP::ATMOS::IsDolbyAtmos(filename) )
	    {
	      type = ESS_DCDATA_DOLBY_ATMOS;
	    }
	}
    }
  else if ( Kumu::PathIsDirectory(filename) )
    {
      char next_file[Kumu::MaxFilePath];
      Kumu::DirScanner Scanner;
      Result_t result = Scanner.Open(filename);

      if ( ASDCP_SUCCESS(result) )
	{
	  while ( ASDCP_SUCCESS(Scanner.GetNext(next_file)) )
	    {
	      if ( next_file[0] == '.' ) // no hidden files or internal links
		continue;

	      result = Reader.OpenRead(Kumu::PathJoin(filename, next_file));

	      if ( ASDCP_SUCCESS(result) )
		{
		  result = Reader.Read(FB.Data(), FB.Capacity(), &read_count);
		  Reader.Close();
		}

	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( memcmp(FB.RoData(), ASDCP::JP2K::Magic, sizeof(ASDCP::JP2K::Magic)) == 0 )
		    {
		      type = ESS_JPEG_2000;
		    }
	          else if(memcmp(FB.RoData(), AS_02::ACES::Magic, sizeof(AS_02::ACES::Magic)) == 0)
	            {
	                type = ESS_AS02_ACES;
	            }
			  else if (memcmp(FB.RoData(), ASDCP::JXS::Magic, sizeof(ASDCP::JXS::Magic)) == 0)
		  {
			  type = ESS_JPEG_XS;
		  }
		  else if ( ASDCP_SUCCESS(WavHeader.ReadFromBuffer(FB.RoData(), read_count, &data_offset)) )
		    {
		      switch ( WavHeader.samplespersec )
			{
			case 48000: type = ESS_PCM_24b_48k; break;
			case 96000: type = ESS_PCM_24b_96k; break;
			default:
			  return RESULT_FORMAT;
			}
		    }
		  else if ( ASDCP_SUCCESS(RF64Header.ReadFromBuffer(FB.RoData(), read_count, &data_offset)) )
		    {
		      switch ( RF64Header.samplespersec )
			{
			case 48000: type = ESS_PCM_24b_48k; break;
			case 96000: type = ESS_PCM_24b_96k; break;
			default:
			  return RESULT_FORMAT;
			}
		    }
		  else if ( ASDCP::ATMOS::IsDolbyAtmos(Kumu::PathJoin(filename, next_file)) )
		    {
		      type = ESS_DCDATA_DOLBY_ATMOS;
		    }
		  else
		    {
		      type = ESS_DCDATA_UNKNOWN;
		    }
		}
	      
	      break;
	    }
	}
    }

  return result;
}

//
Result_t
ASDCP::EncryptFrameBuffer(const ASDCP::FrameBuffer& FBin, ASDCP::FrameBuffer& FBout, AESEncContext* Ctx)
{
  ASDCP_TEST_NULL(Ctx);
  FBout.Size(0);

  // size the buffer
  Result_t result = FBout.Capacity(calc_esv_length(FBin.Size(), FBin.PlaintextOffset()));

  // write the IV
  byte_t* p = FBout.Data();

  // write the IV to the frame buffer
  Ctx->GetIVec(p);
  p += CBC_BLOCK_SIZE;


  // encrypt the check value to the frame buffer
  if ( ASDCP_SUCCESS(result) )
    {
      result = Ctx->EncryptBlock(ESV_CheckValue, p, CBC_BLOCK_SIZE);
      p += CBC_BLOCK_SIZE;
    }

  // write optional plaintext region
  if ( FBin.PlaintextOffset() > 0 )
    {
      assert(FBin.PlaintextOffset() <= FBin.Size());
      memcpy(p, FBin.RoData(), FBin.PlaintextOffset());
      p += FBin.PlaintextOffset();
    }

  ui32_t ct_size = FBin.Size() - FBin.PlaintextOffset();
  ui32_t diff = ct_size % CBC_BLOCK_SIZE;
  ui32_t block_size = ct_size - diff;
  assert((block_size % CBC_BLOCK_SIZE) == 0);

  // encrypt the ciphertext region essence data
  if ( ASDCP_SUCCESS(result) )
    {
      result = Ctx->EncryptBlock(FBin.RoData() + FBin.PlaintextOffset(), p, block_size);
      p += block_size;
    }

  // construct and encrypt the padding
  if ( ASDCP_SUCCESS(result) )
    {
      byte_t the_last_block[CBC_BLOCK_SIZE];

      if ( diff > 0 )
	memcpy(the_last_block, FBin.RoData() + FBin.PlaintextOffset() + block_size, diff);

      for (ui32_t i = 0; diff < CBC_BLOCK_SIZE; diff++, i++ )
	the_last_block[diff] = i;

      result = Ctx->EncryptBlock(the_last_block, p, CBC_BLOCK_SIZE);
    }

  if ( ASDCP_SUCCESS(result) )
    FBout.Size(calc_esv_length(FBin.Size(), FBin.PlaintextOffset()));

  return result;
}

//
Result_t
ASDCP::DecryptFrameBuffer(const ASDCP::FrameBuffer& FBin, ASDCP::FrameBuffer& FBout, AESDecContext* Ctx)
{
  ASDCP_TEST_NULL(Ctx);
  assert(FBout.Capacity() >= FBin.SourceLength());

  ui32_t ct_size = FBin.SourceLength() - FBin.PlaintextOffset();
  ui32_t diff = ct_size % CBC_BLOCK_SIZE;
  ui32_t block_size = ct_size - diff;
  assert(block_size);
  assert((block_size % CBC_BLOCK_SIZE) == 0);

  const byte_t* buf = FBin.RoData();

  // get ivec
  Ctx->SetIVec(buf);
  buf += CBC_BLOCK_SIZE;

  // decrypt and test check value
  byte_t CheckValue[CBC_BLOCK_SIZE];
  Result_t result = Ctx->DecryptBlock(buf, CheckValue, CBC_BLOCK_SIZE);
  buf += CBC_BLOCK_SIZE;

  if ( memcmp(CheckValue, ESV_CheckValue, CBC_BLOCK_SIZE) != 0 )
    return RESULT_CHECKFAIL;

  // copy plaintext region
  if ( FBin.PlaintextOffset() > 0 )
    {
      memcpy(FBout.Data(), buf, FBin.PlaintextOffset());
      buf += FBin.PlaintextOffset();
    }

  // decrypt all but last block
  if ( ASDCP_SUCCESS(result) )
    {
      result = Ctx->DecryptBlock(buf, FBout.Data() + FBin.PlaintextOffset(), block_size);
      buf += block_size;
    }

  // decrypt last block
  if ( ASDCP_SUCCESS(result) )
    {
      byte_t the_last_block[CBC_BLOCK_SIZE];
      result = Ctx->DecryptBlock(buf, the_last_block, CBC_BLOCK_SIZE);

      if ( the_last_block[diff] != 0 )
	{
	  DefaultLogSink().Error("Unexpected non-zero padding value.\n");
	  return RESULT_FORMAT;
	}

      if ( diff > 0 )
	memcpy(FBout.Data() + FBin.PlaintextOffset() + block_size, the_last_block, diff);
    }

  if ( ASDCP_SUCCESS(result) )
    FBout.Size(FBin.SourceLength());

  return result;
}


//
Result_t
ASDCP::IntegrityPack::CalcValues(const ASDCP::FrameBuffer& FB, const byte_t* AssetID,
				 ui32_t sequence, HMACContext* HMAC)
{
  ASDCP_TEST_NULL(AssetID);
  ASDCP_TEST_NULL(HMAC);
  byte_t* p = Data;
  HMAC->Reset();

  static byte_t ber_4[MXF_BER_LENGTH] = {0x83, 0, 0, 0};

  // update HMAC with essence data
  HMAC->Update(FB.RoData(), FB.Size());

  // track file ID length
  memcpy(p, ber_4, MXF_BER_LENGTH);
  *(p+3) = UUIDlen;;
  p += MXF_BER_LENGTH;

  // track file ID
  memcpy(p, AssetID, UUIDlen);
  p += UUIDlen;

  // sequence length
  memcpy(p, ber_4, MXF_BER_LENGTH);
  *(p+3) = sizeof(ui64_t);
  p += MXF_BER_LENGTH;

  // sequence number
  Kumu::i2p<ui64_t>(KM_i64_BE(sequence), p);
  p += sizeof(ui64_t);

  // HMAC length
  memcpy(p, ber_4, MXF_BER_LENGTH);
  *(p+3) = HMAC_SIZE;
  p += MXF_BER_LENGTH;

  // update HMAC with intpack values
  HMAC->Update(Data, klv_intpack_size - HMAC_SIZE);

  // finish & write HMAC
  HMAC->Finalize();
  HMAC->GetHMACValue(p);

  assert(p + HMAC_SIZE == Data + klv_intpack_size);

  return RESULT_OK;
}


Result_t
ASDCP::IntegrityPack::TestValues(const ASDCP::FrameBuffer& FB, const byte_t* AssetID,
				 ui32_t sequence, HMACContext* HMAC)
{
  ASDCP_TEST_NULL(AssetID);
  ASDCP_TEST_NULL(HMAC);

  // find the start of the intpack
  byte_t* p = (byte_t*)FB.RoData() + ( FB.Size() - klv_intpack_size );

  // test the AssetID length
  if ( ! Kumu::read_test_BER(&p, UUIDlen) )
        return RESULT_HMACFAIL;

  // test the AssetID
  if ( memcmp(p, AssetID, UUIDlen) != 0 )
    {
      DefaultLogSink().Error("IntegrityPack failure: AssetID mismatch.\n");
      return RESULT_HMACFAIL;
    }
  p += UUIDlen;
  
  // test the sequence length
  if ( ! Kumu::read_test_BER(&p, sizeof(ui64_t)) )
        return RESULT_HMACFAIL;

  ui32_t test_sequence = (ui32_t)KM_i64_BE(Kumu::cp2i<ui64_t>(p));

  // test the sequence value
  if ( test_sequence != sequence )
    {
      DefaultLogSink().Error("IntegrityPack failure: sequence is %u, expecting %u.\n", test_sequence, sequence);
      return RESULT_HMACFAIL;
    }

  p += sizeof(ui64_t);

  // test the HMAC length
  if ( ! Kumu::read_test_BER(&p, HMAC_SIZE) )
        return RESULT_HMACFAIL;

  // test the HMAC
  HMAC->Reset();
  HMAC->Update(FB.RoData(), FB.Size() - HMAC_SIZE);
  HMAC->Finalize();

  Result_t result = RESULT_OK;
  result = HMAC->TestHMACValue(p);

  if (KM_FAILURE(result))
    {
      Result_t r = RESULT_OK;
      char hmac_str[HMAC_SIZE*10];
      char found_str[HMAC_SIZE*10];
      byte_t hmac_buf[HMAC_SIZE];


      Kumu::bin2hex(p, HMAC_SIZE, found_str, HMAC_SIZE*10);

      r = HMAC->GetHMACValue(hmac_buf);
      if ( KM_SUCCESS( r ) )
        {
          Kumu::bin2hex(hmac_buf, HMAC_SIZE, hmac_str, HMAC_SIZE*10);
        }
      else
        {
          snprintf(hmac_str, HMAC_SIZE*10, " - read error - ");
        }

      DefaultLogSink().Error("IntegrityPack failure: HMAC is %s, expecting %s.\n", found_str, hmac_str);
    }

  return result;
}

//
// end AS_DCP_MXF.cpp
//
