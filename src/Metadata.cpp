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
ASDCP::MXF::Identification::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Identification].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, ThisGenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, CompanyName));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, ProductName));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi16(OBJ_READ_ARGS(Identification, ProductVersion));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, VersionString));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, ProductUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, ModificationDate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi16(OBJ_READ_ARGS(Identification, ToolkitVersion));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Identification, Platform));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "  ThisGenerationUID  = %s\n",  ThisGenerationUID.ToString(identbuf));
  fprintf(stream, "  CompanyName        = %s\n",  CompanyName.ToString(identbuf));
  fprintf(stream, "  ProductName        = %s\n",  ProductName.ToString(identbuf));
  fprintf(stream, "  ProductVersion     = %hu\n", ProductVersion);
  fprintf(stream, "  VersionString      = %s\n",  VersionString.ToString(identbuf));
  fprintf(stream, "  ProductUID         = %s\n",  ProductUID.ToString(identbuf));
  fprintf(stream, "  ModificationDate   = %s\n",  ModificationDate.ToString(identbuf));
  fprintf(stream, "  ToolkitVersion     = %hu\n", ToolkitVersion);
  fprintf(stream, "  Platform           = %s\n",  Platform.ToString(identbuf));

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::ContentStorage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_ContentStorage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(ContentStorage, Packages));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(ContentStorage, EssenceContainerData));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "  GenerationUID      = %s\n",  GenerationUID.ToString(identbuf));
  fprintf(stream, "  Packages:\n");  Packages.Dump(stream);
  fprintf(stream, "  EssenceContainerData:\n");  EssenceContainerData.Dump(stream);

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::GenericPackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

  Result_t result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
  //      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageUID));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericPackage, Name));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageCreationDate));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericPackage, PackageModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericPackage, Tracks));

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::GenericPackage::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_DefaultObject].ul, 0);
}

//
void
ASDCP::MXF::GenericPackage::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  GenerationUID      = %s\n", GenerationUID.ToString(identbuf));
  fprintf(stream, "  PackageUID         = %s\n", PackageUID.ToString(identbuf));
  fprintf(stream, "  Name               = %s\n", Name.ToString(identbuf));
  fprintf(stream, "  PackageCreationDate= %s\n", PackageCreationDate.ToString(identbuf));
  fprintf(stream, "  PackageModifiedDate= %s\n", PackageModifiedDate.ToString(identbuf));
  fprintf(stream, "  Tracks:\n");  Tracks.Dump(stream);

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::MaterialPackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_MaterialPackage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      return GenericPackage::InitFromBuffer(p, l);
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
  GenericPackage::Dump(stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::SourcePackage::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_SourcePackage].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      return GenericPackage::InitFromBuffer(p, l);
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
  GenericPackage::Dump(stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::Track::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Track].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericTrack, TrackNumber));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericTrack, TrackName));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericTrack, Sequence));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Track, EditRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(Track, Origin));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "  GenerationUID      = %s\n",  GenerationUID.ToString(identbuf));
  fprintf(stream, "  TrackID            = %lu\n", TrackID);
  fprintf(stream, "  TrackNumber        = %lu\n", TrackNumber);
  fprintf(stream, "  TrackName          = %s\n",  TrackName.ToString(identbuf));
  fprintf(stream, "  Sequence           = %s\n",  Sequence.ToString(identbuf));
  fprintf(stream, "  EditRate           = %s\n",  EditRate.ToString(identbuf));
  fprintf(stream, "  Origin             = %s\n",  i64sz(Origin, identbuf));

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::Sequence::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Sequence].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Sequence, StructuralComponents));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  const MDDEntry* Entry = GetMDDEntry(DataDefinition.Data());

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  DataDefinition     = %s (%s)\n", DataDefinition.ToString(identbuf), (Entry ? Entry->name : "Unknown"));
  fprintf(stream, "  Duration           = %s\n", ui64sz(Duration, identbuf));
  fprintf(stream, "  StructuralComponents:\n");  StructuralComponents.Dump(stream);

  fputs("==========================================================================\n", stream);
}



//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::SourceClip::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_SourceClip].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(SourceClip, StartPosition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(SourceClip, SourcePackageID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(SourceClip, SourceTrackID));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  DataDefinition     = %s\n", DataDefinition.ToString(identbuf));
  fprintf(stream, "  StartPosition      = %s\n", ui64sz(StartPosition, identbuf));
  fprintf(stream, "  SourcePackageID    = %s\n", SourcePackageID.ToString(identbuf));
  fprintf(stream, "  SourcePackageID    = %u\n", SourceTrackID);

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::TimecodeComponent::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_TimecodeComponent].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(StructuralComponent, DataDefinition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(StructuralComponent, Duration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi16(OBJ_READ_ARGS(TimecodeComponent, RoundedTimecodeBase));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(TimecodeComponent, StartTimecode));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(TimecodeComponent, DropFrame));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  DataDefinition     = %s\n", DataDefinition.ToString(identbuf));
  fprintf(stream, "  Duration           = %s\n", ui64sz(Duration, identbuf));
  fprintf(stream, "  RoundedTimecodeBase= %u\n", RoundedTimecodeBase);
  fprintf(stream, "  StartTimecode      = %s\n", ui64sz(StartTimecode, identbuf));
  fprintf(stream, "  DropFrame          = %d\n", DropFrame);

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::WaveAudioDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_WaveAudioDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      MXF::Rational TmpRat;

      //InterchangeObject_InstanceUID
      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, SampleRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(FileDescriptor, ContainerDuration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(FileDescriptor, LinkedTrackID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, EssenceContainer));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, AudioSamplingRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, Locked));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, ChannelCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericSoundEssenceDescriptor, QuantizationBits));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi16(OBJ_READ_ARGS(WaveAudioDescriptor, BlockAlign));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(WaveAudioDescriptor, AvgBps));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "          InstanceUID: %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "        LinkedTrackID: %lu\n", LinkedTrackID);
  fprintf(stream, "     EssenceContainer: %s\n",  EssenceContainer.ToString(identbuf));
  fprintf(stream, "...\n");

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::MPEG2VideoDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_MPEG2VideoDescriptor].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
      MXF::Rational TmpRat;
      ui8_t tmp_delay;

      //InterchangeObject_InstanceUID
      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS_R(FileDescriptor, SampleRate, TmpRat));
      SampleRate = TmpRat;
      ui64_t tmpDuration = 0;
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS_R(FileDescriptor, ContainerDuration, tmpDuration));
      ContainerDuration = tmpDuration;
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(FileDescriptor, LinkedTrackID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, EssenceContainer));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS_R(GenericPictureEssenceDescriptor, AspectRatio, TmpRat));
      AspectRatio = TmpRat;
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, FrameLayout));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredWidth));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(GenericPictureEssenceDescriptor, StoredHeight));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, ComponentDepth));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, HorizontalSubsampling));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(CDCIEssenceDescriptor, VerticalSubsampling));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(CDCIEssenceDescriptor, ColorSiting));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(MPEG2VideoDescriptor,  CodedContentType));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(MPEG2VideoDescriptor,  BitRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(MPEG2VideoDescriptor,  ProfileAndLevel));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS_R(MPEG2VideoDescriptor,  LowDelay, tmp_delay));
      LowDelay = (tmp_delay > 0);
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "          InstanceUID: %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "        LinkedTrackID: %lu\n", LinkedTrackID);
  fprintf(stream, "     EssenceContainer: %s\n",  EssenceContainer.ToString(identbuf));
  ASDCP::MPEG2::VideoDescriptorDump(*this, stream);

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::FileDescriptor::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l); // any of a variety of ULs, really

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericDescriptor, Locators));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenericDescriptor, SubDescriptors));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(FileDescriptor, LinkedTrackID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, SampleRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(FileDescriptor, ContainerDuration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, EssenceContainer));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(FileDescriptor, Codec));
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
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "          InstanceUID: %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "        GenerationUID: %lu\n", GenerationUID.ToString(identbuf));
  fprintf(stream, "        LinkedTrackID: %lu\n", LinkedTrackID);
  fprintf(stream, "     EssenceContainer: %s\n",  EssenceContainer.ToString(identbuf));
  fprintf(stream, "...\n");

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::GenericPictureEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_GenericPictureEssenceDescriptor].ul, 0);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::RGBAEssenceDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_RGBAEssenceDescriptor].ul, 0);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::JPEG2000PictureSubDescriptor::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_JPEG2000PictureSubDescriptor].ul, 0);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::CryptographicFramework::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_CryptographicFramework].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicFramework, ContextSR));
    }

  return result;
}

//
void
ASDCP::MXF::CryptographicFramework::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  ContextSR          = %s\n", ContextSR.ToString(identbuf));

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::CryptographicContext::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_CryptographicContext].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicContext, ContextID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicContext, SourceEssenceContainer));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicContext, CipherAlgorithm));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicContext, MICAlgorithm));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(CryptographicContext, CryptographicKeyID));
    }

  return result;
}

//
void
ASDCP::MXF::CryptographicContext::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n", InstanceUID.ToString(identbuf));
  fprintf(stream, "  ContextID          = %s\n", ContextID.ToString(identbuf));
  fprintf(stream, "  SourceEssenceCnt   = %s\n", SourceEssenceContainer.ToString(identbuf));
  fprintf(stream, "  CipherAlgorithm    = %s\n", CipherAlgorithm.ToString(identbuf));
  fprintf(stream, "  MICAlgorithm       = %s\n", MICAlgorithm.ToString(identbuf));
  fprintf(stream, "  CryptographicKeyID = %s\n", CryptographicKeyID.ToString(identbuf));

  fputs("==========================================================================\n", stream);
}

//
// end MXF.cpp
//
