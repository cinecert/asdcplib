/*
Copyright (c) 2006, John Hurst
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
/*! \file    MDD.[h|cpp]
    \version $Id$
    \brief   MXF Metadata Dictionary
*/

#ifndef _MDD_H_
#define _MDD_H_

//
namespace ASDCP {
    enum MDD_t {
        MDD_MICAlgorithm_NONE,  // 0
        MDD_MXFInterop_OPAtom,  // 1
        MDD_OPAtom,  // 2
        MDD_OP1a,  // 3
        MDD_GCMulti,  // 4
        MDD_PictureDataDef,  // 5
        MDD_SoundDataDef,  // 6
        MDD_TimecodeDataDef,  // 7
        MDD_DescriptiveMetaDataDef,  // 8
        MDD_WAVWrapping,  // 9
        MDD_MPEG2_VESWrapping,  // 10
        MDD_JPEG_2000Wrapping,  // 11
        MDD_JPEG2000Essence,  // 12
        MDD_MPEG2Essence,  // 13
        MDD_MXFInterop_CryptEssence,  // 14
        MDD_CryptEssence,  // 15
        MDD_WAVEssence,  // 16
        MDD_JP2KEssenceCompression,  // 17
        MDD_CipherAlgorithm_AES,  // 18
        MDD_MICAlgorithm_HMAC_SHA1,  // 19
        MDD_KLVFill,  // 20
        MDD_PartitionMetadata_MajorVersion,  // 21
        MDD_PartitionMetadata_MinorVersion,  // 22
        MDD_PartitionMetadata_KAGSize,  // 23
        MDD_PartitionMetadata_ThisPartition,  // 24
        MDD_PartitionMetadata_PreviousPartition,  // 25
        MDD_PartitionMetadata_FooterPartition,  // 26
        MDD_PartitionMetadata_HeaderByteCount,  // 27
        MDD_PartitionMetadata_IndexByteCount,  // 28
        MDD_PartitionMetadata_IndexSID,  // 29
        MDD_PartitionMetadata_BodyOffset,  // 30
        MDD_PartitionMetadata_BodySID,  // 31
        MDD_PartitionMetadata_OperationalPattern,  // 32
        MDD_PartitionMetadata_EssenceContainers,  // 33
        MDD_OpenHeader,  // 34
        MDD_OpenCompleteHeader,  // 35
        MDD_ClosedHeader,  // 36
        MDD_ClosedCompleteHeader,  // 37
        MDD_OpenBodyPartition,  // 38
        MDD_OpenCompleteBodyPartition,  // 39
        MDD_ClosedBodyPartition,  // 40
        MDD_ClosedCompleteBodyPartition,  // 41
        MDD_Footer,  // 42
        MDD_CompleteFooter,  // 43
        MDD_Primer,  // 44
        MDD_Primer_LocalTagEntryBatch,  // 45
        MDD_LocalTagEntryBatch_Primer_LocalTag,  // 46
        MDD_LocalTagEntryBatch_Primer_UID,  // 47
        MDD_InterchangeObject_InstanceUID,  // 48
        MDD_GenerationInterchangeObject_GenerationUID,  // 49
        MDD_DefaultObject,  // 50
        MDD_IndexTableSegmentBase_IndexEditRate,  // 51
        MDD_IndexTableSegmentBase_IndexStartPosition,  // 52
        MDD_IndexTableSegmentBase_IndexDuration,  // 53
        MDD_IndexTableSegmentBase_EditUnitByteCount,  // 54
        MDD_IndexTableSegmentBase_IndexSID,  // 55
        MDD_IndexTableSegmentBase_BodySID,  // 56
        MDD_IndexTableSegmentBase_SliceCount,  // 57
        MDD_IndexTableSegmentBase_PosTableCount,  // 58
        MDD_IndexTableSegment,  // 59
        MDD_IndexTableSegment_DeltaEntryArray,  // 60
        MDD_DeltaEntryArray_IndexTableSegment_PosTableIndex,  // 61
        MDD_DeltaEntryArray_IndexTableSegment_Slice,  // 62
        MDD_DeltaEntryArray_IndexTableSegment_ElementDelta,  // 63
        MDD_IndexTableSegment_IndexEntryArray,  // 64
        MDD_IndexEntryArray_IndexTableSegment_TemporalOffset,  // 65
        MDD_IndexEntryArray_IndexTableSegment_AnchorOffset,  // 66
        MDD_IndexEntryArray_IndexTableSegment_Flags,  // 67
        MDD_IndexEntryArray_IndexTableSegment_StreamOffset,  // 68
        MDD_IndexEntryArray_IndexTableSegment_SliceOffsetArray,  // 69
        MDD_IndexEntryArray_IndexTableSegment_PosTableArray,  // 70
        MDD_RandomIndexMetadata,  // 71
        MDD_PartitionArray_RandomIndexMetadata_BodySID,  // 72
        MDD_PartitionArray_RandomIndexMetadata_ByteOffset,  // 73
        MDD_RandomIndexMetadata_Length,  // 74
        MDD_RandomIndexMetadataV10,  // 75
        MDD_Preface,  // 76
        MDD_Preface_LastModifiedDate,  // 77
        MDD_Preface_Version,  // 78
        MDD_Preface_ObjectModelVersion,  // 79
        MDD_Preface_PrimaryPackage,  // 80
        MDD_Preface_Identifications,  // 81
        MDD_Preface_ContentStorage,  // 82
        MDD_Preface_OperationalPattern,  // 83
        MDD_Preface_EssenceContainers,  // 84
        MDD_Preface_DMSchemes,  // 85
        MDD_Identification,  // 86
        MDD_Identification_ThisGenerationUID,  // 87
        MDD_Identification_CompanyName,  // 88
        MDD_Identification_ProductName,  // 89
        MDD_Identification_ProductVersion,  // 90
        MDD_Identification_VersionString,  // 91
        MDD_Identification_ProductUID,  // 92
        MDD_Identification_ModificationDate,  // 93
        MDD_Identification_ToolkitVersion,  // 94
        MDD_Identification_Platform,  // 95
        MDD_ContentStorage,  // 96
        MDD_ContentStorage_Packages,  // 97
        MDD_ContentStorage_EssenceContainerData,  // 98
        MDD_ContentStorageKludge_V10Packages,  // 99
        MDD_EssenceContainerData,  // 100
        MDD_EssenceContainerData_LinkedPackageUID,  // 101
        MDD_EssenceContainerData_IndexSID,  // 102
        MDD_EssenceContainerData_BodySID,  // 103
        MDD_GenericPackage_PackageUID,  // 104
        MDD_GenericPackage_Name,  // 105
        MDD_GenericPackage_PackageCreationDate,  // 106
        MDD_GenericPackage_PackageModifiedDate,  // 107
        MDD_GenericPackage_Tracks,  // 108
        MDD_NetworkLocator,  // 109
        MDD_NetworkLocator_URLString,  // 110
        MDD_TextLocator,  // 111
        MDD_TextLocator_LocatorName,  // 112
        MDD_GenericTrack_TrackID,  // 113
        MDD_GenericTrack_TrackNumber,  // 114
        MDD_GenericTrack_TrackName,  // 115
        MDD_GenericTrack_Sequence,  // 116
        MDD_StaticTrack,  // 117
        MDD_Track,  // 118
        MDD_Track_EditRate,  // 119
        MDD_Track_Origin,  // 120
        MDD_EventTrack,  // 121
        MDD_EventTrack_EventEditRate,  // 122
        MDD_EventTrack_EventOrigin,  // 123
        MDD_StructuralComponent_DataDefinition,  // 124
        MDD_StructuralComponent_Duration,  // 125
        MDD_Sequence,  // 126
        MDD_Sequence_StructuralComponents,  // 127
        MDD_TimecodeComponent,  // 128
        MDD_TimecodeComponent_RoundedTimecodeBase,  // 129
        MDD_TimecodeComponent_StartTimecode,  // 130
        MDD_TimecodeComponent_DropFrame,  // 131
        MDD_SourceClip,  // 132
        MDD_SourceClip_StartPosition,  // 133
        MDD_SourceClip_SourcePackageID,  // 134
        MDD_SourceClip_SourceTrackID,  // 135
        MDD_DMSegment,  // 136
        MDD_DMSegment_EventStartPosition,  // 137
        MDD_DMSegment_EventComment,  // 138
        MDD_DMSegment_TrackIDs,  // 139
        MDD_DMSegment_DMFramework,  // 140
        MDD_DMSourceClip,  // 141
        MDD_DMSourceClip_DMSourceClipTrackIDs,  // 142
        MDD_MaterialPackage,  // 143
        MDD_SourcePackage,  // 144
        MDD_SourcePackage_Descriptor,  // 145
        MDD_GenericDescriptor_Locators,  // 146
        MDD_GenericDescriptor_SubDescriptors,  // 147
        MDD_FileDescriptor,  // 148
        MDD_FileDescriptor_LinkedTrackID,  // 149
        MDD_FileDescriptor_SampleRate,  // 150
        MDD_FileDescriptor_ContainerDuration,  // 151
        MDD_FileDescriptor_EssenceContainer,  // 152
        MDD_FileDescriptor_Codec,  // 153
        MDD_GenericPictureEssenceDescriptor,  // 154
        MDD_GenericPictureEssenceDescriptor_SignalStandard,  // 155
        MDD_GenericPictureEssenceDescriptor_FrameLayout,  // 156
        MDD_GenericPictureEssenceDescriptor_StoredWidth,  // 157
        MDD_GenericPictureEssenceDescriptor_StoredHeight,  // 158
        MDD_GenericPictureEssenceDescriptor_StoredF2Offset,  // 159
        MDD_GenericPictureEssenceDescriptor_SampledWidth,  // 160
        MDD_GenericPictureEssenceDescriptor_SampledHeight,  // 161
        MDD_GenericPictureEssenceDescriptor_SampledXOffset,  // 162
        MDD_GenericPictureEssenceDescriptor_SampledYOffset,  // 163
        MDD_GenericPictureEssenceDescriptor_DisplayHeight,  // 164
        MDD_GenericPictureEssenceDescriptor_DisplayWidth,  // 165
        MDD_GenericPictureEssenceDescriptor_DisplayXOffset,  // 166
        MDD_GenericPictureEssenceDescriptor_DisplayYOffset,  // 167
        MDD_GenericPictureEssenceDescriptor_DisplayF2Offset,  // 168
        MDD_GenericPictureEssenceDescriptor_AspectRatio,  // 169
        MDD_GenericPictureEssenceDescriptor_ActiveFormatDescriptor,  // 170
        MDD_GenericPictureEssenceDescriptor_VideoLineMap,  // 171
        MDD_GenericPictureEssenceDescriptor_AlphaTransparency,  // 172
        MDD_GenericPictureEssenceDescriptor_Gamma,  // 173
        MDD_GenericPictureEssenceDescriptor_ImageAlignmentOffset,  // 174
        MDD_GenericPictureEssenceDescriptor_ImageStartOffset,  // 175
        MDD_GenericPictureEssenceDescriptor_ImageEndOffset,  // 176
        MDD_GenericPictureEssenceDescriptor_FieldDominance,  // 177
        MDD_GenericPictureEssenceDescriptor_PictureEssenceCoding,  // 178
        MDD_CDCIEssenceDescriptor,  // 179
        MDD_CDCIEssenceDescriptor_ComponentDepth,  // 180
        MDD_CDCIEssenceDescriptor_HorizontalSubsampling,  // 181
        MDD_CDCIEssenceDescriptor_VerticalSubsampling,  // 182
        MDD_CDCIEssenceDescriptor_ColorSiting,  // 183
        MDD_CDCIEssenceDescriptor_ReversedByteOrder,  // 184
        MDD_CDCIEssenceDescriptor_PaddingBits,  // 185
        MDD_CDCIEssenceDescriptor_AlphaSampleDepth,  // 186
        MDD_CDCIEssenceDescriptor_BlackRefLevel,  // 187
        MDD_CDCIEssenceDescriptor_WhiteReflevel,  // 188
        MDD_CDCIEssenceDescriptor_ColorRange,  // 189
        MDD_RGBAEssenceDescriptor,  // 190
        MDD_RGBAEssenceDescriptor_ComponentMaxRef,  // 191
        MDD_RGBAEssenceDescriptor_ComponentMinRef,  // 192
        MDD_RGBAEssenceDescriptor_AlphaMaxRef,  // 193
        MDD_RGBAEssenceDescriptor_AlphaMinRef,  // 194
        MDD_RGBAEssenceDescriptor_ScanningDirection,  // 195
        MDD_RGBAEssenceDescriptor_PixelLayout,  // 196
        MDD_RGBAEssenceDescriptor_Palette,  // 197
        MDD_RGBAEssenceDescriptor_PaletteLayout,  // 198
        MDD_GenericSoundEssenceDescriptor,  // 199
        MDD_GenericSoundEssenceDescriptor_AudioSamplingRate,  // 200
        MDD_GenericSoundEssenceDescriptor_Locked,  // 201
        MDD_GenericSoundEssenceDescriptor_AudioRefLevel,  // 202
        MDD_GenericSoundEssenceDescriptor_ElectroSpatialFormulation,  // 203
        MDD_GenericSoundEssenceDescriptor_ChannelCount,  // 204
        MDD_GenericSoundEssenceDescriptor_QuantizationBits,  // 205
        MDD_GenericSoundEssenceDescriptor_DialNorm,  // 206
        MDD_GenericSoundEssenceDescriptor_SoundEssenceCompression,  // 207
        MDD_GenericDataEssenceDescriptor,  // 208
        MDD_GenericDataEssenceDescriptor_DataEssenceCoding,  // 209
        MDD_MultipleDescriptor,  // 210
        MDD_MultipleDescriptor_SubDescriptorUIDs,  // 211
        MDD_MPEG2VideoDescriptor,  // 212
        MDD_MPEG2VideoDescriptor_SingleSequence,  // 213
        MDD_MPEG2VideoDescriptor_ConstantBFrames,  // 214
        MDD_MPEG2VideoDescriptor_CodedContentType,  // 215
        MDD_MPEG2VideoDescriptor_LowDelay,  // 216
        MDD_MPEG2VideoDescriptor_ClosedGOP,  // 217
        MDD_MPEG2VideoDescriptor_IdenticalGOP,  // 218
        MDD_MPEG2VideoDescriptor_MaxGOP,  // 219
        MDD_MPEG2VideoDescriptor_BPictureCount,  // 220
        MDD_MPEG2VideoDescriptor_BitRate,  // 221
        MDD_MPEG2VideoDescriptor_ProfileAndLevel,  // 222
        MDD_WaveAudioDescriptor,  // 223
        MDD_WaveAudioDescriptor_BlockAlign,  // 224
        MDD_WaveAudioDescriptor_SequenceOffset,  // 225
        MDD_WaveAudioDescriptor_AvgBps,  // 226
        MDD_WaveAudioDescriptor_PeakEnvelope,  // 227
        MDD_JPEG2000PictureSubDescriptor,  // 228
        MDD_JPEG2000PictureSubDescriptor_Rsize,  // 229
        MDD_JPEG2000PictureSubDescriptor_Xsize,  // 230
        MDD_JPEG2000PictureSubDescriptor_Ysize,  // 231
        MDD_JPEG2000PictureSubDescriptor_XOsize,  // 232
        MDD_JPEG2000PictureSubDescriptor_YOsize,  // 233
        MDD_JPEG2000PictureSubDescriptor_XTsize,  // 234
        MDD_JPEG2000PictureSubDescriptor_YTsize,  // 235
        MDD_JPEG2000PictureSubDescriptor_XTOsize,  // 236
        MDD_JPEG2000PictureSubDescriptor_YTOsize,  // 237
        MDD_JPEG2000PictureSubDescriptor_Csize,  // 238
        MDD_JPEG2000PictureSubDescriptor_PictureComponentSizing,  // 239
        MDD_JPEG2000PictureSubDescriptor_CodingStyleDefault,  // 240
        MDD_JPEG2000PictureSubDescriptor_QuantizationDefault,  // 241
        MDD_DM_Framework,  // 242
        MDD_DM_Set,  // 243
        MDD_EncryptedContainerLabel,  // 244
        MDD_CryptographicFrameworkLabel,  // 245
        MDD_CryptographicFramework,  // 246
        MDD_CryptographicFramework_ContextSR,  // 247
        MDD_CryptographicContext,  // 248
        MDD_CryptographicContext_ContextID,  // 249
        MDD_CryptographicContext_SourceEssenceContainer,  // 250
        MDD_CryptographicContext_CipherAlgorithm,  // 251
        MDD_CryptographicContext_MICAlgorithm,  // 252
        MDD_CryptographicContext_CryptographicKeyID,  // 253
    }; // enum MDD_t
} // namespaceASDCP


#endif // _MDD_H_

//
// end MDD.h
//
