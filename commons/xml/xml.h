/* 
 * xml.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xml.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2010 Red Snapper Ltd. http://www.redsnapper.net
 * The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef OBYX_XML_H
#define OBYX_XML_H

//List of entire API used by Obyx.
/*

namespace {
	typedef unsigned short XMLCh;
	typedef unsigned char XMLByte;
}
namespace xercesc {
	
#define	chNull 0
	
//forward looking declarations
	class DOMDocument;
	class DOMNode;
	class DOMElement;
	class DynamicContext;
	
	class TranscodeToStr {
	public:
		TranscodeToStr(const XMLCh*,const char*);
		TranscodeToStr(const XMLCh*,unsigned long,const char*);
		operator XMLByte*();
		XMLByte* adopt();
	};
	
	class TranscodeFromStr {
	public:
		TranscodeFromStr(const XMLByte*,unsigned long,const char*);
		operator XMLCh*();
		XMLCh* adopt();
	};
	
	class XMLUni {
	public:
		static XMLCh* fgXMLChEncodingString;
		static XMLCh* fgUTF8EncodingString;
		static XMLCh* fgDOMErrorHandler;
		static XMLCh* fgDOMWRTDiscardDefaultContent;
		static XMLCh* fgDOMWRTFormatPrettyPrint;
		static XMLCh* fgDOMXMLDeclaration;
		static XMLCh* fgDOMWRTBOM;
		static XMLCh* fgDOMValidateIfSchema;
		static XMLCh* fgDOMValidate;
		static XMLCh* fgDOMResourceResolver;
		static XMLCh* fgXercesValidationErrorAsFatal;
        static XMLCh* fgXercesSkipDTDValidation;
        static XMLCh* fgXercesUserAdoptsDOMDocument;
        static XMLCh* fgXercesContinueAfterFatalError;
		static XMLCh* fgXercesCacheGrammarFromParse;
		static XMLCh* fgXercesUseCachedGrammarInParse;
		static XMLCh* fgDOMElementContentWhitespace;
        static XMLCh* fgDOMNamespaces;
		static XMLCh* fgXercesSchema;
		static XMLCh* fgXercesIgnoreAnnotations;
		static XMLCh* fgXercesLoadExternalDTD;
		static XMLCh* fgXercesIgnoreCachedDTD;
		static XMLCh* fgXercesIdentityConstraintChecking;
		static XMLCh* fgDOMDatatypeNormalization; 
		static XMLCh* fgXercesDOMHasPSVIInfo;
	};
	class DOMErrorHandler {};
	class XMLFormatTarget {
	public:
		virtual ~XMLFormatTarget();
	};
	class MemBufFormatTarget: public XMLFormatTarget {
	public:
		XMLCh* getRawBuffer();
	};
	class Grammar {
	public:
		enum GrammarType {DTDGrammarType, SchemaGrammarType};
	};
	class DOMLSOutput {
	public:
		void setByteStream(MemBufFormatTarget*);
		void setEncoding(XMLCh*);
		void release();	
	};
	class DOMConfiguration  {
	public:
		void setParameter(const XMLCh*,void *);
		void setParameter(const XMLCh*,bool);
		void setParameter(const XMLCh*,XMLCh*);
		bool canSetParameter(const XMLCh*,bool);
	};
	
	class DOMImplementation  {
	public:
		DOMDocument* createDocument() const;
	};
	class DOMLSResourceResolver {};
	class DOMLSInput {
	public:
		void setPublicId(const XMLCh*);
		void setSystemId(const XMLCh*);
		void setByteStream(DOMLSInput*);
		void setEncoding(const XMLCh*); //This must be done.
		void release();
		virtual ~DOMLSInput();
	};
	class MemBufInputSource : public DOMLSInput {
	public:
		MemBufInputSource(XMLByte*,unsigned long,const XMLCh*,bool = true);
		void setCopyBufToStream(bool);
		virtual ~MemBufInputSource();
	};
	class DOMLSParser {
	public:
		enum ActionType {ACTION_APPEND_AS_CHILDREN,ACTION_INSERT_BEFORE,ACTION_INSERT_AFTER,ACTION_REPLACE,ACTION_REPLACE_CHILDREN};
		DOMConfiguration* getDomConfig();
		void resetDocumentPool();
		void resetCachedGrammarPool();
		DOMDocument* parse(DOMLSInput*);
		DOMDocument* parseURI(char*);
		Grammar* loadGrammar(DOMLSInput*, Grammar::GrammarType, bool = false);
		void release();
	};
	class DOMLSSerializer {
	public:
		DOMConfiguration* getDomConfig();
		static bool write(const DOMNode*,DOMLSOutput*);
		void release();
	};
	class DOMImplementationLS : public DOMImplementation {
	public:
		enum mode {MODE_SYNCHRONOUS};
		DOMLSOutput* createLSOutput();
		DOMLSSerializer* createLSSerializer();
		DOMLSParser* createLSParser(mode,void*);
		DOMLSInput* createLSInput();
	};
	class DOMImplementationRegistry {
		public:
			static DOMImplementation* getDOMImplementation(const XMLCh*);
	};
	class DOMNamedNodeMap {
	public:
		unsigned getLength() const;
		void* item(unsigned long) const;
	};
	class DOMXPathExpression {};
	class DOMNode {
	public:
		enum NodeType {ELEMENT_NODE,DOCUMENT_NODE,ATTRIBUTE_NODE,NOTATION_NODE,
			TEXT_NODE,CDATA_SECTION_NODE,ENTITY_NODE,ENTITY_REFERENCE_NODE,
			DOCUMENT_FRAGMENT_NODE,PROCESSING_INSTRUCTION_NODE,COMMENT_NODE,DOCUMENT_TYPE_NODE};
		const XMLCh* getLocalName() const;
		DOMNode* getFirstChild() const;
		DOMNode* getPreviousSibling() const;
		DOMNode* getNextSibling() const;
		DOMNode* getParentNode() const;
		NodeType getNodeType() const;
		const XMLCh* getNodeValue() const;
		DOMDocument* getOwnerDocument() const;
		DOMNode* cloneNode(bool) const;
		const XMLCh* getTextContent() const;
		const XMLCh* getNamespaceURI() const;
		const XMLCh* getPrefix() const;
		const XMLCh* getNodeName() const;
		void setNodeValue(const XMLCh*);
		DOMNode *removeChild(DOMNode*);
		void appendChild(DOMNode*);
		void insertBefore(DOMNode*,DOMNode*);
		bool isEqualNode(DOMNode*);
		void release();
		virtual ~DOMNode();
	};
	class DOMProcessingInstruction : public DOMNode {
	};
	class DOMAttr : public DOMNode {
	public:
		const XMLCh* getName() const;
		DOMElement* getOwnerElement() const;
	};
	class DOMText : public DOMNode {
	public:
		const XMLCh* getName() const;
	};
	class DOMElement : public DOMNode {
	public:
		DOMNamedNodeMap* getAttributes() const;
		void setAttributeNode(const DOMAttr*);
		void removeAttribute(const XMLCh*);
		DOMAttr* removeAttributeNode(DOMAttr*);
		DOMAttr* getAttributeNode(const XMLCh*);
	};
	class DOMDocumentFragment : public DOMNode {
	};
	class DOMDocument : public DOMNode {
		public:
		DOMElement* getDocumentElement() const;
		DOMProcessingInstruction* createProcessingInstruction(const XMLCh*,void*);
		DOMNode* importNode(const DOMNode*,bool = false);
		DOMText* createTextNode(const XMLCh*);
		DOMAttr* createAttribute(const XMLCh*);
		DOMNode* createComment(const XMLCh*);
		void appendChild(DOMNode*);
		void release();
		void normalize();
		DOMDocumentFragment* createDocumentFragment();
	};
	class DOMLocator {
		public:
			DOMNode* getRelatedNode() const;
	};
	class DOMError {
		public:
			enum Severity {DOM_SEVERITY_WARNING};
			Severity getSeverity() const;
			const XMLCh* getMessage() const;
			DOMLocator* getLocation() const;
		virtual ~DOMError();
	};
	class DOMException: public DOMError {
	public:
		virtual ~DOMException();
	};
	class OutOfMemoryException: public DOMError {
	public:
	};
	class DOMLSException: public DOMException {
	};
	class DOMXPathException: public DOMException {
	};
	class XMLException: public DOMException {
	};
	class XMLUri {
	public:
		XMLUri(const XMLCh*);
		XMLUri(XMLUri*,const XMLCh*);
		XMLCh* getUriText();
	};
	
	class XMLDouble {
	public:
		XMLDouble(const XMLCh*);
		double getValue();
		static void release(void*);
	};
	
	class XMLString {
	public:
		static unsigned stringLen(const char*);
		static unsigned stringLen(const XMLCh*);
		static void binToText(long,XMLCh*,long,long);
		static void release(void*);
	};
//	XQilla
	class XercesConfiguration {
	public:
		enum sort {gXerces};
		static DOMNode* createNode(DOMNode*,DynamicContext*);
	};
	class XQillaPlatformUtils {
	public:
		static void initialize();
		static void terminate();
	};
	class Item {
	public:
		class Ptr {
		public:
			operator Item*();
			bool notNull() const;
			Item* operator->() const;
		};
		XMLCh* asString(DynamicContext*);
		bool isNode() const;
		DOMNode* getInterface(XercesConfiguration::sort);
		virtual ~Item();
	};
	class Sequence {
	public:
		void clear();
		unsigned long getLength();
		Item::Ptr item(unsigned long);
	};
	class DynamicContext {
	public:
		void setNamespaceBinding(const XMLCh*,const XMLCh*);
		XercesConfiguration* getConfiguration();
		void setContextItem(DOMNode*);
	};
	class XQQueryResult {
	public:
		Sequence toSequence(DynamicContext*);
	};
	
	class XQQuery {
	public:
		DynamicContext* createDynamicContext();
		XQQueryResult* execute(DynamicContext*);
	};
	class XQilla {
	public:
		enum Language {XPATH2_FULLTEXT};
		static DynamicContext* createContext(Language,XercesConfiguration*);
		static XQQuery* parse(const XMLCh*,DynamicContext*,void*,long);
	};
	class XQException {
	public:
		const XMLCh* getError() const;
	};
	class XQillaException {
	public:
		const XMLCh* getString() const;
	};
}

template<class TYPE>
class AutoRelease
{
public:
	AutoRelease(TYPE *p)
    : p_(p) {}
	~AutoRelease()
	{
		if(p_ != 0)
			p_->release();
	}
	
	TYPE &operator*() const
	{
		return *p_;
	}
	TYPE *operator->() const
	{
		return p_;
	}
	operator TYPE*() const
	{
		return p_;
	}
	TYPE *get() const
	{
		return p_;
	}
	TYPE *adopt()
	{
		TYPE *tmp = p_;
		p_ = 0;
		return tmp;
	}
	TYPE *swap(TYPE *p)
	{
		TYPE *tmp = p_;
		p_ = p;
		return tmp;
	}
	void set(TYPE *p)
	{
		if(p_ != 0)
			p_->release();
		p_ = p;
	}
	
private:
	AutoRelease(const AutoRelease<TYPE> &);
	AutoRelease<TYPE> &operator=(const AutoRelease<TYPE> &);
	
	TYPE *p_;
};
*/

//xercesc/xqilla proper
#include <xercesc/util/XercesDefs.hpp>
#include <xqilla/xqilla-dom3.hpp>
#include <xqilla/exceptions/XQException.hpp>
#include <xqilla/xqilla-simple.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLUri.hpp>
#include <xercesc/util/XMLDouble.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <xercesc/validators/common/GrammarResolver.hpp>

#include <xercesc/framework/XMLGrammarPoolImpl.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>


#include "commons/xml/xmlerrhandler.h"
#include "commons/xml/xmlrsrchandler.h"
#include "commons/xml/xmlparser.h"
#include "commons/xml/manager.h"
#endif

