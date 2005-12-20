//
// klvwalk.cpp
//

#include <AS_DCP.h>
#include <MXF.h>
#include <hex_utils.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace ASDCP;


//------------------------------------------------------------------------------------------
//

// There is no header file thet defines this function.
// You just have to know it's there...
void set_debug_mode(bool info_mode, bool debug_mode);


int
main(int argc, char** argv)
{
  Result_t result = RESULT_OK;
  bool read_mxf = false;
  bool rewrite_mxf = false;
  int arg_i = 1;
  set_debug_mode(true, true);

  if ( strcmp(argv[1], "-r") == 0 )
    {
      read_mxf = true;
      arg_i++;
    }
  else if ( strcmp(argv[1], "-w") == 0 )
    {
      rewrite_mxf = true;
      arg_i++;
      assert(argc - arg_i == 2);
    }

  fprintf(stderr, "Opening file %s\n", argv[arg_i]);

  if ( read_mxf )
    {
      ASDCP::FileReader        Reader;
      ASDCP::MXF::OPAtomHeader Header;

      result = Reader.OpenRead(argv[arg_i]);

      if ( ASDCP_SUCCESS(result) )
	result = Header.InitFromFile(Reader);

      //      if ( ASDCP_SUCCESS(result) )
      Header.Dump();

      if ( ASDCP_SUCCESS(result) )
	{
	  ASDCP::MXF::OPAtomIndexFooter Index;
	  result = Reader.Seek(Header.FooterPartition);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      Index.m_Lookup = &Header.m_Primer;
	      result = Index.InitFromFile(Reader);
	    }

	  if ( ASDCP_SUCCESS(result) )
	    Index.Dump();
	}
    }
  else if ( rewrite_mxf )
    {
      ASDCP::FileReader        Reader;
      ASDCP::FileWriter        Writer;
      ASDCP::MXF::OPAtomHeader Header;
      ASDCP::MXF::OPAtomIndexFooter Index;

      result = Reader.OpenRead(argv[arg_i++]);

      if ( ASDCP_SUCCESS(result) )
	result = Header.InitFromFile(Reader);

      if ( ASDCP_SUCCESS(result) )
	result = Reader.Seek(Header.FooterPartition);

      if ( ASDCP_SUCCESS(result) )
	result = Index.InitFromFile(Reader);

      Header.m_Primer.ClearTagList();

      if ( ASDCP_SUCCESS(result) )
	result = Writer.OpenWrite(argv[arg_i]);

      if ( ASDCP_SUCCESS(result) )
	result = Header.WriteToFile(Writer);

//      if ( ASDCP_SUCCESS(result) )
//	result = Index.WriteToFile(Writer);

      // essence packets

      // index

      // RIP
    }
  else // dump klv
    {
      ASDCP::FileReader Reader;
      KLVFilePacket KP;

      result = Reader.OpenRead(argv[arg_i]);

      if ( ASDCP_SUCCESS(result) )
	result = KP.InitFromFile(Reader);

      while ( ASDCP_SUCCESS(result) )
	{
	  KP.Dump(stderr, true);
	  result = KP.InitFromFile(Reader);
	}

      if( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

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

  return 0;
}


//
// end klvwalk.cpp
//










