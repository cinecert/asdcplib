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
        MDD_OPAtom,  // 1
        MDD_OP1a,  // 2
        MDD_GCMulti,  // 3
        MDD_PictureDataDef,  // 4
        MDD_SoundDataDef,  // 5
        MDD_TimecodeDataDef,  // 6
        MDD_DescriptiveMetaDataDef,  // 7
        MDD_WAVWrapping,  // 8
        MDD_MPEG2_VESWrapping,  // 9
        MDD_JPEG_2000Wrapping,  // 10
        MDD_JPEG2000Essence,  // 11
        MDD_MPEG2Essence,  // 12
        MDD_CryptEssence,  // 13
        MDD_WAVEssence,  // 14
        MDD_JP2KEssenceCompression,  // 15
        MDD_CipherAlgorithm_AES,  // 16
        MDD_MICAlgorithm_HMAC_SHA1,  // 17
        MDD_KLVFill,  // 18
        MDD_PartitionMetadata_MajorVersion,  // 19
        MDD_PartitionMetadata_MinorVersion,  // 20
        MDD_PartitionMetadata_KAGSize,  // 21
        MDD_PartitionMetadata_ThisPartition,  // 22
        MDD_PartitionMetadata_PreviousPartition,  // 23
        MDD_PartitionMetadata_FooterPartition,  // 24
        MDD_PartitionMetadata_HeaderByteCount,  // 25
        MDD_PartitionMetadata_IndexByteCount,  // 26
        MDD_PartitionMetadata_IndexSID,  // 27
        MDD_PartitionMetadata_BodyOffset,  // 28
        MDD_PartitionMetadata_BodySID,  // 29
        MDD_PartitionMetadata_OperationalPattern,  // 30
        MDD_PartitionMetadata_EssenceContainers,  // 31
        MDD_OpenHeader,  // 32
        MDD_OpenCompleteHeader,  // 33
        MDD_ClosedHeader,  // 34
        MDD_ClosedCompleteHeader,  // 35
        MDD_OpenBodyPartition,  // 36
        MDD_OpenCompleteBodyPartition,  // 37
        MDD_ClosedBodyPartition,  // 38
        MDD_ClosedCompleteBodyPartition,  // 39
        MDD_Footer,  // 40
        MDD_CompleteFooter,  // 41
        MDD_Primer,  // 42
        MDD_Primer_LocalTagEntryBatch,  // 43
        MDD_LocalTagEntryBatch_Primer_LocalTag,  // 44
        MDD_LocalTagEntryBatch_Primer_UID,  // 45
        MDD_InterchangeObject_InstanceUID,  // 46
        MDD_GenerationInterchangeObject_GenerationUID,  // 47
        MDD_DefaultObject,  // 48
        MDD_IndexTableSegmentBase_IndexEditRate,  // 49
        MDD_IndexTableSegmentBase_IndexStartPosition,  // 50
        MDD_IndexTableSegmentBase_IndexDuration,  // 51
        MDD_IndexTableSegmentBase_EditUnitByteCount,  // 52
        MDD_IndexTableSegmentBase_IndexSID,  // 53
        MDD_IndexTableSegmentBase_BodySID,  // 54
        MDD_IndexTableSegmentBase_SliceCount,  // 55
        MDD_IndexTableSegmentBase_PosTableCount,  // 56
        MDD_IndexTableSegment,  // 57
        MDD_IndexTableSegment_DeltaEntryArray,  // 58
        MDD_DeltaEntryArray_IndexTableSegment_PosTableIndex,  // 59
        MDD_DeltaEntryArray_IndexTableSegment_Slice,  // 60
        MDD_DeltaEntryArray_IndexTableSegment_ElementDelta,  // 61
        MDD_IndexTableSegment_IndexEntryArray,  // 62
        MDD_IndexEntryArray_IndexTableSegment_TemporalOffset,  // 63
        MDD_IndexEntryArray_IndexTableSegment_AnchorOffset,  // 64
        MDD_IndexEntryArray_IndexTableSegment_Flags,  // 65
        MDD_IndexEntryArray_IndexTableSegment_StreamOffset,  // 66
        MDD_IndexEntryArray_IndexTableSegment_SliceOffsetArray,  // 67
        MDD_IndexEntryArray_IndexTableSegment_PosTableArray,  // 68
        MDD_RandomIndexMetadata,  // 69
        MDD_PartitionArray_RandomIndexMetadata_BodySID,  // 70
        MDD_PartitionArray_RandomIndexMetadata_ByteOffset,  // 71
        MDD_RandomIndexMetadata_Length,  // 72
        MDD_RandomIndexMetadataV10,  // 73
        MDD_Preface,  // 74
        MDD_Preface_LastModifiedDate,  // 75
        MDD_Preface_Version,  // 76
        MDD_Preface_ObjectModelVersion,  // 77
        MDD_Preface_PrimaryPackage,  // 78
        MDD_Preface_Identifications,  // 79
        MDD_Preface_ContentStorage,  // 80
        MDD_Preface_OperationalPattern,  // 81
        MDD_Preface_EssenceContainers,  // 82
        MDD_Preface_DMSchemes,  // 83
        MDD_Identification,  // 84
        MDD_Identification_ThisGenerationUID,  // 85
        MDD_Identification_CompanyName,  // 86
        MDD_Identification_ProductName,  // 87
        MDD_Identification_ProductVersion,  // 88
        MDD_Identification_VersionString,  // 89
        MDD_Identification_ProductUID,  // 90
        MDD_Identification_ModificationDate,  // 91
        MDD_Identification_ToolkitVersion,  // 92
        MDD_Identification_Platform,  // 93
        MDD_ContentStorage,  // 94
        MDD_ContentStorage_Packages,  // 95
        MDD_ContentStorage_EssenceContainerData,  // 96
        MDD_ContentStorageKludge_V10Packages,  // 97
        MDD_EssenceContainerData,  // 98
        MDD_EssenceContainerData_LinkedPackageUID,  // 99
        MDD_EssenceContainerData_IndexSID,  // 100
        MDD_EssenceContainerData_BodySID,  // 101
        MDD_GenericPackage_PackageUID,  // 102
        MDD_GenericPackage_Name,  // 103
        MDD_GenericPackage_PackageCreationDate,  // 104
        MDD_GenericPackage_PackageModifiedDate,  // 105
        MDD_GenericPackage_Tracks,  // 106
        MDD_NetworkLocator,  // 107
        MDD_NetworkLocator_URLString,  // 108
        MDD_TextLocator,  // 109
        MDD_TextLocator_LocatorName,  // 110
        MDD_GenericTrack_TrackID,  // 111
        MDD_GenericTrack_TrackNumber,  // 112
        MDD_GenericTrack_TrackName,  // 113
        MDD_GenericTrack_Sequence,  // 114
        MDD_StaticTrack,  // 115
        MDD_Track,  // 116
        MDD_Track_EditRate,  // 117
        MDD_Track_Origin,  // 118
        MDD_EventTrack,  // 119
        MDD_EventTrack_EventEditRate,  // 120
        MDD_EventTrack_EventOrigin,  // 121
        MDD_StructuralComponent_DataDefinition,  // 122
        MDD_StructuralComponent_Duration,  // 123
        MDD_Sequence,  // 124
        MDD_Sequence_StructuralComponents,  // 125
        MDD_TimecodeComponent,  // 126
        MDD_TimecodeComponent_RoundedTimecodeBase,  // 127
        MDD_TimecodeComponent_StartTimecode,  // 128
        MDD_TimecodeComponent_DropFrame,  // 129
        MDD_SourceClip,  // 130
        MDD_SourceClip_StartPosition,  // 131
        MDD_SourceClip_SourcePackageID,  // 132
        MDD_SourceClip_SourceTrackID,  // 133
        MDD_DMSegment,  // 134
        MDD_DMSegment_EventStartPosition,  // 135
        MDD_DMSegment_EventComment,  // 136
        MDD_DMSegment_TrackIDs,  // 137
        MDD_DMSegment_DMFramework,  // 138
        MDD_DMSourceClip,  // 139
        MDD_DMSourceClip_DMSourceClipTrackIDs,  // 140
        MDD_MaterialPackage,  // 141
        MDD_SourcePackage,  // 142
        MDD_SourcePackage_Descriptor,  // 143
        MDD_GenericDescriptor_Locators,  // 144
        MDD_GenericDescriptor_SubDescriptors,  // 145
        MDD_FileDescriptor,  // 146
        MDD_FileDescriptor_LinkedTrackID,  // 147
        MDD_FileDescriptor_SampleRate,  // 148
        MDD_FileDescriptor_ContainerDuration,  // 149
        MDD_FileDescriptor_EssenceContainer,  // 150
        MDD_FileDescriptor_Codec,  // 151
        MDD_GenericPictureEssenceDescriptor,  // 152
        MDD_GenericPictureEssenceDescriptor_SignalStandard,  // 153
        MDD_GenericPictureEssenceDescriptor_FrameLayout,  // 154
        MDD_GenericPictureEssenceDescriptor_StoredWidth,  // 155
        MDD_GenericPictureEssenceDescriptor_StoredHeight,  // 156
        MDD_GenericPictureEssenceDescriptor_StoredF2Offset,  // 157
        MDD_GenericPictureEssenceDescriptor_SampledWidth,  // 158
        MDD_GenericPictureEssenceDescriptor_SampledHeight,  // 159
        MDD_GenericPictureEssenceDescriptor_SampledXOffset,  // 160
        MDD_GenericPictureEssenceDescriptor_SampledYOffset,  // 161
        MDD_GenericPictureEssenceDescriptor_DisplayHeight,  // 162
        MDD_GenericPictureEssenceDescriptor_DisplayWidth,  // 163
        MDD_GenericPictureEssenceDescriptor_DisplayXOffset,  // 164
        MDD_GenericPictureEssenceDescriptor_DisplayYOffset,  // 165
        MDD_GenericPictureEssenceDescriptor_DisplayF2Offset,  // 166
        MDD_GenericPictureEssenceDescriptor_AspectRatio,  // 167
        MDD_GenericPictureEssenceDescriptor_ActiveFormatDescriptor,  // 168
        MDD_GenericPictureEssenceDescriptor_VideoLineMap,  // 169
        MDD_GenericPictureEssenceDescriptor_AlphaTransparency,  // 170
        MDD_GenericPictureEssenceDescriptor_Gamma,  // 171
        MDD_GenericPictureEssenceDescriptor_ImageAlignmentOffset,  // 172
        MDD_GenericPictureEssenceDescriptor_ImageStartOffset,  // 173
        MDD_GenericPictureEssenceDescriptor_ImageEndOffset,  // 174
        MDD_GenericPictureEssenceDescriptor_FieldDominance,  // 175
        MDD_GenericPictureEssenceDescriptor_PictureEssenceCoding,  // 176
        MDD_CDCIEssenceDescriptor,  // 177
        MDD_CDCIEssenceDescriptor_ComponentDepth,  // 178
        MDD_CDCIEssenceDescriptor_HorizontalSubsampling,  // 179
        MDD_CDCIEssenceDescriptor_VerticalSubsampling,  // 180
        MDD_CDCIEssenceDescriptor_ColorSiting,  // 181
        MDD_CDCIEssenceDescriptor_ReversedByteOrder,  // 182
        MDD_CDCIEssenceDescriptor_PaddingBits,  // 183
        MDD_CDCIEssenceDescriptor_AlphaSampleDepth,  // 184
        MDD_CDCIEssenceDescriptor_BlackRefLevel,  // 185
        MDD_CDCIEssenceDescriptor_WhiteReflevel,  // 186
        MDD_CDCIEssenceDescriptor_ColorRange,  // 187
        MDD_RGBAEssenceDescriptor,  // 188
        MDD_RGBAEssenceDescriptor_ComponentMaxRef,  // 189
        MDD_RGBAEssenceDescriptor_ComponentMinRef,  // 190
        MDD_RGBAEssenceDescriptor_AlphaMaxRef,  // 191
        MDD_RGBAEssenceDescriptor_AlphaMinRef,  // 192
        MDD_RGBAEssenceDescriptor_ScanningDirection,  // 193
        MDD_RGBAEssenceDescriptor_PixelLayout,  // 194
        MDD_RGBAEssenceDescriptor_Palette,  // 195
        MDD_RGBAEssenceDescriptor_PaletteLayout,  // 196
        MDD_GenericSoundEssenceDescriptor,  // 197
        MDD_GenericSoundEssenceDescriptor_AudioSamplingRate,  // 198
        MDD_GenericSoundEssenceDescriptor_Locked,  // 199
        MDD_GenericSoundEssenceDescriptor_AudioRefLevel,  // 200
        MDD_GenericSoundEssenceDescriptor_ElectroSpatialFormulation,  // 201
        MDD_GenericSoundEssenceDescriptor_ChannelCount,  // 202
        MDD_GenericSoundEssenceDescriptor_QuantizationBits,  // 203
        MDD_GenericSoundEssenceDescriptor_DialNorm,  // 204
        MDD_GenericSoundEssenceDescriptor_SoundEssenceCompression,  // 205
        MDD_GenericDataEssenceDescriptor,  // 206
        MDD_GenericDataEssenceDescriptor_DataEssenceCoding,  // 207
        MDD_MultipleDescriptor,  // 208
        MDD_MultipleDescriptor_SubDescriptorUIDs,  // 209
        MDD_MPEG2VideoDescriptor,  // 210
        MDD_MPEG2VideoDescriptor_SingleSequence,  // 211
        MDD_MPEG2VideoDescriptor_ConstantBFrames,  // 212
        MDD_MPEG2VideoDescriptor_CodedContentType,  // 213
        MDD_MPEG2VideoDescriptor_LowDelay,  // 214
        MDD_MPEG2VideoDescriptor_ClosedGOP,  // 215
        MDD_MPEG2VideoDescriptor_IdenticalGOP,  // 216
        MDD_MPEG2VideoDescriptor_MaxGOP,  // 217
        MDD_MPEG2VideoDescriptor_BPictureCount,  // 218
        MDD_MPEG2VideoDescriptor_BitRate,  // 219
        MDD_MPEG2VideoDescriptor_ProfileAndLevel,  // 220
        MDD_WaveAudioDescriptor,  // 221
        MDD_WaveAudioDescriptor_BlockAlign,  // 222
        MDD_WaveAudioDescriptor_SequenceOffset,  // 223
        MDD_WaveAudioDescriptor_AvgBps,  // 224
        MDD_WaveAudioDescriptor_PeakEnvelope,  // 225
        MDD_JPEG2000PictureSubDescriptor,  // 226
        MDD_JPEG2000PictureSubDescriptor_Rsize,  // 227
        MDD_JPEG2000PictureSubDescriptor_Xsize,  // 228
        MDD_JPEG2000PictureSubDescriptor_Ysize,  // 229
        MDD_JPEG2000PictureSubDescriptor_XOsize,  // 230
        MDD_JPEG2000PictureSubDescriptor_YOsize,  // 231
        MDD_JPEG2000PictureSubDescriptor_XTsize,  // 232
        MDD_JPEG2000PictureSubDescriptor_YTsize,  // 233
        MDD_JPEG2000PictureSubDescriptor_XTOsize,  // 234
        MDD_JPEG2000PictureSubDescriptor_YTOsize,  // 235
        MDD_JPEG2000PictureSubDescriptor_Csize,  // 236
        MDD_JPEG2000PictureSubDescriptor_PictureComponentSizing,  // 237
        MDD_JPEG2000PictureSubDescriptor_CodingStyleDefault,  // 238
        MDD_JPEG2000PictureSubDescriptor_QuantizationDefault,  // 239
        MDD_DM_Framework,  // 240
        MDD_DM_Set,  // 241
        MDD_EncryptedContainerLabel,  // 242
        MDD_CryptographicFrameworkLabel,  // 243
        MDD_CryptographicFramework,  // 244
        MDD_CryptographicFramework_ContextSR,  // 245
        MDD_CryptographicContext,  // 246
        MDD_CryptographicContext_ContextID,  // 247
        MDD_CryptographicContext_SourceEssenceContainer,  // 248
        MDD_CryptographicContext_CipherAlgorithm,  // 249
        MDD_CryptographicContext_MICAlgorithm,  // 250
        MDD_CryptographicContext_CryptographicKeyID,  // 251
        MDD_EncryptedTriplet,  // 252
        MDD_EncryptedTriplet_ContextIDLink,  // 253
        MDD_EncryptedTriplet_PlaintextOffset,  // 254
        MDD_EncryptedTriplet_SourceKey,  // 255
        MDD_EncryptedTriplet_SourceLength,  // 256
        MDD_EncryptedTriplet_EncryptedSourceValue,  // 257
        MDD_EncryptedTriplet_TrackFileID,  // 258
        MDD_EncryptedTriplet_SequenceNumber,  // 259
        MDD_EncryptedTriplet_MIC,  // 260
        MDD_CipherAlgorithmAES128CBC,  // 261
        MDD_HMACAlgorithmSHA1128,  // 262
    }; // enum MDD_t
} // namespaceASDCP


#endif // _MDD_H_

//
// end MDD.h
//
