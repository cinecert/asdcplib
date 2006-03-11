/*
Copyright (c) 2005-2006, John Hurst
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


#include "Metadata.h"
#include "Mutex.h"
#include "hex_utils.h"

const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

//------------------------------------------------------------------------------------------

//
enum FLT_t
  {
    FLT_Preface,
    FLT_IndexTableSegment,
    FLT_Identification,
    FLT_ContentStorage,
    FLT_EssenceContainerData,
    FLT_MaterialPackage,
    FLT_SourcePackage,
    FLT_StaticTrack,
    FLT_Track,
    FLT_Sequence,
    FLT_SourceClip,
    FLT_TimecodeComponent,
    FLT_FileDescriptor,
    FLT_GenericSoundEssenceDescriptor,
    FLT_WaveAudioDescriptor,
    FLT_GenericPictureEssenceDescriptor,
    FLT_RGBAEssenceDescriptor,
    FLT_JPEG2000PictureSubDescriptor,
    FLT_CDCIEssenceDescriptor,
    FLT_MPEG2VideoDescriptor,
    FLT_DMSegment,
    FLT_CryptographicFramework,
    FLT_CryptographicContext,
  };

//
typedef std::map<ASDCP::UL, FLT_t>::iterator FLi_t;

class FactoryList : public std::map<ASDCP::UL, FLT_t>
{
  ASDCP::Mutex m_Lock;

public:
  FactoryList() {}
  ~FactoryList() {}

  bool Empty() {
    ASDCP::AutoMutex BlockLock(m_Lock);
    return empty();
  }

  FLi_t Find(const byte_t* label) {
    ASDCP::AutoMutex BlockLock(m_Lock);
    return find(label);
  }

  FLi_t End() {
    ASDCP::AutoMutex BlockLock(m_Lock);
    return end();
  }

};

//
static FactoryList s_FactoryList;

//
ASDCP::MXF::InterchangeObject*
ASDCP::MXF::CreateObject(const byte_t* label)
{
  if ( label == 0 )
    return 0;

  if ( s_FactoryList.empty() )
    {
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_Preface), FLT_Preface));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_IndexTableSegment), FLT_IndexTableSegment));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_Identification), FLT_Identification));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_ContentStorage), FLT_ContentStorage));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_EssenceContainerData), FLT_EssenceContainerData));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_MaterialPackage), FLT_MaterialPackage));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_SourcePackage), FLT_SourcePackage));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_StaticTrack), FLT_StaticTrack));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_Track), FLT_Track));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_Sequence), FLT_Sequence));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_SourceClip), FLT_SourceClip));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_TimecodeComponent), FLT_TimecodeComponent));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_FileDescriptor), FLT_FileDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_GenericSoundEssenceDescriptor), FLT_GenericSoundEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_WaveAudioDescriptor), FLT_WaveAudioDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_GenericPictureEssenceDescriptor), FLT_GenericPictureEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_RGBAEssenceDescriptor), FLT_RGBAEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_JPEG2000PictureSubDescriptor), FLT_JPEG2000PictureSubDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_CDCIEssenceDescriptor), FLT_CDCIEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_MPEG2VideoDescriptor), FLT_MPEG2VideoDescriptor));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_DMSegment), FLT_DMSegment));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_CryptographicFramework), FLT_CryptographicFramework));
      s_FactoryList.insert(FactoryList::value_type(Dict::ul(MDD_CryptographicContext), FLT_CryptographicContext));
    }

  FLi_t i = s_FactoryList.find(label);

  if ( i == s_FactoryList.end() )
    return new InterchangeObject;

  switch ( i->second )
    {
      case FLT_Preface: return new Preface;
      case FLT_IndexTableSegment: return new IndexTableSegment;
      case FLT_Identification: return new Identification;
      case FLT_ContentStorage: return new ContentStorage;
      case FLT_EssenceContainerData: return new EssenceContainerData;
      case FLT_MaterialPackage: return new MaterialPackage;
      case FLT_SourcePackage: return new SourcePackage;
      case FLT_StaticTrack: return new StaticTrack;
      case FLT_Track: return new Track;
      case FLT_Sequence: return new Sequence;
      case FLT_SourceClip: return new SourceClip;
      case FLT_TimecodeComponent: return new TimecodeComponent;
      case FLT_FileDescriptor: return new FileDescriptor;
      case FLT_GenericSoundEssenceDescriptor: return new GenericSoundEssenceDescriptor;
      case FLT_WaveAudioDescriptor: return new WaveAudioDescriptor;
      case FLT_GenericPictureEssenceDescriptor: return new GenericPictureEssenceDescriptor;
      case FLT_RGBAEssenceDescriptor: return new RGBAEssenceDescriptor;
      case FLT_JPEG2000PictureSubDescriptor: return new JPEG2000PictureSubDescriptor;
      case FLT_CDCIEssenceDescriptor: return new CDCIEssenceDescriptor;
      case FLT_MPEG2VideoDescriptor: return new MPEG2VideoDescriptor;
      case FLT_DMSegment: return new DMSegment;
      case FLT_CryptographicFramework: return new CryptographicFramework;
      case FLT_CryptographicContext: return new CryptographicContext;
    }

  return 0;
}


//------------------------------------------------------------------------------------------
// KLV Sets



//------------------------------------------------------------------------------------------
// Identification

//
ASDCP::Result_t
ASDCP::MXF::Identification::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::Identification::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::Identification::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ThisGenerationUID", ThisGenerationUID.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "CompanyName", CompanyName.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ProductName", ProductName.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ProductVersion", ProductVersion.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "VersionString", VersionString.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ProductUID", ProductUID.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ModificationDate", ModificationDate.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ToolkitVersion", ToolkitVersion.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Platform", Platform.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::Identification::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_Identification);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::Identification::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_Identification);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// ContentStorage

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, Packages));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(ContentStorage, EssenceContainerData));
  return result;
}

//
void
ASDCP::MXF::ContentStorage::Dump(FILE* stream)
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
ASDCP::MXF::ContentStorage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_ContentStorage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_ContentStorage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// EssenceContainerData

//
ASDCP::Result_t
ASDCP::MXF::EssenceContainerData::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::EssenceContainerData::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(EssenceContainerData, LinkedPackageUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, BodySID));
  return result;
}

//
void
ASDCP::MXF::EssenceContainerData::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "LinkedPackageUID", LinkedPackageUID.ToString(identbuf));
  fprintf(stream, "  %22s = %d\n",  "IndexSID", IndexSID);
  fprintf(stream, "  %22s = %d\n",  "BodySID", BodySID);
}

//
ASDCP::Result_t
ASDCP::MXF::EssenceContainerData::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_EssenceContainerData);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::EssenceContainerData::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_EssenceContainerData);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPackage

//
ASDCP::Result_t
ASDCP::MXF::GenericPackage::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::GenericPackage::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::GenericPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "PackageUID", PackageUID.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Name", Name.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "PackageCreationDate", PackageCreationDate.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "PackageModifiedDate", PackageModifiedDate.ToString(identbuf));
  fprintf(stream, "  %22s:\n",  "Tracks");
  Tracks.Dump(stream);
}


//------------------------------------------------------------------------------------------
// MaterialPackage

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  return result;
}

//
void
ASDCP::MXF::MaterialPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_MaterialPackage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_MaterialPackage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourcePackage

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourcePackage, Descriptor));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPackage::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourcePackage, Descriptor));
  return result;
}

//
void
ASDCP::MXF::SourcePackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "Descriptor", Descriptor.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_SourcePackage);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_SourcePackage);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericTrack

//
ASDCP::Result_t
ASDCP::MXF::GenericTrack::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::GenericTrack::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::GenericTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "TrackID", TrackID);
  fprintf(stream, "  %22s = %d\n",  "TrackNumber", TrackNumber);
  fprintf(stream, "  %22s = %s\n",  "TrackName", TrackName.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Sequence", Sequence.ToString(identbuf));
}


//------------------------------------------------------------------------------------------
// StaticTrack

//
ASDCP::Result_t
ASDCP::MXF::StaticTrack::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::StaticTrack::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  return result;
}

//
void
ASDCP::MXF::StaticTrack::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MXF::StaticTrack::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_StaticTrack);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::StaticTrack::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_StaticTrack);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// Track

//
ASDCP::Result_t
ASDCP::MXF::Track::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericTrack::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(Track, Origin));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Track::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericTrack::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Track, EditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(Track, Origin));
  return result;
}

//
void
ASDCP::MXF::Track::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "EditRate", EditRate.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Origin", i64sz(Origin, identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::Track::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_Track);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::Track::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_Track);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// StructuralComponent

//
ASDCP::Result_t
ASDCP::MXF::StructuralComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::StructuralComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(StructuralComponent, Duration));
  return result;
}

//
void
ASDCP::MXF::StructuralComponent::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "DataDefinition", DataDefinition.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Duration", i64sz(Duration, identbuf));
}


//------------------------------------------------------------------------------------------
// Sequence

//
ASDCP::Result_t
ASDCP::MXF::Sequence::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Sequence, StructuralComponents));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Sequence::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Sequence, StructuralComponents));
  return result;
}

//
void
ASDCP::MXF::Sequence::Dump(FILE* stream)
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
ASDCP::MXF::Sequence::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_Sequence);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::Sequence::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_Sequence);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// SourceClip

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(SourceClip, StartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(SourceClip, SourcePackageID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(SourceClip, SourceTrackID));
  return result;
}

//
void
ASDCP::MXF::SourceClip::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "StartPosition", i64sz(StartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "SourcePackageID", SourcePackageID.ToString(identbuf));
  fprintf(stream, "  %22s = %d\n",  "SourceTrackID", SourceTrackID);
}

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_SourceClip);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_SourceClip);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// TimecodeComponent

//
ASDCP::Result_t
ASDCP::MXF::TimecodeComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = StructuralComponent::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TimecodeComponent::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = StructuralComponent::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(TimecodeComponent, RoundedTimecodeBase));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(TimecodeComponent, StartTimecode));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(TimecodeComponent, DropFrame));
  return result;
}

//
void
ASDCP::MXF::TimecodeComponent::Dump(FILE* stream)
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
ASDCP::MXF::TimecodeComponent::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_TimecodeComponent);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::TimecodeComponent::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_TimecodeComponent);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericDescriptor

//
ASDCP::Result_t
ASDCP::MXF::GenericDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::GenericDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}

//
void
ASDCP::MXF::GenericDescriptor::Dump(FILE* stream)
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
ASDCP::MXF::FileDescriptor::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::FileDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::FileDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "LinkedTrackID", LinkedTrackID);
  fprintf(stream, "  %22s = %s\n",  "SampleRate", SampleRate.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ContainerDuration", i64sz(ContainerDuration, identbuf));
  fprintf(stream, "  %22s = %s\n",  "EssenceContainer", EssenceContainer.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Codec", Codec.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::FileDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_FileDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::FileDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_FileDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericSoundEssenceDescriptor

//
ASDCP::Result_t
ASDCP::MXF::GenericSoundEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::GenericSoundEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::GenericSoundEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "AudioSamplingRate", AudioSamplingRate.ToString(identbuf));
  fprintf(stream, "  %22s = %d\n",  "Locked", Locked);
  fprintf(stream, "  %22s = %d\n",  "AudioRefLevel", AudioRefLevel);
  fprintf(stream, "  %22s = %d\n",  "ChannelCount", ChannelCount);
  fprintf(stream, "  %22s = %d\n",  "QuantizationBits", QuantizationBits);
  fprintf(stream, "  %22s = %d\n",  "DialNorm", DialNorm);
}

//
ASDCP::Result_t
ASDCP::MXF::GenericSoundEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_GenericSoundEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::GenericSoundEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_GenericSoundEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// WaveAudioDescriptor

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericSoundEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(WaveAudioDescriptor, AvgBps));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericSoundEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(WaveAudioDescriptor, BlockAlign));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(WaveAudioDescriptor, SequenceOffset));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(WaveAudioDescriptor, AvgBps));
  return result;
}

//
void
ASDCP::MXF::WaveAudioDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericSoundEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "BlockAlign", BlockAlign);
  fprintf(stream, "  %22s = %d\n",  "SequenceOffset", SequenceOffset);
  fprintf(stream, "  %22s = %d\n",  "AvgBps", AvgBps);
}

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_WaveAudioDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_WaveAudioDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// GenericPictureEssenceDescriptor

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = FileDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = FileDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  return result;
}

//
void
ASDCP::MXF::GenericPictureEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "FrameLayout", FrameLayout);
  fprintf(stream, "  %22s = %d\n",  "StoredWidth", StoredWidth);
  fprintf(stream, "  %22s = %d\n",  "StoredHeight", StoredHeight);
  fprintf(stream, "  %22s = %s\n",  "AspectRatio", AspectRatio.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_GenericPictureEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_GenericPictureEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// RGBAEssenceDescriptor

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPictureEssenceDescriptor::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(RGBAEssenceDescriptor, PixelLayout));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = GenericPictureEssenceDescriptor::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMaxRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, ComponentMinRef));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(RGBAEssenceDescriptor, PixelLayout));
  return result;
}

//
void
ASDCP::MXF::RGBAEssenceDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fprintf(stream, "  %22s = %d\n",  "ComponentMaxRef", ComponentMaxRef);
  fprintf(stream, "  %22s = %d\n",  "ComponentMinRef", ComponentMinRef);
  fprintf(stream, "  %22s = %s\n",  "PixelLayout", PixelLayout.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_RGBAEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_RGBAEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// JPEG2000PictureSubDescriptor

//
ASDCP::Result_t
ASDCP::MXF::JPEG2000PictureSubDescriptor::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::JPEG2000PictureSubDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::JPEG2000PictureSubDescriptor::Dump(FILE* stream)
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
  fprintf(stream, "  %22s = %s\n",  "PictureComponentSizing", PictureComponentSizing.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "CodingStyleDefault", CodingStyleDefault.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "QuantizationDefault", QuantizationDefault.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::JPEG2000PictureSubDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_JPEG2000PictureSubDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_JPEG2000PictureSubDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CDCIEssenceDescriptor

//
ASDCP::Result_t
ASDCP::MXF::CDCIEssenceDescriptor::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::CDCIEssenceDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::CDCIEssenceDescriptor::Dump(FILE* stream)
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
ASDCP::MXF::CDCIEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_CDCIEssenceDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::CDCIEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_CDCIEssenceDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// MPEG2VideoDescriptor

//
ASDCP::Result_t
ASDCP::MXF::MPEG2VideoDescriptor::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::MPEG2VideoDescriptor::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::MPEG2VideoDescriptor::Dump(FILE* stream)
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
ASDCP::MXF::MPEG2VideoDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_MPEG2VideoDescriptor);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::MPEG2VideoDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_MPEG2VideoDescriptor);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// DMSegment

//
ASDCP::Result_t
ASDCP::MXF::DMSegment::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(DMSegment, DMFramework));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::DMSegment::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(DMSegment, EventStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, EventComment));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(DMSegment, DMFramework));
  return result;
}

//
void
ASDCP::MXF::DMSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "EventStartPosition", i64sz(EventStartPosition, identbuf));
  fprintf(stream, "  %22s = %s\n",  "EventComment", EventComment.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "DMFramework", DMFramework.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::DMSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_DMSegment);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::DMSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_DMSegment);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicFramework

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(CryptographicFramework, ContextSR));
  return result;
}

//
void
ASDCP::MXF::CryptographicFramework::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextSR", ContextSR.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_CryptographicFramework);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_CryptographicFramework);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//------------------------------------------------------------------------------------------
// CryptographicContext

//
ASDCP::Result_t
ASDCP::MXF::CryptographicContext::InitFromTLVSet(TLVReader& TLVSet)
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
ASDCP::MXF::CryptographicContext::WriteToTLVSet(TLVWriter& TLVSet)
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
ASDCP::MXF::CryptographicContext::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContextID", ContextID.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "SourceEssenceContainer", SourceEssenceContainer.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "CipherAlgorithm", CipherAlgorithm.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "MICAlgorithm", MICAlgorithm.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "CryptographicKeyID", CryptographicKeyID.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicContext::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_CryptographicContext);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicContext::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_CryptographicContext);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
// end MXF.cpp
//
