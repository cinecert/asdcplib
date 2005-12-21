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
#include <hex_utils.h>



//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::Identification::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::ContentStorage::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  return result;
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


//
void
ASDCP::MXF::MaterialPackage::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = GenericPackage::InitFromTLVSet(TLVSet);
  return result;
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


//
void
ASDCP::MXF::SourcePackage::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericPackage::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::Track::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericTrack::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::StructuralComponent::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
  return result;
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::Sequence::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::SourceClip::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::TimecodeComponent::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  StructuralComponent::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::GenericDescriptor::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
  return result;
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::FileDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::GenericSoundEssenceDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::WaveAudioDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericSoundEssenceDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::GenericPictureEssenceDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  FileDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::RGBAEssenceDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::JPEG2000PictureSubDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::CDCIEssenceDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  GenericPictureEssenceDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::MPEG2VideoDescriptor::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  CDCIEssenceDescriptor::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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


//
void
ASDCP::MXF::CryptographicFramework::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

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
void
ASDCP::MXF::CryptographicContext::Dump(FILE* stream)
{
//  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fputs("==========================================================================\n", stream);
}


//
// end MXF.cpp
//
