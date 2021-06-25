/*
Copyright (c) 2005-2014, John Hurst,
Copyright (c) 2020, Thomas Richter Fraunhofer IIS,
Copyright (c) 2020, Christian Minuth Fraunhofer IIS,
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
/* !\file    JXS.h
\version $Id$
\brief   JPEG XS constants and data structures

This is not a complete enumeration of all things JXS.There is just enough here to
support parsing picture metadata from a codestream header.
*/

#ifndef _JXS_H_
#define _JXS_H_

// AS_DCP.h is included only for it's base type definitions.
#include <KM_platform.h>
#include <KM_util.h>
#include <Metadata.h>
#include <assert.h>

namespace ASDCP
{
	namespace JXS
	{
		const ui32_t MaxComponents = 4; // ISO 21122-1 Annex A.2 up to 8 components
		const ui32_t MaxHorizontalLevels = 15;
		const ui32_t MaxVerticalLevels = 2;

#pragma pack(1)
		struct ImageComponent_t  // Essentially, a lookalike of the CDT marker, just with less bit-packing
		{
		  ui8_t Bc;  // Bitdepth (literal, not -1)
		  ui8_t Sx;
		  ui8_t Sy;  // Subsampling factors, horizontal and vertically. Bit-packed in the marker.
		};
#pragma pack()

                const byte_t Magic[] = { 0xff, 0x10, 0xff };

		bool lookup_ColorPrimaries(int value, ASDCP::UL& ul);
		bool lookup_TransferCharacteristic(int value, ASDCP::UL& ul);
		bool lookup_CodingEquations(int value, ASDCP::UL& ul);

		enum Marker_t
		{
			MRK_NIL = 0,
			MRK_SOC = 0xff10, // Start of codestream
			MRK_EOC = 0xff11, // End of codestream
			MRK_PIH = 0xff12, // Picture header
			MRK_CDT = 0xff13, // Component table
			MRK_WGT = 0xff14, // Weights table
			MRK_COM = 0xff15, // Extensions marker
			MRK_NLT = 0xff16, // Nonlinearity marker (21122-1 2nd)
			MRK_CWD = 0xff17, // Component-dependent decomposition marker (21122-1 2nd)
			MRK_CTS = 0xff18, // Colour transformation spec marker (21122-1 2nd)
			MRK_CRG = 0xff19, // Component registration marker (21122-1 2nd)
			MRK_SLH = 0xff20, // Slice header
			MRK_CAP = 0xff50  // Capabilities marker
		};

		const char* GetMarkerString(Marker_t m);

		//
		class Marker
		{
			KM_NO_COPY_CONSTRUCT(Marker);

		public:
			Marker_t m_Type;
			bool     m_IsSegment;
			ui32_t   m_DataSize;
			const byte_t* m_Data;

			Marker() : m_Type(MRK_NIL), m_IsSegment(false), m_DataSize(0), m_Data(0) {}
			~Marker() {}

			void Dump(FILE* stream = 0) const;
		};

		//
		ASDCP::Result_t GetNextMarker(const byte_t**, Marker&);

		// accessor objects for marker segments
		namespace Accessor
		{
			// capability marker
			class CAP
			{
				const byte_t* m_MarkerData;
				ui32_t m_DataSize;
				KM_NO_COPY_CONSTRUCT(CAP);
				CAP();

			public:
				CAP(const Marker& M)
				{
					assert(M.m_Type == MRK_CAP);
					m_MarkerData = M.m_Data;
					m_DataSize = M.m_DataSize -2;
				}

				~CAP() {}

				inline ui16_t Size()   const { return KM_i16_BE(*(ui16_t*)(m_MarkerData -2 )); }

				void Dump(FILE* stream = 0) const;
			};

			// Nonlinearity marker(21122 - 1 2nd)
			class NLT
			{
				const byte_t* m_MarkerData;
				KM_NO_COPY_CONSTRUCT(NLT);
				NLT();

			public:
				NLT(const Marker& M)
				{
					assert(M.m_Type == MRK_NLT);
					m_MarkerData = M.m_Data;
				}

				~NLT() {}

				inline ui16_t Size()   const { return KM_i16_BE(*(ui16_t*)(m_MarkerData - 2)); }

				void Dump(FILE* stream = 0) const;
			};

			// picture header
			class PIH
			{
				const byte_t* m_MarkerData;
				ui32_t        m_DataSize;
				KM_NO_COPY_CONSTRUCT(PIH);
				PIH();

			public:
				PIH(const Marker& M)
				{
					assert(M.m_Type == MRK_PIH);
					m_MarkerData = M.m_Data;
					m_DataSize = M.m_DataSize;
				}

				~PIH() {}

				inline ui16_t LpihSize()	const { return KM_i16_BE(*(ui16_t*)(m_MarkerData - 2)); }
				inline ui32_t LcodSize()	const { return KM_i32_BE(*(ui32_t*)(m_MarkerData)); }
				inline ui16_t Ppih()		const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 4)); }
				inline ui16_t Plev()		const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 6)); }
				inline ui16_t Wf()			const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 8)); }
				inline ui16_t Hf()			const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 10)); }
				inline ui16_t Cw()			const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 12)); }
				inline ui16_t Hsl()			const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 14)); }
				inline ui8_t  Nc()			const { return (*(ui8_t*)(m_MarkerData + 16)); }
				inline ui8_t  Ng()			const { return *(ui8_t*)(m_MarkerData + 17); }
				inline ui8_t  Ss()			const { return *(ui8_t*)(m_MarkerData + 18); }
				inline ui8_t  Cpih()		const { return (*(ui8_t*)(m_MarkerData + 21)) & 0x0f; }
				inline ui8_t  Nlx()			const { return (*(ui8_t*)(m_MarkerData + 22)) >> 4; }
				inline ui8_t  Nly()			const { return (*(ui8_t*)(m_MarkerData + 22)) & 0x0f; }

				void Dump(FILE* stream = 0) const;
			};

			class CDT
			{
				const byte_t* m_MarkerData;
				KM_NO_COPY_CONSTRUCT(CDT);
				CDT();

			public:
				CDT(const Marker& M)
				{
					assert(M.m_Type == MRK_CDT);
					m_MarkerData = M.m_Data;
				}

				~CDT() {}

				inline ui8_t  Bc(int i)          const { return *(m_MarkerData + (i << 1)); }
				inline ui8_t  Sx(int i)          const { return *(m_MarkerData + 1 + (i << 1)) >> 4; }
				inline ui8_t  Sy(int i)          const { return *(m_MarkerData + 1 + (i << 1)) & 0x0f; }

				void Dump(FILE* stream = 0) const;
			};

			class WGT
			{
				const byte_t* m_MarkerData;
				KM_NO_COPY_CONSTRUCT(WGT);
				WGT();

			public:
				WGT(const Marker& M)
				{
					assert(M.m_Type == MRK_WGT);
					m_MarkerData = M.m_Data;
				}

				~WGT() {}

				void Dump(FILE* stream = 0) const;
			};

			class COM
			{
				const byte_t* m_MarkerData;
				KM_NO_COPY_CONSTRUCT(COM);
				COM();

			public:
				COM(const Marker& M)
				{
					assert(M.m_Type == MRK_COM);
					m_MarkerData = M.m_Data;
				}

				~COM() {}

				void Dump(FILE* stream = 0) const;
			};
		}

	  //
	  class FrameBuffer : public ASDCP::FrameBuffer
	  {
	  public:
		  FrameBuffer() {}
		  FrameBuffer(ui32_t size) { Capacity(size); }
		  virtual ~FrameBuffer() {}

		  // Print debugging information to stream (stderr default)
		  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	  };

	  // An object which opens and reads a JPEG XS codestream file.  The file is expected
	  // to contain exactly one complete frame of picture essence as an unwrapped (raw)
	  // ISO/IEC 21122 codestream.
	  class CodestreamParser
	  {
		  class h__CodestreamParser;
		  mem_ptr<h__CodestreamParser> m_Parser;
		  ASDCP_NO_COPY_CONSTRUCT(CodestreamParser);

	  public:
		  CodestreamParser();
		  virtual ~CodestreamParser();

		  // Opens a file for reading, parses enough data to provide a complete
			  // set of stream metadata for the MXFWriter below.
		  // The frame buffer's PlaintextOffset parameter will be set to the first
		  // byte of the data segment. Set this value to zero if you want
		  // encrypted headers.
		  Result_t OpenReadFrame(const std::string& filename, FrameBuffer&) const;

		  // Fill the MXF descriptor objects with the values from the file's codestream.
		  // Returns RESULT_INIT if the file is not open.
		  Result_t FillPictureDescriptor(
                      ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
		      ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_subdescriptor) const;
	  };

	  // Parses the data in the frame buffer to fill in the picture descriptor. Copies
	  // the offset of the image data into start_of_data. Returns error if the parser fails.
	  Result_t ParseMetadataIntoDesc(const FrameBuffer&,
					 ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
					 ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_subdescriptor,
					 byte_t* start_of_data = 0);

	  // An object which reads a sequence of files containing JPEG XS pictures.
	  class SequenceParser
	  {
		  class h__SequenceParser;
		  mem_ptr<h__SequenceParser> m_Parser;
		  ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

	  public:
		  SequenceParser();
		  virtual ~SequenceParser();

		  // Opens a directory for reading.  The directory is expected to contain one or
		  // more files, each containing the codestream for exactly one picture. The
		  // files must be named such that the frames are in temporal order when sorted
		  // alphabetically by filename. The parser will automatically parse enough data
		  // from the first file to provide a complete set of stream metadata for the
		  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
		  // parser will check the metadata for each codestream and fail if a
		  // mismatch is detected.
		  Result_t OpenRead(const std::string& filename) const;

		  // Opens a file sequence for reading.  The sequence is expected to contain one or
		  // more filenames, each naming a file containing the codestream for exactly one
		  // picture. The parser will automatically parse enough data
		  // from the first file to provide a complete set of stream metadata for the
		  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
		  // parser will check the metadata for each codestream and fail if a
		  // mismatch is detected.
		  Result_t OpenRead(const std::list<std::string>& file_list) const;

		  // Fill a PictureDescriptor struct with the values from the first file's codestream.
		  // Returns RESULT_INIT if the directory is not open.
		  Result_t FillPictureDescriptor(
                      ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
		      ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_subdescriptor) const;

		  // Rewind the directory to the beginning.
		  Result_t Reset() const;

		  // Reads the next sequential frame in the directory and places it in the
		  // frame buffer. Fails if the buffer is too small or the direcdtory
		  // contains no more files.
		  // The frame buffer's PlaintextOffset parameter will be set to the first
		  // byte of the data segment. Set this value to zero if you want
		  // encrypted headers.
		  Result_t ReadFrame(FrameBuffer&) const;
	  };

	} //namespace JXS
} // namespace ASDCP

#endif // _JXS_H_

//
// end JXS.h
//
