/*
Copyright (c) 2004-2013, John Hurst,
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
\brief   JPEG XS parser implementation

This is not a complete enumeration of all things JXS.There is just enough here to
support parsing picture metadata from a codestream header.
*/

#include "JXS.h"
#include <KM_log.h>
using Kumu::DefaultLogSink;

//
ASDCP::Result_t
ASDCP::JXS::GetNextMarker(const byte_t** buf, JXS::Marker& Marker)
{
	assert((buf != 0) && (*buf != 0));

	if (*(*buf)++ != 0xff)
		return ASDCP::RESULT_FAIL;

	Marker.m_Type = (Marker_t)(0xff00 | *(*buf)++);
	Marker.m_IsSegment = Marker.m_Type != MRK_SOC && Marker.m_Type != MRK_SLH && Marker.m_Type != MRK_EOC;

	if (Marker.m_IsSegment)
	{
		Marker.m_DataSize = *(*buf)++ << 8;
		Marker.m_DataSize |= *(*buf)++;
		Marker.m_DataSize -= 2;
		Marker.m_Data = *buf;
		*buf += Marker.m_DataSize;
	}

	return ASDCP::RESULT_OK;
}

//
const char*
ASDCP::JXS::GetMarkerString(Marker_t m)
{
	switch (m)
	{
	case MRK_NIL: return "NIL"; break;
	case MRK_SOC: return "SOC: Start of codestream"; break;
	case MRK_EOC: return "SOT: End of codestream"; break;
	case MRK_PIH: return "PIH: Picture header"; break;
	case MRK_CDT: return "CDT: Component table"; break;
	case MRK_WGT: return "WGT: Weights table"; break;
	case MRK_COM: return "COM: Extension marker"; break;
	case MRK_NLT: return "NLT: Nonlinearity marker"; break;
	case MRK_CWD: return "CWD: Component dependent wavelet decomposition marker"; break;
	case MRK_CTS: return "CTS: Colour transformation specification marker"; break;
	case MRK_CRG: return "CRG: Component registration marker"; break;
	case MRK_SLH: return "SLH: Slice header"; break;
	case MRK_CAP: return "CAP: Capabilities marker"; break;
	}

	return "Unknown marker code";
}

//TODO: Dump functions for markers

//
void
ASDCP::JXS::Accessor::CAP::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "CAP: \n");
	fprintf(stream, "  Size: %hu\n", Size());
	Kumu::hexdump(m_MarkerData, m_DataSize, stream);
}

//
void
ASDCP::JXS::Accessor::NLT::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "SLZ: \n");
	fprintf(stream, "  Size: %hu\n", Size());
}

//
void
ASDCP::JXS::Accessor::PIH::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "PIH: \n");
	fprintf(stream, "  LpihSize: %hu\n", LpihSize());
	fprintf(stream, "  LcodSize: %u\n", LcodSize());
	fprintf(stream, "  Ppih: %hu\n", Ppih());
	fprintf(stream, "  Plev: %hu\n", Plev());
	fprintf(stream, "  Wf: %hu\n", Wf());
	fprintf(stream, "  Hf: %hu\n", Hf());
	fprintf(stream, "  Cw: %hu\n", Cw());
	fprintf(stream, "  Hsl: %hu\n", Hsl());
	fprintf(stream, "  Nc: %hhu\n", Nc());
	fprintf(stream, "  Ng: %hhu\n", Ng());
	fprintf(stream, "  Ss: %hhu\n", Ss());
	fprintf(stream, "  Cpih: %hhu\n", Cpih());
	fprintf(stream, "  Nlx: %hhu\n", Nlx());
	fprintf(stream, "  Nly: %hhu\n", Nly());
	Kumu::hexdump(m_MarkerData, m_DataSize, stream);
}

//
void
ASDCP::JXS::Accessor::CDT::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "CDT: \n");

	for (ui32_t i = 0; i < 3; i++) {
	  fprintf(stream, " Component %u Bc: %hhu\n", i,Bc(i));
	  fprintf(stream, " Component %u Sx: %hhu\n", i,Sx(i));
	  fprintf(stream, " Component %u Sy: %hhu\n", i,Sy(i));
	}
}

//
bool
ASDCP::JXS::lookup_ColorPrimaries(int cicp_value, ASDCP::UL& ul)
{
  const ASDCP::Dictionary& dict = DefaultSMPTEDict();
  switch ( cicp_value )
    {
    case 1:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_ITU709);
      break;
    case 5:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_ITU470_PAL);
      break;
    case 6:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_SMPTE170M);
      break;
    case 9:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_ITU2020);
      break;
    case 10:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_SMPTE_DCDM);
      break;
    case 11:
      ul = dict.ul(ASDCP::MDD_TheatricalViewingEnvironment);
      break;
    case 12:
      ul = dict.ul(ASDCP::MDD_ColorPrimaries_P3D65);
      break;

    default:
      return false;
      break;
    }

  return true;
}

//
bool
ASDCP::JXS::lookup_TransferCharacteristic(int cicp_value, ASDCP::UL& ul)
{
  const ASDCP::Dictionary& dict = DefaultSMPTEDict();
  switch ( cicp_value )
    {
    case 1:
    case 6:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_ITU709);
      break;
    case 5: // Display Gamma 2.8, BT.470-6 This does not seem to be supported
    case 9: // Log(100:1) range This does not seem to be supported
    case 10:// Log(100*Sqrt(10):1 range)
      return Kumu::RESULT_NOTIMPL;
      break;
    case 8:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_linear);
      break;
    case 11:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_IEC6196624_xvYCC);
      break;
    case 13:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_sRGB);
      break;
    case 14:
    case 15:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_ITU2020);
      break;
    case 16:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_SMPTEST2084);
      break;
    case 17:
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_ST428);
      break;
    case 18: // HLG
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_HLG);
      break;
    case 12: // Rec. BT.1361
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_BT1361);
      break;
    case 4: // Rec. BT.470
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_BT470);
      break;
    case 7: // SMPTE 240M
      ul = dict.ul(ASDCP::MDD_TransferCharacteristic_ST240M);
      break;
    case 2: // Unspecified. This leaves the data intentionally undefined.
      break;

    default:
      return false;
      break;
    }

  return true;
}

//
bool
ASDCP::JXS::lookup_CodingEquations(int value, ASDCP::UL& ul)
{
  const ASDCP::Dictionary& dict = DefaultSMPTEDict();
  switch ( value )
    {
    case 0: // Identity matrix. Use the BGR coding equations.
      ul = dict.ul(ASDCP::MDD_CodingEquations_BGR);
      break;
    case 4: // Title 47.
    case 10: // ITU 2020 constant luminance? Does not seem to be supported
    case 11: // SMPTE ST-2085
      return Kumu::RESULT_NOTIMPL;
    case 1:
      ul = dict.ul(ASDCP::MDD_CodingEquations_709);
      break;
      // Note: Matrix=2 does not set the optional parameter. This is intentional.
    case 5:
    case 6:
      ul = dict.ul(ASDCP::MDD_CodingEquations_601);
      break;
    case 9: // ITU 2020 non-constant luminance?
      ul = dict.ul(ASDCP::MDD_CodingEquations_Rec2020);
      break;
    case 2: // This is unspecified. The metadata item remains undefined on purpose.
      break;
    case 7: // ST 240M
      ul = dict.ul(ASDCP::MDD_CodingEquations_ST240M);
      break;
    case 8: // YCgCo
      ul = dict.ul(ASDCP::MDD_CodingEquations_YCGCO);
      break;

    default:
      return false;
      break;
    }

  return true;
}


//------------------------------------------------------------------------------------------


//
void
ASDCP::JXS::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %7u bytes", m_FrameNumber, m_Size);
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}

//
// end JXS.cpp
//
