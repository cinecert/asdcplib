/*
Copyright (c) 2004, John Hurst
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
/*! \file    AS_DCP_AES.h
    \version $Id$       
    \brief   AS-DCP library, AES wrapper
*/


#include <assert.h>
#include <AS_DCP.h>

using namespace ASDCP;
const int KEY_SIZE_BITS = 128;


#ifndef ASDCP_WITHOUT_OPENSSL
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/err.h>

void
print_ssl_error()
{
  char err_buf[256];
  unsigned long errval = ERR_get_error();
  DefaultLogSink().Error("OpenSSL: %s\n", ERR_error_string(errval, err_buf));
}

#endif


//------------------------------------------------------------------------------------------

#ifdef ASDCP_WITHOUT_OPENSSL
class ASDCP::AESEncContext::h__AESContext
#else
class ASDCP::AESEncContext::h__AESContext : public AES_KEY
#endif
{
public:
  byte_t m_IVec[CBC_BLOCK_SIZE];
};


ASDCP::AESEncContext::AESEncContext()  {}
ASDCP::AESEncContext::~AESEncContext() {}

// Initializes Rijndael CBC encryption context.
// Returns error if the key argument is NULL.
ASDCP::Result_t
ASDCP::AESEncContext::InitKey(const byte_t* key)
{
  ASDCP_TEST_NULL(key);

  if ( m_Context )
    return RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  m_Context = new h__AESContext;

  if ( AES_set_encrypt_key(key, KEY_SIZE_BITS, m_Context) )
    {
      print_ssl_error();
      return RESULT_CRYPT_INIT;
    }

  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}


// Set the value of the 16 byte CBC Initialization Vector. This operation may be performed
// any number of times for a given key.
// Returns error if the i_vec argument is NULL.
ASDCP::Result_t
ASDCP::AESEncContext::SetIVec(const byte_t* i_vec)
{
  ASDCP_TEST_NULL(i_vec);

  if ( ! m_Context )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  memcpy(m_Context->m_IVec, i_vec, CBC_BLOCK_SIZE);
  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}


// Retrieve the value of the  16 byte CBC Initialization Vector.
// Returns error if the i_vec argument is NULL.
ASDCP::Result_t
ASDCP::AESEncContext::GetIVec(byte_t* i_vec) const
{
  ASDCP_TEST_NULL(i_vec);

  if ( ! m_Context )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  memcpy(i_vec, m_Context->m_IVec, CBC_BLOCK_SIZE);
  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}


// Encrypt a 16 byte block of data.
// Returns error if either argument is NULL.
ASDCP::Result_t
ASDCP::AESEncContext::EncryptBlock(const byte_t* pt_buf, byte_t* ct_buf, ui32_t block_size)
{
  ASDCP_TEST_NULL(pt_buf);
  ASDCP_TEST_NULL(ct_buf);
  assert(block_size > 0);
  assert( block_size % CBC_BLOCK_SIZE == 0 );

  if ( m_Context.empty() )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  h__AESContext* Ctx = m_Context;
  byte_t tmp_buf[CBC_BLOCK_SIZE];
  const byte_t* in_p = pt_buf;
  byte_t* out_p = ct_buf;

  while ( block_size )
    {
      // xor with the previous block
      for ( ui32_t i = 0; i < CBC_BLOCK_SIZE; i++ )
	tmp_buf[i] = in_p[i] ^ Ctx->m_IVec[i]; 
          
      AES_encrypt(tmp_buf, Ctx->m_IVec, Ctx);
      memcpy(out_p, Ctx->m_IVec, CBC_BLOCK_SIZE);

      in_p += CBC_BLOCK_SIZE;
      out_p += CBC_BLOCK_SIZE;
      block_size -= CBC_BLOCK_SIZE;
    }

  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}


//------------------------------------------------------------------------------------------

#ifdef ASDCP_WITHOUT_OPENSSL
class ASDCP::AESDecContext::h__AESContext
#else
class ASDCP::AESDecContext::h__AESContext : public AES_KEY
#endif
{
public:
  byte_t m_IVec[CBC_BLOCK_SIZE];
};

ASDCP::AESDecContext::AESDecContext()  {}
ASDCP::AESDecContext::~AESDecContext() {}


// Initializes Rijndael CBC decryption context.
// Returns error if the key argument is NULL.
ASDCP::Result_t
ASDCP::AESDecContext::InitKey(const byte_t* key)
{
  ASDCP_TEST_NULL(key);

  if ( m_Context )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  m_Context = new h__AESContext;

  if ( AES_set_decrypt_key(key, KEY_SIZE_BITS, m_Context) )
    {
      print_ssl_error();
      return RESULT_CRYPT_INIT;
    }

  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}

// Initializes 16 byte CBC Initialization Vector. This operation may be performed
// any number of times for a given key.
// Returns error if the i_vec argument is NULL.
ASDCP::Result_t
ASDCP::AESDecContext::SetIVec(const byte_t* i_vec)
{
  ASDCP_TEST_NULL(i_vec);

  if ( ! m_Context )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  memcpy(m_Context->m_IVec, i_vec, CBC_BLOCK_SIZE);
  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}

// Decrypt a 16 byte block of data.
// Returns error if either argument is NULL.
ASDCP::Result_t
ASDCP::AESDecContext::DecryptBlock(const byte_t* ct_buf, byte_t* pt_buf, ui32_t block_size)
{
  ASDCP_TEST_NULL(ct_buf);
  ASDCP_TEST_NULL(pt_buf);
  assert(block_size > 0);
  assert( block_size % CBC_BLOCK_SIZE == 0 );

  if ( m_Context.empty() )
    return  RESULT_INIT;

#ifndef ASDCP_WITHOUT_OPENSSL
  register h__AESContext* Ctx = m_Context;

  const byte_t* in_p = ct_buf;
  byte_t* out_p = pt_buf;

  while ( block_size )
    {
      AES_decrypt(in_p, out_p, Ctx);  

      for ( ui32_t i = 0; i < CBC_BLOCK_SIZE; i++ )
	out_p[i] ^= Ctx->m_IVec[i];

      memcpy(Ctx->m_IVec, in_p, CBC_BLOCK_SIZE);

      in_p += CBC_BLOCK_SIZE;
      out_p += CBC_BLOCK_SIZE;
      block_size -= CBC_BLOCK_SIZE;
    }

  return RESULT_OK;
#else // ASDCP_WITHOUT_OPENSSL
  return RESULT_FAIL;
#endif // ASDCP_WITHOUT_OPENSSL
}

//------------------------------------------------------------------------------------------


static byte_t ipad[KeyLen] = { 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
			       0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36 };

static byte_t opad[KeyLen] = { 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
			       0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c };

class HMACContext::h__HMACContext
{
#ifndef ASDCP_WITHOUT_OPENSSL
  SHA_CTX m_SHA;
#endif // ASDCP_WITHOUT_OPENSSL
  byte_t m_key[KeyLen];
  ASDCP_NO_COPY_CONSTRUCT(h__HMACContext);

public:
  byte_t sha_value[HMAC_SIZE];
  bool   m_Final;

  h__HMACContext() : m_Final(false) {}
  ~h__HMACContext() {}

  //
  void SetKey(const byte_t* key)
  {
    static byte_t key_nonce[KeyLen] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 
					0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
#ifndef ASDCP_WITHOUT_OPENSSL
    byte_t sha_buf[SHA_DIGEST_LENGTH];

    // 7.10: MICKey = trunc( SHA1 ( key, key_nonce ) )
    SHA_CTX SHA;
    SHA1_Init(&SHA);
    SHA1_Update(&SHA, key, KeyLen);
    SHA1_Update(&SHA, key_nonce, KeyLen);
    SHA1_Final(sha_buf, &SHA);
    memcpy(m_key, sha_buf, KeyLen);

    Reset();
#endif // ASDCP_WITHOUT_OPENSSL
  }

  void
  Reset()
  {
#ifndef ASDCP_WITHOUT_OPENSSL
    byte_t xor_buf[KeyLen];
    memset(sha_value, 0, HMAC_SIZE);
    m_Final = false;
    SHA1_Init(&m_SHA);

    // H(K XOR opad, H(K XOR ipad, text))
    //                 ^^^^^^^^^^
    for ( ui32_t i = 0; i < KeyLen; i++ )
      xor_buf[i] = m_key[i] ^ ipad[i];

    SHA1_Update(&m_SHA, xor_buf, KeyLen);
#endif // ASDCP_WITHOUT_OPENSSL
  }

  //
  void
  Update(const byte_t* buf, ui32_t buf_len)
  {
#ifndef ASDCP_WITHOUT_OPENSSL
    // H(K XOR opad, H(K XOR ipad, text))
    //                             ^^^^
    SHA1_Update(&m_SHA, buf, buf_len);
#endif // ASDCP_WITHOUT_OPENSSL
  }

  //
  void
  Finalize()
  {
#ifndef ASDCP_WITHOUT_OPENSSL
    // H(K XOR opad, H(K XOR ipad, text))
    // ^^^^^^^^^^^^^^^
    SHA1_Final(sha_value, &m_SHA);

    SHA_CTX SHA;
    SHA1_Init(&SHA);

    byte_t xor_buf[KeyLen];

    for ( ui32_t i = 0; i < KeyLen; i++ )
      xor_buf[i] = m_key[i] ^ opad[i];
    
    SHA1_Update(&SHA, xor_buf, KeyLen);
    SHA1_Update(&SHA, sha_value, HMAC_SIZE);

    SHA1_Final(sha_value, &SHA);
    m_Final = true;
#endif // ASDCP_WITHOUT_OPENSSL
  }
};


HMACContext::HMACContext()
{
}

HMACContext::~HMACContext()
{
}


//
Result_t
HMACContext::InitKey(const byte_t* key)
{
  ASDCP_TEST_NULL(key);

  m_Context = new h__HMACContext;
  m_Context->SetKey(key);
  return RESULT_OK;
}


//
void
HMACContext::Reset()
{
  if ( ! m_Context.empty() )
    m_Context->Reset();
}


//
Result_t
HMACContext::Update(const byte_t* buf, ui32_t buf_len)
{
  ASDCP_TEST_NULL(buf);

  if ( m_Context.empty() || m_Context->m_Final )
    return RESULT_INIT;

  m_Context->Update(buf, buf_len);
  return RESULT_OK;
}


//
Result_t
HMACContext::Finalize()
{
  if ( m_Context.empty() || m_Context->m_Final )
    return RESULT_INIT;
  
  m_Context->Finalize();
  return RESULT_OK;
}


//
Result_t
HMACContext::GetHMACValue(byte_t* buf) const
{
  ASDCP_TEST_NULL(buf);

  if ( m_Context.empty() || ! m_Context->m_Final )
    return RESULT_INIT;

  memcpy(buf, m_Context->sha_value, HMAC_SIZE);
  return RESULT_OK;
}


//
Result_t
HMACContext::TestHMACValue(const byte_t* buf) const
{
  ASDCP_TEST_NULL(buf);

  if ( m_Context.empty() || ! m_Context->m_Final )
    return RESULT_INIT;
  
  return ( memcmp(buf, m_Context->sha_value, HMAC_SIZE) == 0 ) ? RESULT_OK : RESULT_HMACFAIL;
}



//
// end AS_DCP_AES.cpp
//
