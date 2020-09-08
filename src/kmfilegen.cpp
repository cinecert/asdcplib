/*
Copyright (c) 2005-2009, John Hurst
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
/*! \file    kmfilegen.cpp
    \version $Id$
    \brief   large file test program
*/


#include "AS_DCP.h"
#include <iostream>
#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_aes.h>
#include <assert.h>

using namespace Kumu;

// constants
static const char* PROGRAM_NAME = "kmfilegen";  // program name for messages
const ui32_t RNG_KEY_SIZE = 16;
const ui32_t RNG_KEY_SIZE_BITS = 128;
const ui32_t RNG_BLOCK_SIZE = AES_BLOCKLEN;

// globals
ui32_t      s_Nonce = 0;
FortunaRNG  s_RNG;


//------------------------------------------------------------------------------------------
//
// command line option parser class

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)    if ( ++i >= argc || argv[(i)][0] == '-' ) \
                                 { \
                                   fprintf(stderr, "Argument not found for option -%c.\n", (c)); \
                                   return; \
                                 }

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2005-2009 John Hurst\n\
%s is part of the asdcplib DCP tools package.\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, Kumu::Version(), PROGRAM_NAME, PROGRAM_NAME);
}


//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-c <file-size>] [-v] <output-file>\n\
\n\
       %s [-o <fwd|rev|rand>] [-v] <input-file>\n\
\n\
       %s [-w <output-file>] [-v] <input-file>\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -c <file-size>     - Create test file containing <file-size> megabytes of data\n\
  -h | -help         - Show help\n\
  -o <fwd|rev|rand>  - Specify order used when validating a file.\n\
                       One of fwd|rev|rand, default is rand\n\
  -v                 - Verbose. Prints informative messages to stderr\n\
  -V                 - Show version information\n\
  -w <output-file>   - Read-Validate-Write - file is written to <output-file>\n\
                       (sequential read only)\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME, PROGRAM_NAME, PROGRAM_NAME);
}

enum MajorMode_t {
  MMT_NONE,
  MMT_CREATE,
  MMT_VALIDATE,
  MMT_VAL_WRITE
};

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  const char* order;     // one of fwd|rev|rand
  bool   verbose_flag;   // true if the verbose option was selected
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  std::string filename;  // filename to be processed
  std::string write_filename;  // filename to write with val_write_flag
  ui32_t chunk_count;
  MajorMode_t mode;      // MajorMode selector

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), order(""), verbose_flag(false),
    version_flag(false), help_flag(false),
    chunk_count(0), mode(MMT_VALIDATE)
  {
    //    order = "rand";

    for ( int i = 1; i < argc; i++ )
      {

	if ( (strcmp( argv[i], "-help") == 0) )
	  {
	    help_flag = true;
	    continue;
	  }
     
	if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'c':
		mode = MMT_CREATE;
		TEST_EXTRA_ARG(i, 'c');
		chunk_count = Kumu::xabs(strtol(argv[i], 0, 10));
		break;
		
	      case 'V': version_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'v': verbose_flag = true; break;

	      case 'o':
		TEST_EXTRA_ARG(i, 'o');
		order = argv[i];

		if ( strcmp(order, "fwd" ) != 0 
		     && strcmp(order, "rev" ) != 0
		     && strcmp(order, "rand" ) != 0 )
		  {
		    fprintf(stderr, "Unexpected order token: %s, expecting fwd|rev|rand\n", order);
		    return;
		  }

		break;

	      case 'w':
		mode = MMT_VAL_WRITE;
		TEST_EXTRA_ARG(i, 'w');
		write_filename = argv[i];
		break;
		    
	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else
	  {
	    if (argv[i][0] != '-' )
	      {
		if ( ! filename.empty() )
		  {
		    fprintf(stderr, "Extra filename found: %s\n", argv[i]);
		    return;
		  }
		else
		  filename = argv[i];
	      }
	    else
	      {
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
      }
    
    if ( help_flag || version_flag )
      return;
    
    if ( filename.empty() )
      {
	fprintf(stderr, "Filename required.\n");
	return;
      }
    
    if ( mode != MMT_VALIDATE && strcmp(order, "") != 0 )
      {
	fprintf(stderr, "-o option not valid with -c or -w options.\n");
	return;
      }
    else
      if ( strcmp(order, "") == 0 )
	order = "rand";

    if ( filename == write_filename )
      {
	fprintf(stderr, "Output and input files must be different.\n");
	return;
      }
    
    error_flag = false;
  }
};

//------------------------------------------------------------------------------------------


//
#pragma pack(4)
class CTR_Setup
{
  AES_ctx  m_Context;
  byte_t   m_key[RNG_KEY_SIZE];
  byte_t   m_preamble[8];
  ui32_t   m_nonce;
  ui32_t   m_ctr;

  KM_NO_COPY_CONSTRUCT(CTR_Setup);

public:
  CTR_Setup() {}
  ~CTR_Setup() {}

  inline ui32_t Nonce()     { return KM_i32_LE(m_nonce); }
  inline ui32_t WriteSize() { return ( sizeof(m_key) + sizeof(m_preamble)
				       + sizeof(m_nonce) + sizeof(m_ctr) ); }

  //
  void SetupWrite(byte_t* buf)
  {
    assert(buf);
    s_RNG.FillRandom(m_key, WriteSize());
    assert(s_Nonce > 0);
    m_nonce = KM_i32_LE(s_Nonce--);
    m_ctr &= KM_i32_LE(0x7fffffff); // make sure we have 2GB headroom
    memcpy(buf, m_key, WriteSize());
    AES_init_ctx(&m_Context, m_key);
  }

  //
  void SetupRead(const byte_t* buf)
  {
    assert(buf);
    memcpy(m_key, buf, WriteSize());
    AES_init_ctx(&m_Context, m_key);
  }

  //
  void FillRandom(byte_t* buf, ui32_t buf_len)
  {
    ui32_t gen_count = 0;
    while ( gen_count + RNG_BLOCK_SIZE <= buf_len )
      {
	memcpy(buf + gen_count, m_preamble, RNG_BLOCK_SIZE); 
	AES_encrypt(&m_Context, buf + gen_count);
	m_ctr = KM_i32_LE(KM_i32_LE(m_ctr) + 1);
	gen_count += RNG_BLOCK_SIZE;
      }
  }
};

//
Result_t
CreateLargeFile(CommandOptions& Options)
{
  ui32_t write_total = 0;
  ui32_t write_count = 0;
  FileWriter  Writer;
  ByteString  FB;

  FB.Capacity(Megabyte);
  assert(FB.Capacity() == Megabyte);

  fprintf(stderr, "Writing %u chunks:\n", Options.chunk_count);
  s_Nonce = Options.chunk_count;
  Result_t result = Writer.OpenWrite(Options.filename);

  while ( KM_SUCCESS(result) && write_total < Options.chunk_count )
    {
      if ( KM_SUCCESS(result))
	{
	  CTR_Setup counter;
	  counter.SetupWrite(FB.Data());
	  counter.FillRandom(FB.Data() + counter.WriteSize(), Megabyte - counter.WriteSize());
	  result = Writer.Write(FB.RoData(), Megabyte, &write_count);
	  assert(write_count == Megabyte);
	  fprintf(stderr, "\r%8u ", ++write_total);
	}
    }
  
  fputs("\n", stderr);

  return result;
}

//
Result_t
validate_chunk(ByteString& FB, ByteString& CB, ui32_t* nonce_value)
{
  assert(nonce_value);
  CTR_Setup counter;
  counter.SetupRead(FB.RoData());

  counter.FillRandom(CB.Data() + counter.WriteSize(),
		     Megabyte - counter.WriteSize());

  if ( memcmp(FB.RoData() + counter.WriteSize(),
	      CB.RoData() + counter.WriteSize(),
	      Megabyte - counter.WriteSize()) != 0 )
    {
      fprintf(stderr, "Check data mismatched in chunk\n");
      return RESULT_FAIL;
    }

  *nonce_value = counter.Nonce();

  return RESULT_OK;
}

//
struct read_list_t
{
  ui32_t nonce;
  Kumu::fpos_t position;
};

//
void
randomize_list(read_list_t* read_list, ui32_t check_total)
{
  static ui32_t tmp_ints[4];
  static ui32_t seq = 0;

  for ( ui32_t j = 0; j < check_total; j++ )
    {
      if ( seq > 3 )
	seq = 0;

      if ( seq == 0 )
	s_RNG.FillRandom((byte_t*)tmp_ints, 16);

      ui32_t i = tmp_ints[seq++] % (check_total - 1);

      if ( i == j )
	continue;

      read_list_t t = read_list[i];
      read_list[i] = read_list[j];
      read_list[j] = t;
    }
}

//
Result_t
ReadValidateWriteLargeFile(CommandOptions& Options)
{
  assert(!Options.write_filename.empty());
  ui32_t  check_total = 0;
  ui32_t  write_total = 0;
  ui32_t  read_count = 0;
  ui32_t write_count = 0;
  FileReader  Reader;
  FileWriter  Writer;
  ByteString  FB, CB; // Frame Buffer and Check Buffer


  FB.Capacity(Megabyte);
  assert(FB.Capacity() == Megabyte);
  CB.Capacity(Megabyte);
  assert(CB.Capacity() == Megabyte);

  Result_t result = Reader.OpenRead(Options.filename);

  if ( KM_SUCCESS(result) )
    result = Writer.OpenWrite(Options.write_filename);

  // read the first chunk and get set up
  while ( KM_SUCCESS(result) )
    {
      result = Reader.Read(FB.Data(), Megabyte, &read_count);

      if ( KM_SUCCESS(result) )
	{
	  if ( read_count < Megabyte )
	    {
	      fprintf(stderr, "Read() returned short buffer: %u\n", read_count);
	      result = RESULT_FAIL;
	    }

	  result = validate_chunk(FB, CB, &check_total);

	  if ( KM_SUCCESS(result) )
	    {
	      result = Writer.Write(FB.RoData(), Megabyte, &write_count);
	      assert(write_count == Megabyte);
	      fprintf(stderr, "\r%8u ", ++write_total);
	    }
	}
      else if ( result == RESULT_ENDOFFILE )
	{
	  result = RESULT_OK;
	  break;
	}
    }

  fputs("\n", stderr);
  return result;
}


//
Result_t
ValidateLargeFile(CommandOptions& Options)
{
  ui32_t  check_total = 0;
  ui32_t  read_count = 0;
  ui32_t  read_list_i = 0;
  read_list_t* read_list = 0;
  FileReader Reader;
  ByteString FB, CB; // Frame Buffer and Check Buffer

  FB.Capacity(Megabyte);
  assert(FB.Capacity() == Megabyte);
  CB.Capacity(Megabyte);
  assert(CB.Capacity() == Megabyte);

  Result_t result = Reader.OpenRead(Options.filename);

  // read the first chunk and get set up
  if ( KM_SUCCESS(result) )
    {
      result = Reader.Read(FB.Data(), Megabyte, &read_count);

      if ( read_count < Megabyte )
	{
	  fprintf(stderr, "Read() returned short buffer: %u\n", read_count);
	  result = RESULT_FAIL;
	}
      else if ( KM_SUCCESS(result) )
	result = validate_chunk(FB, CB, &check_total);

      if ( KM_SUCCESS(result) )
	{
	  fprintf(stderr, "Validating %u chunk%s in %s order:\n",
		  check_total, (check_total == 1 ? "" : "s"), Options.order);
	  assert(read_list == 0);
	  read_list = (read_list_t*)malloc(check_total * sizeof(read_list_t));
	  assert(read_list);

	  // Set up an index to the chunks. The chunks are written
	  // to the file in order of descending nonce value.
	  if ( strcmp(Options.order, "fwd") == 0 )
	    {
	      for ( ui32_t i = 0; i < check_total; i++ )
		{
		  read_list[i].nonce = check_total - i;
		  Kumu::fpos_t ofst = check_total - read_list[i].nonce;
		  read_list[i].position = ofst * (Kumu::fpos_t)Megabyte;
		}
	    }
	  else
	    {
	      for ( ui32_t i = 0; i < check_total; i++ )
		{
		  read_list[i].nonce = i + 1;
		  Kumu::fpos_t ofst = check_total - read_list[i].nonce;
		  read_list[i].position = ofst * (Kumu::fpos_t)Megabyte;
		}

	      if ( strcmp(Options.order, "rand") == 0 )
		randomize_list(read_list, check_total); // this makes it random
	    }
	}
    }

  if ( KM_SUCCESS(result) )
    {
      assert(read_list);
      ui32_t nonce = 0;

      for ( read_list_i = 0;
	    read_list_i < check_total && KM_SUCCESS(result);
	    read_list_i++ )
	{
	  fprintf(stderr, "\r%8u [%8u] ", read_list_i+1, read_list[read_list_i].nonce);
	  result = Reader.Seek(read_list[read_list_i].position);

	  if ( KM_SUCCESS(result) )
	    result = Reader.Read(FB.Data(), Megabyte, &read_count);

	  if ( result == RESULT_ENDOFFILE )
	    break;

	  else if ( read_count < Megabyte )
	    {
	      fprintf(stderr, "Read() returned short buffer: %u\n", read_count);
	      result = RESULT_FAIL;
	    }
	  else if ( KM_SUCCESS(result) )
	    {
	      result = validate_chunk(FB, CB, &nonce);
	      
	      if ( nonce != read_list[read_list_i].nonce )
		{
		  fprintf(stderr, "Nonce mismatch: expecting %u, got %u\n",
			  nonce, read_list[read_list_i].nonce);

		  return RESULT_FAIL;
		}
	    }
	}
    }

  fputs("\n", stderr);

  if ( result == RESULT_ENDOFFILE )
    {
      if ( check_total == read_list_i )
	result = RESULT_OK;
      else
	{
	  fprintf(stderr, "Unexpected chunk count, got %u, wanted %u\n",
		  read_list_i, check_total);
	  result = RESULT_FAIL;
	}
    }

  return result;
}

//
int
main(int argc, const char **argv)
{
  Result_t result = RESULT_FAIL;
  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.version_flag || Options.help_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  switch ( Options.mode )
    {

    case MMT_CREATE:
      result = CreateLargeFile(Options);
      break;

    case MMT_VALIDATE:
      result = ValidateLargeFile(Options);
      break;

    case MMT_VAL_WRITE:
      result = ReadValidateWriteLargeFile(Options);
      break;
    }

  if ( result != RESULT_OK )
    {
      fputs("Program stopped on error.\n", stderr);

      if ( result != RESULT_FAIL )
	{
	  fputs(result.Label(), stderr);
	  fputc('\n', stderr);
	}

      return 1;
    }

  return 0;
}


//
// end kmfilegen.cpp
//
