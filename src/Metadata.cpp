/*
Copyright (c) 2005-2009, John Hurst
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

static InterchangeObject* Preface_Factory(const Dictionary& Dict) { return new Preface(Dict); }
static InterchangeObject* IndexTableSegment_Factory(const Dictionary& Dict) { return new IndexTableSegment(Dict); }

static InterchangeObject* Identification_Factory(const Dictionary& Dict) { return new Identification(Dict); }
static InterchangeObject* ContentStorage_Factory(const Dictionary& Dict) { return new ContentStorage(Dict); }
static InterchangeObject* EssenceContainerData_Factory(const Dictionary& Dict) { return new EssenceContainerData(Dict); }
static InterchangeObject* MaterialPackage_Factory(const Dictionary& Dict) { return new MaterialPackage(Dict); }
static InterchangeObject* SourcePackage_Factory(const Dictionary& Dict) { return new SourcePackage(Dict); }
static InterchangeObject* StaticTrack_Factory(const Dictionary& Dict) { return new StaticTrack(Dict); }
static InterchangeObject* Track_Factory(const Dictionary& Dict) { return new Track(Dict); }
static InterchangeObject* Sequence_Factory(const Dictionary& Dict) { return new Sequence(Dict); }
static InterchangeObject* SourceClip_Factory(const Dictionary& Dict) { return new SourceClip(Dict); }
static InterchangeObject* TimecodeComponent_Factory(const Dictionary& Dict) { return new TimecodeComponent(Dict); }
static InterchangeObject* FileDescriptor_Factory(const Dictionary& Dict) { return new FileDescriptor(Dict); }
static InterchangeObject* GenericSoundEssenceDescriptor_Factory(const Dictionary& Dict) { return new GenericSoundEssenceDescriptor(Dict); }
static InterchangeObject* WaveAudioDescriptor_Factory(const Dictionary& Dict) { return new WaveAudioDescriptor(Dict); }
static InterchangeObject* GenericPictureEssenceDescriptor_Factory(const Dictionary& Dict) { return new GenericPictureEssenceDescriptor(Dict); }
static InterchangeObject* RGBAEssenceDescriptor_Factory(const Dictionary& Dict) { return new RGBAEssenceDescriptor(Dict); }
static InterchangeObject* JPEG2000PictureSubDescriptor_Factory(const Dictionary& Dict) { return new JPEG2000PictureSubDescriptor(Dict); }
static InterchangeObject* CDCIEssenceDescriptor_Factory(const Dictionary& Dict) { return new CDCIEssenceDescriptor(Dict); }
static InterchangeObject* MPEG2VideoDescriptor_Factory(const Dictionary& Dict) { return new MPEG2VideoDescriptor(Dict); }
static InterchangeObject* DMSegment_Factory(const Dictionary& Dict) { return new DMSegment(Dict); }
static InterchangeObject* CryptographicFramework_Factory(const Dictionary& Dict) { return new CryptographicFramework(Dict); }
static InterchangeObject* CryptographicContext_Factory(const Dictionary& Dict) { return new CryptographicContext(Dict); }
static InterchangeObject* GenericDataEssenceDescriptor_Factory(const Dictionary& Dict) { return new GenericDataEssenceDescriptor(Dict); }
static InterchangeObject* TimedTextDescriptor_Factory(const Dictionary& Dict) { return new TimedTextDescriptor(Dict); }
static InterchangeObject* TimedTextResourceSubDescriptor_Factory(const Dictionary& Dict) { return new TimedTextResourceSubDescriptor(Dict); }
static InterchangeObject* StereoscopicPictureSubDescriptor_Factory(const Dictionary& Dict) { return new StereoscopicPictureSubDescriptor(Dict); }


void
ASDCP::MXF::Metadata_InitTypes(const Dictionary& Dict)
{
  SetObjectFactory(Dict.ul(MDD_Preface), Preface_Factory);
  SetObjectFactory(Dict.ul(MDD_IndexTableSegment), IndexTableSegment_Factory);

  SetObjectFactory(Dict.ul(MDD_Identification), Identification_Factory);
  SetObjectFactory(Dict.ul(MDD_ContentStorage), ContentStorage_Factory);
  SetObjectFactory(Dict.ul(MDD_EssenceContainerData), EssenceContainerData_Factory);
  SetObjectFactory(Dict.ul(MDD_MaterialPackage), MaterialPackage_Factory);
  SetObjectFactory(Dict.ul(MDD_SourcePackage), SourcePackage_Factory);
  SetObjectFactory(Dict.ul(MDD_StaticTrack), StaticTrack_Factory);
  SetObjectFactory(Dict.ul(MDD_Track), Track_Factory);
  SetObjectFactory(Dict.ul(MDD_Sequence), Sequence_Factory);
  SetObjectFactory(Dict.ul(MDD_SourceClip), SourceClip_Factory);
  SetObjectFactory(Dict.ul(MDD_TimecodeComponent), TimecodeComponent_Factory);
  SetObjectFactory(Dict.ul(MDD_FileDescriptor), FileDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_GenericSoundEssenceDescriptor), GenericSoundEssenceDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_WaveAudioDescriptor), WaveAudioDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_GenericPictureEssenceDescriptor), GenericPictureEssenceDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_RGBAEssenceDescriptor), RGBAEssenceDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_JPEG2000PictureSubDescriptor), JPEG2000PictureSubDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_CDCIEssenceDescriptor), CDCIEssenceDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_MPEG2VideoDescriptor), MPEG2VideoDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_DMSegment), DMSegment_Factory);
  SetObjectFactory(Dict.ul(MDD_CryptographicFramework), CryptographicFramework_Factory);
  SetObjectFactory(Dict.ul(MDD_CryptographicContext), CryptographicContext_Factory);
  SetObjectFactory(Dict.ul(MDD_GenericDataEssenceDescriptor), GenericDataEssenceDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_TimedTextDescriptor), TimedTextDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_TimedTextResourceSubDescriptor), TimedTextResourceSubDescriptor_Factory);
  SetObjectFactory(Dict.ul(MDD_StereoscopicPictureSubDescriptor), StereoscopicPictureSubDescriptor_Factory);
}

//------------------------------------------------------------------------------------------
// KLV Sets



//------------------------------------------------------------------------------------------
// Identification

//
ASDCP::Result_t
Identification::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_Identification);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Identification::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_Identification);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// ContentStorage

//
ASDCP::Result_t
ContentStorage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
ASDCP::Result_t
ContentStorage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, EssenceContainerData));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_ContentStorage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ContentStorage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_ContentStorage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// EssenceContainerData

//
ASDCP::Result_t
EssenceContainerData::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, BodySID));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_EssenceContainerData);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
EssenceContainerData::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_EssenceContainerData);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPackage

//
ASDCP::Result_t
GenericPackage::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
MaterialPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_MaterialPackage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MaterialPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_MaterialPackage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourcePackage

//
ASDCP::Result_t
SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourcePackage, Descriptor));
  return result;
}

//
ASDCP::Result_t
SourcePackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourcePackage, Descriptor));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_SourcePackage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourcePackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_SourcePackage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericTrack

//
ASDCP::Result_t
GenericTrack::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericTrack, TrackNumber));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericTrack, TrackName));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericTrack, Sequence));
  return result;
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
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StaticTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_StaticTrack);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StaticTrack::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_StaticTrack);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// Track

//
ASDCP::Result_t
Track::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(Track, Origin));
  return result;
}

//
ASDCP::Result_t
Track::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(Track, Origin));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_Track);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Track::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_Track);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StructuralComponent

//
ASDCP::Result_t
StructuralComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
  return result;
}

//
ASDCP::Result_t
StructuralComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(StructuralComponent, Duration));
  return result;
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
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Sequence, StructuralComponents));
  return result;
}

//
ASDCP::Result_t
Sequence::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Sequence, StructuralComponents));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_Sequence);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
Sequence::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_Sequence);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourceClip

//
ASDCP::Result_t
SourceClip::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(SourceClip, SourceTrackID));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_SourceClip);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
SourceClip::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_SourceClip);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimecodeComponent

//
ASDCP::Result_t
TimecodeComponent::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(TimecodeComponent, DropFrame));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_TimecodeComponent);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimecodeComponent::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_TimecodeComponent);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDescriptor

//
ASDCP::Result_t
GenericDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
ASDCP::Result_t
GenericDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, SubDescriptors));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_FileDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
FileDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_FileDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericSoundEssenceDescriptor

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_GenericSoundEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericSoundEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_GenericSoundEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// WaveAudioDescriptor

//
ASDCP::Result_t
WaveAudioDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = GenericSoundEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(WaveAudioDescriptor, AvgBps));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(WaveAudioDescriptor, ChannelAssignment));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_WaveAudioDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
WaveAudioDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_WaveAudioDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPictureEssenceDescriptor

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_GenericPictureEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_GenericPictureEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// RGBAEssenceDescriptor

//
ASDCP::Result_t
RGBAEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  return result;
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_RGBAEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_RGBAEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// JPEG2000PictureSubDescriptor

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_JPEG2000PictureSubDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_JPEG2000PictureSubDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CDCIEssenceDescriptor

//
ASDCP::Result_t
CDCIEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, ComponentDepth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, VerticalSubsampling));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(CDCIEssenceDescriptor, ColorSiting));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_CDCIEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CDCIEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_CDCIEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MPEG2VideoDescriptor

//
ASDCP::Result_t
MPEG2VideoDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = CDCIEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, CodedContentType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, LowDelay));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, BitRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(MPEG2VideoDescriptor, ProfileAndLevel));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_MPEG2VideoDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
MPEG2VideoDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_MPEG2VideoDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DMSegment

//
ASDCP::Result_t
DMSegment::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_DMSegment);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
DMSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_DMSegment);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicFramework

//
ASDCP::Result_t
CryptographicFramework::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
ASDCP::Result_t
CryptographicFramework::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicFramework, ContextSR));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_CryptographicFramework);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicFramework::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_CryptographicFramework);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicContext

//
ASDCP::Result_t
CryptographicContext::InitFromTLVSet(TLVReader& TLVSet)
{
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
  m_Typeinfo = &m_Dict.Type(MDD_CryptographicContext);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
CryptographicContext::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_CryptographicContext);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDataEssenceDescriptor

//
ASDCP::Result_t
GenericDataEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDataEssenceDescriptor, DataEssenceCoding));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_GenericDataEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
GenericDataEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_GenericDataEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextDescriptor

//
ASDCP::Result_t
TimedTextDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = GenericDataEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, ResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, UCSEncoding));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextDescriptor, NamespaceURI));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_TimedTextDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_TimedTextDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimedTextResourceSubDescriptor

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
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
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, AncillaryResourceID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, MIMEMediaType));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(TimedTextResourceSubDescriptor, EssenceStreamID));
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_TimedTextResourceSubDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
TimedTextResourceSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_TimedTextResourceSubDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StereoscopicPictureSubDescriptor

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  return result;
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
  m_Typeinfo = &m_Dict.Type(MDD_StereoscopicPictureSubDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
StereoscopicPictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &m_Dict.Type(MDD_StereoscopicPictureSubDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
// end Metadata.cpp
//
