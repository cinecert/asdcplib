/*
Copyright (c) 2005-2014, John Hurst
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
/*! \file    JP2K.cpp
    \version $Id$
    \brief   JPEG 2000 parser implementation

    This is not a complete implementation of all things JP2K.  There is just enough here to
    support parsing picture metadata from a codestream header.
*/

#include <JP2K.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;


//
ASDCP::Result_t
ASDCP::JP2K::GetNextMarker(const byte_t** buf, JP2K::Marker& Marker)
{
  assert((buf != 0) && (*buf != 0 ));
  
  if (*(*buf)++ != 0xff )
    return ASDCP::RESULT_FAIL;

  Marker.m_Type = (Marker_t)(0xff00 | *(*buf)++);
  Marker.m_IsSegment = Marker.m_Type != MRK_SOC && Marker.m_Type != MRK_SOD && Marker.m_Type != MRK_EOC;

  if ( Marker.m_IsSegment )
    {
      Marker.m_DataSize = *(*buf)++ << 8;
      Marker.m_DataSize |= *(*buf)++;
      Marker.m_DataSize -= 2;
      Marker.m_Data = *buf;
      *buf += Marker.m_DataSize;
    }

  /* TODO: why is this here?
  if ( Marker.m_DataSize != 0 && Marker.m_DataSize < 3 )
    {
      DefaultLogSink().Error("Illegal data size: %u\n", Marker.m_DataSize);
      return ASDCP::RESULT_FAIL;
    }
	*/
  return ASDCP::RESULT_OK;
}


//-------------------------------------------------------------------------------------------------------
//

//
void
ASDCP::JP2K::Accessor::SIZ::ReadComponent(const ui32_t index, ASDCP::JP2K::ImageComponent_t& IC) const
{
  assert ( index < Csize() );
  const byte_t* p = m_MarkerData + 36 + (index * 3);
  IC.Ssize = *p++;
  IC.XRsize = *p++;
  IC.YRsize = *p;
}

//
void
ASDCP::JP2K::Accessor::SIZ::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "SIZ: \n");
  fprintf(stream, "  Rsize: %hu\n", Rsize());
  fprintf(stream, "  Xsize: %u\n",  Xsize());
  fprintf(stream, "  Ysize: %u\n",  Ysize());
  fprintf(stream, " XOsize: %u\n",  XOsize());
  fprintf(stream, " YOsize: %u\n",  YOsize());
  fprintf(stream, " XTsize: %u\n",  XTsize());
  fprintf(stream, " YTsize: %u\n",  YTsize());
  fprintf(stream, "XTOsize: %u\n",  XTOsize());
  fprintf(stream, "YTOsize: %u\n",  YTOsize());
  fprintf(stream, "  Csize: %u\n",  Csize());

  if ( Csize() > 0 )
    {
      fprintf(stream, "Components\n");

      for ( ui32_t i = 0; i < Csize(); i++ )
	{
	  ImageComponent_t TmpComp;
	  ReadComponent(i, TmpComp);
	  fprintf(stream, "%u: ", i);
	  fprintf(stream, "%u, %u, %u\n", TmpComp.Ssize, TmpComp.XRsize, TmpComp.YRsize);
	}
    }
}

//
void
ASDCP::JP2K::Accessor::COD::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "COD: \n");
  const char* prog_order_str = "RESERVED";
  const char* transformations_str = prog_order_str;

  switch ( ProgOrder() )
    {
    case 0: prog_order_str = "LRCP"; break;
    case 1: prog_order_str = "RLCP"; break;
    case 2: prog_order_str = "RPCL"; break;
    case 3: prog_order_str = "PCRL"; break;
    case 4: prog_order_str = "CPRL"; break;
    }

  switch ( Transformation() )
    {
    case 0: transformations_str = "9/7"; break;
    case 1: transformations_str = "5/3"; break;
    }

  fprintf(stream, "      ProgOrder: %s\n", prog_order_str);
  fprintf(stream, "         Layers: %hu\n", Layers());
  fprintf(stream, "   DecompLevels: %hhu\n", DecompLevels());
  fprintf(stream, " CodeBlockWidth: %d\n", 1 << CodeBlockWidth());
  fprintf(stream, "CodeBlockHeight: %d\n", 1 << CodeBlockHeight());
  fprintf(stream, " CodeBlockStyle: %d\n", CodeBlockStyle());
  fprintf(stream, " Transformation: %s\n", transformations_str);
}

//
const char*
ASDCP::JP2K::Accessor::GetQuantizationTypeString(const Accessor::QuantizationType_t t)
{
  switch ( t )
    {
    case QT_NONE: return "none";
    case QT_DERIVED: return "scalar derived";
    case QT_EXP: return "scalar expounded";
    }

  return "**UNKNOWN**";
}

//
void
ASDCP::JP2K::Accessor::QCD::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "QCD: \n");
  fprintf(stream, "QuantizationType: %s\n", GetQuantizationTypeString(QuantizationType()));
  fprintf(stream, "       GuardBits: %d\n", GuardBits());
  fprintf(stream, "           SPqcd: %d\n", GuardBits());
  Kumu::hexdump(m_MarkerData, m_DataSize, stream);
}

//
void
ASDCP::JP2K::Accessor::COM::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  if ( IsText() )
    {
      std::string tmp_str;
      tmp_str.assign((char*)CommentData(), CommentSize());
      fprintf(stream, "COM:%s\n", tmp_str.c_str());
    }
  else
    {
      fprintf(stream, "COM:\n");
      Kumu::hexdump(CommentData(), CommentSize(), stream);
    }
}

//
void
ASDCP::JP2K::Accessor::PRF::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "PRF: \n");

  if (N() == 0) {

	  fprintf(stream, "     N/A");

  } else {

	  for (ui16_t i = 1; i <= N(); i++) {
		  fprintf(stream, "pprf(%d): %d\n", i, pprf(i));
	  }

  }
}

//
void
ASDCP::JP2K::Accessor::CPF::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "CPF: \n");

  if (N() == 0) {

	  fprintf(stream, "     N/A");

  } else {

	  for (ui16_t i = 1; i <= N(); i++) {
		  fprintf(stream, "pcpf(%d): %d\n", i, pcpf(i));
	  }

  }
}


//
void
ASDCP::JP2K::Accessor::CAP::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "CAP: \n");

  ui32_t pcap = this->pcap();

  if (pcap == 0) {

	  fprintf(stream, "     None");

  } else {

	  for (i32_t b = 32, i = 1; b > 0; b--) {

		  if ((pcap >> (32 - b)) & 0x1) {

			  fprintf(stream, "     ccap(%d): %d\n", b, this->ccap(i++));

		  }
	  }
  }
}

//-------------------------------------------------------------------------------------------------------
//


//
void
ASDCP::JP2K::Marker::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Marker%s 0x%04x: %s", (m_IsSegment ? " segment" : ""), m_Type, GetMarkerString(m_Type));  

  if ( m_IsSegment )
    fprintf(stream, ", 0x%0x bytes", m_DataSize);

  fputc('\n', stream);
}

//
const char*
ASDCP::JP2K::GetMarkerString(Marker_t m)
{
  switch ( m )
    {
    case MRK_NIL: return "NIL"; break;
    case MRK_SOC: return "SOC: Start of codestream"; break;
    case MRK_SOT: return "SOT: Start of tile-part"; break;
    case MRK_SOD: return "SOD: Start of data"; break;
    case MRK_EOC: return "EOC: End of codestream"; break;
    case MRK_SIZ: return "SIZ: Image and tile size"; break;
    case MRK_COD: return "COD: Coding style default"; break;
    case MRK_COC: return "COC: Coding style component"; break;
    case MRK_RGN: return "RGN: Region of interest"; break;
    case MRK_QCD: return "QCD: Quantization default"; break;
    case MRK_QCC: return "QCC: Quantization component"; break;
    case MRK_POC: return "POC: Progression order change"; break;
    case MRK_TLM: return "TLM: Tile-part lengths"; break;
    case MRK_PLM: return "PLM: Packet length, main header"; break;
    case MRK_PLT: return "PLT: Packet length, tile-part header"; break;
    case MRK_PPM: return "PPM: Packed packet headers, main header"; break;
    case MRK_PPT: return "PPT: Packed packet headers, tile-part header"; break;
    case MRK_SOP: return "SOP: Start of packet"; break;
    case MRK_EPH: return "EPH: End of packet header"; break;
    case MRK_CRG: return "CRG: Component registration"; break;
    case MRK_COM: return "COM: Comment"; break;
    case MRK_CPF: return "CPF: Corresponding profile"; break;
    case MRK_CAP: return "CAP: Capabilities"; break;
    case MRK_PRF: return "PRF: Profile"; break;
    }

  return "Unknown marker code";
}

//
// end JP2K.cpp
//
