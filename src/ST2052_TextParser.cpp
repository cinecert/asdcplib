/*
Copyright (c) 2013-2014, John Hurst
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
/*! \file    ST2052_TimedText.cpp
    \version $Id$       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/


#include "AS_02_internal.h"
#include "KM_xml.h"

using namespace Kumu;
using namespace ASDCP;

using Kumu::DefaultLogSink;

// TODO: 
const char* c_dcst_namespace_name = "http://www.smpte-ra.org/schemas/428-7/2007/DCST";

//------------------------------------------------------------------------------------------

typedef std::map<Kumu::UUID, ASDCP::TimedText::MIMEType_t> ResourceTypeMap_t;

class AS_02::TimedText::ST2052_TextParser::h__TextParser
{
  XMLElement  m_Root;
  ResourceTypeMap_t m_ResourceTypes;
  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__TextParser);

public:
  std::string m_Filename;
  std::string m_XMLDoc;
  TimedTextDescriptor  m_TDesc;
  ASDCP::mem_ptr<ASDCP::TimedText::LocalFilenameResolver> m_DefaultResolver;

  h__TextParser() : m_Root("**ParserRoot**")
  {
    memset(&m_TDesc.AssetID, 0, UUIDlen);
  }

  ~h__TextParser() {}

  ASDCP::TimedText::IResourceResolver* GetDefaultResolver()
  {
    if ( m_DefaultResolver.empty() )
      {
	ASDCP::TimedText::LocalFilenameResolver *resolver = new ASDCP::TimedText::LocalFilenameResolver;
	resolver->OpenRead(PathDirname(m_Filename));
	m_DefaultResolver = resolver;
      }
    
    return m_DefaultResolver;
  }

  Result_t OpenRead(const std::string& filename);
  Result_t OpenRead(const std::string& xml_doc, const std::string& filename);
  Result_t ReadAncillaryResource(const UUID& uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
				 const ASDCP::TimedText::IResourceResolver& Resolver) const;
};

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead(const std::string& filename)
{
  Result_t result = ReadFileIntoString(filename, m_XMLDoc);

  if ( KM_SUCCESS(result) )
    {
      m_Filename = filename;
      result = OpenRead();
    }

  return result;
}

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead(const std::string& xml_doc, const std::string& filename)
{
  m_XMLDoc = xml_doc;
  m_Filename = filename;
  return OpenRead();
}

//
Result_t
AS_02::TimedText::ST2052_TextParser::h__TextParser::OpenRead()
{
  if ( ! m_Root.ParseString(m_XMLDoc.c_str()) )
    return RESULT_FORMAT;

  m_TDesc.EncodingName = "UTF-8"; // the XML parser demands UTF-8
  m_TDesc.ResourceList.clear();
  m_TDesc.ContainerDuration = 0;
  const XMLNamespace* ns = m_Root.Namespace();

  if ( ns == 0 )
    {
      DefaultLogSink(). Warn("Document has no namespace name, assuming %s\n", c_dcst_namespace_name);
      m_TDesc.NamespaceName = c_dcst_namespace_name;
    }
  else
    {
      m_TDesc.NamespaceName = ns->Name();
    }

  return RESULT_OK;
}


//------------------------------------------------------------------------------------------

AS_02::TimedText::ST2052_TextParser::ST2052_TextParser()
{
}

AS_02::TimedText::ST2052_TextParser::~ST2052_TextParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::OpenRead(const std::string& filename) const
{
  const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = new h__TextParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = 0;

  return result;
}

// Parses an XML document to provide a complete set of stream metadata for the MXFWriter below.
Result_t
AS_02::TimedText::ST2052_TextParser::OpenRead(const std::string& xml_doc, const std::string& filename) const
{
  const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = new h__TextParser;

  Result_t result = m_Parser->OpenRead(xml_doc, filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<AS_02::TimedText::ST2052_TextParser*>(this)->m_Parser = 0;

  return result;
}

//
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::FillTimedTextDescriptor(TimedTextDescriptor& TDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  TDesc = m_Parser->m_TDesc;
  return RESULT_OK;
}

// Reads the complete Timed Text Resource into the given string.
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::ReadTimedTextResource(std::string& s) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  s = m_Parser->m_XMLDoc;
  return RESULT_OK;
}

//
ASDCP::Result_t
AS_02::TimedText::ST2052_TextParser::ReadAncillaryResource(const Kumu::UUID& uuid, ASDCP::TimedText::FrameBuffer& FrameBuf,
							   const ASDCP::TimedText::IResourceResolver* Resolver) const
{
  return RESULT_NOTIMPL;
}


//
// end ST2052_TextParser.cpp
//
