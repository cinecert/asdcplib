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
#include "MDD.h"
#include "Mutex.h"
#include "hex_utils.h"


//------------------------------------------------------------------------------------------

//
enum FLT_t
  {
    FLT_Preface,
    FLT_IndexTableSegment,
    FLT_Identification,
    FLT_ContentStorage,
    FLT_MaterialPackage,
    FLT_SourcePackage,
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
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_Preface].ul, FLT_Preface));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_IndexTableSegment].ul, FLT_IndexTableSegment));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_Identification].ul, FLT_Identification));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_ContentStorage].ul, FLT_ContentStorage));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_MaterialPackage].ul, FLT_MaterialPackage));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_SourcePackage].ul, FLT_SourcePackage));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_Track].ul, FLT_Track));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_Sequence].ul, FLT_Sequence));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_SourceClip].ul, FLT_SourceClip));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_TimecodeComponent].ul, FLT_TimecodeComponent));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_FileDescriptor].ul, FLT_FileDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_GenericSoundEssenceDescriptor].ul, FLT_GenericSoundEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_WaveAudioDescriptor].ul, FLT_WaveAudioDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_GenericPictureEssenceDescriptor].ul, FLT_GenericPictureEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_RGBAEssenceDescriptor].ul, FLT_RGBAEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_JPEG2000PictureSubDescriptor].ul, FLT_JPEG2000PictureSubDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_CDCIEssenceDescriptor].ul, FLT_CDCIEssenceDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_MPEG2VideoDescriptor].ul, FLT_MPEG2VideoDescriptor));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_CryptographicFramework].ul, FLT_CryptographicFramework));
      s_FactoryList.insert(FactoryList::value_type(s_MDD_Table[MDDindex_CryptographicContext].ul, FLT_CryptographicContext));
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
      case FLT_MaterialPackage: return new MaterialPackage;
      case FLT_SourcePackage: return new SourcePackage;
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
      case FLT_CryptographicFramework: return new CryptographicFramework;
      case FLT_CryptographicContext: return new CryptographicContext;
    }
  
  return new InterchangeObject;
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
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(Identification, ProductVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, VersionString));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ProductUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, ModificationDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(Identification, ToolkitVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Identification, Platform));
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
  fprintf(stream, "  %22s = %d\n",  "ProductVersion", ProductVersion);
  fprintf(stream, "  %22s = %s\n",  "VersionString", VersionString.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ProductUID", ProductUID.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "ModificationDate", ModificationDate.ToString(identbuf));
  fprintf(stream, "  %22s = %d\n",  "ToolkitVersion", ToolkitVersion);
  fprintf(stream, "  %22s = %s\n",  "Platform", Platform.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::Identification::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Identification].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Identification::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_Identification].ul, 0);
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
void
ASDCP::MXF::ContentStorage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %s:\n",  "Packages");
  Packages.Dump(stream);
  fprintf(stream, "  %s:\n",  "EssenceContainerData");
  EssenceContainerData.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_ContentStorage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_ContentStorage].ul, 0);
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
  fprintf(stream, "  %s:\n",  "Tracks");
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_MaterialPackage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_MaterialPackage].ul, 0);
}

//------------------------------------------------------------------------------------------
// SourcePackage

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
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
}

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_SourcePackage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_SourcePackage].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Track].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Track::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_Track].ul, 0);
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
void
ASDCP::MXF::Sequence::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fprintf(stream, "  %s:\n",  "StructuralComponents");
  StructuralComponents.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MXF::Sequence::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Sequence].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Sequence::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_Sequence].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_SourceClip].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_SourceClip].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_TimecodeComponent].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TimecodeComponent::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_TimecodeComponent].ul, 0);
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
void
ASDCP::MXF::GenericDescriptor::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  *identbuf = 0;

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %s:\n",  "Locators");
  Locators.Dump(stream);
  fprintf(stream, "  %s:\n",  "SubDescriptors");
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_FileDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::FileDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_FileDescriptor].ul, 0);
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
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, ElectroSpatialFormulation));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, DialNorm));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, SoundEssenceCompression));
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
  fprintf(stream, "  %22s = %d\n",  "ElectroSpatialFormulation", ElectroSpatialFormulation);
  fprintf(stream, "  %22s = %d\n",  "ChannelCount", ChannelCount);
  fprintf(stream, "  %22s = %d\n",  "QuantizationBits", QuantizationBits);
  fprintf(stream, "  %22s = %d\n",  "DialNorm", DialNorm);
  fprintf(stream, "  %22s = %s\n",  "SoundEssenceCompression", SoundEssenceCompression.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::GenericSoundEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_GenericSoundEssenceDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::GenericSoundEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_GenericSoundEssenceDescriptor].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_WaveAudioDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_WaveAudioDescriptor].ul, 0);
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
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, DisplayWidth));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, DisplayHeight));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, AspectRatio));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, Gamma));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, PictureEssenceCoding));
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
  fprintf(stream, "  %22s = %d\n",  "DisplayWidth", DisplayWidth);
  fprintf(stream, "  %22s = %d\n",  "DisplayHeight", DisplayHeight);
  fprintf(stream, "  %22s = %s\n",  "AspectRatio", AspectRatio.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "Gamma", Gamma.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "PictureEssenceCoding", PictureEssenceCoding.ToString(identbuf));
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_GenericPictureEssenceDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_GenericPictureEssenceDescriptor].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_RGBAEssenceDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_RGBAEssenceDescriptor].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_JPEG2000PictureSubDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_JPEG2000PictureSubDescriptor].ul, 0);
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
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(CDCIEssenceDescriptor, ReversedByteOrder));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, BlackRefLevel));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, WhiteReflevel));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, ColorRange));
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
  fprintf(stream, "  %22s = %d\n",  "ReversedByteOrder", ReversedByteOrder);
  fprintf(stream, "  %22s = %d\n",  "BlackRefLevel", BlackRefLevel);
  fprintf(stream, "  %22s = %d\n",  "WhiteReflevel", WhiteReflevel);
  fprintf(stream, "  %22s = %d\n",  "ColorRange", ColorRange);
}

//
ASDCP::Result_t
ASDCP::MXF::CDCIEssenceDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_CDCIEssenceDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::CDCIEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_CDCIEssenceDescriptor].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_MPEG2VideoDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::MPEG2VideoDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_MPEG2VideoDescriptor].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_CryptographicFramework].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_CryptographicFramework].ul, 0);
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
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_CryptographicContext].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      result = InitFromTLVSet(MemRDR);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::CryptographicContext::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_CryptographicContext].ul, 0);
}

//
// end MXF.cpp
//
