
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
