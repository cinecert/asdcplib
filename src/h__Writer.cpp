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
void
ASDCP::h__Writer::InitHeader()
{
  assert(m_EssenceDescriptor);

  m_HeaderPart.m_Primer.ClearTagList();
  m_HeaderPart.m_Preface = new Preface;
  m_HeaderPart.AddChildObject(m_HeaderPart.m_Preface);

  // Set the Operational Pattern label -- we're just starting and have no RIP or index,
  // so we tell the world by using OP1a
  m_HeaderPart.m_Preface->OperationalPattern = UL(Dict::ul(MDD_OP1a));
  m_HeaderPart.OperationalPattern = m_HeaderPart.m_Preface->OperationalPattern;

  // First RIP Entry
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )
    m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(0, 0)); // 3-part, no essence in header
  else
    m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(1, 0)); // 2-part, essence in header

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
}

//
template <class ClipT>
struct TrackSet
{
  MXF::Track*    Track;
  MXF::Sequence* Sequence;
  ClipT*         Clip;

  TrackSet() : Track(0), Sequence(0), Clip(0) {}
};

//
template <class PackageT, class ClipT>
TrackSet<ClipT>
CreateTrackAndSequence(OPAtomHeader& Header, PackageT& Package, const std::string TrackName,
		       const MXF::Rational& EditRate, const UL& Definition, ui32_t TrackID)
{
  TrackSet<ClipT> NewTrack;

  NewTrack.Track = new Track;
  Header.AddChildObject(NewTrack.Track);
  NewTrack.Track->EditRate = EditRate;
  Package.Tracks.push_back(NewTrack.Track->InstanceUID);
  NewTrack.Track->TrackID = TrackID;
  NewTrack.Track->TrackName = TrackName.c_str();

  NewTrack.Sequence = new Sequence;
  Header.AddChildObject(NewTrack.Sequence);
  NewTrack.Track->Sequence = NewTrack.Sequence->InstanceUID;
  NewTrack.Sequence->DataDefinition = Definition;

  return NewTrack;
}

//
template <class PackageT>
TrackSet<TimecodeComponent>
CreateTimecodeTrack(OPAtomHeader& Header, PackageT& Package,
		    const MXF::Rational& EditRate, ui32_t TCFrameRate, ui64_t TCStart)
{
  UL TCUL(Dict::ul(MDD_TimecodeDataDef));

  TrackSet<TimecodeComponent> NewTrack = CreateTrackAndSequence<PackageT, TimecodeComponent>(Header, Package, "Timecode Track", EditRate, TCUL, 1);

  NewTrack.Clip = new TimecodeComponent;
  Header.AddChildObject(NewTrack.Clip);
  NewTrack.Sequence->StructuralComponents.push_back(NewTrack.Clip->InstanceUID);
  NewTrack.Clip->RoundedTimecodeBase = TCFrameRate;
  NewTrack.Clip->StartTimecode = TCStart;
  NewTrack.Clip->DataDefinition = TCUL;

  return NewTrack;
}


//
void
ASDCP::h__Writer::AddSourceClip(const MXF::Rational& EditRate, ui32_t TCFrameRate,
				const std::string& TrackName, const UL& DataDefinition,
				const std::string& PackageLabel)
{
  //
  ContentStorage* Storage = new ContentStorage;
  m_HeaderPart.AddChildObject(Storage);
  m_HeaderPart.m_Preface->ContentStorage = Storage->InstanceUID;

  EssenceContainerData* ECD = new EssenceContainerData;
  m_HeaderPart.AddChildObject(ECD);
  Storage->EssenceContainerData.push_back(ECD->InstanceUID);
  ECD->IndexSID = 129;
  ECD->BodySID = 1;

  UUID assetUUID(m_Info.AssetUUID);
  UMID SourcePackageUMID, MaterialPackageUMID;
  SourcePackageUMID.MakeUMID(0x0f, assetUUID);
  MaterialPackageUMID.MakeUMID(0x0f); // unidentified essence

  //
  // Material Package
  //
  m_MaterialPackage = new MaterialPackage;
  m_MaterialPackage->Name = "AS-DCP Material Package";
  m_MaterialPackage->PackageUID = MaterialPackageUMID;
  m_HeaderPart.AddChildObject(m_MaterialPackage);
  Storage->Packages.push_back(m_MaterialPackage->InstanceUID);

  TrackSet<TimecodeComponent> MPTCTrack = CreateTimecodeTrack<MaterialPackage>(m_HeaderPart, *m_MaterialPackage,
									       EditRate, TCFrameRate, 0);
  m_DurationUpdateList.push_back(&(MPTCTrack.Sequence->Duration));
  m_DurationUpdateList.push_back(&(MPTCTrack.Clip->Duration));

  TrackSet<SourceClip> MPTrack = CreateTrackAndSequence<MaterialPackage, SourceClip>(m_HeaderPart, *m_MaterialPackage,
									   TrackName, EditRate, DataDefinition, 2);
  m_DurationUpdateList.push_back(&(MPTrack.Sequence->Duration));

  MPTrack.Clip = new SourceClip;
  m_HeaderPart.AddChildObject(MPTrack.Clip);
  MPTrack.Sequence->StructuralComponents.push_back(MPTrack.Clip->InstanceUID);
  MPTrack.Clip->DataDefinition = DataDefinition;
  MPTrack.Clip->SourcePackageID = SourcePackageUMID;
  MPTrack.Clip->SourceTrackID = 2;
  m_DurationUpdateList.push_back(&(MPTrack.Clip->Duration));

  
  //
  // File (Source) Package
  //
  m_FilePackage = new SourcePackage;
  m_FilePackage->Name = PackageLabel.c_str();
  m_FilePackage->PackageUID = SourcePackageUMID;
  ECD->LinkedPackageUID = SourcePackageUMID;

  m_HeaderPart.AddChildObject(m_FilePackage);
  Storage->Packages.push_back(m_FilePackage->InstanceUID);

  TrackSet<TimecodeComponent> FPTCTrack = CreateTimecodeTrack<SourcePackage>(m_HeaderPart, *m_FilePackage,
									     EditRate, TCFrameRate, ui64_C(3600) * TCFrameRate);
  m_DurationUpdateList.push_back(&(FPTCTrack.Sequence->Duration));
  m_DurationUpdateList.push_back(&(FPTCTrack.Clip->Duration));

  TrackSet<SourceClip> FPTrack = CreateTrackAndSequence<SourcePackage, SourceClip>(m_HeaderPart, *m_FilePackage,
									 TrackName, EditRate, DataDefinition, 2);
  m_DurationUpdateList.push_back(&(FPTrack.Sequence->Duration));

  FPTrack.Clip = new SourceClip;
  m_HeaderPart.AddChildObject(FPTrack.Clip);
  FPTrack.Sequence->StructuralComponents.push_back(FPTrack.Clip->InstanceUID);
  FPTrack.Clip->DataDefinition = DataDefinition;

  // for now we do not allow setting this value, so all files will be 'original'
  FPTrack.Clip->SourceTrackID = 0;
  FPTrack.Clip->SourcePackageID = NilUMID;
  m_DurationUpdateList.push_back(&(FPTrack.Clip->Duration));

  m_EssenceDescriptor->LinkedTrackID = FPTrack.Track->TrackID;
}

//
void
ASDCP::h__Writer::AddDMSegment(const MXF::Rational& EditRate, ui32_t TCFrameRate,
				const std::string& TrackName, const UL& DataDefinition,
			       const std::string& PackageLabel, const UMID& SourcePackageUMID)
{
  //
  ContentStorage* Storage = new ContentStorage;
  m_HeaderPart.AddChildObject(Storage);
  m_HeaderPart.m_Preface->ContentStorage = Storage->InstanceUID;

  EssenceContainerData* ECD = new EssenceContainerData;
  m_HeaderPart.AddChildObject(ECD);
  Storage->EssenceContainerData.push_back(ECD->InstanceUID);
  ECD->IndexSID = 129;
  ECD->BodySID = 1;

  //  UUID assetUUID(m_Info.AssetUUID);
  UMID MaterialPackageUMID;
  //  SourcePackageUMID.MakeUMID(0x0f, assetUUID);
  MaterialPackageUMID.MakeUMID(0x0f); // unidentified essence

  //
  // Material Package
  //
  m_MaterialPackage = new MaterialPackage;
  m_MaterialPackage->Name = "AS-DCP Material Package";
  m_MaterialPackage->PackageUID = MaterialPackageUMID;
  m_HeaderPart.AddChildObject(m_MaterialPackage);
  Storage->Packages.push_back(m_MaterialPackage->InstanceUID);

  TrackSet<TimecodeComponent> MPTCTrack = CreateTimecodeTrack<MaterialPackage>(m_HeaderPart, *m_MaterialPackage,
									       EditRate, TCFrameRate, 0);
  m_DurationUpdateList.push_back(&(MPTCTrack.Sequence->Duration));
  m_DurationUpdateList.push_back(&(MPTCTrack.Clip->Duration));

  TrackSet<DMSegment> MPTrack = CreateTrackAndSequence<MaterialPackage, DMSegment>(m_HeaderPart, *m_MaterialPackage,
							       TrackName, EditRate, DataDefinition, 2);
  m_DurationUpdateList.push_back(&(MPTrack.Sequence->Duration));

  MPTrack.Clip = new DMSegment;
  m_HeaderPart.AddChildObject(MPTrack.Clip);
  MPTrack.Sequence->StructuralComponents.push_back(MPTrack.Clip->InstanceUID);
  MPTrack.Clip->DataDefinition = DataDefinition;
  //  MPTrack.Clip->SourcePackageID = SourcePackageUMID;
  //  MPTrack.Clip->SourceTrackID = 2;
  m_DurationUpdateList.push_back(&(MPTrack.Clip->Duration));

  
  //
  // File (Source) Package
  //
  m_FilePackage = new SourcePackage;
  m_FilePackage->Name = PackageLabel.c_str();
  m_FilePackage->PackageUID = SourcePackageUMID;
  ECD->LinkedPackageUID = SourcePackageUMID;

  m_HeaderPart.AddChildObject(m_FilePackage);
  Storage->Packages.push_back(m_FilePackage->InstanceUID);

  TrackSet<TimecodeComponent> FPTCTrack = CreateTimecodeTrack<SourcePackage>(m_HeaderPart, *m_FilePackage,
									     EditRate, TCFrameRate, ui64_C(3600) * TCFrameRate);
  m_DurationUpdateList.push_back(&(FPTCTrack.Sequence->Duration));
  m_DurationUpdateList.push_back(&(FPTCTrack.Clip->Duration));

  TrackSet<DMSegment> FPTrack = CreateTrackAndSequence<SourcePackage, DMSegment>(m_HeaderPart, *m_FilePackage,
										 TrackName, EditRate, DataDefinition, 2);
  m_DurationUpdateList.push_back(&(FPTrack.Sequence->Duration));

  FPTrack.Clip = new DMSegment;
  m_HeaderPart.AddChildObject(FPTrack.Clip);
  FPTrack.Sequence->StructuralComponents.push_back(FPTrack.Clip->InstanceUID);
  FPTrack.Clip->DataDefinition = DataDefinition;
  FPTrack.Clip->EventComment = "D-Cinema Timed Text";

  m_DurationUpdateList.push_back(&(FPTrack.Clip->Duration));
  m_EssenceDescriptor->LinkedTrackID = FPTrack.Track->TrackID;
}

//
void
ASDCP::h__Writer::AddEssenceDescriptor(const UL& WrappingUL)
{
  //
  // Essence Descriptor
  //
  m_EssenceDescriptor->EssenceContainer = WrappingUL;
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
}

//
Result_t
ASDCP::h__Writer::CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit)
{
  Result_t result = RESULT_OK;

  // create a body partition if we're writing proper 429-3/OP-Atom
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )
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
  else
    {
      m_HeaderPart.BodySID = 1;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      // Index setup
      Kumu::fpos_t ECoffset = m_File.Tell();
      m_FooterPart.IndexSID = 129;

      if ( BytesPerEditUnit == 0 )
	m_FooterPart.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
      else
	m_FooterPart.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);
    }

  return result;
}

//
Result_t
ASDCP::h__Writer::WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
				 const std::string& TrackName, const UL& DataDefinition,
				 const MXF::Rational& EditRate, ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  InitHeader();
  AddSourceClip(EditRate, TCFrameRate, TrackName, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( KM_SUCCESS(result) )
    result = CreateBodyPart(EditRate, BytesPerEditUnit);

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

  //  m_MPTCSequence->Duration = m_MPTimecode->Duration = m_MPClSequence->Duration = m_MPClip->Duration = 
  //    m_FPTCSequence->Duration = m_FPTimecode->Duration = m_FPClSequence->Duration = m_FPClip->Duration = 

  DurationElementList_t::iterator dli = m_DurationUpdateList.begin();

  for (; dli != m_DurationUpdateList.end(); dli++ )
    **dli = m_FramesWritten;

  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
  m_FooterPart.PreviousPartition = m_HeaderPart.m_RIP.PairArray.back().ByteOffset;

  Kumu::fpos_t here = m_File.Tell();
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(0, here)); // Last RIP Entry
  m_HeaderPart.FooterPartition = here;

  // re-label the partition
  UL OPAtomUL(Dict::ul(MDD_OPAtom));

  if ( m_Info.LabelSetType == LS_MXF_INTEROP )
    OPAtomUL.Set(Dict::ul(MDD_MXFInterop_OPAtom));
  
  m_HeaderPart.OperationalPattern = OPAtomUL;
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
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  m_File.Close();
  return result;
}

//
// end h__Writer.cpp
//
