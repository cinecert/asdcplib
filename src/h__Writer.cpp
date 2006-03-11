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
/*! \file    h__Writer.cpp
    \version $Id$
    \brief   MXF file writer base class
*/

#include "AS_DCP_internal.h"
#include "MemIO.h"
#include "Timecode.h"
#include "KLV.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

// a magic number identifying asdcplib
#ifndef WM_BUILD_NUMBER
#define WM_BUILD_NUMBER 0x4A48
#endif



//
ASDCP::h__Writer::h__Writer() : m_EssenceDescriptor(0), m_FramesWritten(0), m_StreamOffset(0)
{
}

ASDCP::h__Writer::~h__Writer()
{
}


const byte_t PictDD_Data[SMPTE_UL_LENGTH] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
  0x01, 0x03, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00 };

const byte_t SoundDD_Data[SMPTE_UL_LENGTH] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
  0x01, 0x03, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00 };

const byte_t TCDD_Data[SMPTE_UL_LENGTH] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
  0x01, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00 };

const byte_t DMDD_Data[SMPTE_UL_LENGTH] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01,
  0x01, 0x03, 0x02, 0x01, 0x10, 0x00, 0x00, 0x00 };


//
// add DMS CryptographicFramework entry to source package
void
AddDMScrypt(Partition& HeaderPart, SourcePackage& Package, WriterInfo& Descr, const UL& WrappingUL)
{
  // Essence Track
  StaticTrack* NewTrack = new StaticTrack;
  HeaderPart.AddChildObject(NewTrack);
  Package.Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackName = "Descriptive Track";
  NewTrack->TrackID = 3;

  Sequence* Seq = new Sequence;
  HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;
  Seq->DataDefinition = UL(DMDD_Data);

  DMSegment* Segment = new DMSegment;
  HeaderPart.AddChildObject(Segment);
  Seq->StructuralComponents.push_back(Segment->InstanceUID);
  Segment->EventComment = "AS-DCP KLV Encryption";
  
  CryptographicFramework* CFW = new CryptographicFramework;
  HeaderPart.AddChildObject(CFW);
  Segment->DMFramework = CFW->InstanceUID;

  CryptographicContext* Context = new CryptographicContext;
  HeaderPart.AddChildObject(Context);
  CFW->ContextSR = Context->InstanceUID;

  Context->ContextID.Set(Descr.ContextID);
  Context->SourceEssenceContainer = WrappingUL; // ??????
  Context->CipherAlgorithm.Set(CipherAlgorithm_AES);
  Context->MICAlgorithm.Set( Descr.UsesHMAC ? MICAlgorithm_HMAC_SHA1 : MICAlgorithm_NONE );
  Context->CryptographicKeyID.Set(Descr.CryptographicKeyID);
}

//
Result_t
ASDCP::h__Writer::WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
				 const MXF::Rational& EditRate,
				 ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);

  m_HeaderPart.m_Primer.ClearTagList();
  m_HeaderPart.m_Preface = new Preface;
  m_HeaderPart.AddChildObject(m_HeaderPart.m_Preface);

  // Set the Operational Pattern label -- we're just starting and have no RIP or index,
  // so we tell the world by using OP1a
  m_HeaderPart.m_Preface->OperationalPattern = UL(OP1aUL);
  m_HeaderPart.OperationalPattern = m_HeaderPart.m_Preface->OperationalPattern;
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(1, 0)); // First RIP Entry

  //
  // Identification
  //
  Identification* Ident = new Identification;
  m_HeaderPart.AddChildObject(Ident);
  m_HeaderPart.m_Preface->Identifications.push_back(Ident->InstanceUID);

  Ident->ThisGenerationUID.GenRandomValue();
  Ident->CompanyName = m_Info.CompanyName.c_str();
  Ident->ProductName = m_Info.ProductName.c_str();
  Ident->VersionString = m_Info.ProductVersion.c_str();
  Ident->ProductUID.Set(m_Info.ProductUUID);
  // Ident->Platform = WM_PLATFORM;
  Ident->ToolkitVersion.Release = VersionType::RL_DEVELOPMENT;
  Ident->ToolkitVersion.Major = VERSION_MAJOR;
  Ident->ToolkitVersion.Minor = VERSION_APIMINOR;
  Ident->ToolkitVersion.Patch = VERSION_IMPMINOR;
  Ident->ToolkitVersion.Build = WM_BUILD_NUMBER;

  //
  ContentStorage* Storage = new ContentStorage;
  m_HeaderPart.AddChildObject(Storage);
  m_HeaderPart.m_Preface->ContentStorage = Storage->InstanceUID;

  EssenceContainerData* ECD = new EssenceContainerData;
  m_HeaderPart.AddChildObject(ECD);
  Storage->EssenceContainerData.push_back(ECD->InstanceUID);
  ECD->IndexSID = 129;
  ECD->BodySID = 1;

  //
  // Material Package
  //
  UMID PackageUMID;
  PackageUMID.MakeUMID(0x0d); // Using mixed type. What is the actual type?

  m_MaterialPackage = new MaterialPackage;
  m_MaterialPackage->Name = "AS-DCP Material Package";
  m_MaterialPackage->PackageUID = PackageUMID;

  m_HeaderPart.AddChildObject(m_MaterialPackage);
  Storage->Packages.push_back(m_MaterialPackage->InstanceUID);

  // Timecode Track
  Track* NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_MaterialPackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 1;
  NewTrack->TrackName = "Timecode Track";

  Sequence* Seq = new Sequence;
  m_HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;
  Seq->DataDefinition = UL(TCDD_Data);

  m_MPTimecode = new TimecodeComponent;
  m_HeaderPart.AddChildObject(m_MPTimecode);
  Seq->StructuralComponents.push_back(m_MPTimecode->InstanceUID);
  m_MPTimecode->RoundedTimecodeBase = TCFrameRate;
  m_MPTimecode->StartTimecode = ui64_C(0);
  m_MPTimecode->DataDefinition = UL(TCDD_Data);

  // Essence Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_MaterialPackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 2;
  NewTrack->TrackName = "Essence Track";

  Seq = new Sequence;
  m_HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;

  m_MPClip = new SourceClip;
  m_HeaderPart.AddChildObject(m_MPClip);
  Seq->StructuralComponents.push_back(m_MPClip->InstanceUID);

  //
  // File (Source) Package
  //
  UUID assetUUID(m_Info.AssetUUID);
  PackageUMID.MakeUMID(0x0f, assetUUID);

  m_FilePackage = new SourcePackage;
  m_FilePackage->Name = PackageLabel.c_str();
  m_FilePackage->PackageUID = PackageUMID;
  ECD->LinkedPackageUID = PackageUMID;
  m_MPClip->SourcePackageID = PackageUMID;
  m_MPClip->SourceTrackID = 1;

  m_HeaderPart.AddChildObject(m_FilePackage);
  Storage->Packages.push_back(m_FilePackage->InstanceUID);

  // Timecode Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_FilePackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 1;
  NewTrack->TrackName = "Timecode Track";

  Seq = new Sequence;
  m_HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;
  Seq->DataDefinition = UL(TCDD_Data);

  m_FPTimecode = new TimecodeComponent;
  m_HeaderPart.AddChildObject(m_FPTimecode);
  Seq->StructuralComponents.push_back(m_FPTimecode->InstanceUID);
  m_FPTimecode->RoundedTimecodeBase = TCFrameRate;
  m_FPTimecode->StartTimecode = ui64_C(86400); // 01:00:00:00
  m_FPTimecode->DataDefinition = UL(TCDD_Data);

  // Essence Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_FilePackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 2;
  NewTrack->TrackName = "Essence Track";

  Seq = new Sequence;
  m_HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;

  m_FPClip = new SourceClip;
  m_HeaderPart.AddChildObject(m_FPClip);
  Seq->StructuralComponents.push_back(m_FPClip->InstanceUID);

  //
  // Essence Descriptor
  //
  m_EssenceDescriptor->EssenceContainer = WrappingUL;
  m_EssenceDescriptor->LinkedTrackID = NewTrack->TrackID;

  // link the Material Package to the File Package ?????????????????????????????????
  Seq->StructuralComponents.push_back(NewTrack->InstanceUID);
  m_HeaderPart.m_Preface->PrimaryPackage = m_FilePackage->InstanceUID;

  //
  // Encryption Descriptor
  //
  if ( m_Info.EncryptedEssence )
    {
      UL CryptEssenceUL(WrappingUL_Data_Crypt);
      m_HeaderPart.EssenceContainers.push_back(CryptEssenceUL);
      m_HeaderPart.m_Preface->EssenceContainers.push_back(CryptEssenceUL);
      m_HeaderPart.m_Preface->DMSchemes.push_back(UL(CryptoFrameworkUL_Data));
      AddDMScrypt(m_HeaderPart, *m_FilePackage, m_Info, WrappingUL);
    }
  else
    {
      UL GCUL(GCMulti_Data);
      m_HeaderPart.EssenceContainers.push_back(GCUL);
      m_HeaderPart.EssenceContainers.push_back(WrappingUL);
      m_HeaderPart.m_Preface->EssenceContainers = m_HeaderPart.EssenceContainers;
    }

  m_HeaderPart.AddChildObject(m_EssenceDescriptor);
  m_FilePackage->Descriptor = m_EssenceDescriptor->InstanceUID;

  // Write the header partition
  Result_t result = m_HeaderPart.WriteToFile(m_File, HeaderPadding);

  //
  // Body Partition
  //


  //
  // Index setup
  //
  fpos_t ECoffset = m_File.Tell();

  if ( BytesPerEditUnit == 0 )
    m_FooterPart.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
  else
    m_FooterPart.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);

  return result;
}



// standard method of writing a plaintext or encrypted frame
Result_t
ASDCP::h__Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
				  AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result;
  IntegrityPack IntPack;

  byte_t overhead[128];
  MemIOWriter Overhead(overhead, 128);

  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("Cannot write empty frame buffer\n");
      return RESULT_EMPTY_FB;
    }

  if ( m_Info.EncryptedEssence )
    {
      if ( ! Ctx )
	return RESULT_CRYPT_CTX;

      if ( m_Info.UsesHMAC && ! HMAC )
	return RESULT_HMAC_CTX;

      if ( FrameBuf.PlaintextOffset() > FrameBuf.Size() )
	return RESULT_LARGE_PTO;

      // encrypt the essence data (create encrypted source value)
      result = EncryptFrameBuffer(FrameBuf, m_CtFrameBuf, Ctx);

      // create HMAC
      if ( ASDCP_SUCCESS(result) && m_Info.UsesHMAC )
      	result = IntPack.CalcValues(m_CtFrameBuf, m_Info.AssetUUID, m_FramesWritten + 1, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{ // write UL
	  Overhead.WriteRaw((byte_t*)CryptEssenceUL_Data, klv_key_size);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + m_CtFrameBuf.Size();

	  if ( m_Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (klv_length_size * 3); // for empty intpack

	  Overhead.WriteBER(ETLength, klv_length_size);                  // write encrypted triplet length
	  Overhead.WriteBER(UUIDlen, klv_length_size);                   // write ContextID length
	  Overhead.WriteRaw(m_Info.ContextID, UUIDlen);                  // write ContextID
	  Overhead.WriteBER(sizeof(ui64_t), klv_length_size);            // write PlaintextOffset length
	  Overhead.WriteUi64BE(FrameBuf.PlaintextOffset());              // write PlaintextOffset
	  Overhead.WriteBER(klv_key_size, klv_length_size);              // write essence UL length
	  Overhead.WriteRaw((byte_t*)EssenceUL, klv_key_size);           // write the essence UL
	  Overhead.WriteBER(sizeof(ui64_t), klv_length_size);            // write SourceLength length
	  Overhead.WriteUi64BE(FrameBuf.Size());                         // write SourceLength
	  Overhead.WriteBER(m_CtFrameBuf.Size(), klv_length_size);       // write ESV length

	  result = m_File.Writev(Overhead.Data(), Overhead.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += Overhead.Size();
	  // write encrypted source value
	  result = m_File.Writev((byte_t*)m_CtFrameBuf.RoData(), m_CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += m_CtFrameBuf.Size();

	  byte_t hmoverhead[512];
	  MemIOWriter HMACOverhead(hmoverhead, 512);

	  // write the HMAC
	  if ( m_Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, klv_length_size);
	    }

	  // write HMAC
	  result = m_File.Writev(HMACOverhead.Data(), HMACOverhead.Size());
	  m_StreamOffset += HMACOverhead.Size();
	}
    }
  else
    {
      Overhead.WriteRaw((byte_t*)EssenceUL, klv_key_size);
      Overhead.WriteBER(FrameBuf.Size(), klv_length_size);
      result = m_File.Writev(Overhead.Data(), Overhead.Size());
 
      if ( ASDCP_SUCCESS(result) )
	result = m_File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

      if ( ASDCP_SUCCESS(result) )
	m_StreamOffset += Overhead.Size() + FrameBuf.Size();
    }

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Writev();

  return result;
}


// standard method of writing the header and footer of a completed MXF file
//
Result_t
ASDCP::h__Writer::WriteMXFFooter()
{
  // Set top-level file package correctly for OP-Atom
  m_MPTimecode->Duration = m_FramesWritten;
  m_MPClip->Duration = m_FramesWritten;
  m_FPTimecode->Duration = m_FramesWritten;
  m_FPClip->Duration = m_FramesWritten;
  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;

  fpos_t here = m_File.Tell();
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(1, here)); // Third RIP Entry
  m_HeaderPart.FooterPartition = here;
  m_HeaderPart.BodySID = 1;
  m_HeaderPart.IndexSID = m_FooterPart.IndexSID;
  m_HeaderPart.OperationalPattern = UL(OPAtomUL);
  m_HeaderPart.m_Preface->OperationalPattern = m_HeaderPart.OperationalPattern;

  m_FooterPart.OperationalPattern = m_HeaderPart.OperationalPattern;
  m_FooterPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  m_FooterPart.FooterPartition = here;
  m_FooterPart.ThisPartition = here;

  Result_t result = m_FooterPart.WriteToFile(m_File, m_FramesWritten);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.m_RIP.WriteToFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.WriteToFile(m_File, HeaderPadding);

  m_File.Close();
  return result;
}

//
// end h__Writer.cpp
//
