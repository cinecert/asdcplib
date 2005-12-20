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
/*! \file    jp2k-test.cpp
    \version $Id$
    \brief   JP2K parser test
*/

#include <AS_DCP.h>
#include <FileIO.h>
#include <JP2K.h>
using namespace ASDCP;
using namespace ASDCP::JP2K;

//
int
main(int argc, const char** argv)
{
  ASDCP::JP2K::FrameBuffer FB;
  Marker        MyMarker;

  if ( argc < 2 )
    return 1;

  FB.Capacity(1024*1024*2);
  CodestreamParser Parser;

  Result_t result = Parser.OpenReadFrame(argv[1], FB);

  if ( result != RESULT_OK )
    {
      fputs("Program stopped on error.\n", stderr);

      if ( result != RESULT_FAIL )
        {
          fputs(GetResultString(result), stderr);
          fputc('\n', stderr);
        }

      return 1;
    }

  const byte_t* p = FB.RoData();
  const byte_t* end_p = p + FB.Size();

  hexdump(p, 256, stderr);

  while ( p < end_p && ASDCP_SUCCESS(GetNextMarker(&p, MyMarker)) )
    {
      MyMarker.Dump();

      switch ( MyMarker.m_Type )
	{
	case MRK_SOD:
	  p = end_p;
	  break;

	case MRK_SIZ:
	  {
	    Accessor::SIZ SIZ_(MyMarker);
	    hexdump(MyMarker.m_Data - 2, MyMarker.m_DataSize + 2, stderr);
	    SIZ_.Dump();
	  }
	  break;

	case MRK_COM:
	  {
	    Accessor::COM COM_(MyMarker);
	    COM_.Dump();
	  }
	  break;
	}
    }
      
  return 0;
}


//
// end jp2k-test.cpp
//
