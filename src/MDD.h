/*
Copyright (c) 2006-2009, John Hurst
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
        MDD_PartitionMetadata,  // 1
        MDD_StructuralComponent,  // 2
        MDD_AES3PCMDescriptor_Emphasis,  // 3
        MDD_AES3PCMDescriptor_BlockStartOffset,  // 4
        MDD_AES3PCMDescriptor_AuxBitsMode,  // 5
        MDD_AES3PCMDescriptor_ChannelStatusMode,  // 6
        MDD_AES3PCMDescriptor_FixedChannelStatusData,  // 7
        MDD_AES3PCMDescriptor_UserDataMode,  // 8
        MDD_AES3PCMDescriptor_FixedUserData,  // 9
        MDD_XMLDocumentTextIndirect,  // 10
        MDD_XMLDocumentText_RFC2152,  // 11
        MDD_MXFInterop_OPAtom,  // 12
        MDD_OPAtom,  // 13
        MDD_OP1a,  // 14
        MDD_GCMulti,  // 15
        MDD_PictureDataDef,  // 16
        MDD_SoundDataDef,  // 17
        MDD_TimecodeDataDef,  // 18
        MDD_DescriptiveMetaDataDef,  // 19
        MDD_WAVWrapping,  // 20
        MDD_MPEG2_VESWrapping,  // 21
        MDD_JPEG_2000Wrapping,  // 22
        MDD_JPEG2000Essence,  // 23
        MDD_MPEG2Essence,  // 24
        MDD_MXFInterop_CryptEssence,  // 25
        MDD_CryptEssence,  // 26
        MDD_WAVEssence,  // 27
        MDD_JP2KEssenceCompression_2K,  // 28
        MDD_JP2KEssenceCompression_4K,  // 29
        MDD_CipherAlgorithm_AES,  // 30
        MDD_MICAlgorithm_HMAC_SHA1,  // 31
        MDD_KLVFill,  // 32
        MDD_PartitionMetadata_MajorVersion,  // 33
        MDD_PartitionMetadata_MinorVersion,  // 34
        MDD_PartitionMetadata_KAGSize,  // 35
        MDD_PartitionMetadata_ThisPartition,  // 36
        MDD_PartitionMetadata_PreviousPartition,  // 37
        MDD_PartitionMetadata_FooterPartition,  // 38
        MDD_PartitionMetadata_HeaderByteCount,  // 39
        MDD_PartitionMetadata_IndexByteCount,  // 40
        MDD_PartitionMetadata_IndexSID,  // 41
        MDD_PartitionMetadata_BodyOffset,  // 42
        MDD_PartitionMetadata_BodySID,  // 43
        MDD_PartitionMetadata_OperationalPattern,  // 44
        MDD_PartitionMetadata_EssenceContainers,  // 45
        MDD_OpenHeader,  // 46
        MDD_OpenCompleteHeader,  // 47
        MDD_ClosedHeader,  // 48
        MDD_ClosedCompleteHeader,  // 49
        MDD_OpenBodyPartition,  // 50
        MDD_OpenCompleteBodyPartition,  // 51
        MDD_ClosedBodyPartition,  // 52
        MDD_ClosedCompleteBodyPartition,  // 53
        MDD_Footer,  // 54
        MDD_CompleteFooter,  // 55
        MDD_Primer,  // 56
        MDD_Primer_LocalTagEntryBatch,  // 57
        MDD_LocalTagEntryBatch_Primer_LocalTag,  // 58
        MDD_LocalTagEntryBatch_Primer_UID,  // 59
        MDD_InterchangeObject_InstanceUID,  // 60
        MDD_GenerationInterchangeObject_GenerationUID,  // 61
        MDD_DefaultObject,  // 62
        MDD_IndexTableSegmentBase_IndexEditRate,  // 63
        MDD_IndexTableSegmentBase_IndexStartPosition,  // 64
        MDD_IndexTableSegmentBase_IndexDuration,  // 65
        MDD_IndexTableSegmentBase_EditUnitByteCount,  // 66
        MDD_IndexTableSegmentBase_IndexSID,  // 67
        MDD_IndexTableSegmentBase_BodySID,  // 68
        MDD_IndexTableSegmentBase_SliceCount,  // 69
        MDD_IndexTableSegmentBase_PosTableCount,  // 70
        MDD_V10IndexTableSegment,  // 71
        MDD_V10IndexTableSegment_V10DeltaEntryArray,  // 72
        MDD_V10DeltaEntryArray_V10IndexTableSegment_Reorder,  // 73
        MDD_V10DeltaEntryArray_V10IndexTableSegment_Slice,  // 74
        MDD_V10DeltaEntryArray_V10IndexTableSegment_ElementDelta,  // 75
        MDD_V10IndexTableSegment_V10IndexEntryArray,  // 76
        MDD_V10IndexEntryArray_V10IndexTableSegment_TemporalOffset,  // 77
        MDD_V10IndexEntryArray_V10IndexTableSegment_AnchorOffset,  // 78
        MDD_V10IndexEntryArray_V10IndexTableSegment_Flags,  // 79
        MDD_V10IndexEntryArray_V10IndexTableSegment_StreamOffset,  // 80
        MDD_V10IndexEntryArray_V10IndexTableSegment_SliceOffsetArray,  // 81
        MDD_IndexTableSegment,  // 82
        MDD_IndexTableSegment_DeltaEntryArray,  // 83
        MDD_DeltaEntryArray_IndexTableSegment_PosTableIndex,  // 84
        MDD_DeltaEntryArray_IndexTableSegment_Slice,  // 85
        MDD_DeltaEntryArray_IndexTableSegment_ElementDelta,  // 86
        MDD_IndexTableSegment_IndexEntryArray,  // 87
        MDD_IndexEntryArray_IndexTableSegment_TemporalOffset,  // 88
        MDD_IndexEntryArray_IndexTableSegment_AnchorOffset,  // 89
        MDD_IndexEntryArray_IndexTableSegment_Flags,  // 90
        MDD_IndexEntryArray_IndexTableSegment_StreamOffset,  // 91
        MDD_IndexEntryArray_IndexTableSegment_SliceOffsetArray,  // 92
        MDD_IndexEntryArray_IndexTableSegment_PosTableArray,  // 93
        MDD_RandomIndexMetadata,  // 94
        MDD_PartitionArray_RandomIndexMetadata_BodySID,  // 95
        MDD_PartitionArray_RandomIndexMetadata_ByteOffset,  // 96
        MDD_RandomIndexMetadata_Length,  // 97
        MDD_RandomIndexMetadataV10,  // 98
        MDD_Preface,  // 99
        MDD_Preface_LastModifiedDate,  // 100
        MDD_Preface_Version,  // 101
        MDD_Preface_ObjectModelVersion,  // 102
        MDD_Preface_PrimaryPackage,  // 103
        MDD_Preface_Identifications,  // 104
        MDD_Preface_ContentStorage,  // 105
        MDD_Preface_OperationalPattern,  // 106
        MDD_Preface_EssenceContainers,  // 107
        MDD_Preface_DMSchemes,  // 108
        MDD_Identification,  // 109
        MDD_Identification_ThisGenerationUID,  // 110
        MDD_Identification_CompanyName,  // 111
        MDD_Identification_ProductName,  // 112
        MDD_Identification_ProductVersion,  // 113
        MDD_Identification_VersionString,  // 114
        MDD_Identification_ProductUID,  // 115
        MDD_Identification_ModificationDate,  // 116
        MDD_Identification_ToolkitVersion,  // 117
        MDD_Identification_Platform,  // 118
        MDD_ContentStorage,  // 119
        MDD_ContentStorage_Packages,  // 120
        MDD_ContentStorage_EssenceContainerData,  // 121
        MDD_ContentStorageKludge_V10Packages,  // 122
        MDD_EssenceContainerData,  // 123
        MDD_EssenceContainerData_LinkedPackageUID,  // 124
        MDD_EssenceContainerData_IndexSID,  // 125
        MDD_EssenceContainerData_BodySID,  // 126
        MDD_GenericPackage,  // 127
        MDD_GenericPackage_PackageUID,  // 128
        MDD_GenericPackage_Name,  // 129
        MDD_GenericPackage_PackageCreationDate,  // 130
        MDD_GenericPackage_PackageModifiedDate,  // 131
        MDD_GenericPackage_Tracks,  // 132
        MDD_NetworkLocator,  // 133
        MDD_NetworkLocator_URLString,  // 134
        MDD_TextLocator,  // 135
        MDD_TextLocator_LocatorName,  // 136
        MDD_GenericTrack,  // 137
        MDD_GenericTrack_TrackID,  // 138
        MDD_GenericTrack_TrackNumber,  // 139
        MDD_GenericTrack_TrackName,  // 140
        MDD_GenericTrack_Sequence,  // 141
        MDD_StaticTrack,  // 142
        MDD_Track,  // 143
        MDD_Track_EditRate,  // 144
        MDD_Track_Origin,  // 145
        MDD_EventTrack,  // 146
        MDD_EventTrack_EventEditRate,  // 147
        MDD_EventTrack_EventOrigin,  // 148
        MDD_StructuralComponent_DataDefinition,  // 149
        MDD_StructuralComponent_Duration,  // 150
        MDD_Sequence,  // 151
        MDD_Sequence_StructuralComponents,  // 152
        MDD_TimecodeComponent,  // 153
        MDD_TimecodeComponent_RoundedTimecodeBase,  // 154
        MDD_TimecodeComponent_StartTimecode,  // 155
        MDD_TimecodeComponent_DropFrame,  // 156
        MDD_SourceClip,  // 157
        MDD_SourceClip_StartPosition,  // 158
        MDD_SourceClip_SourcePackageID,  // 159
        MDD_SourceClip_SourceTrackID,  // 160
        MDD_DMSegment,  // 161
        MDD_DMSegment_EventStartPosition,  // 162
        MDD_DMSegment_EventComment,  // 163
        MDD_DMSegment_TrackIDs,  // 164
        MDD_DMSegment_DMFramework,  // 165
        MDD_DMSourceClip,  // 166
        MDD_DMSourceClip_DMSourceClipTrackIDs,  // 167
        MDD_MaterialPackage,  // 168
        MDD_SourcePackage,  // 169
        MDD_SourcePackage_Descriptor,  // 170
        MDD_GenericDescriptor_Locators,  // 171
        MDD_GenericDescriptor_SubDescriptor,  // 172
        MDD_GenericDescriptor_SubDescriptors,  // 173
        MDD_FileDescriptor,  // 174
        MDD_FileDescriptor_LinkedTrackID,  // 175
        MDD_FileDescriptor_SampleRate,  // 176
        MDD_FileDescriptor_ContainerDuration,  // 177
        MDD_FileDescriptor_EssenceContainer,  // 178
        MDD_FileDescriptor_Codec,  // 179
        MDD_GenericPictureEssenceDescriptor,  // 180
        MDD_GenericPictureEssenceDescriptor_SignalStandard,  // 181
        MDD_GenericPictureEssenceDescriptor_FrameLayout,  // 182
        MDD_GenericPictureEssenceDescriptor_StoredWidth,  // 183
        MDD_GenericPictureEssenceDescriptor_StoredHeight,  // 184
        MDD_GenericPictureEssenceDescriptor_StoredF2Offset,  // 185
        MDD_GenericPictureEssenceDescriptor_SampledWidth,  // 186
        MDD_GenericPictureEssenceDescriptor_SampledHeight,  // 187
        MDD_GenericPictureEssenceDescriptor_SampledXOffset,  // 188
        MDD_GenericPictureEssenceDescriptor_SampledYOffset,  // 189
        MDD_GenericPictureEssenceDescriptor_DisplayHeight,  // 190
        MDD_GenericPictureEssenceDescriptor_DisplayWidth,  // 191
        MDD_GenericPictureEssenceDescriptor_DisplayXOffset,  // 192
        MDD_GenericPictureEssenceDescriptor_DisplayYOffset,  // 193
        MDD_GenericPictureEssenceDescriptor_DisplayF2Offset,  // 194
        MDD_GenericPictureEssenceDescriptor_AspectRatio,  // 195
        MDD_GenericPictureEssenceDescriptor_ActiveFormatDescriptor,  // 196
        MDD_GenericPictureEssenceDescriptor_VideoLineMap,  // 197
        MDD_GenericPictureEssenceDescriptor_AlphaTransparency,  // 198
        MDD_GenericPictureEssenceDescriptor_Gamma,  // 199
        MDD_GenericPictureEssenceDescriptor_ImageAlignmentOffset,  // 200
        MDD_GenericPictureEssenceDescriptor_ImageStartOffset,  // 201
        MDD_GenericPictureEssenceDescriptor_ImageEndOffset,  // 202
        MDD_GenericPictureEssenceDescriptor_FieldDominance,  // 203
        MDD_GenericPictureEssenceDescriptor_PictureEssenceCoding,  // 204
        MDD_CDCIEssenceDescriptor,  // 205
        MDD_CDCIEssenceDescriptor_ComponentDepth,  // 206
        MDD_CDCIEssenceDescriptor_HorizontalSubsampling,  // 207
        MDD_CDCIEssenceDescriptor_VerticalSubsampling,  // 208
        MDD_CDCIEssenceDescriptor_ColorSiting,  // 209
        MDD_CDCIEssenceDescriptor_ReversedByteOrder,  // 210
        MDD_CDCIEssenceDescriptor_PaddingBits,  // 211
        MDD_CDCIEssenceDescriptor_AlphaSampleDepth,  // 212
        MDD_CDCIEssenceDescriptor_BlackRefLevel,  // 213
        MDD_CDCIEssenceDescriptor_WhiteReflevel,  // 214
        MDD_CDCIEssenceDescriptor_ColorRange,  // 215
        MDD_RGBAEssenceDescriptor,  // 216
        MDD_RGBAEssenceDescriptor_ComponentMaxRef,  // 217
        MDD_RGBAEssenceDescriptor_ComponentMinRef,  // 218
        MDD_RGBAEssenceDescriptor_AlphaMaxRef,  // 219
        MDD_RGBAEssenceDescriptor_AlphaMinRef,  // 220
        MDD_RGBAEssenceDescriptor_ScanningDirection,  // 221
        MDD_RGBAEssenceDescriptor_PixelLayout,  // 222
        MDD_RGBAEssenceDescriptor_Palette,  // 223
        MDD_RGBAEssenceDescriptor_PaletteLayout,  // 224
        MDD_GenericSoundEssenceDescriptor,  // 225
        MDD_GenericSoundEssenceDescriptor_AudioSamplingRate,  // 226
        MDD_GenericSoundEssenceDescriptor_Locked,  // 227
        MDD_GenericSoundEssenceDescriptor_AudioRefLevel,  // 228
        MDD_GenericSoundEssenceDescriptor_ElectroSpatialFormulation,  // 229
        MDD_GenericSoundEssenceDescriptor_ChannelCount,  // 230
        MDD_GenericSoundEssenceDescriptor_QuantizationBits,  // 231
        MDD_GenericSoundEssenceDescriptor_DialNorm,  // 232
        MDD_GenericSoundEssenceDescriptor_SoundEssenceCompression,  // 233
        MDD_GenericDataEssenceDescriptor,  // 234
        MDD_GenericDataEssenceDescriptor_DataEssenceCoding,  // 235
        MDD_VBIDataDescriptor,  // 236
        MDD_MultipleDescriptor,  // 237
        MDD_MultipleDescriptor_SubDescriptorUIDs,  // 238
        MDD_MPEG2VideoDescriptor,  // 239
        MDD_MPEG2VideoDescriptor_SingleSequence,  // 240
        MDD_MPEG2VideoDescriptor_ConstantBFrames,  // 241
        MDD_MPEG2VideoDescriptor_CodedContentType,  // 242
        MDD_MPEG2VideoDescriptor_LowDelay,  // 243
        MDD_MPEG2VideoDescriptor_ClosedGOP,  // 244
        MDD_MPEG2VideoDescriptor_IdenticalGOP,  // 245
        MDD_MPEG2VideoDescriptor_MaxGOP,  // 246
        MDD_MPEG2VideoDescriptor_BPictureCount,  // 247
        MDD_MPEG2VideoDescriptor_BitRate,  // 248
        MDD_MPEG2VideoDescriptor_ProfileAndLevel,  // 249
        MDD_WaveAudioDescriptor,  // 250
        MDD_WaveAudioDescriptor_BlockAlign,  // 251
        MDD_WaveAudioDescriptor_SequenceOffset,  // 252
        MDD_WaveAudioDescriptor_AvgBps,  // 253
        MDD_WaveAudioDescriptor_ChannelAssignment,  // 254
        MDD_WaveAudioDescriptor_PeakEnvelopeVersion,  // 255
        MDD_WaveAudioDescriptor_PeakEnvelopeFormat,  // 256
        MDD_WaveAudioDescriptor_PointsPerPeakValue,  // 257
        MDD_WaveAudioDescriptor_PeakEnvelopeBlockSize,  // 258
        MDD_WaveAudioDescriptor_PeakChannels,  // 259
        MDD_WaveAudioDescriptor_PeakFrames,  // 260
        MDD_WaveAudioDescriptor_PeakOfPeaksPosition,  // 261
        MDD_WaveAudioDescriptor_PeakEnvelopeTimestamp,  // 262
        MDD_WaveAudioDescriptor_PeakEnvelopeData,  // 263
        MDD_AES3PCMDescriptor,  // 264
        MDD_DM_Framework,  // 265
        MDD_DM_Set,  // 266
        MDD_JPEG2000PictureSubDescriptor,  // 267
        MDD_JPEG2000PictureSubDescriptor_Rsiz,  // 268
        MDD_JPEG2000PictureSubDescriptor_Xsiz,  // 269
        MDD_JPEG2000PictureSubDescriptor_Ysiz,  // 270
        MDD_JPEG2000PictureSubDescriptor_XOsiz,  // 271
        MDD_JPEG2000PictureSubDescriptor_YOsiz,  // 272
        MDD_JPEG2000PictureSubDescriptor_XTsiz,  // 273
        MDD_JPEG2000PictureSubDescriptor_YTsiz,  // 274
        MDD_JPEG2000PictureSubDescriptor_XTOsiz,  // 275
        MDD_JPEG2000PictureSubDescriptor_YTOsiz,  // 276
        MDD_JPEG2000PictureSubDescriptor_Csiz,  // 277
        MDD_JPEG2000PictureSubDescriptor_PictureComponentSizing,  // 278
        MDD_JPEG2000PictureSubDescriptor_CodingStyleDefault,  // 279
        MDD_JPEG2000PictureSubDescriptor_QuantizationDefault,  // 280
        MDD_StereoscopicEssenceSubDescriptor,  // 281
        MDD_EncryptedContainerLabel,  // 282
        MDD_CryptographicFrameworkLabel,  // 283
        MDD_CryptographicFramework,  // 284
        MDD_CryptographicFramework_ContextSR,  // 285
        MDD_CryptographicContext,  // 286
        MDD_CryptographicContext_ContextID,  // 287
        MDD_CryptographicContext_SourceEssenceContainer,  // 288
        MDD_CryptographicContext_CipherAlgorithm,  // 289
        MDD_CryptographicContext_MICAlgorithm,  // 290
        MDD_CryptographicContext_CryptographicKeyID,  // 291
        MDD_EncryptedTriplet,  // 292
        MDD_EncryptedTriplet_ContextIDLink,  // 293
        MDD_EncryptedTriplet_PlaintextOffset,  // 294
        MDD_EncryptedTriplet_SourceKey,  // 295
        MDD_EncryptedTriplet_SourceLength,  // 296
        MDD_EncryptedTriplet_EncryptedSourceValue,  // 297
        MDD_EncryptedTriplet_TrackFileID,  // 298
        MDD_EncryptedTriplet_SequenceNumber,  // 299
        MDD_EncryptedTriplet_MIC,  // 300
        MDD_CipherAlgorithmAES128CBC,  // 301
        MDD_HMACAlgorithmSHA1128,  // 302
        MDD_EssenceContainerData_ContentStorage_EssenceContainer,  // 303
        MDD_GenerationInterchangeObject,  // 304
        MDD_RandomIndexMetadata_PartitionArray,  // 305
    }; // enum MDD_t
} // namespaceASDCP


#endif // _MDD_H_

//
// end MDD.h
//
