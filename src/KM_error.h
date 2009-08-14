/*
Copyright (c) 2004-2009, John Hurst
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
  /*! \file    KM_error.h
    \version $Id$
    \brief   error reporting support
  */



#ifndef _KM_ERROR_H_
#define _KM_ERROR_H_

namespace Kumu
{
  // Result code container. Both a signed integer and a text string are stored in the object.
  // When defining your own codes your choice of integer values is mostly unconstrained, but pay
  // attention to the numbering in the other libraries that use Kumu. Values between -99 and 99
  // are reserved for Kumu.

  class Result_t
    {
      int value;
      const char* label;
      Result_t();

    public:
      static const Result_t& Find(int);
      static Result_t Delete(int);

      Result_t(int v, const char* l);
      ~Result_t();

      inline bool        operator==(const Result_t& rhs) const { return value == rhs.value; }
      inline bool        operator!=(const Result_t& rhs) const { return value != rhs.value; }
      inline bool        Success() const { return ( value >= 0 ); }
      inline bool        Failure() const { return ( value < 0 ); }

      inline int         Value() const { return value; }
      inline operator    int() const { return value; }

      inline const char* Label() const { return label; }
      inline operator    const char*() const { return label; }
    };

  const Result_t RESULT_FALSE      ( 1,   "Successful but not true.");
  const Result_t RESULT_OK         ( 0,   "Success.");
  const Result_t RESULT_FAIL       (-1,   "An undefined error was detected.");
  const Result_t RESULT_PTR        (-2,   "An unexpected NULL pointer was given.");
  const Result_t RESULT_NULL_STR   (-3,   "An unexpected empty string was given.");
  const Result_t RESULT_ALLOC      (-4,   "Error allocating memory.");
  const Result_t RESULT_PARAM      (-5,   "Invalid parameter.");
  const Result_t RESULT_NOTIMPL    (-6,   "Unimplemented Feature.");
  const Result_t RESULT_SMALLBUF   (-7,   "The given buffer is too small.");
  const Result_t RESULT_INIT       (-8,   "The object is not yet initialized.");
  const Result_t RESULT_NOT_FOUND  (-9,   "The requested file does not exist on the system.");
  const Result_t RESULT_NO_PERM    (-10,  "Insufficient privilege exists to perform the operation.");
  const Result_t RESULT_STATE      (-11,  "Object state error.");
  const Result_t RESULT_CONFIG     (-12,  "Invalid configuration option detected.");
  const Result_t RESULT_FILEOPEN   (-13,  "File open failure.");
  const Result_t RESULT_BADSEEK    (-14,  "An invalid file location was requested.");
  const Result_t RESULT_READFAIL   (-15,  "File read error.");
  const Result_t RESULT_WRITEFAIL  (-16,  "File write error.");
  const Result_t RESULT_ENDOFFILE  (-17,  "Attempt to read past end of file.");
  const Result_t RESULT_FILEEXISTS (-18,  "Filename already exists.");
  const Result_t RESULT_NOTAFILE   (-19,  "Filename not found.");
  const Result_t RESULT_UNKNOWN    (-20,  "Unknown result code.");
  const Result_t RESULT_DIR_CREATE (-21,  "Unable to create directory.");
  // -22 is reserved
 
} // namespace Kumu

//--------------------------------------------------------------------------------
// convenience macros

// Convenience macros for managing return values in predicates
# define KM_SUCCESS(v) (((v) < 0) ? 0 : 1)
# define KM_FAILURE(v) (((v) < 0) ? 1 : 0)


// Returns RESULT_PTR if the given argument is NULL.
// See Result_t above for an explanation of RESULT_* symbols.
# define KM_TEST_NULL(p) \
  if ( (p) == 0  ) { \
    return Kumu::RESULT_PTR; \
  }

// Returns RESULT_PTR if the given argument is NULL. See Result_t
// in WaimeaCore for an explanation of RESULT_* symbols. It then assumes
// that the argument is a pointer to a string and returns
// RESULT_NULL_STR if the first character is '\0'.
//
# define KM_TEST_NULL_STR(p) \
  KM_TEST_NULL(p); \
  if ( (p)[0] == '\0' ) { \
    return Kumu::RESULT_NULL_STR; \
  }

namespace Kumu
{
  // simple tracing mechanism
  class DTrace_t
  {
    DTrace_t();
    
  protected:
    const char* m_Label;
    Result_t*   m_Watch;
    int         m_Line;
    const char* m_File;
    int         m_Sequence;

  public:
    DTrace_t(const char* Label, Result_t* Watch, int Line, const char* File);
    ~DTrace_t();
  };
}

#ifdef KM_TRACE
#define WDTRACE(l) DTrace_t __wl__Trace__((l), 0, __LINE__, __FILE__)
#define WDTRACER(l,r) DTrace_t __wl__Trace__((l), &(r), __LINE__, __FILE__)
#else
#define WDTRACE(l)
#define WDTRACER(l,r)
#endif


#endif // _KM_ERROR_H_

//
// end KM_error.h
//
