/*
Copyright (c) 2003-2005, John Hurst
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
/*! \file    AS_DCP.h
    \version $Id$       
    \brief   AS-DCP library, public interface

The asdcplib library is a set of wrapper objects that offer simplified
access to files conforming to the file formats proposed by the SMPTE
D-Cinema packaging working group DC28.20.  The file format, labeled
AS-DCP, is described in series of separate documents which include but
may not be limited to:

 o AS-DCP Track File Specification
 o AS-DCP Track File Essence Encryption Specification
 o AS-DCP Operational Constraints Specification
 o SMPTE 330M - UMID
 o SMPTE 336M - KLV
 o SMPTE 377M - MXF
 o SMPTE 390M - OP-Atom
 o SMPTE 379M - Generic Container
 o SMPTE 381M - MPEG2 picture
 o SMPTE XXXM - JPEG 2000 picture
 o SMPTE 382M - WAV/PCM sound
 o IETF RFC 2104 - HMAC/SHA1
 o NIST FIPS 197 - AES (Rijndael)

The following use cases are supported by the library:

 o Write a plaintext MPEG2 Video Elementary Stream to a plaintext ASDCP file
 o Write a plaintext MPEG2 Video Elementary Stream to a ciphertext ASDCP file
 o Read a plaintext MPEG2 Video Elementary Stream from a plaintext ASDCP file
 o Read a plaintext MPEG2 Video Elementary Stream from a ciphertext ASDCP file
 o Read a ciphertext MPEG2 Video Elementary Stream from a ciphertext ASDCP file
 o Write one or more plaintext JPEG 2000 codestreams to a plaintext ASDCP file
 o Write one or more plaintext JPEG 2000 codestreams to a ciphertext ASDCP file
 o Read one or more plaintext JPEG 2000 codestreams from a plaintext ASDCP file
 o Read one or more plaintext JPEG 2000 codestreams from a ciphertext ASDCP file
 o Read one or more ciphertext JPEG 2000 codestreams from a ciphertext ASDCP file
 o Write one or more plaintext PCM audio streams to a plaintext ASDCP file
 o Write one or more plaintext PCM audio streams to a ciphertext ASDCP file
 o Read one or more plaintext PCM audio streams from a plaintext ASDCP file
 o Read one or more plaintext PCM audio streams from a ciphertext ASDCP file
 o Read one or more ciphertext PCM audio streams from a ciphertext ASDCP file
 o Read header metadata from an ASDCP file

This project depends upon the following library:
 - OpenSSL http://www.openssl.org/

*/

#ifndef _AS_DCP_H__
#define _AS_DCP_H__

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <math.h>

//--------------------------------------------------------------------------------
// common integer types
// supply your own by defining ASDCP_NO_BASE_TYPES

#ifndef ASDCP_NO_BASE_TYPES
typedef unsigned char  byte_t;
typedef char           i8_t;
typedef unsigned char  ui8_t;
typedef short          i16_t;
typedef unsigned short ui16_t;
typedef int            i32_t;
typedef unsigned int   ui32_t;
#endif


//--------------------------------------------------------------------------------
// convenience macros

// Convenience macros for managing return values in predicates
#define ASDCP_SUCCESS(v) (((v) < 0) ? 0 : 1)
#define ASDCP_FAILURE(v) (((v) < 0) ? 1 : 0)


// Returns RESULT_PTR if the given argument is NULL.
// See Result_t below for an explanation of RESULT_* symbols.
#define ASDCP_TEST_NULL(p) \
  if ( (p) == 0  ) { \
    return ASDCP::RESULT_PTR; \
  }

// Returns RESULT_PTR if the given argument is NULL. See Result_t
// below for an explanation of RESULT_* symbols. It then assumes
// that the argument is a pointer to a string and returns
// RESULT_NULL_STR if the first character is '\0'.
//
#define ASDCP_TEST_NULL_STR(p) \
  ASDCP_TEST_NULL(p); \
  if ( (p)[0] == '\0' ) { \
    return ASDCP::RESULT_NULL_STR; \
  }

// Produces copy constructor boilerplate. Allows convenient private
// declatarion of copy constructors to prevent the compiler from
// silently manufacturing default methods.
#define ASDCP_NO_COPY_CONSTRUCT(T)   \
          T(const T&); \
          T& operator=(const T&)


//--------------------------------------------------------------------------------
// All library components are defined in the namespace ASDCP
//
namespace ASDCP {
  // The version number consists of three segments: major, API minor, and
  // implementation minor. Whenever a change is made to AS_DCP.h, the API minor
  // version will increment. Changes made to the internal implementation will
  // result in the incrementing of the implementation minor version.

  // For example, if asdcplib version 1.0.0 were modified to accomodate changes
  // in file format, and if no changes were made to AS_DCP.h, the new version would be
  // 1.0.1. If changes were also required in AS_DCP.h, the new version would be 1.1.1.
  const ui32_t VERSION_MAJOR = 1;
  const ui32_t VERSION_APIMINOR = 0;
  const ui32_t VERSION_IMPMINOR = 4;
  const char* Version();

  // UUIDs are passed around as strings of UUIDlen bytes
  const ui32_t UUIDlen = 16;

  // Encryption keys are passed around as strings of KeyLen bytes
  const ui32_t KeyLen = 16;

  // Encryption key IDs are passed around as strings of KeyIDlen bytes
  const ui32_t KeyIDlen = 16;


  //---------------------------------------------------------------------------------
  // message logging

  // Error and debug messages will be delivered to an object having this interface.
  // The default implementation sends only LOG_ERROR and LOG_WARN messages to stderr.
  // To receive LOG_INFO or LOG_DEBUG messages, or to send messages somewhere other
  // than stderr, implement this interface and register an instance of your new class
  // by calling SetDefaultLogSink().
  class ILogSink
    {
    public:
      enum LogType_t { LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };
      
      virtual ~ILogSink() {}
      virtual void Error(const char*, ...) = 0; // receives error messges
      virtual void Warn(const char*, ...) = 0;  // receives warning messges
      virtual void Info(const char*, ...) = 0;  // receives info messages
      virtual void Debug(const char*, ...) = 0; // receives debug messages
      virtual void Logf(LogType_t, const char*, ...) = 0; // log a formatted string with positional parameters
      virtual void vLogf(LogType_t, const char*, va_list*) = 0; // log a formatted string with a va_list struct
    };

  // Sets the internal default sink to the given receiver. If the given value
  // is zero, sets the default sink to the internally allocated stderr sink.
  void SetDefaultLogSink(ILogSink* = 0);

  // Returns the internal default sink.
  ILogSink& DefaultLogSink();

  //---------------------------------------------------------------------------------
  // return values

  // Each method or subroutine in this library that is not void or does not directly
  // return a value will instead return a result code from this enumeration.
  enum Result_t {
    RESULT_FALSE      =  1,   // successful but negative
    RESULT_OK         =  0,   // No errors detected
    RESULT_FAIL       = -1,   // An undefined error was detected
    RESULT_PTR        = -2,   // An unexpected NULL pointer was given
    RESULT_NULL_STR   = -3,   // An unexpected empty string was given
    RESULT_SMALLBUF   = -4,   // The given frame buffer is too small
    RESULT_INIT       = -5,   // The object is not yet initialized
    RESULT_NOT_FOUND  = -6,   // The requested file does not exist on the system
    RESULT_NO_PERM    = -7,   // Insufficient privilege exists to perform the operation
    RESULT_FILEOPEN   = -8,   // Failure opening file
    RESULT_FORMAT     = -9,   // The file format is not proper OP-Atom/AS-DCP
    RESULT_BADSEEK    = -10,  // An invalid file location was requested
    RESULT_READFAIL   = -11,  // File read error
    RESULT_WRITEFAIL  = -12,  // File write error
    RESULT_RAW_ESS    = -13,  // Unknown raw essence file type
    RESULT_RAW_FORMAT = -14,  // Raw essence format invalid
    RESULT_STATE      = -15,  // Object state error
    RESULT_ENDOFFILE  = -16,  // Attempt to read past end of file
    RESULT_CONFIG     = -17,  // Invalid configuration option detected
    RESULT_RANGE      = -18,  // Frame number out of range
    RESULT_CRYPT_CTX  = -19,  // AESEncContext required when writing to encrypted file
    RESULT_LARGE_PTO  = -20,  // Plaintext offset exceeds frame buffer size
    RESULT_ALLOC      = -21,  // Error allocating memory
    RESULT_CAPEXTMEM  = -22,  // Cannot resize externally allocated memory
    RESULT_CHECKFAIL  = -23,  // The check value did not decrypt correctly
    RESULT_HMACFAIL   = -24,  // HMAC authentication failure
    RESULT_HMAC_CTX   = -25,  // HMAC context required
    RESULT_CRYPT_INIT = -26,  // Error initializing block cipher context
    RESULT_EMPTY_FB   = -27,  // Attempted to write an empty frame buffer
  };

  // Returns a pointer to an English language string describing the given result code.
  // If the result code is not a valid member of the Result_t enum, the string
  // "**UNKNOWN**" will be returned.
  const char* GetResultString(Result_t);

  //---------------------------------------------------------------------------------
  // file identification

  // The file accessors in this library implement a bounded set of essence types.
  // This list will be expanded when support for new types is added to the library.
  enum EssenceType_t {
    ESS_UNKNOWN,     // the file is not a supported AS-DCP essence container
    ESS_MPEG2_VES,   // the file contains an MPEG video elementary stream
    ESS_JPEG_2000,   // the file contains one or more JPEG 2000 codestreams
    ESS_PCM_24b_48k  // the file contains one or more PCM audio pairs
  };


  // Determine the type of essence contained in the given MXF file. RESULT_OK
  // is returned if the file is successfully opened and contains a valid MXF
  // stream. If there is an error, the result code will indicate the reason.
  Result_t EssenceType(const char* filename, EssenceType_t& type);

  // Determine the type of essence contained in the given raw file. RESULT_OK
  // is returned if the file is successfully opened and contains a known
  // stream type. If there is an error, the result code will indicate the reason.
  Result_t RawEssenceType(const char* filename, EssenceType_t& type);

  // Locate the named object in the file header and dump it to the given stream.
  // The default dump stream is stderr.
  Result_t FindObject(const char* filename, const char* objname, FILE* = 0);


  // A simple container for rational numbers.
  struct Rational
  {
    i32_t Numerator;
    i32_t Denominator;

    Rational() : Numerator(0), Denominator(0) {}
    Rational(i32_t n, i32_t d) : Numerator(n), Denominator(d) {}

    inline double Quotient() const {
      return (double)Numerator / (double)Denominator;
    }

    inline bool operator==(const Rational& rhs) const {
      return ( rhs.Numerator == Numerator && rhs.Denominator == Denominator );
    }

    inline bool operator!=(const Rational& rhs) const {
      return ( rhs.Numerator != Numerator || rhs.Denominator != Denominator );
    }
  };

  // common edit rates, use these instead of hard coded constants
  const Rational EditRate_24(24,1);
  const Rational EditRate_23_98(24000,1001);
  const Rational EditRate_48(48,1);
  const Rational SampleRate_48k(48000,1);

  // Non-reference counting container for internal member objects.
  // Please do not use this class for any other purpose.
  template <class T>
    class mem_ptr
    {
      T* m_p; // the thing we point to
      mem_ptr(T&);

    public:
      mem_ptr() : m_p(0) {}
      mem_ptr(T* p) : m_p(p) {}
      ~mem_ptr() { delete m_p; }

      inline T&   operator*()  const { return *m_p; }
      inline T*   operator->() const { return m_p; }
      inline      operator T*()const { return m_p; }
      inline const mem_ptr<T>& operator=(T* p) { set(p); return *this; }
      inline T*   set(T* p)          { delete m_p; m_p = p; return m_p; }
      inline T*   get()        const { return m_p; }
      inline void release()          { m_p = 0; }
      inline bool empty()      const { return m_p == 0; }
    };

  //---------------------------------------------------------------------------------
  // cryptographic support

  // The following classes define interfaces to Rijndael contexts having the following properties:
  //  o 16 byte key
  //  o CBC mode with 16 byte block size
  const ui32_t CBC_KEY_SIZE = 16;
  const ui32_t CBC_BLOCK_SIZE = 16;
  const ui32_t HMAC_SIZE = 20;

  //
  class AESEncContext
    {
      class h__AESContext;
      mem_ptr<h__AESContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(AESEncContext);

    public:
      AESEncContext();
      ~AESEncContext();

      // Initializes Rijndael CBC encryption context.
      // Returns error if the key argument is NULL.
      Result_t InitKey(const byte_t* key);
      
      // Initializes 16 byte CBC Initialization Vector. This operation may be performed
      // any number of times for a given key.
      // Returns error if the i_vec argument is NULL.
      Result_t SetIVec(const byte_t* i_vec);
      Result_t GetIVec(byte_t* i_vec) const;

      // Encrypt a block of data. The block size must be a multiple of CBC_BLOCK_SIZE.
      // Returns error if either argument is NULL.
      Result_t EncryptBlock(const byte_t* pt_buf, byte_t* ct_buf, ui32_t block_size);
    };

  //
  class AESDecContext
    {
      class h__AESContext;
      mem_ptr<h__AESContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(AESDecContext);

    public:
      AESDecContext();
      ~AESDecContext();

      // Initializes Rijndael CBC decryption context.
      // Returns error if the key argument is NULL.
      Result_t InitKey(const byte_t* key);

      // Initializes 16 byte CBC Initialization Vector. This operation may be performed
      // any number of times for a given key.
      // Returns error if the i_vec argument is NULL.
      Result_t SetIVec(const byte_t* i_vec);

      // Decrypt a block of data. The block size must be a multiple of CBC_BLOCK_SIZE.
      // Returns error if either argument is NULL.
      Result_t DecryptBlock(const byte_t* ct_buf, byte_t* pt_buf, ui32_t block_size);
    };

  //
  class HMACContext
    {
      class h__HMACContext;
      mem_ptr<h__HMACContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(HMACContext);

    public:
      HMACContext();
      ~HMACContext();

      // Initializes HMAC context. The key argument must point to a binary
      // key that is CBC_KEY_SIZE bytes in length. Returns error if the key
      // argument is NULL.
      Result_t InitKey(const byte_t* key);

      // Reset internal state, allows repeated cycles of Update -> Finalize
      void Reset();

      // Add data to the digest. Returns error if the key argument is NULL or
      // if the digest has been finalized.
      Result_t Update(const byte_t* buf, ui32_t buf_len);

      // Finalize digest.  Returns error if the digest has already been finalized.
      Result_t Finalize();

      // Writes HMAC value to given buffer. buf must point to a writable area of
      // memory that is at least HMAC_SIZE bytes in length. Returns error if the
      // buf argument is NULL or if the digest has not been finalized.
      Result_t GetHMACValue(byte_t* buf) const;

      // Tests the given value against the finalized value in the object. buf must
      // point to a readable area of memory that is at least HMAC_SIZE bytes in length.
      // Returns error if the buf argument is NULL or if the values do ot match.
      Result_t TestHMACValue(const byte_t* buf) const;
    };

  //---------------------------------------------------------------------------------
  // WriterInfo class - encapsulates writer identification details used for
  // OpenWrite() calls.  Replace these values at runtime to identify your product.
  //
  struct WriterInfo
  {
    byte_t      ProductUUID[UUIDlen];
    byte_t      AssetUUID[UUIDlen];
    byte_t      ContextID[UUIDlen];
    byte_t      CryptographicKeyID[KeyIDlen];
    bool        EncryptedEssence; // true if essence data is (or is to be) encrypted
    bool        UsesHMAC;         // true if HMAC exists or is to be calculated
    std::string ProductVersion;
    std::string CompanyName;
    std::string ProductName;
    
    WriterInfo() : EncryptedEssence(false), UsesHMAC(false) {
      static byte_t default_ProductUUID_Data[UUIDlen] = { 0x43, 0x05, 0x9a, 0x1d, 0x04, 0x32, 0x41, 0x01,
							  0xb8, 0x3f, 0x73, 0x68, 0x15, 0xac, 0xf3, 0x1d };
      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      memset(AssetUUID, 0, UUIDlen);
      memset(ContextID, 0, UUIDlen);
      memset(CryptographicKeyID, 0, KeyIDlen);

      ProductVersion = "Unreleased ";
      ProductVersion += Version();
      CompanyName = "DCI";
      ProductName = "asdcplib";
    }
  };

  // Print WriterInfo to stream, stderr by default.
  void WriterInfoDump(const WriterInfo&, FILE* = 0);

  //---------------------------------------------------------------------------------
  // frame buffer base class
  //
  // The supported essence types are stored using per-frame KLV packetization. The
  // following class implements essence-neutral functionality for managing a buffer
  // containing a frame of essence.

  class FrameBuffer
    {
      ASDCP_NO_COPY_CONSTRUCT(FrameBuffer);

    protected:
      byte_t* m_Data;          // pointer to memory area containing frame data
      ui32_t  m_Capacity;      // size of memory area pointed to by m_Data
      bool    m_OwnMem;        // if false, m_Data points to externally allocated memory
      ui32_t  m_Size;          // size of frame data in memory area pointed to by m_Data
      ui32_t  m_FrameNumber;   // delivery-order frame number

      // It is possible to read raw ciphertext from an encrypted AS-DCP file.
      // After reading an encrypted AS-DCP file in raw mode, the frame buffer will
      // contain the encrypted source value portion of the Encrypted Triplet, followed
      // by the integrity pack, if it exists.
      // The buffer will begin with the IV and CheckValue, followed by encrypted essence
      // and optional integrity pack
      // The SourceLength and PlaintextOffset values from the packet will be held in the
      // following variables:
      ui32_t  m_SourceLength;       // plaintext length (delivered plaintext+decrypted ciphertext)
      ui32_t  m_PlaintextOffset;    // offset to first byte of ciphertext

     public:
      FrameBuffer();
      virtual ~FrameBuffer();

      // Instructs the object to use an externally allocated buffer. The external
      // buffer will not be cleaned up by the frame buffer when it exits.
      // Call with (0,0) to revert to internally allocated buffer.
      // Returns error if the buf_addr argument is NULL and buf_size is non-zero.
      Result_t SetData(byte_t* buf_addr, ui32_t buf_size);

      // Sets the size of the internally allocate buffer. Returns RESULT_CAPEXTMEM
      // if the object is using an externally allocated buffer via SetData();
      // Resets content size to zero.
      Result_t Capacity(ui32_t cap);

      // returns the size of the buffer
      inline ui32_t  Capacity() const { return m_Capacity; }

      // returns a const pointer to the essence data
      inline const byte_t* RoData() const { return m_Data; }

      // returns a non-const pointer to the essence data
      inline byte_t* Data() { return m_Data; }

      // set the size of the buffer's contents
      inline ui32_t  Size(ui32_t size) { return m_Size = size; }

      // returns the size of the buffer's contents
      inline ui32_t  Size() const { return m_Size; }

      // Sets the absolute frame number of this frame in the file in delivery order.
      inline void    FrameNumber(ui32_t num) { m_FrameNumber = num; }

      // Returns the absolute frame number of this frame in the file in delivery order.
      inline ui32_t  FrameNumber() const { return m_FrameNumber; }

      // Sets the length of the plaintext essence data
      inline void    SourceLength(ui32_t len) { m_SourceLength = len; }

      // When this value is 0 (zero), the buffer contains only plaintext. When it is
      // non-zero, the buffer contains raw ciphertext and the return value is the length
      // of the original plaintext.
      inline ui32_t  SourceLength() const { return m_SourceLength; }

      // Sets the offset into the buffer at which encrypted data begins
      inline void    PlaintextOffset(ui32_t ofst) { m_PlaintextOffset = ofst; }

      // Returns offset into buffer of first byte of ciphertext.
      inline ui32_t  PlaintextOffset() const { return m_PlaintextOffset; }
    };


  //---------------------------------------------------------------------------------
  // MPEG2 video elementary stream support

  //
  namespace MPEG2
    {
      // MPEG picture coding type
      enum FrameType_t {
	FRAME_U = 0x00, // Unknown
	FRAME_I = 0x01, // I-Frame
	FRAME_P = 0x02, // P-Frame
	FRAME_B = 0x03  // B-Frame
      };

      // convert FrameType_t to char
      inline char FrameTypeChar(FrameType_t type)
	{
	  switch ( type )
	    {
	    case FRAME_I: return 'I';
	    case FRAME_B: return 'B';
	    case FRAME_P: return 'P';
	    default: return 'U';
	    }
	}

      // Structure represents the metadata elements in the file header's
      // MPEG2VideoDescriptor object.
      struct VideoDescriptor
	{
	  Rational EditRate;                // 
	  ui32_t   FrameRate;               // 
	  Rational SampleRate;              // 
	  ui8_t    FrameLayout;             // 
	  ui32_t   StoredWidth;             // 
	  ui32_t   StoredHeight;            // 
	  Rational AspectRatio;             // 
	  ui32_t   ComponentDepth;          // 
	  ui32_t   HorizontalSubsampling;   // 
	  ui32_t   VerticalSubsampling;     // 
	  ui8_t    ColorSiting;             // 
	  ui8_t    CodedContentType;        // 
	  bool     LowDelay;                // 
	  ui32_t   BitRate;                 // 
	  ui8_t    ProfileAndLevel;         // 
	  ui32_t   ContainerDuration;       // 
      };

      // Print VideoDescriptor to stream, stderr by default.
      void VideoDescriptorDump(const VideoDescriptor&, FILE* = 0);

      // A container for MPEG frame data.
      class FrameBuffer : public ASDCP::FrameBuffer
	{
	  ASDCP_NO_COPY_CONSTRUCT(FrameBuffer); // TODO: should have copy construct

	protected:
	  FrameType_t m_FrameType;
	  ui8_t       m_TemporalOffset;
	  bool        m_ClosedGOP;
	  bool        m_GOPStart;

	public:
	  FrameBuffer() :
	    m_FrameType(FRAME_U), m_TemporalOffset(0),
	    m_ClosedGOP(false), m_GOPStart(false) {}

	  FrameBuffer(ui32_t size) :
	    m_FrameType(FRAME_U), m_TemporalOffset(0),
	    m_ClosedGOP(false), m_GOPStart(false)
	    {
	      Capacity(size);
	    }
	    
	  virtual ~FrameBuffer() {}

	  // Sets the MPEG frame type of the picture data in the frame buffer.
	  inline void FrameType(FrameType_t type) { m_FrameType = type; }

	  // Returns the MPEG frame type of the picture data in the frame buffer.
	  inline FrameType_t FrameType() const { return m_FrameType; }

	  // Sets the MPEG temporal offset of the picture data in the frame buffer.
	  inline void TemporalOffset(ui8_t offset) { m_TemporalOffset = offset; }

	  // Returns the MPEG temporal offset of the picture data in the frame buffer.
	  inline ui8_t TemporalOffset() const { return m_TemporalOffset; }

	  // Sets the MPEG GOP 'start' attribute for the frame buffer.
	  inline void GOPStart(bool start) { m_GOPStart = start; }

	  // True if the frame in the buffer is the first in the GOP (in transport order)
	  inline bool GOPStart() const { return m_GOPStart; }

	  // Sets the MPEG GOP 'closed' attribute for the frame buffer.
	  inline void ClosedGOP(bool closed) { m_ClosedGOP = closed; }

	  // Returns true if the frame in the buffer is from a closed GOP, false if
	  // the frame is from an open GOP.  Always returns false unless GOPStart()
	  // returns true.
	  inline bool ClosedGOP() const { return m_ClosedGOP; }

	  // Print object state to stream, include n bytes of frame data if indicated.
	  // Default stream is stderr.
	  void    Dump(FILE* = 0, ui32_t dump_len = 0) const;
	};


      // An object which opens and reads an MPEG2 Video Elementary Stream file.  The call to
      // OpenRead() reads metadata from the file and populates an internal VideoDescriptor object.
      // Each subsequent call to ReadFrame() reads exactly one frame from the stream into the
      // given FrameBuffer object.
      class Parser
	{
	  class h__Parser;
	  mem_ptr<h__Parser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(Parser);

	public:
	  Parser();
	  virtual ~Parser();

	  // Opens the stream for reading, parses enough data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const char* filename) const;

	  // Fill a VideoDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillVideoDescriptor(VideoDescriptor&) const;

	  // Rewind the stream to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the input file and places it in the
	  // frame buffer. Fails if the buffer is too small or the stream is empty.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // data byte of the first slice. Set this value to zero if you want
	  // encrypted headers.
	  Result_t ReadFrame(FrameBuffer&) const;
	};

      // A class which creates and writes MPEG frame data to an AS-DCP format MXF file.
      // Not yet implemented
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const char* filename, const WriterInfo&,
			     const VideoDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      // A class which reads MPEG frame data from an AS-DCP format MXF file.
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const char* filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a VideoDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillVideoDescriptor(VideoDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Calculates the first frame in transport order of the GOP in which the requested
	  // frame is located.  Calls ReadFrame() to fetch the frame at the calculated position.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t ReadFrameGOPStart(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Calculates the first frame in transport order of the GOP in which the requested
	  // frame is located.  Sets key_frame_number to the number of the frame at the calculated position.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FindFrameGOPStart(ui32_t frame_number, ui32_t& key_frame_number) const;

	  // Returns the type of the frame at the given position.
	  // Returns RESULT_INIT if the file is not open or RESULT_RANGE if the index is out of range.
	  Result_t FrameType(ui32_t frame_number, FrameType_t&) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace MPEG2

  //
  namespace PCM
    {
      struct AudioDescriptor
	{
	  Rational SampleRate;         // rate of frame wrapping
	  Rational AudioSamplingRate;  // rate of audio sample
	  ui32_t   Locked;             // 
	  ui32_t   ChannelCount;       // number of channels
	  ui32_t   QuantizationBits;   // number of bits per single-channel sample
	  ui32_t   BlockAlign;         // number of bytes ber sample, all channels
	  ui32_t   AvgBps;             // 
	  ui32_t   LinkedTrackID;      // 
	  ui32_t   ContainerDuration;  // number of frames
      };

      // Print debugging information to stream (stderr default)
      void   AudioDescriptorDump(const AudioDescriptor&, FILE* = 0);

      // Returns size in bytes of a single sample of data described by ADesc
      inline ui32_t CalcSampleSize(const AudioDescriptor& ADesc)
	{
	  return (ADesc.QuantizationBits / 8) * ADesc.ChannelCount;
	}

      // Returns number of samples per frame of data described by ADesc
      inline ui32_t CalcSamplesPerFrame(const AudioDescriptor& ADesc)
	{
	  double tmpd = ADesc.AudioSamplingRate.Quotient() / ADesc.SampleRate.Quotient();
	  return (ui32_t)ceil(tmpd);
	}

      // Returns the size in bytes of a frame of data described by ADesc
      inline ui32_t CalcFrameBufferSize(const AudioDescriptor& ADesc)
	{
	  return CalcSampleSize(ADesc) * CalcSamplesPerFrame(ADesc);
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

      // An object which opens and reads a WAV file.  The call to OpenRead() reads metadata from
      // the file and populates an internal AudioDescriptor object. Each subsequent call to
      // ReadFrame() reads exactly one frame from the stream into the given FrameBuffer object.
      // A "frame" is either 2000 or 2002 samples, depending upon the value of PictureRate.
      class WAVParser
	{
	  class h__WAVParser;
	  mem_ptr<h__WAVParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(WAVParser);

	public:
	  WAVParser();
	  virtual ~WAVParser();

	  // Opens the stream for reading, parses enough data to provide a complete
	  // set of stream metadata for the MXFWriter below. PictureRate controls
	  // ther frame rate for the MXF frame wrapping option.
	  Result_t OpenRead(const char* filename, const Rational& PictureRate) const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillAudioDescriptor(AudioDescriptor&) const;

	  // Rewind the stream to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the input file and places it in the
	  // frame buffer. Fails if the buffer is too small or the stream is empty.
	  Result_t ReadFrame(FrameBuffer&) const;
	};


      //
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const char* filename, const WriterInfo&,
			     const AudioDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const char* filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillAudioDescriptor(AudioDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace PCM

  //
  namespace JP2K
    {
      const ui32_t MaxComponents = 3;
      const ui32_t DefaultCodingDataLength = 64;

      struct ImageComponent
      {
	byte_t Ssize;
	byte_t XRsize;
	byte_t YRsize;
      };

      struct PictureDescriptor
      {
	Rational       EditRate;
	ui32_t         ContainerDuration;
	Rational       SampleRate;
	ui32_t         StoredWidth;
	ui32_t         StoredHeight;
	Rational       AspectRatio;
	ui16_t         Rsize;
	ui32_t         Xsize;
	ui32_t         Ysize;
	ui32_t         XOsize;
	ui32_t         YOsize;
	ui32_t         XTsize;
	ui32_t         YTsize;
	ui32_t         XTOsize;
	ui32_t         YTOsize;
	ui16_t         Csize;
	ImageComponent ImageComponents[MaxComponents];
	byte_t         CodingStyle[DefaultCodingDataLength];
	ui32_t         CodingStyleLength;
	byte_t         QuantDefault[DefaultCodingDataLength];
	ui32_t         QuantDefaultLength;
      };

      // Print debugging information to stream (stderr default)
      void   PictureDescriptorDump(const PictureDescriptor&, FILE* = 0);

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


      // An object which opens and reads a JPEG 2000 codestream file.  The file is expected
      // to contain exactly one complete frame of picture essence as an unwrapped (raw)
      // ISO/IEC 15444 codestream.
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
	  Result_t OpenReadFrame(const char* filename, FrameBuffer&) const;

	  // Fill a PictureDescriptor struct with the values from the file's codestream.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;
	};

      // An object which reads a sequence of files containing JPEG 2000 pictures.
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
	  Result_t OpenRead(const char* filename, bool pedantic = false) const;

	  // Fill a PictureDescriptor struct with the values from the first file's codestream.
	  // Returns RESULT_INIT if the directory is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;

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


      //
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const char* filename, const WriterInfo&,
			     const PictureDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const char* filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace JP2K
} // namespace ASDCP


#endif // _AS_DCP_H__

//
// end AS_DCP.h
//
