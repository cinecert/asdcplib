/*
Copyright (c) 2005-2012, John Hurst
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
/*! \file    Metadata.cpp
    \version $Id$       
    \brief   AS-DCP library, MXF Metadata Sets implementation
*/


#include <KM_mutex.h>
#include "Metadata.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

//------------------------------------------------------------------------------------------

static InterchangeObject* Preface_Factory(const Dictionary*& Dict) { return new Preface(Dict); }
static InterchangeObject* IndexTableSegment_Factory(const Dictionary*& Dict) { return new IndexTableSegment(Dict); }

static InterchangeObject* Identification_Factory(const Dictionary*& Dict) { return new Identification(Dict); }
static InterchangeObject* ContentStorage_Factory(const Dictionary*& Dict) { return new ContentStorage(Dict); }
static InterchangeObject* EssenceContainerData_Factory(const Dictionary*& Dict) { return new EssenceContainerData(Dict); }
static InterchangeObject* MaterialPackage_Factory(const Dictionary*& Dict) { return new MaterialPackage(Dict); }
static InterchangeObject* SourcePackage_Factory(const Dictionary*& Dict) { return new SourcePackage(Dict); }
static InterchangeObject* StaticTrack_Factory(const Dictionary*& Dict) { return new StaticTrack(Dict); }
static InterchangeObject* Track_Factory(const Dictionary*& Dict) { return new Track(Dict); }
static InterchangeObject* Sequence_Factory(const Dictionary*& Dict) { return new Sequence(Dict); }
static InterchangeObject* SourceClip_Factory(const Dictionary*& Dict) { return new SourceClip(Dict); }
static InterchangeObject* TimecodeComponent_Factory(const Dictionary*& Dict) { return new TimecodeComponent(Dict); }
static InterchangeObject* FileDescriptor_Factory(const Dictionary*& Dict) { return new FileDescriptor(Dict); }
static InterchangeObject* GenericSoundEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericSoundEssenceDescriptor(Dict); }
static InterchangeObject* WaveAudioDescriptor_Factory(const Dictionary*& Dict) { return new WaveAudioDescriptor(Dict); }
static InterchangeObject* GenericPictureEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericPictureEssenceDescriptor(Dict); }
static InterchangeObject* RGBAEssenceDescriptor_Factory(const Dictionary*& Dict) { return new RGBAEssenceDescriptor(Dict); }
static InterchangeObject* JPEG2000PictureSubDescriptor_Factory(const Dictionary*& Dict) { return new JPEG2000PictureSubDescriptor(Dict); }
static InterchangeObject* CDCIEssenceDescriptor_Factory(const Dictionary*& Dict) { return new CDCIEssenceDescriptor(Dict); }
static InterchangeObject* MPEG2VideoDescriptor_Factory(const Dictionary*& Dict) { return new MPEG2VideoDescriptor(Dict); }
static InterchangeObject* DMSegment_Factory(const Dictionary*& Dict) { return new DMSegment(Dict); }
static InterchangeObject* CryptographicFramework_Factory(const Dictionary*& Dict) { return new CryptographicFramework(Dict); }
static InterchangeObject* CryptographicContext_Factory(const Dictionary*& Dict) { return new CryptographicContext(Dict); }
static InterchangeObject* GenericDataEssenceDescriptor_Factory(const Dictionary*& Dict) { return new GenericDataEssenceDescriptor(Dict); }
static InterchangeObject* TimedTextDescriptor_Factory(const Dictionary*& Dict) { return new TimedTextDescriptor(Dict); }
static InterchangeObject* TimedTextResourceSubDescriptor_Factory(const Dictionary*& Dict) { return new TimedTextResourceSubDescriptor(Dict); }
static InterchangeObject* StereoscopicPictureSubDescriptor_Factory(const Dictionary*& Dict) { return new StereoscopicPictureSubDescriptor(Dict); }
static InterchangeObject* NetworkLocator_Factory(const Dictionary*& Dict) { return new NetworkLocator(Dict); }
static InterchangeObject* MCALabelSubDescriptor_Factory(const Dictionary*& Dict) { return new MCALabelSubDescriptor(Dict); }
static InterchangeObject* AudioChannelLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new AudioChannelLabelSubDescriptor(Dict); }
static InterchangeObject* SoundfieldGroupLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new SoundfieldGroupLabelSubDescriptor(Dict); }
static InterchangeObject* GroupOfSoundfieldGroupsLabelSubDescriptor_Factory(const Dictionary*& Dict) { return new GroupOfSoundfieldGroupsLabelSubDescriptor(Dict); }


void
ASDCP::MXF::Metadata_InitTypes(const Dictionary*& Dict)
{
  assert(Dict);
  SetObjectFactory(Dict->ul(MDD_Preface), Preface_Factory);
  SetObjectFactory(Dict->ul(MDD_IndexTableSegment), IndexTableSegment_Factory);

  SetObjectFactory(Dict->ul(MDD_Identification), Identification_Factory);
  SetObjectFactory(Dict->ul(MDD_ContentStorage), ContentStorage_Factory);
  SetObjectFactory(Dict->ul(MDD_EssenceContainerData), EssenceContainerData_Factory);
  SetObjectFactory(Dict->ul(MDD_MaterialPackage), MaterialPackage_Factory);
  SetObjectFactory(Dict->ul(MDD_SourcePackage), SourcePackage_Factory);
  SetObjectFactory(Dict->ul(MDD_StaticTrack), StaticTrack_Factory);
  SetObjectFactory(Dict->ul(MDD_Track), Track_Factory);
  SetObjectFactory(Dict->ul(MDD_Sequence), Sequence_Factory);
  SetObjectFactory(Dict->ul(MDD_SourceClip), SourceClip_Factory);
  SetObjectFactory(Dict->ul(MDD_TimecodeComponent), TimecodeComponent_Factory);
  SetObjectFactory(Dict->ul(MDD_FileDescriptor), FileDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericSoundEssenceDescriptor), GenericSoundEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_WaveAudioDescriptor), WaveAudioDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericPictureEssenceDescriptor), GenericPictureEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_RGBAEssenceDescriptor), RGBAEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_JPEG2000PictureSubDescriptor), JPEG2000PictureSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_CDCIEssenceDescriptor), CDCIEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_MPEG2VideoDescriptor), MPEG2VideoDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_DMSegment), DMSegment_Factory);
  SetObjectFactory(Dict->ul(MDD_CryptographicFramework), CryptographicFramework_Factory);
  SetObjectFactory(Dict->ul(MDD_CryptographicContext), CryptographicContext_Factory);
  SetObjectFactory(Dict->ul(MDD_GenericDataEssenceDescriptor), GenericDataEssenceDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_TimedTextDescriptor), TimedTextDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_TimedTextResourceSubDescriptor), TimedTextResourceSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_StereoscopicPictureSubDescriptor), StereoscopicPictureSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_NetworkLocator), NetworkLocator_Factory);
  SetObjectFactory(Dict->ul(MDD_MCALabelSubDescriptor), MCALabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_AudioChannelLabelSubDescriptor), AudioChannelLabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_SoundfieldGroupLabelSubDescriptor), SoundfieldGroupLabelSubDescriptor_Factory);
  SetObjectFactory(Dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor), GroupOfSoundfieldGroupsLabelSubDescriptor_Factory);
}

//------------------------------------------------------------------------------------------
// KLV Sets



//------------------------------------------------------------------------------------------
// Identification

//
ASDCP::Result_t
Identification::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ThisGenerationUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, CompanyName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, VersionString));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ModificationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ToolkitVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, Platform));
  return result;
}

//
ASDCP::Result_t
Identification::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ThisGenerationUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, CompanyName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, VersionString));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ProductUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ModificationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, ToolkitVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Identification, Platform));
  return result;
}

//
void
Identification::Copy(const Identification& rhs)
{
  ThisGenerationUID = rhs.ThisGenerationUID;
  CompanyName = rhs.CompanyName;
  ProductName = rhs.ProductName;
  ProductVersion = rhs.ProductVersion;
  VersionString = rhs.VersionString;
  ProductUID = rhs.ProductUID;
  ModificationDate = rhs.ModificationDate;
  ToolkitVersion = rhs.ToolkitVersion;
  Platform = rhs.Platform;
}

//
void
Identification::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ThisGenerationUID", ThisGenerationUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CompanyName", CompanyName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductName", ProductName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductVersion", ProductVersion.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "VersionString", VersionString.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ProductUID", ProductUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ModificationDate", ModificationDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ToolkitVersion", ToolkitVersion.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Platform", Platform.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
Identification::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Identification));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Identification::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Identification));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// ContentStorage

//
ASDCP::Result_t
ContentStorage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
ASDCP::Result_t
ContentStorage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
void
ContentStorage::Copy(const ContentStorage& rhs)
{
  Packages = rhs.Packages;
  EssenceContainerData = rhs.EssenceContainerData;
}

//
void
ContentStorage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s:\n",  "Packages");
  Packages.Dump(stream);
  fprintf(stream, "  %22s:\n",  "EssenceContainerData");
  EssenceContainerData.Dump(stream);
}

//
ASDCP::Result_t
ContentStorage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_ContentStorage));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ContentStorage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_ContentStorage));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// EssenceContainerData

//
ASDCP::Result_t
EssenceContainerData::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
ASDCP::Result_t
EssenceContainerData::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
void
EssenceContainerData::Copy(const EssenceContainerData& rhs)
{
  LinkedPackageUID = rhs.LinkedPackageUID;
  IndexSID = rhs.IndexSID;
  BodySID = rhs.BodySID;
}

//
void
EssenceContainerData::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "LinkedPackageUID", LinkedPackageUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "IndexSID", IndexSID);
  fprintf(stream, "  %22s = %d\n",  "BodySID", BodySID);
}

//
ASDCP::Result_t
EssenceContainerData::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_EssenceContainerData));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
EssenceContainerData::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_EssenceContainerData));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPackage

//
ASDCP::Result_t
GenericPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, Name));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageCreationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPackage, Tracks));
  return result;
}

//
ASDCP::Result_t
GenericPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, Name));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageCreationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, PackageModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPackage, Tracks));
  return result;
}

//
void
GenericPackage::Copy(const GenericPackage& rhs)
{
  PackageUID = rhs.PackageUID;
  Name = rhs.Name;
  PackageCreationDate = rhs.PackageCreationDate;
  PackageModifiedDate = rhs.PackageModifiedDate;
  Tracks = rhs.Tracks;
}

//
void
GenericPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "PackageUID", PackageUID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Name", Name.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "PackageCreationDate", PackageCreationDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "PackageModifiedDate", PackageModifiedDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s:\n",  "Tracks");
  Tracks.Dump(stream);
}


//------------------------------------------------------------------------------------------
// MaterialPackage

//
ASDCP::Result_t
MaterialPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
MaterialPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  return result;
}

//
void
MaterialPackage::Copy(const MaterialPackage& rhs)
{
}

//
void
MaterialPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
}

//
ASDCP::Result_t
MaterialPackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MaterialPackage));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MaterialPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MaterialPackage));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourcePackage

//
ASDCP::Result_t
SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourcePackage, Descriptor));
  return result;
}

//
ASDCP::Result_t
SourcePackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourcePackage, Descriptor));
  return result;
}

//
void
SourcePackage::Copy(const SourcePackage& rhs)
{
  Descriptor = rhs.Descriptor;
}

//
void
SourcePackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "Descriptor", Descriptor.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
SourcePackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SourcePackage));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourcePackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SourcePackage));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericTrack

//
ASDCP::Result_t
GenericTrack::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackNumber));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericTrack, TrackName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericTrack, Sequence));
  return result;
}

//
ASDCP::Result_t
GenericTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackNumber));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericTrack, TrackName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericTrack, Sequence));
  return result;
}

//
void
GenericTrack::Copy(const GenericTrack& rhs)
{
  TrackID = rhs.TrackID;
  TrackNumber = rhs.TrackNumber;
  TrackName = rhs.TrackName;
  Sequence = rhs.Sequence;
}

//
void
GenericTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "TrackID", TrackID);
  fprintf(stream, "  %22s = %d\n",  "TrackNumber", TrackNumber);
  fprintf(stream, "  %22s = %s\n",  "TrackName", TrackName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Sequence", Sequence.EncodeString(identbuf, IdentBufferLen));
}


//------------------------------------------------------------------------------------------
// StaticTrack

//
ASDCP::Result_t
StaticTrack::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StaticTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  return result;
}

//
void
StaticTrack::Copy(const StaticTrack& rhs)
{
}

//
void
StaticTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
}

//
ASDCP::Result_t
StaticTrack::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_StaticTrack));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StaticTrack::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_StaticTrack));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// Track

//
ASDCP::Result_t
Track::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(Track, Origin));
  return result;
}

//
ASDCP::Result_t
Track::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(Track, Origin));
  return result;
}

//
void
Track::Copy(const Track& rhs)
{
  EditRate = rhs.EditRate;
  Origin = rhs.Origin;
}

//
void
Track::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "EditRate", EditRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Origin", i64sz(Origin, identbuf));
}

//
ASDCP::Result_t
Track::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Track));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Track::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Track));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StructuralComponent

//
ASDCP::Result_t
StructuralComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
  return result;
}

//
ASDCP::Result_t
StructuralComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(StructuralComponent, Duration));
  return result;
}

//
void
StructuralComponent::Copy(const StructuralComponent& rhs)
{
  DataDefinition = rhs.DataDefinition;
  Duration = rhs.Duration;
}

//
void
StructuralComponent::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Duration", i64sz(Duration, identbuf));
}


//------------------------------------------------------------------------------------------
// Sequence

//
ASDCP::Result_t
Sequence::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Sequence, StructuralComponents));
  return result;
}

//
ASDCP::Result_t
Sequence::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Sequence, StructuralComponents));
  return result;
}

//
void
Sequence::Copy(const Sequence& rhs)
{
  StructuralComponents = rhs.StructuralComponents;
}

//
void
Sequence::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s:\n",  "StructuralComponents");
  StructuralComponents.Dump(stream);
}

//
ASDCP::Result_t
Sequence::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Sequence));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Sequence::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_Sequence));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourceClip

//
ASDCP::Result_t
SourceClip::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
ASDCP::Result_t
SourceClip::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
void
SourceClip::Copy(const SourceClip& rhs)
{
  StartPosition = rhs.StartPosition;
  SourcePackageID = rhs.SourcePackageID;
  SourceTrackID = rhs.SourceTrackID;
}

//
void
SourceClip::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "StartPosition", i64sz(StartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "SourcePackageID", SourcePackageID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "SourceTrackID", SourceTrackID);
}

//
ASDCP::Result_t
SourceClip::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SourceClip));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourceClip::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SourceClip));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimecodeComponent

//
ASDCP::Result_t
TimecodeComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
ASDCP::Result_t
TimecodeComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
void
TimecodeComponent::Copy(const TimecodeComponent& rhs)
{
  RoundedTimecodeBase = rhs.RoundedTimecodeBase;
  StartTimecode = rhs.StartTimecode;
  DropFrame = rhs.DropFrame;
}

//
void
TimecodeComponent::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "RoundedTimecodeBase", RoundedTimecodeBase);
  fprintf(stream, "  %22s = %s\n",  "StartTimecode", i64sz(StartTimecode, identbuf));
  fprintf(stream, "  %22s = %d\n",  "DropFrame", DropFrame);
}

//
ASDCP::Result_t
TimecodeComponent::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimecodeComponent));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimecodeComponent::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimecodeComponent));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDescriptor

//
ASDCP::Result_t
GenericDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
ASDCP::Result_t
GenericDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
void
GenericDescriptor::Copy(const GenericDescriptor& rhs)
{
  Locators = rhs.Locators;
  SubDescriptors = rhs.SubDescriptors;
}

//
void
GenericDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s:\n",  "Locators");
  Locators.Dump(stream);
  fprintf(stream, "  %22s:\n",  "SubDescriptors");
  SubDescriptors.Dump(stream);
}


//------------------------------------------------------------------------------------------
// FileDescriptor

//
ASDCP::Result_t
FileDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(FileDescriptor, LinkedTrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(FileDescriptor, SampleRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(FileDescriptor, ContainerDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(FileDescriptor, EssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(FileDescriptor, Codec));
  return result;
}

//
ASDCP::Result_t
FileDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(FileDescriptor, LinkedTrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(FileDescriptor, SampleRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(FileDescriptor, ContainerDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(FileDescriptor, EssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(FileDescriptor, Codec));
  return result;
}

//
void
FileDescriptor::Copy(const FileDescriptor& rhs)
{
  LinkedTrackID = rhs.LinkedTrackID;
  SampleRate = rhs.SampleRate;
  ContainerDuration = rhs.ContainerDuration;
  EssenceContainer = rhs.EssenceContainer;
  Codec = rhs.Codec;
}

//
void
FileDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "LinkedTrackID", LinkedTrackID);
  fprintf(stream, "  %22s = %s\n",  "SampleRate", SampleRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "ContainerDuration", i64sz(ContainerDuration, identbuf));
  fprintf(stream, "  %22s = %s\n",  "EssenceContainer", EssenceContainer.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "Codec", Codec.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
FileDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_FileDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
FileDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_FileDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericSoundEssenceDescriptor

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, AudioSamplingRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, Locked));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, AudioRefLevel));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, DialNorm));
  return result;
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, AudioSamplingRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, Locked));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, AudioRefLevel));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericSoundEssenceDescriptor, DialNorm));
  return result;
}

//
void
GenericSoundEssenceDescriptor::Copy(const GenericSoundEssenceDescriptor& rhs)
{
  AudioSamplingRate = rhs.AudioSamplingRate;
  Locked = rhs.Locked;
  AudioRefLevel = rhs.AudioRefLevel;
  ChannelCount = rhs.ChannelCount;
  QuantizationBits = rhs.QuantizationBits;
  DialNorm = rhs.DialNorm;
}

//
void
GenericSoundEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AudioSamplingRate", AudioSamplingRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "Locked", Locked);
  fprintf(stream, "  %22s = %d\n",  "AudioRefLevel", AudioRefLevel);
  fprintf(stream, "  %22s = %d\n",  "ChannelCount", ChannelCount);
  fprintf(stream, "  %22s = %d\n",  "QuantizationBits", QuantizationBits);
  fprintf(stream, "  %22s = %d\n",  "DialNorm", DialNorm);
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericSoundEssenceDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericSoundEssenceDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// WaveAudioDescriptor

//
ASDCP::Result_t
WaveAudioDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericSoundEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(WaveAudioDescriptor, AvgBps));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(WaveAudioDescriptor, ChannelAssignment));
  return result;
}

//
ASDCP::Result_t
WaveAudioDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericSoundEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(WaveAudioDescriptor, AvgBps));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(WaveAudioDescriptor, ChannelAssignment));
  return result;
}

//
void
WaveAudioDescriptor::Copy(const WaveAudioDescriptor& rhs)
{
  BlockAlign = rhs.BlockAlign;
  SequenceOffset = rhs.SequenceOffset;
  AvgBps = rhs.AvgBps;
  ChannelAssignment = rhs.ChannelAssignment;
}

//
void
WaveAudioDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericSoundEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "BlockAlign", BlockAlign);
  fprintf(stream, "  %22s = %d\n",  "SequenceOffset", SequenceOffset);
  fprintf(stream, "  %22s = %d\n",  "AvgBps", AvgBps);
  fprintf(stream, "  %22s = %s\n",  "ChannelAssignment", ChannelAssignment.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
WaveAudioDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_WaveAudioDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
WaveAudioDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_WaveAudioDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPictureEssenceDescriptor

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, PictureEssenceCoding));
  return result;
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, PictureEssenceCoding));
  return result;
}

//
void
GenericPictureEssenceDescriptor::Copy(const GenericPictureEssenceDescriptor& rhs)
{
  FrameLayout = rhs.FrameLayout;
  StoredWidth = rhs.StoredWidth;
  StoredHeight = rhs.StoredHeight;
  AspectRatio = rhs.AspectRatio;
  PictureEssenceCoding = rhs.PictureEssenceCoding;
}

//
void
GenericPictureEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "FrameLayout", FrameLayout);
  fprintf(stream, "  %22s = %d\n",  "StoredWidth", StoredWidth);
  fprintf(stream, "  %22s = %d\n",  "StoredHeight", StoredHeight);
  fprintf(stream, "  %22s = %s\n",  "AspectRatio", AspectRatio.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "PictureEssenceCoding", PictureEssenceCoding.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericPictureEssenceDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericPictureEssenceDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// RGBAEssenceDescriptor

//
ASDCP::Result_t
RGBAEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  return result;
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  return result;
}

//
void
RGBAEssenceDescriptor::Copy(const RGBAEssenceDescriptor& rhs)
{
  ComponentMaxRef = rhs.ComponentMaxRef;
  ComponentMinRef = rhs.ComponentMinRef;
}

//
void
RGBAEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "ComponentMaxRef", ComponentMaxRef);
  fprintf(stream, "  %22s = %d\n",  "ComponentMinRef", ComponentMinRef);
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_RGBAEssenceDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_RGBAEssenceDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// JPEG2000PictureSubDescriptor

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Rsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Xsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Ysize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, XTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, YTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, Csize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, PictureComponentSizing));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, CodingStyleDefault));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(JPEG2000PictureSubDescriptor, QuantizationDefault));
  return result;
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Rsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Xsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Ysize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YTsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, XTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, YTOsize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, Csize));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, PictureComponentSizing));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, CodingStyleDefault));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(JPEG2000PictureSubDescriptor, QuantizationDefault));
  return result;
}

//
void
JPEG2000PictureSubDescriptor::Copy(const JPEG2000PictureSubDescriptor& rhs)
{
  Rsize = rhs.Rsize;
  Xsize = rhs.Xsize;
  Ysize = rhs.Ysize;
  XOsize = rhs.XOsize;
  YOsize = rhs.YOsize;
  XTsize = rhs.XTsize;
  YTsize = rhs.YTsize;
  XTOsize = rhs.XTOsize;
  YTOsize = rhs.YTOsize;
  Csize = rhs.Csize;
  PictureComponentSizing = rhs.PictureComponentSizing;
  CodingStyleDefault = rhs.CodingStyleDefault;
  QuantizationDefault = rhs.QuantizationDefault;
}

//
void
JPEG2000PictureSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "Rsize", Rsize);
  fprintf(stream, "  %22s = %d\n",  "Xsize", Xsize);
  fprintf(stream, "  %22s = %d\n",  "Ysize", Ysize);
  fprintf(stream, "  %22s = %d\n",  "XOsize", XOsize);
  fprintf(stream, "  %22s = %d\n",  "YOsize", YOsize);
  fprintf(stream, "  %22s = %d\n",  "XTsize", XTsize);
  fprintf(stream, "  %22s = %d\n",  "YTsize", YTsize);
  fprintf(stream, "  %22s = %d\n",  "XTOsize", XTOsize);
  fprintf(stream, "  %22s = %d\n",  "YTOsize", YTOsize);
  fprintf(stream, "  %22s = %d\n",  "Csize", Csize);
  fprintf(stream, "  %22s = %s\n",  "PictureComponentSizing", PictureComponentSizing.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CodingStyleDefault", CodingStyleDefault.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "QuantizationDefault", QuantizationDefault.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_JPEG2000PictureSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_JPEG2000PictureSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CDCIEssenceDescriptor

//
ASDCP::Result_t
CDCIEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, ComponentDepth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, VerticalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(CDCIEssenceDescriptor, ColorSiting));
  return result;
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, ComponentDepth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, VerticalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, ColorSiting));
  return result;
}

//
void
CDCIEssenceDescriptor::Copy(const CDCIEssenceDescriptor& rhs)
{
  ComponentDepth = rhs.ComponentDepth;
  HorizontalSubsampling = rhs.HorizontalSubsampling;
  VerticalSubsampling = rhs.VerticalSubsampling;
  ColorSiting = rhs.ColorSiting;
}

//
void
CDCIEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "ComponentDepth", ComponentDepth);
  fprintf(stream, "  %22s = %d\n",  "HorizontalSubsampling", HorizontalSubsampling);
  fprintf(stream, "  %22s = %d\n",  "VerticalSubsampling", VerticalSubsampling);
  fprintf(stream, "  %22s = %d\n",  "ColorSiting", ColorSiting);
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CDCIEssenceDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CDCIEssenceDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MPEG2VideoDescriptor

//
ASDCP::Result_t
MPEG2VideoDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = CDCIEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(MPEG2VideoDescriptor, CodedContentType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(MPEG2VideoDescriptor, LowDelay));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(MPEG2VideoDescriptor, BitRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(MPEG2VideoDescriptor, ProfileAndLevel));
  return result;
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = CDCIEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, CodedContentType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, LowDelay));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, BitRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, ProfileAndLevel));
  return result;
}

//
void
MPEG2VideoDescriptor::Copy(const MPEG2VideoDescriptor& rhs)
{
  CodedContentType = rhs.CodedContentType;
  LowDelay = rhs.LowDelay;
  BitRate = rhs.BitRate;
  ProfileAndLevel = rhs.ProfileAndLevel;
}

//
void
MPEG2VideoDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  CDCIEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "CodedContentType", CodedContentType);
  fprintf(stream, "  %22s = %d\n",  "LowDelay", LowDelay);
  fprintf(stream, "  %22s = %d\n",  "BitRate", BitRate);
  fprintf(stream, "  %22s = %d\n",  "ProfileAndLevel", ProfileAndLevel);
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MPEG2VideoDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MPEG2VideoDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DMSegment

//
ASDCP::Result_t
DMSegment::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(DMSegment, Duration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, DMFramework));
  return result;
}

//
ASDCP::Result_t
DMSegment::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(DMSegment, Duration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, DMFramework));
  return result;
}

//
void
DMSegment::Copy(const DMSegment& rhs)
{
  DataDefinition = rhs.DataDefinition;
  EventStartPosition = rhs.EventStartPosition;
  Duration = rhs.Duration;
  EventComment = rhs.EventComment;
  DMFramework = rhs.DMFramework;
}

//
void
DMSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "EventStartPosition", i64sz(EventStartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "Duration", i64sz(Duration, identbuf));
  fprintf(stream, "  %22s = %s\n",  "EventComment", EventComment.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "DMFramework", DMFramework.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
DMSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_DMSegment));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
DMSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_DMSegment));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicFramework

//
ASDCP::Result_t
CryptographicFramework::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
ASDCP::Result_t
CryptographicFramework::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
void
CryptographicFramework::Copy(const CryptographicFramework& rhs)
{
  ContextSR = rhs.ContextSR;
}

//
void
CryptographicFramework::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextSR", ContextSR.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
CryptographicFramework::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CryptographicFramework));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicFramework::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CryptographicFramework));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicContext

//
ASDCP::Result_t
CryptographicContext::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, ContextID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, SourceEssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, CipherAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, MICAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicContext, CryptographicKeyID));
  return result;
}

//
ASDCP::Result_t
CryptographicContext::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, ContextID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, SourceEssenceContainer));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, CipherAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, MICAlgorithm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicContext, CryptographicKeyID));
  return result;
}

//
void
CryptographicContext::Copy(const CryptographicContext& rhs)
{
  ContextID = rhs.ContextID;
  SourceEssenceContainer = rhs.SourceEssenceContainer;
  CipherAlgorithm = rhs.CipherAlgorithm;
  MICAlgorithm = rhs.MICAlgorithm;
  CryptographicKeyID = rhs.CryptographicKeyID;
}

//
void
CryptographicContext::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextID", ContextID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "SourceEssenceContainer", SourceEssenceContainer.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CipherAlgorithm", CipherAlgorithm.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MICAlgorithm", MICAlgorithm.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "CryptographicKeyID", CryptographicKeyID.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
CryptographicContext::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CryptographicContext));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicContext::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_CryptographicContext));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDataEssenceDescriptor

//
ASDCP::Result_t
GenericDataEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
}

//
void
GenericDataEssenceDescriptor::Copy(const GenericDataEssenceDescriptor& rhs)
{
  DataEssenceCoding = rhs.DataEssenceCoding;
}

//
void
GenericDataEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataEssenceCoding", DataEssenceCoding.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericDataEssenceDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GenericDataEssenceDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextDescriptor

//
ASDCP::Result_t
TimedTextDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, ResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, UCSEncoding));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextDescriptor, NamespaceURI));
  return result;
}

//
ASDCP::Result_t
TimedTextDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, ResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, UCSEncoding));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, NamespaceURI));
  return result;
}

//
void
TimedTextDescriptor::Copy(const TimedTextDescriptor& rhs)
{
  ResourceID = rhs.ResourceID;
  UCSEncoding = rhs.UCSEncoding;
  NamespaceURI = rhs.NamespaceURI;
}

//
void
TimedTextDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDataEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ResourceID", ResourceID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "UCSEncoding", UCSEncoding.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "NamespaceURI", NamespaceURI.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
TimedTextDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimedTextDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimedTextDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextResourceSubDescriptor

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, AncillaryResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, MIMEMediaType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(TimedTextResourceSubDescriptor, EssenceStreamID));
  return result;
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, AncillaryResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, MIMEMediaType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, EssenceStreamID));
  return result;
}

//
void
TimedTextResourceSubDescriptor::Copy(const TimedTextResourceSubDescriptor& rhs)
{
  AncillaryResourceID = rhs.AncillaryResourceID;
  MIMEMediaType = rhs.MIMEMediaType;
  EssenceStreamID = rhs.EssenceStreamID;
}

//
void
TimedTextResourceSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AncillaryResourceID", AncillaryResourceID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MIMEMediaType", MIMEMediaType.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "EssenceStreamID", EssenceStreamID);
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimedTextResourceSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_TimedTextResourceSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StereoscopicPictureSubDescriptor

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  return result;
}

//
void
StereoscopicPictureSubDescriptor::Copy(const StereoscopicPictureSubDescriptor& rhs)
{
}

//
void
StereoscopicPictureSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_StereoscopicPictureSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_StereoscopicPictureSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// NetworkLocator

//
ASDCP::Result_t
NetworkLocator::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(NetworkLocator, URLString));
  return result;
}

//
ASDCP::Result_t
NetworkLocator::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(NetworkLocator, URLString));
  return result;
}

//
void
NetworkLocator::Copy(const NetworkLocator& rhs)
{
  URLString = rhs.URLString;
}

//
void
NetworkLocator::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "URLString", URLString.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
NetworkLocator::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_NetworkLocator));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
NetworkLocator::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_NetworkLocator));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MCALabelSubDescriptor

//
ASDCP::Result_t
MCALabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCALabelDictionaryID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCALinkID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCATagSymbol));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, MCATagName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(MCALabelSubDescriptor, MCAChannelID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(MCALabelSubDescriptor, RFC5646SpokenLanguage));
  return result;
}

//
ASDCP::Result_t
MCALabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCALabelDictionaryID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCALinkID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCATagSymbol));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCATagName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(MCALabelSubDescriptor, MCAChannelID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(MCALabelSubDescriptor, RFC5646SpokenLanguage));
  return result;
}

//
void
MCALabelSubDescriptor::Copy(const MCALabelSubDescriptor& rhs)
{
  MCALabelDictionaryID = rhs.MCALabelDictionaryID;
  MCALinkID = rhs.MCALinkID;
  MCATagSymbol = rhs.MCATagSymbol;
  MCATagName = rhs.MCATagName;
  MCAChannelID = rhs.MCAChannelID;
  RFC5646SpokenLanguage = rhs.RFC5646SpokenLanguage;
}

//
void
MCALabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "MCALabelDictionaryID", MCALabelDictionaryID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MCALinkID", MCALinkID.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MCATagSymbol", MCATagSymbol.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "MCATagName", MCATagName.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %d\n",  "MCAChannelID", MCAChannelID);
  fprintf(stream, "  %22s = %s\n",  "RFC5646SpokenLanguage", RFC5646SpokenLanguage.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
MCALabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MCALabelSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MCALabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_MCALabelSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// AudioChannelLabelSubDescriptor

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(AudioChannelLabelSubDescriptor, SoundfieldGroupLinkID));
  return result;
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(AudioChannelLabelSubDescriptor, SoundfieldGroupLinkID));
  return result;
}

//
void
AudioChannelLabelSubDescriptor::Copy(const AudioChannelLabelSubDescriptor& rhs)
{
  SoundfieldGroupLinkID = rhs.SoundfieldGroupLinkID;
}

//
void
AudioChannelLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "SoundfieldGroupLinkID", SoundfieldGroupLinkID.EncodeString(identbuf, IdentBufferLen));
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_AudioChannelLabelSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
AudioChannelLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_AudioChannelLabelSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SoundfieldGroupLabelSubDescriptor

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SoundfieldGroupLabelSubDescriptor, GroupOfSoundfieldGroupsLinkID));
  return result;
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SoundfieldGroupLabelSubDescriptor, GroupOfSoundfieldGroupsLinkID));
  return result;
}

//
void
SoundfieldGroupLabelSubDescriptor::Copy(const SoundfieldGroupLabelSubDescriptor& rhs)
{
  GroupOfSoundfieldGroupsLinkID = rhs.GroupOfSoundfieldGroupsLinkID;
}

//
void
SoundfieldGroupLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
  fprintf(stream, "  %22s:\n",  "GroupOfSoundfieldGroupsLinkID");
  GroupOfSoundfieldGroupsLinkID.Dump(stream);
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SoundfieldGroupLabelSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SoundfieldGroupLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_SoundfieldGroupLabelSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GroupOfSoundfieldGroupsLabelSubDescriptor

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  assert(m_Dict);
  Result_t result = MCALabelSubDescriptor::WriteToTLVSet(TLVSet);
  return result;
}

//
void
GroupOfSoundfieldGroupsLabelSubDescriptor::Copy(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs)
{
}

//
void
GroupOfSoundfieldGroupsLabelSubDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  MCALabelSubDescriptor::Dump(stream);
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor));
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GroupOfSoundfieldGroupsLabelSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  m_Typeinfo = &(m_Dict->Type(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor));
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
// end Metadata.cpp
//
