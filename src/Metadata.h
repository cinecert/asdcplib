/*
Copyright (c) 2005-2021, John Hurst
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
/*! \file    Metadata.h
    \version $Id$
    \brief   MXF metadata objects
*/

#ifndef _Metadata_H_
#define _Metadata_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
      void Metadata_InitTypes(const Dictionary* Dict);

      //

      //
      class Identification : public InterchangeObject
	{
	  Identification();

	public:
          UUID ThisGenerationUID;
          UTF16String CompanyName;
          UTF16String ProductName;
          VersionType ProductVersion;
          UTF16String VersionString;
          UUID ProductUID;
          Kumu::Timestamp ModificationDate;
          VersionType ToolkitVersion;
          optional_property<UTF16String > Platform;

      Identification(const Dictionary* d);
      Identification(const Identification& rhs);
      virtual ~Identification() {}

      const Identification& operator=(const Identification& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Identification& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "Identification"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ContentStorage : public InterchangeObject
	{
	  ContentStorage();

	public:
          Batch<UUID> Packages;
          Batch<UUID> EssenceContainerData;

      ContentStorage(const Dictionary* d);
      ContentStorage(const ContentStorage& rhs);
      virtual ~ContentStorage() {}

      const ContentStorage& operator=(const ContentStorage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const ContentStorage& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "ContentStorage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class EssenceContainerData : public InterchangeObject
	{
	  EssenceContainerData();

	public:
          UMID LinkedPackageUID;
          optional_property<ui32_t > IndexSID;
          ui32_t BodySID;

      EssenceContainerData(const Dictionary* d);
      EssenceContainerData(const EssenceContainerData& rhs);
      virtual ~EssenceContainerData() {}

      const EssenceContainerData& operator=(const EssenceContainerData& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const EssenceContainerData& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "EssenceContainerData"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPackage : public InterchangeObject
	{
	  GenericPackage();

	public:
          UMID PackageUID;
          optional_property<UTF16String > Name;
          Kumu::Timestamp PackageCreationDate;
          Kumu::Timestamp PackageModifiedDate;
          Array<UUID> Tracks;

      GenericPackage(const Dictionary* d);
      GenericPackage(const GenericPackage& rhs);
      virtual ~GenericPackage() {}

      const GenericPackage& operator=(const GenericPackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericPackage& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericPackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class MaterialPackage : public GenericPackage
	{
	  MaterialPackage();

	public:
          optional_property<UUID > PackageMarker;

      MaterialPackage(const Dictionary* d);
      MaterialPackage(const MaterialPackage& rhs);
      virtual ~MaterialPackage() {}

      const MaterialPackage& operator=(const MaterialPackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MaterialPackage& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "MaterialPackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourcePackage : public GenericPackage
	{
	  SourcePackage();

	public:
          UUID Descriptor;

      SourcePackage(const Dictionary* d);
      SourcePackage(const SourcePackage& rhs);
      virtual ~SourcePackage() {}

      const SourcePackage& operator=(const SourcePackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SourcePackage& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "SourcePackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericTrack : public InterchangeObject
	{
	  GenericTrack();

	public:
          ui32_t TrackID;
          ui32_t TrackNumber;
          optional_property<UTF16String > TrackName;
          optional_property<UUID > Sequence;

      GenericTrack(const Dictionary* d);
      GenericTrack(const GenericTrack& rhs);
      virtual ~GenericTrack() {}

      const GenericTrack& operator=(const GenericTrack& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericTrack& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericTrack"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class StaticTrack : public GenericTrack
	{
	  StaticTrack();

	public:

      StaticTrack(const Dictionary* d);
      StaticTrack(const StaticTrack& rhs);
      virtual ~StaticTrack() {}

      const StaticTrack& operator=(const StaticTrack& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StaticTrack& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "StaticTrack"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class Track : public GenericTrack
	{
	  Track();

	public:
          Rational EditRate;
          ui64_t Origin;

      Track(const Dictionary* d);
      Track(const Track& rhs);
      virtual ~Track() {}

      const Track& operator=(const Track& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Track& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "Track"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StructuralComponent : public InterchangeObject
	{
	  StructuralComponent();

	public:
          UL DataDefinition;
          optional_property<ui64_t > Duration;

      StructuralComponent(const Dictionary* d);
      StructuralComponent(const StructuralComponent& rhs);
      virtual ~StructuralComponent() {}

      const StructuralComponent& operator=(const StructuralComponent& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StructuralComponent& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "StructuralComponent"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class Sequence : public StructuralComponent
	{
	  Sequence();

	public:
          Array<UUID> StructuralComponents;

      Sequence(const Dictionary* d);
      Sequence(const Sequence& rhs);
      virtual ~Sequence() {}

      const Sequence& operator=(const Sequence& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Sequence& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "Sequence"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourceClip : public StructuralComponent
	{
	  SourceClip();

	public:
          ui64_t StartPosition;
          UMID SourcePackageID;
          ui32_t SourceTrackID;

      SourceClip(const Dictionary* d);
      SourceClip(const SourceClip& rhs);
      virtual ~SourceClip() {}

      const SourceClip& operator=(const SourceClip& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SourceClip& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "SourceClip"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimecodeComponent : public StructuralComponent
	{
	  TimecodeComponent();

	public:
          ui16_t RoundedTimecodeBase;
          ui64_t StartTimecode;
          ui8_t DropFrame;

      TimecodeComponent(const Dictionary* d);
      TimecodeComponent(const TimecodeComponent& rhs);
      virtual ~TimecodeComponent() {}

      const TimecodeComponent& operator=(const TimecodeComponent& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimecodeComponent& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TimecodeComponent"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDescriptor : public InterchangeObject
	{
	  GenericDescriptor();

	public:
          Array<UUID> Locators;
          Array<UUID> SubDescriptors;

      GenericDescriptor(const Dictionary* d);
      GenericDescriptor(const GenericDescriptor& rhs);
      virtual ~GenericDescriptor() {}

      const GenericDescriptor& operator=(const GenericDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class FileDescriptor : public GenericDescriptor
	{
	  FileDescriptor();

	public:
          optional_property<ui32_t > LinkedTrackID;
          Rational SampleRate;
          optional_property<ui64_t > ContainerDuration;
          UL EssenceContainer;
          optional_property<UL > Codec;

      FileDescriptor(const Dictionary* d);
      FileDescriptor(const FileDescriptor& rhs);
      virtual ~FileDescriptor() {}

      const FileDescriptor& operator=(const FileDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const FileDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "FileDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericSoundEssenceDescriptor : public FileDescriptor
	{
	  GenericSoundEssenceDescriptor();

	public:
          Rational AudioSamplingRate;
          ui8_t Locked;
          optional_property<ui8_t > AudioRefLevel;
          optional_property<ui8_t > ElectroSpatialFormulation;
          ui32_t ChannelCount;
          ui32_t QuantizationBits;
          optional_property<ui8_t > DialNorm;
          UL SoundEssenceCoding;
          optional_property<ui8_t > ReferenceAudioAlignmentLevel;
          optional_property<Rational > ReferenceImageEditRate;

      GenericSoundEssenceDescriptor(const Dictionary* d);
      GenericSoundEssenceDescriptor(const GenericSoundEssenceDescriptor& rhs);
      virtual ~GenericSoundEssenceDescriptor() {}

      const GenericSoundEssenceDescriptor& operator=(const GenericSoundEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericSoundEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericSoundEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class WaveAudioDescriptor : public GenericSoundEssenceDescriptor
	{
	  WaveAudioDescriptor();

	public:
          ui16_t BlockAlign;
          optional_property<ui8_t > SequenceOffset;
          ui32_t AvgBps;
          optional_property<UL > ChannelAssignment;

      WaveAudioDescriptor(const Dictionary* d);
      WaveAudioDescriptor(const WaveAudioDescriptor& rhs);
      virtual ~WaveAudioDescriptor() {}

      const WaveAudioDescriptor& operator=(const WaveAudioDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const WaveAudioDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "WaveAudioDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPictureEssenceDescriptor : public FileDescriptor
	{
	  GenericPictureEssenceDescriptor();

	public:
          optional_property<ui8_t > SignalStandard;
          ui8_t FrameLayout;
          ui32_t StoredWidth;
          ui32_t StoredHeight;
          optional_property<ui32_t > StoredF2Offset;
          optional_property<ui32_t > SampledWidth;
          optional_property<ui32_t > SampledHeight;
          optional_property<ui32_t > SampledXOffset;
          optional_property<ui32_t > SampledYOffset;
          optional_property<ui32_t > DisplayHeight;
          optional_property<ui32_t > DisplayWidth;
          optional_property<ui32_t > DisplayXOffset;
          optional_property<ui32_t > DisplayYOffset;
          optional_property<ui32_t > DisplayF2Offset;
          Rational AspectRatio;
          optional_property<ui8_t > ActiveFormatDescriptor;
          optional_property<ui8_t > AlphaTransparency;
          optional_property<UL > TransferCharacteristic;
          optional_property<ui32_t > ImageAlignmentOffset;
          optional_property<ui32_t > ImageStartOffset;
          optional_property<ui32_t > ImageEndOffset;
          optional_property<ui8_t > FieldDominance;
          UL PictureEssenceCoding;
          optional_property<UL > CodingEquations;
          optional_property<UL > ColorPrimaries;
          optional_property<Batch<UL> > AlternativeCenterCuts;
          optional_property<ui32_t > ActiveWidth;
          optional_property<ui32_t > ActiveHeight;
          optional_property<ui32_t > ActiveXOffset;
          optional_property<ui32_t > ActiveYOffset;
          optional_property<LineMapPair > VideoLineMap;
          optional_property<ThreeColorPrimaries > MasteringDisplayPrimaries;
          optional_property<ColorPrimary > MasteringDisplayWhitePointChromaticity;
          optional_property<ui32_t > MasteringDisplayMaximumLuminance;
          optional_property<ui32_t > MasteringDisplayMinimumLuminance;

      GenericPictureEssenceDescriptor(const Dictionary* d);
      GenericPictureEssenceDescriptor(const GenericPictureEssenceDescriptor& rhs);
      virtual ~GenericPictureEssenceDescriptor() {}

      const GenericPictureEssenceDescriptor& operator=(const GenericPictureEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericPictureEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericPictureEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class RGBAEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  RGBAEssenceDescriptor();

	public:
          optional_property<ui32_t > ComponentMaxRef;
          optional_property<ui32_t > ComponentMinRef;
          optional_property<ui32_t > AlphaMinRef;
          optional_property<ui32_t > AlphaMaxRef;
          optional_property<ui8_t > ScanningDirection;
          RGBALayout PixelLayout;

      RGBAEssenceDescriptor(const Dictionary* d);
      RGBAEssenceDescriptor(const RGBAEssenceDescriptor& rhs);
      virtual ~RGBAEssenceDescriptor() {}

      const RGBAEssenceDescriptor& operator=(const RGBAEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const RGBAEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "RGBAEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class JPEG2000PictureSubDescriptor : public InterchangeObject
	{
	  JPEG2000PictureSubDescriptor();

	public:
          ui16_t Rsize;
          ui32_t Xsize;
          ui32_t Ysize;
          ui32_t XOsize;
          ui32_t YOsize;
          ui32_t XTsize;
          ui32_t YTsize;
          ui32_t XTOsize;
          ui32_t YTOsize;
          ui16_t Csize;
          optional_property<Raw > PictureComponentSizing;
          optional_property<Raw > CodingStyleDefault;
          optional_property<Raw > QuantizationDefault;
          optional_property<RGBALayout > J2CLayout;
          optional_property<J2KExtendedCapabilitiesType > J2KExtendedCapabilities;
          optional_property<Array<Kumu::ArchivableUi16> > J2KProfile;
          optional_property<Array<Kumu::ArchivableUi16> > J2KCorrespondingProfile;

      JPEG2000PictureSubDescriptor(const Dictionary* d);
      JPEG2000PictureSubDescriptor(const JPEG2000PictureSubDescriptor& rhs);
      virtual ~JPEG2000PictureSubDescriptor() {}

      const JPEG2000PictureSubDescriptor& operator=(const JPEG2000PictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const JPEG2000PictureSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "JPEG2000PictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CDCIEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  CDCIEssenceDescriptor();

	public:
          ui32_t ComponentDepth;
          ui32_t HorizontalSubsampling;
          optional_property<ui32_t > VerticalSubsampling;
          optional_property<ui8_t > ColorSiting;
          optional_property<ui8_t > ReversedByteOrder;
          optional_property<ui16_t > PaddingBits;
          optional_property<ui32_t > AlphaSampleDepth;
          optional_property<ui32_t > BlackRefLevel;
          optional_property<ui32_t > WhiteReflevel;
          optional_property<ui32_t > ColorRange;

      CDCIEssenceDescriptor(const Dictionary* d);
      CDCIEssenceDescriptor(const CDCIEssenceDescriptor& rhs);
      virtual ~CDCIEssenceDescriptor() {}

      const CDCIEssenceDescriptor& operator=(const CDCIEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CDCIEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "CDCIEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MPEG2VideoDescriptor : public CDCIEssenceDescriptor
	{
	  MPEG2VideoDescriptor();

	public:
          optional_property<ui8_t > SingleSequence;
          optional_property<ui8_t > ConstantBFrames;
          optional_property<ui8_t > CodedContentType;
          optional_property<ui8_t > LowDelay;
          optional_property<ui8_t > ClosedGOP;
          optional_property<ui8_t > IdenticalGOP;
          optional_property<ui8_t > MaxGOP;
          optional_property<ui8_t > BPictureCount;
          optional_property<ui32_t > BitRate;
          optional_property<ui8_t > ProfileAndLevel;

      MPEG2VideoDescriptor(const Dictionary* d);
      MPEG2VideoDescriptor(const MPEG2VideoDescriptor& rhs);
      virtual ~MPEG2VideoDescriptor() {}

      const MPEG2VideoDescriptor& operator=(const MPEG2VideoDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MPEG2VideoDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "MPEG2VideoDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DMSegment : public InterchangeObject
	{
	  DMSegment();

	public:
          UL DataDefinition;
          optional_property<ui64_t > Duration;
          optional_property<ui64_t > EventStartPosition;
          optional_property<UTF16String > EventComment;
          UUID DMFramework;

      DMSegment(const Dictionary* d);
      DMSegment(const DMSegment& rhs);
      virtual ~DMSegment() {}

      const DMSegment& operator=(const DMSegment& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DMSegment& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "DMSegment"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicFramework : public InterchangeObject
	{
	  CryptographicFramework();

	public:
          UUID ContextSR;

      CryptographicFramework(const Dictionary* d);
      CryptographicFramework(const CryptographicFramework& rhs);
      virtual ~CryptographicFramework() {}

      const CryptographicFramework& operator=(const CryptographicFramework& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CryptographicFramework& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "CryptographicFramework"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicContext : public InterchangeObject
	{
	  CryptographicContext();

	public:
          UUID ContextID;
          UL SourceEssenceContainer;
          UL CipherAlgorithm;
          UL MICAlgorithm;
          UUID CryptographicKeyID;

      CryptographicContext(const Dictionary* d);
      CryptographicContext(const CryptographicContext& rhs);
      virtual ~CryptographicContext() {}

      const CryptographicContext& operator=(const CryptographicContext& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CryptographicContext& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "CryptographicContext"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DescriptiveFramework : public InterchangeObject
	{
	  DescriptiveFramework();

	public:
          optional_property<UUID > LinkedDescriptiveFrameworkPlugInId;

      DescriptiveFramework(const Dictionary* d);
      DescriptiveFramework(const DescriptiveFramework& rhs);
      virtual ~DescriptiveFramework() {}

      const DescriptiveFramework& operator=(const DescriptiveFramework& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DescriptiveFramework& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "DescriptiveFramework"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DescriptiveObject : public InterchangeObject
	{
	  DescriptiveObject();

	public:
          optional_property<UUID > LinkedDescriptiveObjectPlugInId;

      DescriptiveObject(const Dictionary* d);
      DescriptiveObject(const DescriptiveObject& rhs);
      virtual ~DescriptiveObject() {}

      const DescriptiveObject& operator=(const DescriptiveObject& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DescriptiveObject& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "DescriptiveObject"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDataEssenceDescriptor : public FileDescriptor
	{
	  GenericDataEssenceDescriptor();

	public:
          UL DataEssenceCoding;

      GenericDataEssenceDescriptor(const Dictionary* d);
      GenericDataEssenceDescriptor(const GenericDataEssenceDescriptor& rhs);
      virtual ~GenericDataEssenceDescriptor() {}

      const GenericDataEssenceDescriptor& operator=(const GenericDataEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericDataEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericDataEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextDescriptor : public GenericDataEssenceDescriptor
	{
	  TimedTextDescriptor();

	public:
          UUID ResourceID;
          UTF16String UCSEncoding;
          UTF16String NamespaceURI;
          optional_property<UTF16String > RFC5646LanguageTagList;
          optional_property<UTF16String > DisplayType;
          optional_property<UTF16String > IntrinsicPictureResolution;
          optional_property<ui8_t > ZPositionInUse;

      TimedTextDescriptor(const Dictionary* d);
      TimedTextDescriptor(const TimedTextDescriptor& rhs);
      virtual ~TimedTextDescriptor() {}

      const TimedTextDescriptor& operator=(const TimedTextDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimedTextDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TimedTextDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextResourceSubDescriptor : public InterchangeObject
	{
	  TimedTextResourceSubDescriptor();

	public:
          UUID AncillaryResourceID;
          UTF16String MIMEMediaType;
          ui32_t EssenceStreamID;

      TimedTextResourceSubDescriptor(const Dictionary* d);
      TimedTextResourceSubDescriptor(const TimedTextResourceSubDescriptor& rhs);
      virtual ~TimedTextResourceSubDescriptor() {}

      const TimedTextResourceSubDescriptor& operator=(const TimedTextResourceSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimedTextResourceSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TimedTextResourceSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StereoscopicPictureSubDescriptor : public InterchangeObject
	{
	  StereoscopicPictureSubDescriptor();

	public:

      StereoscopicPictureSubDescriptor(const Dictionary* d);
      StereoscopicPictureSubDescriptor(const StereoscopicPictureSubDescriptor& rhs);
      virtual ~StereoscopicPictureSubDescriptor() {}

      const StereoscopicPictureSubDescriptor& operator=(const StereoscopicPictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StereoscopicPictureSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "StereoscopicPictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ContainerConstraintsSubDescriptor : public InterchangeObject
	{
	  ContainerConstraintsSubDescriptor();

	public:

      ContainerConstraintsSubDescriptor(const Dictionary* d);
      ContainerConstraintsSubDescriptor(const ContainerConstraintsSubDescriptor& rhs);
      virtual ~ContainerConstraintsSubDescriptor() {}

      const ContainerConstraintsSubDescriptor& operator=(const ContainerConstraintsSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const ContainerConstraintsSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "ContainerConstraintsSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class NetworkLocator : public InterchangeObject
	{
	  NetworkLocator();

	public:
          UTF16String URLString;

      NetworkLocator(const Dictionary* d);
      NetworkLocator(const NetworkLocator& rhs);
      virtual ~NetworkLocator() {}

      const NetworkLocator& operator=(const NetworkLocator& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const NetworkLocator& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "NetworkLocator"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MCALabelSubDescriptor : public InterchangeObject
	{
	  MCALabelSubDescriptor();

	public:
          UL MCALabelDictionaryID;
          UUID MCALinkID;
          UTF16String MCATagSymbol;
          optional_property<UTF16String > MCATagName;
          optional_property<ui32_t > MCAChannelID;
          optional_property<ISO8String > RFC5646SpokenLanguage;
          optional_property<UTF16String > MCATitle;
          optional_property<UTF16String > MCATitleVersion;
          optional_property<UTF16String > MCATitleSubVersion;
          optional_property<UTF16String > MCAEpisode;
          optional_property<UTF16String > MCAPartitionKind;
          optional_property<UTF16String > MCAPartitionNumber;
          optional_property<UTF16String > MCAAudioContentKind;
          optional_property<UTF16String > MCAAudioElementKind;

      MCALabelSubDescriptor(const Dictionary* d);
      MCALabelSubDescriptor(const MCALabelSubDescriptor& rhs);
      virtual ~MCALabelSubDescriptor() {}

      const MCALabelSubDescriptor& operator=(const MCALabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MCALabelSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "MCALabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class AudioChannelLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  AudioChannelLabelSubDescriptor();

	public:
          optional_property<UUID > SoundfieldGroupLinkID;

      AudioChannelLabelSubDescriptor(const Dictionary* d);
      AudioChannelLabelSubDescriptor(const AudioChannelLabelSubDescriptor& rhs);
      virtual ~AudioChannelLabelSubDescriptor() {}

      const AudioChannelLabelSubDescriptor& operator=(const AudioChannelLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const AudioChannelLabelSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "AudioChannelLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SoundfieldGroupLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  SoundfieldGroupLabelSubDescriptor();

	public:
          optional_property<Array<UUID> > GroupOfSoundfieldGroupsLinkID;

      SoundfieldGroupLabelSubDescriptor(const Dictionary* d);
      SoundfieldGroupLabelSubDescriptor(const SoundfieldGroupLabelSubDescriptor& rhs);
      virtual ~SoundfieldGroupLabelSubDescriptor() {}

      const SoundfieldGroupLabelSubDescriptor& operator=(const SoundfieldGroupLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SoundfieldGroupLabelSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "SoundfieldGroupLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GroupOfSoundfieldGroupsLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  GroupOfSoundfieldGroupsLabelSubDescriptor();

	public:

      GroupOfSoundfieldGroupsLabelSubDescriptor(const Dictionary* d);
      GroupOfSoundfieldGroupsLabelSubDescriptor(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs);
      virtual ~GroupOfSoundfieldGroupsLabelSubDescriptor() {}

      const GroupOfSoundfieldGroupsLabelSubDescriptor& operator=(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GroupOfSoundfieldGroupsLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DCDataDescriptor : public GenericDataEssenceDescriptor
	{
	  DCDataDescriptor();

	public:

      DCDataDescriptor(const Dictionary* d);
      DCDataDescriptor(const DCDataDescriptor& rhs);
      virtual ~DCDataDescriptor() {}

      const DCDataDescriptor& operator=(const DCDataDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DCDataDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "DCDataDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class PrivateDCDataDescriptor : public GenericDataEssenceDescriptor
	{
	  PrivateDCDataDescriptor();

	public:

      PrivateDCDataDescriptor(const Dictionary* d);
      PrivateDCDataDescriptor(const PrivateDCDataDescriptor& rhs);
      virtual ~PrivateDCDataDescriptor() {}

      const PrivateDCDataDescriptor& operator=(const PrivateDCDataDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const PrivateDCDataDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "PrivateDCDataDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DolbyAtmosSubDescriptor : public InterchangeObject
	{
	  DolbyAtmosSubDescriptor();

	public:
          UUID AtmosID;
          ui32_t FirstFrame;
          ui16_t MaxChannelCount;
          ui16_t MaxObjectCount;
          ui8_t AtmosVersion;

      DolbyAtmosSubDescriptor(const Dictionary* d);
      DolbyAtmosSubDescriptor(const DolbyAtmosSubDescriptor& rhs);
      virtual ~DolbyAtmosSubDescriptor() {}

      const DolbyAtmosSubDescriptor& operator=(const DolbyAtmosSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DolbyAtmosSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "DolbyAtmosSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ACESPictureSubDescriptor : public InterchangeObject
	{
	  ACESPictureSubDescriptor();

	public:
          optional_property<UTF16String > ACESAuthoringInformation;
          optional_property<ThreeColorPrimaries > ACESMasteringDisplayPrimaries;
          optional_property<ColorPrimary > ACESMasteringDisplayWhitePointChromaticity;
          optional_property<ui32_t > ACESMasteringDisplayMaximumLuminance;
          optional_property<ui32_t > ACESMasteringDisplayMinimumLuminance;

      ACESPictureSubDescriptor(const Dictionary* d);
      ACESPictureSubDescriptor(const ACESPictureSubDescriptor& rhs);
      virtual ~ACESPictureSubDescriptor() {}

      const ACESPictureSubDescriptor& operator=(const ACESPictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const ACESPictureSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "ACESPictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TargetFrameSubDescriptor : public InterchangeObject
	{
	  TargetFrameSubDescriptor();

	public:
          UUID TargetFrameAncillaryResourceID;
          UTF16String MediaType;
          ui64_t TargetFrameIndex;
          UL TargetFrameTransferCharacteristic;
          UL TargetFrameColorPrimaries;
          ui32_t TargetFrameComponentMaxRef;
          ui32_t TargetFrameComponentMinRef;
          ui32_t TargetFrameEssenceStreamID;
          optional_property<UUID > ACESPictureSubDescriptorInstanceID;
          optional_property<UL > TargetFrameViewingEnvironment;

      TargetFrameSubDescriptor(const Dictionary* d);
      TargetFrameSubDescriptor(const TargetFrameSubDescriptor& rhs);
      virtual ~TargetFrameSubDescriptor() {}

      const TargetFrameSubDescriptor& operator=(const TargetFrameSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TargetFrameSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TargetFrameSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TextBasedDMFramework : public DescriptiveFramework
	{
	  TextBasedDMFramework();

	public:
          optional_property<UUID > ObjectRef;

      TextBasedDMFramework(const Dictionary* d);
      TextBasedDMFramework(const TextBasedDMFramework& rhs);
      virtual ~TextBasedDMFramework() {}

      const TextBasedDMFramework& operator=(const TextBasedDMFramework& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TextBasedDMFramework& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TextBasedDMFramework"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TextBasedObject : public DescriptiveObject
	{
	  TextBasedObject();

	public:
          UL PayloadSchemeID;
          UTF16String TextMIMEMediaType;
          UTF16String RFC5646TextLanguageCode;
          optional_property<UTF16String > TextDataDescription;

      TextBasedObject(const Dictionary* d);
      TextBasedObject(const TextBasedObject& rhs);
      virtual ~TextBasedObject() {}

      const TextBasedObject& operator=(const TextBasedObject& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TextBasedObject& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "TextBasedObject"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericStreamTextBasedSet : public TextBasedObject
	{
	  GenericStreamTextBasedSet();

	public:
          ui32_t GenericStreamSID;

      GenericStreamTextBasedSet(const Dictionary* d);
      GenericStreamTextBasedSet(const GenericStreamTextBasedSet& rhs);
      virtual ~GenericStreamTextBasedSet() {}

      const GenericStreamTextBasedSet& operator=(const GenericStreamTextBasedSet& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericStreamTextBasedSet& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "GenericStreamTextBasedSet"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ISXDDataEssenceDescriptor : public GenericDataEssenceDescriptor
	{
	  ISXDDataEssenceDescriptor();

	public:
          ISO8String NamespaceURI;

      ISXDDataEssenceDescriptor(const Dictionary* d);
      ISXDDataEssenceDescriptor(const ISXDDataEssenceDescriptor& rhs);
      virtual ~ISXDDataEssenceDescriptor() {}

      const ISXDDataEssenceDescriptor& operator=(const ISXDDataEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const ISXDDataEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "ISXDDataEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class PHDRMetadataTrackSubDescriptor : public InterchangeObject
	{
	  PHDRMetadataTrackSubDescriptor();

	public:
          UL DataDefinition;
          ui32_t SourceTrackID;
          ui32_t SimplePayloadSID;

      PHDRMetadataTrackSubDescriptor(const Dictionary* d);
      PHDRMetadataTrackSubDescriptor(const PHDRMetadataTrackSubDescriptor& rhs);
      virtual ~PHDRMetadataTrackSubDescriptor() {}

      const PHDRMetadataTrackSubDescriptor& operator=(const PHDRMetadataTrackSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const PHDRMetadataTrackSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "PHDRMetadataTrackSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class PIMFDynamicMetadataDescriptor : public GenericDataEssenceDescriptor
	{
	  PIMFDynamicMetadataDescriptor();

	public:
          ui32_t GlobalPayloadSID;

      PIMFDynamicMetadataDescriptor(const Dictionary* d);
      PIMFDynamicMetadataDescriptor(const PIMFDynamicMetadataDescriptor& rhs);
      virtual ~PIMFDynamicMetadataDescriptor() {}

      const PIMFDynamicMetadataDescriptor& operator=(const PIMFDynamicMetadataDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const PIMFDynamicMetadataDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "PIMFDynamicMetadataDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class IABEssenceDescriptor : public GenericSoundEssenceDescriptor
	{
	  IABEssenceDescriptor();

	public:

      IABEssenceDescriptor(const Dictionary* d);
      IABEssenceDescriptor(const IABEssenceDescriptor& rhs);
      virtual ~IABEssenceDescriptor() {}

      const IABEssenceDescriptor& operator=(const IABEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const IABEssenceDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "IABEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class IABSoundfieldLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  IABSoundfieldLabelSubDescriptor();

	public:

      IABSoundfieldLabelSubDescriptor(const Dictionary* d);
      IABSoundfieldLabelSubDescriptor(const IABSoundfieldLabelSubDescriptor& rhs);
      virtual ~IABSoundfieldLabelSubDescriptor() {}

      const IABSoundfieldLabelSubDescriptor& operator=(const IABSoundfieldLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const IABSoundfieldLabelSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "IABSoundfieldLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class JPEGXSPictureSubDescriptor : public InterchangeObject
	{
	  JPEGXSPictureSubDescriptor();

	public:
          ui16_t JPEGXSPpih;
          ui16_t JPEGXSPlev;
          ui16_t JPEGXSWf;
          ui16_t JPEGXSHf;
          ui8_t JPEGXSNc;
          Raw JPEGXSComponentTable;
          optional_property<ui16_t > JPEGXSCw;
          optional_property<ui16_t > JPEGXSHsl;
          optional_property<ui32_t > JPEGXSMaximumBitRate;

      JPEGXSPictureSubDescriptor(const Dictionary* d);
      JPEGXSPictureSubDescriptor(const JPEGXSPictureSubDescriptor& rhs);
      virtual ~JPEGXSPictureSubDescriptor() {}

      const JPEGXSPictureSubDescriptor& operator=(const JPEGXSPictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const JPEGXSPictureSubDescriptor& rhs);
      virtual InterchangeObject *Clone() const;
      virtual const char* HasName() { return "JPEGXSPictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _Metadata_H_

//
// end Metadata.h
//
