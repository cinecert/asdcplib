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
#include "KLV.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

// a magic number identifying asdcplib
#ifndef ASDCP_BUILD_NUMBER
#define ASDCP_BUILD_NUMBER 0x6A68
#endif



//
ASDCP::h__Writer::h__Writer() :
  m_HeaderSize(0), m_EssenceStart(0),
  m_MaterialPackage(0), m_MPTCSequence(0), m_MPTimecode(0), m_MPClSequence(0), m_MPClip(0),
  m_FilePackage(0), m_FPTCSequence(0), m_FPTimecode(0), m_FPClSequence(0), m_FPClip(0),
  m_EssenceDescriptor(0), m_FramesWritten(0), m_StreamOffset(0)
{
}

ASDCP::h__Writer::~h__Writer()
{
}

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
  Seq->DataDefinition = UL(Dict::ul(MDD_DescriptiveMetaDataDef));

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
  Context->CipherAlgorithm.Set(Dict::ul(MDD_CipherAlgorithm_AES));
  Context->MICAlgorithm.Set( Descr.UsesHMAC ? Dict::ul(MDD_MICAlgorithm_HMAC_SHA1) : Dict::ul(MDD_MICAlgorithm_NONE) );
  Context->CryptographicKeyID.Set(Descr.CryptographicKeyID);
}

//
Result_t
ASDCP::h__Writer::WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
				 const std::string& TrackName, const UL& DataDefinition,
				 const MXF::Rational& EditRate,
				 ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);

  m_HeaderPart.m_Primer.ClearTagList();
  m_HeaderPart.m_Preface = new Preface;
  m_HeaderPart.AddChildObject(m_HeaderPart.m_Preface);

  // Set the Operational Pattern label -- we're just starting and have no RIP or index,
  // so we tell the world by using OP1a
  m_HeaderPart.m_Preface->OperationalPattern = UL(Dict::ul(MDD_OP1a));
  m_HeaderPart.OperationalPattern = m_HeaderPart.m_Preface->OperationalPattern;
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(0, 0)); // First RIP Entry

  //
  // Identification
  //
  Identification* Ident = new Identification;
  m_HeaderPart.AddChildObject(Ident);
  m_HeaderPart.m_Preface->Identifications.push_back(Ident->InstanceUID);

  Kumu::GenRandomValue(Ident->ThisGenerationUID);
  Ident->CompanyName = m_Info.CompanyName.c_str();
  Ident->ProductName = m_Info.ProductName.c_str();
  Ident->VersionString = m_Info.ProductVersion.c_str();
  Ident->ProductUID.Set(m_Info.ProductUUID);
  Ident->Platform = ASDCP_PLATFORM;
  Ident->ToolkitVersion.Major = VERSION_MAJOR;
  Ident->ToolkitVersion.Minor = VERSION_APIMINOR;
  Ident->ToolkitVersion.Patch = VERSION_IMPMINOR;
  Ident->ToolkitVersion.Build = ASDCP_BUILD_NUMBER;
  Ident->ToolkitVersion.Release = VersionType::RL_RELEASE;

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
  PackageUMID.MakeUMID(0x0f); // unidentified essence
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

  m_MPTCSequence = new Sequence;
  m_HeaderPart.AddChildObject(m_MPTCSequence);
  NewTrack->Sequence = m_MPTCSequence->InstanceUID;
  m_MPTCSequence->DataDefinition = UL(Dict::ul(MDD_TimecodeDataDef));

  m_MPTimecode = new TimecodeComponent;
  m_HeaderPart.AddChildObject(m_MPTimecode);
  m_MPTCSequence->StructuralComponents.push_back(m_MPTimecode->InstanceUID);
  m_MPTimecode->RoundedTimecodeBase = TCFrameRate;
  m_MPTimecode->StartTimecode = ui64_C(0);
  m_MPTimecode->DataDefinition = UL(Dict::ul(MDD_TimecodeDataDef));

  // Essence Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_MaterialPackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 2;
  NewTrack->TrackName = TrackName.c_str();

  m_MPClSequence = new Sequence;
  m_HeaderPart.AddChildObject(m_MPClSequence);
  NewTrack->Sequence = m_MPClSequence->InstanceUID;
  m_MPClSequence->DataDefinition = DataDefinition;

  m_MPClip = new SourceClip;
  m_HeaderPart.AddChildObject(m_MPClip);
  m_MPClSequence->StructuralComponents.push_back(m_MPClip->InstanceUID);
  m_MPClip->DataDefinition = DataDefinition;

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
  m_MPClip->SourceTrackID = 2;

  m_HeaderPart.AddChildObject(m_FilePackage);
  Storage->Packages.push_back(m_FilePackage->InstanceUID);

  // Timecode Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_FilePackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 1;
  NewTrack->TrackName = "Timecode Track";

  m_FPTCSequence = new Sequence;
  m_HeaderPart.AddChildObject(m_FPTCSequence);
  NewTrack->Sequence = m_FPTCSequence->InstanceUID;
  m_FPTCSequence->DataDefinition = UL(Dict::ul(MDD_TimecodeDataDef));

  m_FPTimecode = new TimecodeComponent;
  m_HeaderPart.AddChildObject(m_FPTimecode);
  m_FPTCSequence->StructuralComponents.push_back(m_FPTimecode->InstanceUID);
  m_FPTimecode->RoundedTimecodeBase = TCFrameRate;
  m_FPTimecode->StartTimecode = ui64_C(86400); // 01:00:00:00
  m_FPTimecode->DataDefinition = UL(Dict::ul(MDD_TimecodeDataDef));

  // Essence Track
  NewTrack = new Track;
  m_HeaderPart.AddChildObject(NewTrack);
  NewTrack->EditRate = EditRate;
  m_FilePackage->Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackID = 2;
  NewTrack->TrackName = TrackName.c_str();

  m_FPClSequence = new Sequence;
  m_HeaderPart.AddChildObject(m_FPClSequence);
  NewTrack->Sequence = m_FPClSequence->InstanceUID;
  m_FPClSequence->DataDefinition = DataDefinition;

  m_FPClip = new SourceClip;
  m_HeaderPart.AddChildObject(m_FPClip);
  m_FPClSequence->StructuralComponents.push_back(m_FPClip->InstanceUID);
  m_FPClip->DataDefinition = DataDefinition;

  // for now we do not allow setting this value, so all files will be 'original'
  m_FPClip->SourceTrackID = 0;
  m_FPClip->SourcePackageID = NilUMID;

  //
  // Essence Descriptor
  //
  m_EssenceDescriptor->EssenceContainer = WrappingUL;
  m_EssenceDescriptor->LinkedTrackID = NewTrack->TrackID;
  m_HeaderPart.m_Preface->PrimaryPackage = m_FilePackage->InstanceUID;

  //
  // Encryption Descriptor
  //
  if ( m_Info.EncryptedEssence )
    {
      UL CryptEssenceUL(Dict::ul(MDD_EncryptedContainerLabel));
      m_HeaderPart.EssenceContainers.push_back(CryptEssenceUL);
      m_HeaderPart.m_Preface->DMSchemes.push_back(UL(Dict::ul(MDD_CryptographicFrameworkLabel)));
      AddDMScrypt(m_HeaderPart, *m_FilePackage, m_Info, WrappingUL);
    }
  else
    {
      m_HeaderPart.EssenceContainers.push_back(UL(Dict::ul(MDD_GCMulti)));
      m_HeaderPart.EssenceContainers.push_back(WrappingUL);
    }

  m_HeaderPart.m_Preface->EssenceContainers = m_HeaderPart.EssenceContainers;
  m_HeaderPart.AddChildObject(m_EssenceDescriptor);

  std::list<FileDescriptor*>::iterator sdli = m_EssenceSubDescriptorList.begin();
  for ( ; sdli != m_EssenceSubDescriptorList.end(); sdli++ )
    m_HeaderPart.AddChildObject(*sdli);

  m_FilePackage->Descriptor = m_EssenceDescriptor->InstanceUID;

  // Write the header partition
  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  // create a body partition of we're writing proper 429-3/OP-Atom
  if ( ASDCP_SUCCESS(result) && m_Info.LabelSetType == LS_MXF_SMPTE )
    {
      // Body Partition
      m_BodyPart.EssenceContainers = m_HeaderPart.EssenceContainers;
      m_BodyPart.ThisPartition = m_File.Tell();
      m_BodyPart.BodySID = 1;
      UL OPAtomUL(Dict::ul(MDD_OPAtom));
      m_BodyPart.OperationalPattern = OPAtomUL;
      m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(1, m_BodyPart.ThisPartition)); // Second RIP Entry
      
      UL BodyUL(Dict::ul(MDD_ClosedCompleteBodyPartition));
      result = m_BodyPart.WriteToFile(m_File, BodyUL);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      // Index setup
      Kumu::fpos_t ECoffset = m_File.Tell();

      if ( BytesPerEditUnit == 0 )
	m_FooterPart.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
      else
	m_FooterPart.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);
    }

  return result;
}



// standard method of writing a plaintext or encrypted frame
Result_t
ASDCP::h__Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
				  AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  IntegrityPack IntPack;

  byte_t overhead[128];
  Kumu::MemIOWriter Overhead(overhead, 128);

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
	  if ( m_Info.LabelSetType == LS_MXF_INTEROP )
	    Overhead.WriteRaw(Dict::ul(MDD_MXFInterop_CryptEssence), SMPTE_UL_LENGTH);
	  else
	    Overhead.WriteRaw(Dict::ul(MDD_CryptEssence), SMPTE_UL_LENGTH);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + m_CtFrameBuf.Size();

	  if ( m_Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (MXF_BER_LENGTH * 3); // for empty intpack

	  Overhead.WriteBER(ETLength, MXF_BER_LENGTH);                  // write encrypted triplet length
	  Overhead.WriteBER(UUIDlen, MXF_BER_LENGTH);                   // write ContextID length
	  Overhead.WriteRaw(m_Info.ContextID, UUIDlen);                  // write ContextID
	  Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH);            // write PlaintextOffset length
	  Overhead.WriteUi64BE(FrameBuf.PlaintextOffset());              // write PlaintextOffset
	  Overhead.WriteBER(SMPTE_UL_LENGTH, MXF_BER_LENGTH);              // write essence UL length
	  Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH);           // write the essence UL
	  Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH);            // write SourceLength length
	  Overhead.WriteUi64BE(FrameBuf.Size());                         // write SourceLength
	  Overhead.WriteBER(m_CtFrameBuf.Size(), MXF_BER_LENGTH);       // write ESV length

	  result = m_File.Writev(Overhead.Data(), Overhead.Length());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += Overhead.Length();
	  // write encrypted source value
	  result = m_File.Writev((byte_t*)m_CtFrameBuf.RoData(), m_CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += m_CtFrameBuf.Size();

	  byte_t hmoverhead[512];
	  Kumu::MemIOWriter HMACOverhead(hmoverhead, 512);

	  // write the HMAC
	  if ( m_Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, MXF_BER_LENGTH);
	    }

	  // write HMAC
	  result = m_File.Writev(HMACOverhead.Data(), HMACOverhead.Length());
	  m_StreamOffset += HMACOverhead.Length();
	}
    }
  else
    {
      Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH);
      Overhead.WriteBER(FrameBuf.Size(), MXF_BER_LENGTH);
      result = m_File.Writev(Overhead.Data(), Overhead.Length());
 
      if ( ASDCP_SUCCESS(result) )
	result = m_File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

      if ( ASDCP_SUCCESS(result) )
	m_StreamOffset += Overhead.Length() + FrameBuf.Size();
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

  m_MPTCSequence->Duration = m_MPTimecode->Duration = m_MPClSequence->Duration = m_MPClip->Duration = 
    m_FPTCSequence->Duration = m_FPTimecode->Duration = m_FPClSequence->Duration = m_FPClip->Duration = 
    m_EssenceDescriptor->ContainerDuration = m_FramesWritten;

  Kumu::fpos_t here = m_File.Tell();
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(0, here)); // Third RIP Entry
  m_HeaderPart.FooterPartition = here;

  // re-label the partition
  UL OPAtomUL(Dict::ul(MDD_OPAtom));

  if ( m_Info.LabelSetType == LS_MXF_INTEROP )
    OPAtomUL.Set(Dict::ul(MDD_MXFInterop_OPAtom));
  
  m_HeaderPart.OperationalPattern = OPAtomUL;
  m_HeaderPart.m_Preface->OperationalPattern = m_HeaderPart.OperationalPattern;

  m_FooterPart.PreviousPartition = m_BodyPart.ThisPartition;
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
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  m_File.Close();
  return result;
}

//
// end h__Writer.cpp
//
