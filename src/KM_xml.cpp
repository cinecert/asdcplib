/*
Copyright (c) 2005-2008, John Hurst
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
/*! \file    KM_xml.cpp
    \version $Id$
    \brief   XML writer
*/

#include <KM_xml.h>
#include <KM_log.h>
#include <KM_mutex.h>
#include <stack>
#include <map>

//#undef HAVE_EXPAT
//#define HAVE_XERCES_C

#ifdef HAVE_EXPAT
# ifdef HAVE_XERCES_C
#  error "Both HAVE_EXPAT and HAVE_XERCES_C defined"
# endif
#include <expat.h>
#endif

#ifdef HAVE_XERCES_C
# ifdef HAVE_EXPAT
#  error "Both HAVE_EXPAT and HAVE_XERCES_C defined"
# endif

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/XMLPScanToken.hpp>


XERCES_CPP_NAMESPACE_USE 
#endif

using namespace Kumu;


class ns_map : public std::map<std::string, XMLNamespace*>
{
public:
  ~ns_map()
  {
    while ( ! empty() )
      {
	ns_map::iterator ni = begin();
	delete ni->second;
	erase(ni);
      }
  }
};


Kumu::XMLElement::XMLElement(const char* name) : m_Namespace(0), m_NamespaceOwner(0)
{
  m_Name = name;
}

Kumu::XMLElement::~XMLElement()
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    delete *i;

  delete (ns_map*)m_NamespaceOwner;
}

//
void
Kumu::XMLElement::SetAttr(const char* name, const char* value)
{
  NVPair TmpVal;
  TmpVal.name = name;
  TmpVal.value = value;

  m_AttrList.push_back(TmpVal);
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChild(Kumu::XMLElement* element)
{
  m_ChildList.push_back(element); // takes posession!
  return element;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChild(const char* name)
{
  XMLElement* tmpE = new XMLElement(name);
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const std::string& value)
{
  return AddChildWithContent(name, value.c_str());
}

//
void
Kumu::XMLElement::AppendBody(const std::string& value)
{
  m_Body += value;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const char* value)
{
  assert(name);
  assert(value);
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value)
{
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = prefix;
  tmpE->m_Body += value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
void
Kumu::XMLElement::AddComment(const char* value)
{
  m_Body += "  <!-- ";
  m_Body += value;
  m_Body += " -->\n";
}

//
void
Kumu::XMLElement::Render(std::string& outbuf) const
{
  outbuf = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  RenderElement(outbuf, 0);
}

//
inline void
add_spacer(std::string& outbuf, i32_t depth)
{
  while ( depth-- )
    outbuf+= "  ";
}

//
void
Kumu::XMLElement::RenderElement(std::string& outbuf, ui32_t depth) const
{
  add_spacer(outbuf, depth);

  outbuf += "<";
  outbuf += m_Name;

  // render attributes
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); i++ )
    {
      outbuf += " ";
      outbuf += (*i).name;
      outbuf += "=\"";
      outbuf += (*i).value;
      outbuf += "\"";
    }

  outbuf += ">";

  // body contents and children
  if ( ! m_ChildList.empty() )
    {
      outbuf += "\n";

      // render body
      if ( m_Body.length() > 0 )
	outbuf += m_Body;

      for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
	(*i)->RenderElement(outbuf, depth + 1);

      add_spacer(outbuf, depth);
    }
  else if ( m_Body.length() > 0 )
    {
      outbuf += m_Body;
    }

  outbuf += "</";
  outbuf += m_Name;
  outbuf += ">\n";
}

//
bool
Kumu::XMLElement::HasName(const char* name) const
{
  if ( name == 0 || *name == 0 )
    return false;

  return (m_Name == name);
}


void
Kumu::XMLElement::SetName(const char* name)
{
  if ( name != 0)
    m_Name = name;
}

//
const char*
Kumu::XMLElement::GetAttrWithName(const char* name) const
{
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); i++ )
    {
      if ( (*i).name == name )
	return (*i).value.c_str();
    }

  return 0;
}

//
Kumu::XMLElement*
Kumu::XMLElement::GetChildWithName(const char* name) const
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	return *i;
    }

  return 0;
}

//
const Kumu::ElementList&
Kumu::XMLElement::GetChildrenWithName(const char* name, ElementList& outList) const
{
  assert(name);
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	outList.push_back(*i);

      if ( ! (*i)->m_ChildList.empty() )
	(*i)->GetChildrenWithName(name, outList);
    }

  return outList;
}

//----------------------------------------------------------------------------------------------------

#ifdef HAVE_EXPAT


class ExpatParseContext
{
  KM_NO_COPY_CONSTRUCT(ExpatParseContext);
  ExpatParseContext();
public:
  ns_map*                  Namespaces;
  std::stack<XMLElement*>  Scope;
  XMLElement*              Root;

  ExpatParseContext(XMLElement* root) : Root(root) {
    Namespaces = new ns_map;
    assert(Root);
  }

  ~ExpatParseContext() {}
};

// expat wrapper functions
// 
static void
xph_start(void* p, const XML_Char* name, const XML_Char** attrs)
{
  assert(p);  assert(name);  assert(attrs);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  XMLElement* Element;

  const char* ns_root = name;
  const char* local_name = strchr(name, '|');
  if ( local_name != 0 )
    name = local_name + 1;

  if ( Ctx->Scope.empty() )
    {
      Ctx->Scope.push(Ctx->Root);
    }
  else
    {
      Element = Ctx->Scope.top();
      Ctx->Scope.push(Element->AddChild(name));
    }

  Element = Ctx->Scope.top();
  Element->SetName(name);

  // map the namespace
  std::string key;
  if ( ns_root != name )
    key.assign(ns_root, name - ns_root - 1);
  
  ns_map::iterator ni = Ctx->Namespaces->find(key);
  if ( ni != Ctx->Namespaces->end() )
    Element->SetNamespace(ni->second);

  // set attributes
  for ( int i = 0; attrs[i] != 0; i += 2 )
    {
      if ( ( local_name = strchr(attrs[i], '|') ) == 0 )
	local_name = attrs[i];
      else
	local_name++;

      Element->SetAttr(local_name, attrs[i+1]);
    }
}

//
static void
xph_end(void* p, const XML_Char* name)
{
  assert(p);  assert(name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  Ctx->Scope.pop();
}

//
static void
xph_char(void* p, const XML_Char* data, int len)
{
  assert(p);  assert(data);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;

  if ( len > 0 )
    {
      std::string tmp_str;
      tmp_str.assign(data, len);
      Ctx->Scope.top()->AppendBody(tmp_str);
    }
}

//
void
xph_namespace_start(void* p, const XML_Char* ns_prefix, const XML_Char* ns_name)
{
  assert(p);  assert(ns_name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  
  if ( ns_prefix == 0 )
    ns_prefix = "";

  ns_map::iterator ni = Ctx->Namespaces->find(ns_name);

  if  ( ni != Ctx->Namespaces->end() )
    {
      if ( ni->second->Name() != std::string(ns_name) )
	{
	  DefaultLogSink().Error("Duplicate prefix: %s\n", ns_prefix);
	  return;
	}
    }
  else
    {
      XMLNamespace* Namespace = new XMLNamespace(ns_prefix, ns_name);
      Ctx->Namespaces->insert(ns_map::value_type(ns_name, Namespace));
    }
}

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  XML_Parser Parser = XML_ParserCreateNS("UTF-8", '|');

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  ExpatParseContext Ctx(this);
  XML_SetUserData(Parser, (void*)&Ctx);
  XML_SetElementHandler(Parser, xph_start, xph_end);
  XML_SetCharacterDataHandler(Parser, xph_char);
  XML_SetStartNamespaceDeclHandler(Parser, xph_namespace_start);

  if ( ! XML_Parse(Parser, document.c_str(), document.size(), 1) )
    {
      XML_ParserFree(Parser);
      DefaultLogSink().Error("XML Parse error on line %d: %s\n",
			     XML_GetCurrentLineNumber(Parser),
			     XML_ErrorString(XML_GetErrorCode(Parser)));
      return false;
    }

  XML_ParserFree(Parser);

  if ( ! Ctx.Namespaces->empty() )
    m_NamespaceOwner = (void*)Ctx.Namespaces;

  return true;
}

//------------------------------------------------------------------------------------------

struct xph_test_wrapper
{
  XML_Parser Parser;
  bool  Status;

  xph_test_wrapper(XML_Parser p) : Parser(p), Status(false) {}
};

// expat wrapper functions, map callbacks to IASAXHandler
// 
static void
xph_test_start(void* p, const XML_Char* name, const XML_Char** attrs)
{
  assert(p);
  xph_test_wrapper* Wrapper = (xph_test_wrapper*)p;

  Wrapper->Status = true;
  XML_StopParser(Wrapper->Parser, false);
}


//
bool
Kumu::StringIsXML(const char* document, ui32_t len)
{
  if ( document == 0 )
    return false;

  if ( len == 0 )
    len = strlen(document);

  XML_Parser Parser = XML_ParserCreate("UTF-8");

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  xph_test_wrapper Wrapper(Parser);
  XML_SetUserData(Parser, (void*)&Wrapper);
  XML_SetStartElementHandler(Parser, xph_test_start);

  XML_Parse(Parser, document, len, 1);
  XML_ParserFree(Parser);
  return Wrapper.Status;
}

#endif

//----------------------------------------------------------------------------------------------------

#ifdef HAVE_XERCES_C

static Mutex sg_Lock;
static bool  sg_xml_init = false;


//
void
asdcp_init_xml_dom()
{
  if ( ! sg_xml_init )
    {
      AutoMutex AL(sg_Lock);

      if ( ! sg_xml_init )
	{
	  try
	    {
	      XMLPlatformUtils::Initialize();
	      sg_xml_init = true;
	    }
	  catch (const XMLException &e)
	    {
	      DefaultLogSink().Error("Xerces initialization error: %s\n", e.getMessage());
	    }
  	}
    }
}


//
class MyTreeHandler : public HandlerBase
{
  ns_map*                  m_Namespaces;
  std::stack<XMLElement*>  m_Scope;
  XMLElement*              m_Root;

public:
  MyTreeHandler(XMLElement* root) : m_Namespaces(0), m_Root(root) {
    assert(m_Root);
    m_Namespaces = new ns_map;
  }

  ~MyTreeHandler() {
    delete m_Namespaces;
  }

  ns_map* TakeNamespaceMap() {
    if ( m_Namespaces == 0 || m_Namespaces->empty() )
      return 0;

    ns_map* ret = m_Namespaces;
    m_Namespaces = 0;
    return ret;
  }

  //
  void AddNamespace(const char* ns_prefix, const char* ns_name)
  {
    assert(ns_prefix);
    assert(ns_name);

    if ( ns_prefix[0] == ':' )
      {
	ns_prefix++;
      }
    else
      {
	assert(ns_prefix[0] == 0);
	ns_prefix = "";
      }

    ns_map::iterator ni = m_Namespaces->find(ns_name);

    if  ( ni != m_Namespaces->end() )
      {
	if ( ni->second->Name() != std::string(ns_name) )
	  {
	    DefaultLogSink().Error("Duplicate prefix: %s\n", ns_prefix);
	    return;
	  }
      }
    else
      {
	XMLNamespace* Namespace = new XMLNamespace(ns_prefix, ns_name);
	m_Namespaces->insert(ns_map::value_type(ns_prefix, Namespace));
      }

    assert(!m_Namespaces->empty());
  }

  //
  void startElement(const XMLCh* const x_name,
		    XERCES_CPP_NAMESPACE::AttributeList& attributes)
  {
    assert(x_name);

    const char* tx_name = XMLString::transcode(x_name);
    const char* name = tx_name;
    XMLElement* Element;
    const char* ns_root = name;
    const char* local_name = strchr(name, ':');

    if ( local_name != 0 )
      name = local_name + 1;

    if ( m_Scope.empty() )
      {
	m_Scope.push(m_Root);
      }
    else
      {
	Element = m_Scope.top();
	m_Scope.push(Element->AddChild(name));
      }

    Element = m_Scope.top();
    Element->SetName(name);

    // set attributes
    ui32_t a_len = attributes.getLength();

    for ( ui32_t i = 0; i < a_len; i++)
      {
	const XMLCh* aname = attributes.getName(i);
	const XMLCh* value = attributes.getValue(i);
	assert(aname);
	assert(value);

	char* x_aname = XMLString::transcode(aname);
	char* x_value = XMLString::transcode(value);

	if ( strncmp(x_aname, "xmlns", 5) == 0 )
	  AddNamespace(x_aname+5, x_value);

	if ( ( local_name = strchr(x_aname, ':') ) == 0 )
	  local_name = x_aname;
	else
	  local_name++;

	Element->SetAttr(local_name, x_value);

	XMLString::release(&x_aname);
	XMLString::release(&x_value);
      }

    // map the namespace
    std::string key;
    if ( ns_root != name )
      key.assign(ns_root, name - ns_root - 1);
  
    ns_map::iterator ni = m_Namespaces->find(key);
    if ( ni != m_Namespaces->end() )
      Element->SetNamespace(ni->second);

    XMLString::release((char**)&tx_name);
  }

  void endElement(const XMLCh *const name) {
    m_Scope.pop();
  }

  void characters(const XMLCh *const chars, const unsigned int length)
  {
    if ( length > 0 )
      {
	char* text = XMLString::transcode(chars);
	m_Scope.top()->AppendBody(text);
	XMLString::release(&text);
      }
  }
};

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  if ( document.empty() )
    return false;

  asdcp_init_xml_dom();

  int errorCount = 0;
  SAXParser* parser = new SAXParser();
  parser->setDoValidation(true);
  parser->setDoNamespaces(true);    // optional

  MyTreeHandler* docHandler = new MyTreeHandler(this);
  ErrorHandler* errHandler = (ErrorHandler*)docHandler;
  parser->setDocumentHandler(docHandler);

  try
    {
      MemBufInputSource xmlSource(reinterpret_cast<const XMLByte*>(document.c_str()),
				  static_cast<const unsigned int>(document.size()),
				  "pidc_rules_file");

      parser->parse(xmlSource);
    }
  catch (const XMLException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s\n", message);
      XMLString::release(&message);
      errorCount++;
    }
  catch (const SAXParseException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s at line %d\n", message, e.getLineNumber());
      XMLString::release(&message);
      errorCount++;
    }
  catch (...)
    {
      DefaultLogSink().Error("Unexpected XML parser error\n");
      errorCount++;
    }
  
  if ( errorCount == 0 )
    m_NamespaceOwner = (void*)docHandler->TakeNamespaceMap();

  delete parser;
  delete docHandler;

  return errorCount > 0 ? false : true;
}

//
bool
Kumu::StringIsXML(const char* document, ui32_t len)
{
  if ( document == 0 || *document == 0 )
    return false;

  asdcp_init_xml_dom();

  if ( len == 0 )
    len = strlen(document);

  SAXParser parser;
  XMLPScanToken token;
  bool status = false;

  try
    {
      MemBufInputSource xmlSource(reinterpret_cast<const XMLByte*>(document),
				  static_cast<const unsigned int>(len),
				  "pidc_rules_file");

      if ( parser.parseFirst(xmlSource, token) )
	{
	  if ( parser.parseNext(token) )
	    status = true;
	}
    }
  catch (...)
    {
    }
  
  return status;
}


#endif

//----------------------------------------------------------------------------------------------------

#if ! defined(HAVE_EXPAT) && ! defined(HAVE_XERCES_C)

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  DefaultLogSink().Error("asdcplib compiled without XML parser support.\n");
  return false;
}

//
bool
Kumu::StringIsXML(const char* document, ui32_t len)
{
  DefaultLogSink().Error("Kumu compiled without XML parser support.\n");
  return false;
}

#endif


//
// end KM_xml.cpp
//
