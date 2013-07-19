/*
 * xmlparser.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlparser.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <memory>

#include "commons/xml/xml.h"
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"


using namespace Log;

//need to move config stuff into this class.
namespace XML {
	
//load up the schema strings.
#include "grammars/xmlxsd.h"
#include "grammars/messagexsd.h"
#include "grammars/obyxxsd.h"
#include "grammars/oalxsd.h"
#include "grammars/soapxsd.h"
#include "grammars/soapencodingxsd.h"
#include "grammars/xlinkxsd.h"
#include "grammars/svg.h"
#include "grammars/xhtml1dtd.h"
#include "grammars/xhtml5.h"
#include "grammars/wsdlxsd.h"
#include "grammars/wsdlmimexsd.h"
#include "grammars/jsonxsd.h"


	//#define u_str u_str
	//#define UCS2(x) (const XMLCh*)(x)
//	vector<DOMDocumentType*> Parser::doctypes;
	const u_str Parser::memfile = UCS2(L"[xsd]"); // "[xsd]"
	
	Parser::Parser() : errorHandler(NULL),resourceHandler(NULL),impl(NULL),writer(NULL),xfmt(NULL),parser(NULL),validation(false) {
		const u_str imptype = UCS2(L"XPath2 3.0");	// "XPath2 3.0 for xqilla"
//		const u_str imptype = UCS2(L"LS");	// "LS for xercesc"
		impl = DOMImplementationRegistry::getDOMImplementation( imptype.c_str() );
		errorHandler = new XMLErrorHandler();
		resourceHandler = new XMLResourceHandler();	
	}
	
	void Parser::makerw() {
		makeReader();
		makeWriter();
	}
	
	void Parser::writenode(const DOMNode* const& n, u_str& result) {
		if ( n != NULL ) {
			AutoRelease<DOMLSOutput> output(((DOMImplementationLS*)impl)->createLSOutput());	
			MemBufFormatTarget* mbft = new MemBufFormatTarget();
			output->setByteStream(mbft);
			output->setEncoding(XMLUni::fgXMLChEncodingString); //This must be done.
			if ( writer->write(n,output) ) {
				XMLCh* c_result = (XMLCh*)((MemBufFormatTarget*)mbft)->getRawBuffer();
				if (c_result != NULL) {
					// ((MemBufFormatTarget*)mbft)->getLen() is size of buffer, not of result.
					result = u_str(c_result,XMLString::stringLen(c_result));
				}
			}
			delete mbft;
		} // else {} //node was NULL
	}
	
	void Parser::writedoc(const DOMDocument* const& n,u_str& result) {
		if ( n != NULL ) {
			AutoRelease<DOMLSOutput> output(((DOMImplementationLS*)impl)->createLSOutput());	
			MemBufFormatTarget* mbft = new MemBufFormatTarget();
			output->setByteStream(mbft);
			output->setEncoding(XMLUni::fgXMLChEncodingString); //This must be done.
			if ( writer->write(n,output) ) {
				XMLCh* c_result = (XMLCh*)((MemBufFormatTarget*)mbft)->getRawBuffer();
				if (c_result != NULL) {
					result = u_str(c_result,XMLString::stringLen(c_result));
				}
			}
			delete mbft;
		} // else {} //node was NULL
	}
	
	void Parser::writenode(const DOMNode* const& n, std::string& result) {
		if ( n != NULL ) {
			AutoRelease<DOMLSOutput> output(((DOMImplementationLS*)impl)->createLSOutput());	
			MemBufFormatTarget* mbft = new MemBufFormatTarget();
			output->setByteStream(mbft);
			output->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
			if ( writer->write(n,output) ) {
				char* c_result = (char*)((MemBufFormatTarget*)mbft)->getRawBuffer();
				if (c_result != NULL) {
					result = string(c_result,XMLString::stringLen(c_result));
				}
			}
			delete mbft;
		} // else {} //node was NULL
	}
	
	void Parser::writedoc(const DOMDocument* const& n,std::string& result) {
		if ( n != NULL ) {
			AutoRelease<DOMLSOutput> output(((DOMImplementationLS*)impl)->createLSOutput());	
			MemBufFormatTarget* mbft = new MemBufFormatTarget();
			output->setByteStream(mbft);
			output->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
			if ( writer->write(n,output) ) {
				char* c_result = (char*)((MemBufFormatTarget*)mbft)->getRawBuffer();
				if (c_result != NULL) {
					result = string(c_result,XMLString::stringLen(c_result));
				}
			}
			delete mbft;
			if ( !result.empty() && String::Regex::available()) {
				// NEED TO FIX single quote attributes as well as double quote ones.
				const string textarea_find="<textarea((?:\\s+(?:\\w+:)?\\w+=\"[^\"]+\")+)/>";
				const string textarea_repl="<textarea\\1></textarea>";
				String::Regex::replace(textarea_find,textarea_repl,result,true);
			}
		} // else {} //node was NULL
	}
	
	Parser::~Parser() {
		delete resourceHandler;
		delete errorHandler;
		parser->resetDocumentPool();
		parser->resetCachedGrammarPool();
		parser->release();
		writer->release();
		delete xfmt;			
	}
	
	void Parser::makeWriter() {
		string dummy;
		writer = ((DOMImplementationLS*)impl)->createLSSerializer();
		DOMConfiguration* dc = writer->getDomConfig();
		dc->setParameter(XMLUni::fgDOMErrorHandler,errorHandler);
		dc->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent,true);
		dc->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint,false);
		dc->setParameter(XMLUni::fgDOMXMLDeclaration,Environment::getbenvtf("OBYX_XMLDECL"));
		dc->setParameter(XMLUni::fgDOMWRTBOM,false);
	}
	
	void Parser::validation_set(bool validsetting,bool start) {
		if (validation != validsetting || start) {
			DOMConfiguration* dc = parser->getDomConfig();
			if (validsetting) {
				if (dc->canSetParameter(XMLUni::fgDOMValidateIfSchema, false)) {
					dc->setParameter(XMLUni::fgDOMValidateIfSchema, false);
				}
				if (dc->canSetParameter(XMLUni::fgDOMValidate, true)) {
					dc->setParameter(XMLUni::fgDOMValidate, true);
				}
			} else {
				if (dc->canSetParameter(XMLUni::fgDOMValidate, false)) {
					dc->setParameter(XMLUni::fgDOMValidate, false);
				}
				if (dc->canSetParameter(XMLUni::fgDOMValidateIfSchema, true)) {
					dc->setParameter(XMLUni::fgDOMValidateIfSchema, true);
				}
			}
			validation = validsetting;
		}
	}

	void Parser::makeReader() {
// Xerces-c v3.0 and up. Val_Auto 
		parser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, NULL);
		DOMConfiguration* dc = parser->getDomConfig();
		dc->setParameter(XMLUni::fgDOMErrorHandler,errorHandler);
		dc->setParameter(XMLUni::fgDOMResourceResolver,resourceHandler);  //This is the 'dom' way- it's 'the same' as fgXercesEntityResolver!
		dc->setParameter(XMLUni::fgXercesValidationErrorAsFatal, false);		
        dc->setParameter(XMLUni::fgXercesSkipDTDValidation, false);
        dc->setParameter(XMLUni::fgXercesUserAdoptsDOMDocument, true);
        dc->setParameter(XMLUni::fgXercesContinueAfterFatalError, true);		
		dc->setParameter(XMLUni::fgXercesCacheGrammarFromParse, true);		
		dc->setParameter(XMLUni::fgXercesUseCachedGrammarInParse, true);		
		dc->setParameter(XMLUni::fgDOMElementContentWhitespace, false); //if true, will keep loads of unwanted whitespace.		
        dc->setParameter(XMLUni::fgDOMNamespaces, true);
		dc->setParameter(XMLUni::fgXercesSchema, true);
		dc->setParameter(XMLUni::fgXercesIgnoreAnnotations, true);
		dc->setParameter(XMLUni::fgXercesLoadExternalDTD, false);
		dc->setParameter(XMLUni::fgXercesIgnoreCachedDTD, false);
		dc->setParameter(XMLUni::fgXercesIdentityConstraintChecking,true);
// If we are using an xml config file, we need to choose if this is on or off by default. 
// I think that false is probably the correct move here.
		srv_validation=Environment::getbenvtf("OBYX_VALIDATE_ALWAYS");
		validation_set(srv_validation,true);
		dc->setParameter(XMLUni::fgDOMDatatypeNormalization, true); //Add in datatypes..
		dc->setParameter(XMLUni::fgXercesDOMHasPSVIInfo,false);		//Otherwise we are in trouble - validation will fail
		
		//See http://xerces.apache.org/xerces-c/program-dom-3.html for full list of parameters/features
		//the 'SystemId' values must be the namespace urls.
	
		if (Environment::getbenvtf("OBYX_USING_XHTML5")) {
			resourceHandler->setGrammar(xhtml1dtd,UCS2(L"-//W3C//DTD XHTML 1.0 Strict//EN"),Grammar::DTDGrammarType,true);      //XERCESC-1927: DTDs must be loaded before xml documents.
			resourceHandler->setGrammar(xmlxsd,UCS2(L"http://www.w3.org/XML/1998/namespace"),Grammar::SchemaGrammarType);
			resourceHandler->setGrammar(xlinkxsd,UCS2(L"http://www.w3.org/1999/xlink"),Grammar::SchemaGrammarType);
			resourceHandler->setGrammar(svgxsd,UCS2(L"http://www.w3.org/2000/svg"),Grammar::SchemaGrammarType,true);
			resourceHandler->setGrammar(xhtml5xsd,UCS2(L"http://www.w3.org/1999/xhtml"),Grammar::SchemaGrammarType,true);
		} else {
			resourceHandler->setGrammar(xhtml1dtd,UCS2(L"http://www.w3.org/1999/xhtml"),Grammar::DTDGrammarType,true);      //XERCESC-1927: DTDs must be loaded before xml documents.
			resourceHandler->setGrammar(xmlxsd,UCS2(L"http://www.w3.org/XML/1998/namespace"),Grammar::SchemaGrammarType);
			resourceHandler->setGrammar(xlinkxsd,UCS2(L"http://www.w3.org/1999/xlink"),Grammar::SchemaGrammarType);
			resourceHandler->setGrammar(svgxsd,UCS2(L"http://www.w3.org/2000/svg"),Grammar::SchemaGrammarType,true);
		}
		resourceHandler->setGrammar(obyxxsd,UCS2(L"http://www.obyx.org"),Grammar::SchemaGrammarType);      //I don't really know why this has to be preloaded..
		resourceHandler->setGrammar(messagexsd,UCS2(L"http://www.obyx.org/message"),Grammar::SchemaGrammarType);
		resourceHandler->setGrammar(oalxsd,UCS2(L"http://www.obyx.org/osi-application-layer"),Grammar::SchemaGrammarType);
		resourceHandler->setGrammar(jsonxsd,UCS2(L"http://www.obyx.org/json"),Grammar::SchemaGrammarType);

		//These are all better handled at runtime. The performance hit of preloading them against every request is not so good
		resourceHandler->setGrammar(soapxsd,UCS2(L"http://schemas.xmlsoap.org/soap/envelope/"),Grammar::SchemaGrammarType);
		resourceHandler->setGrammar(soapencodingxsd,UCS2(L"http://schemas.xmlsoap.org/soap/encoding/"),Grammar::SchemaGrammarType);
		resourceHandler->setGrammar(wsdlxsd,UCS2(L"http://schemas.xmlsoap.org/wsdl/"),Grammar::SchemaGrammarType);
		resourceHandler->setGrammar(wsdlmimexsd,UCS2(L"http://schemas.xmlsoap.org/wsdl/mime/"),Grammar::SchemaGrammarType);
	}
	
	DOMDocument* Parser::newDoc(const DOMNode* n) {
		DOMDocument* doc = NULL;
		const DOMNamedNodeMap*  mmn = NULL;
		if (n != NULL) {
			try {
				if (n->getNodeType() == DOMNode::DOCUMENT_NODE) {
					doc = (DOMDocument*)n->cloneNode(true);
				} else {
					doc = impl->createDocument();
					if (n->getNodeType() == DOMNode::ELEMENT_NODE) {
						const DOMElement*root = (const DOMElement*)(n);
						xercesc::DOMNode* inod = doc->importNode(root,true);
						doc->appendChild(inod);
//now handle implicit namespaces as found in schemas.
						const DOMDocument* p = n->getOwnerDocument();
						if ( p != NULL) {
							const DOMElement* pde = p->getDocumentElement();
							if (pde != NULL) {
								mmn=pde->getAttributes(); //now we have the attributes of the parent doc.
								DOMElement* de = doc->getDocumentElement();
								if (de != NULL && mmn != NULL ) {
									size_t n = mmn->getLength();
									for(size_t i=0; i<n; i++) {
										DOMAttr* ai = (DOMAttr*)(mmn->item(i));
										const u_str ain(ai->getName());
										if (ain.compare(0,6,UCS2(L"xmlns:")) == 0) {
											de->removeAttribute(ain.c_str());
											DOMAttr* iattr = (DOMAttr*)(doc->importNode(ai,true));
											de->setAttributeNode(iattr);
										}
									}
								}
							}							
						}
					}
				}
			}
			catch (DOMException e) {
				string err_message;
				Manager::transcode(e.getMessage(),err_message);
				*Logger::log << error << Log::LI << "DOM Copy. Exception message is:" << Log::br << err_message << "\n" << Log::LO << Log::blockend;
			}
		} else {
			doc = impl->createDocument();
		}
		return doc;
	}
	
	DOMDocumentFragment* Parser::newDocFrag(DOMDocument* doc) {
		return doc->createDocumentFragment(); //no root element.
	}
	
	//non-releasing the doc result is not good...
	DOMDocument* Parser::loadDoc(const std::string& xfile) {
		DOMDocument* rslt = NULL;
		if ( ! xfile.empty() ) {
			std::string xmlfile = xfile;		
			bool do_validation = validation;	//only has an effect if VALIDATE_ALWAYS is set.
			if (validation) {
				if ( String::Regex::available() ) {
					if (String::Regex::match(String::Regex::xml_schema_prolog,xfile)) {
						do_validation = false; //This is a grammar.
					} else {
						string f_namespace;
						if (String::Regex::field(String::Regex::xml_namespace_prolog,xfile,2,f_namespace)) {
							u_str ns_load;
							Manager::transcode(f_namespace,ns_load);
							resourceHandler->installGrammar(ns_load); //will only do it the first time.
							do_validation = true;
						} else {
							if (String::Regex::match(String::Regex::xml_doctype_prolog,xfile)) {
								do_validation = true;
							} else {
								do_validation = false;
							}
						}
					}
				} 
			}
			if (!do_validation) {
				validation_off();
			}
			AutoRelease<DOMLSInput> input(((DOMImplementationLS*)impl)->createLSInput());	
			XMLByte* xmlraw = (XMLByte*)(xmlfile.c_str());
			MemBufInputSource* mbis = new MemBufInputSource(xmlraw,xmlfile.size(),memfile.c_str());
			mbis->setCopyBufToStream(false);
			input->setByteStream(mbis);
			input->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
			try {
				rslt = parser->parse(input);
			}
			catch (DOMLSException e) {
				string err_message;
				Manager::transcode(e.getMessage(),err_message);			
				*Logger::log << error << Log::LI << "Error during parsing memory stream. Exception message is:" << Log::br << err_message << "\n" << Log::LO << Log::blockend;
			}
			catch (DOMException e) {
				string err_message;
				Manager::transcode(e.getMessage(),err_message);			
				*Logger::log << error << Log::LI << "Error during parsing memory stream. Exception message is:" << Log::br << err_message << "\n" << Log::LO << Log::blockend;
			}
			catch ( ... ) {
				*Logger::log << error << Log::LI << "Some load error occured with an xml file of length " << (unsigned int)xmlfile.size() << Log::LO << Log::blockend;
			}
			if ( errorHandler->hadErrors() ) {	
				*Logger::log << error << Log::LI << "Text that failed parse" << Log::LO << Log::LI << xmlfile << Log::LO << Log::blockend;
				rslt=NULL;
				errorHandler->resetErrors();
			} 
			delete mbis;
			mbis=NULL;
			if (!do_validation) {
				validation_on();
			}
		}
		return rslt;
	}
	
	void Parser::grammar_reading_on() {
		errorHandler->setGrammar(true);
	}
	
	void Parser::grammar_reading_off() {
		errorHandler->setGrammar(false);
	}
	
	void Parser::validation_off() {
		if (validation) {
			DOMConfiguration* dc = parser->getDomConfig();
			if (dc->canSetParameter(XMLUni::fgDOMValidate, false)) {
				dc->setParameter(XMLUni::fgDOMValidate, false);
			}
		} 
	}
	
	void Parser::validation_on() {
		if (validation) {
			DOMConfiguration* dc = parser->getDomConfig();
			if (dc->canSetParameter(XMLUni::fgDOMValidate, true)) {
				dc->setParameter(XMLUni::fgDOMValidate, true);
			}
		} 
	}
	
	DOMDocument* Parser::loadDoc(const u_str& xmlfile) {
		DOMDocument* rslt = NULL;
		if ( ! xmlfile.empty() ) {
			AutoRelease<DOMLSInput> input(((DOMImplementationLS*)impl)->createLSInput());	
			XMLByte* xmlraw = (XMLByte*)(xmlfile.c_str());
			MemBufInputSource* mbis = new MemBufInputSource(xmlraw,xmlfile.size()*sizeof(XMLCh),memfile.c_str());
			mbis->setCopyBufToStream(false);
			input->setByteStream(mbis);
			input->setEncoding(XMLUni::fgXMLChEncodingString); //This must be done.
			try {
				rslt = parser->parse(input);
			}
			catch ( ... ) {
				*Logger::log << error << Log::LI << "Some load error occured with an xml file of length " << (unsigned int)xmlfile.size() << Log::LO << Log::blockend;
			}
			if ( errorHandler->hadErrors() ) {	
				string exml_file;
				Manager::transcode(xmlfile.c_str(),exml_file);
				*Logger::log << error << Log::LI << "Failed to parse:" << exml_file << Log::LO << Log::blockend;
				rslt=NULL;
				errorHandler->resetErrors();
			}
			delete mbis; 
			mbis=NULL;
		}
		return rslt;
	}
	
	basic_string<XMLCh> Parser::xpath(DOMNode* startnode) { 
		basic_string<XMLCh> xpath;
		if (startnode != NULL) {
			basic_ostringstream<XMLCh> osd;
			XMLCh sibch[8];					//For number conversion.
			const XMLCh notestr[]= {'n','o','t','e',chNull};		// "note"
			const XMLCh attropen[]={'[','@','n','o','t','e','=','"',chNull};
			const XMLCh attrclose[]={'"',']',chNull};
			const XMLCh sls[]= {'/',chNull};		// "/"
			const XMLCh bkt[]= {'(',')',chNull};	// "()"
			const XMLCh osq[]= {'[',chNull};		// "["
			const XMLCh csq[]= {']',chNull};		// "]"
			vector<basic_string<XMLCh> > bits;
			DOMNode* n = startnode;
			while ( n != NULL ) { //horrible.
				basic_ostringstream<XMLCh> oss;
				basic_string<XMLCh> name = n->getNodeName();
				if (name[0] == '#' ) { 
					if (n->getNodeType() != DOMNode::DOCUMENT_NODE) {
						name.erase(0,1); //remove the hash.
						oss << sls << name << bkt;
					}
				} else {
					const XMLCh* note_attr = NULL; 
					if (n->getNodeType() == DOMNode::ELEMENT_NODE ) {
						DOMElement* el = dynamic_cast<DOMElement*>(n);
						if (el != NULL) {
							DOMAttr* anote = el->getAttributeNode(notestr);
							if (anote != NULL) {
								note_attr = anote->getNodeValue();
							}	
						}
					}
					//This is v.slow for travelling through DOMText - will loop through each char.
					unsigned long sibnum = 1;
					xercesc::DOMNode* s = n->getPreviousSibling();
					while ( s != NULL) {
						if (!name.compare(s->getNodeName())) { sibnum++ ;}
						s = s->getPreviousSibling();
					} 
					XMLString::binToText(sibnum,sibch,7,10);
					oss << sls << name << osq << sibch << csq;
					if (note_attr != NULL) {
						oss << attropen << note_attr << attrclose;
					}
				}
				n = n->getParentNode(); //really shouldn't need to dynamic cast this.
				bits.push_back(oss.str());
			}
			while (bits.size() > 0) {
				xpath.append(bits.back());
				bits.pop_back();
			}
		}
		return xpath;
	}

	// DOMDocumentFragment	
	/* All the following could probably be replaced with parser->parseWithContext(input,pt,action); */
	void Parser::insertContext(DOMDocument*& doc,DOMNode*& pt,const basic_string<XMLCh>& ins,DOMLSParser::ActionType action) {
		if (pt != NULL) {
			switch ( pt->getNodeType() ) {
				case DOMNode::ELEMENT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_child_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
 							*Logger::log << error << Log::LI << "XPath action not yet supported for element nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::ATTRIBUTE_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_mustbetext_preceding_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_mustbetext_following_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_mustbetext_set(pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for attributes." << Log::LO << Log::blockend;
						} break;
					}		
				} break;
				case DOMNode::TEXT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for text nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::CDATA_SECTION_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_mustbetext_preceding_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_mustbetext_set(pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for cdata nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::ENTITY_REFERENCE_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for entity_reference nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::ENTITY_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for entity nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::PROCESSING_INSTRUCTION_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_REPLACE_CHILDREN: {
							do_mustbetext_set(pt,ins);  
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for processing-instruction nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::COMMENT_NODE: { //This is all correct!
					switch (action) {
						case DOMLSParser::ACTION_REPLACE_CHILDREN: {
							do_mustbetext_set(pt,ins);  
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for comment nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::DOCUMENT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_child_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							if (doc != NULL) { 
								doc->release();
								doc = NULL;
							}
							if (!ins.empty()) { 
								doc = loadDoc(ins);
							}
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for documents." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::DOCUMENT_TYPE_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for document_type nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::DOCUMENT_FRAGMENT_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for document_fragment nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::NOTATION_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for notation nodes." << Log::LO << Log::blockend;
				} break;
				default: {
					*Logger::log << fatal << Log::LI << "Node Type not recognised. Probably corrupt data." << Log::LO << Log::blockend;
				} break;
			}
		}
	}
	
	void Parser::insertContext(DOMDocument*& doc,DOMNode*& pt,DOMNode* const nins,DOMLSParser::ActionType action) {
		DOMNode* ins = NULL;
		if (nins != NULL) {		//Convert / change nins from DOMDocument to DOMNode if needs be.
			DOMNode::NodeType nt = nins->getNodeType();
			switch (nt) {
				case DOMNode::DOCUMENT_NODE: {
					DOMDocument* d = static_cast<DOMDocument*>(nins);
					ins = d->getDocumentElement();
				} break;
				default: {
					ins = nins;
				} break;
			}
		}
 		if (pt != NULL) {		
			switch ( pt->getNodeType() ) {
				case DOMNode::ELEMENT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_child_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
 							*Logger::log << error << Log::LI << "XPath action not yet supported for element nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::ATTRIBUTE_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_mustbetext_preceding_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_mustbetext_following_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_mustbetext_set(pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for attributes." << Log::LO << Log::blockend;
						} break;
					}		
				} break;
				case DOMNode::TEXT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for text nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::CDATA_SECTION_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_mustbetext_preceding_gap(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_mustbetext_following_gap(pt,ins); //well, this is new.
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_mustbetext_set(pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for cdata nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::ENTITY_REFERENCE_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for entity_reference nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::ENTITY_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for entity nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::PROCESSING_INSTRUCTION_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_REPLACE_CHILDREN: {
							do_mustbetext_set(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for processing-instruction nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::COMMENT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_REPLACE_CHILDREN: {
							do_mustbetext_set(pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_BEFORE: {
							do_maybenode_preceding_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_INSERT_AFTER: {
							do_maybenode_following_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							do_maybenode_set(doc,pt,ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for comment nodes." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::DOCUMENT_NODE: {
					switch (action) {
						case DOMLSParser::ACTION_APPEND_AS_CHILDREN: {
							do_maybenode_child_gap(doc,pt,ins);
						} break;
						case DOMLSParser::ACTION_REPLACE: {
							if (doc != NULL) { 
								doc->release();
								doc = NULL;
							}
							doc = XML::Manager::parser()->newDoc(ins);
						} break;
						default: {
							*Logger::log << error << Log::LI << "XPath action not yet supported for documents." << Log::LO << Log::blockend;
						} break;
					}					
				} break;
				case DOMNode::DOCUMENT_TYPE_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for document_type nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::DOCUMENT_FRAGMENT_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for document_fragment nodes." << Log::LO << Log::blockend;
				} break;
				case DOMNode::NOTATION_NODE: {
					*Logger::log << error << Log::LI << "XPath actions are not yet supported for notation nodes." << Log::LO << Log::blockend;
				} break;
				default: {
					*Logger::log << fatal << Log::LI << "Node Type not recognised. Probably corrupt data." << Log::LO << Log::blockend;
				} break;
			}
		}
	}
	
	bool Parser::loadURI(const std::string& uri, DOMDocument*& doc) {
		bool retval = false;
		char* urichars = const_cast<char*>(uri.c_str());
		try {
			doc = parser->parseURI(urichars);
			retval = true;
		}
		catch (const OutOfMemoryException&) {
			*Logger::log << error << Log::LI << "Out Of Memory Exception" << Log::LO << Log::blockend;
		}
		catch (const XMLException& e) {
			string err_message;
			Manager::transcode(e.getMessage(),err_message);			
			*Logger::log << error << Log::LI << "Error during parsing memory stream. Exception message is:" << Log::br << err_message << "\n" << Log::LO << Log::blockend;
		}
		catch ( ... ) {
			*Logger::log << error << Log::LI << "Some XML Document load error occured with the uri " << uri << Log::LO << Log::blockend;
		}
		if ( errorHandler->hadErrors() ) {		
			*Logger::log << error << Log::LI << "XML Document will be empty." << Log::LO << Log::blockend;
			errorHandler->resetErrors();
		}
		return retval;
	}
	
	void Parser::setGrammar(const std::string& grammarfile,const u_str& namespaceuri, Grammar::GrammarType type) {
		if ( !namespaceuri.empty() ) {
			resourceHandler->setGrammar(grammarfile,namespaceuri,type);
		}
	}
	
	void Parser::getGrammar(std::string& grammarfile,const std::string& namespaceuri,bool release) {
		if ( !namespaceuri.empty() ) {
			resourceHandler->getGrammar(grammarfile,namespaceuri,release);
		}
	}
	
	bool Parser::existsGrammar(const std::string& namespaceuri,bool release) {
		bool retval = false;
		if ( !namespaceuri.empty() ) {
			retval = resourceHandler->existsGrammar(namespaceuri,release);
		}
		return retval;
	}
	
	// Whitespace http://www.w3.org/TR/2008/REC-xml-20081126/#NT-S
	// [3] S  ::=  (#x20 | #x9 | #xD | #xA)+
	/*	XMLString::catString() doesn't do any overflow checking so malloc errors will occur.	*/
	
	void Parser::do_mustbetext_following_gap(DOMNode*& pt,const basic_string<XMLCh>& v) {
		if (! v.empty() ) {
			basic_string<XMLCh> result;
			const XMLCh* const base = pt->getNodeValue();
			result.append(base);
			result.append(v);
			pt->setNodeValue(result.c_str());
		}
	}
	
	void Parser::do_mustbetext_preceding_gap(DOMNode*& pt,const basic_string<XMLCh>& v) {
		if (! v.empty() ) {
			basic_string<XMLCh> result;
			const XMLCh* const base = pt->getNodeValue();
			result.append(v);
			result.append(base);
			pt->setNodeValue(result.c_str());
		}
	}
	
	void Parser::do_mustbetext_set(DOMNode*& pt,const basic_string<XMLCh>& v) {
		if ( v.empty() ) {
			if ( pt->getNodeType() == DOMNode::ATTRIBUTE_NODE ) {
				DOMNode* ptx = pt; //
				DOMAttr* ena = static_cast<DOMAttr*>(ptx);
				DOMElement* en =ena->getOwnerElement();
				DOMAttr* enx = en->removeAttributeNode(ena);
				enx->release(); pt=NULL;
			}
		} else {		
			pt->setNodeValue(v.c_str());
		}
	}
	
	void Parser::do_maybenode_child_gap(DOMDocument*& doc,DOMNode*& pt,const basic_string<XMLCh>& v) {
		if (! v.empty() ) {
			DOMText* vt = doc->createTextNode(v.c_str());
			pt->appendChild(vt);
			doc->normalize();
		}
 	}
	
	void Parser::do_maybenode_preceding_gap(DOMDocument*& doc,DOMNode*& pt,const basic_string<XMLCh>& v) {
		if (! v.empty() ) {
			DOMText* vt = doc->createTextNode(v.c_str());
			pt->getParentNode()->insertBefore(vt,pt); 
			doc->normalize();
		}
	}
	
	void Parser::do_maybenode_following_gap(DOMDocument*& doc,DOMNode*& pt,const basic_string<XMLCh>& v) {
		if (! v.empty() ) {
			DOMText* vt = doc->createTextNode(v.c_str());
			DOMNode* ptf = pt->getNextSibling();
			if (ptf == NULL) {
				pt->getParentNode()->appendChild(vt);
			} else {
				ptf->getParentNode()->insertBefore(vt,ptf); //check if pt is a descendant of doc, rather than a child.
			}
			doc->normalize();
		}
	}
	
	void Parser::do_maybenode_set(DOMDocument*& doc,DOMNode*& pt,const basic_string<XMLCh>& v) {
		DOMNode *xr = pt->getParentNode();
		bool released=false;
		if (! v.empty() ) {
			DOMText* vt = doc->createTextNode(v.c_str());
			xr->insertBefore(vt,pt); 
		} 
		if (!released) {
			DOMNode *xn = xr->removeChild(pt);	
			xn->release();
		}
		doc->normalize();
	}
	
	//now DOMDocument ones..
	void Parser::do_mustbetext_following_gap(DOMNode*& pt, DOMNode* rv) {
		if ( rv != NULL ) {
			u_str v; writenode(rv,v); //writedoc(rv,v);
			if (!v.empty()) {
				u_str nodeval = pt->getNodeValue();
				nodeval.append(v);
				pt->setNodeValue(nodeval.c_str());
			}
		}
	}
	
	void Parser::do_mustbetext_preceding_gap(DOMNode*& pt, DOMNode* rv) {
		if ( rv != NULL ) {
			u_str v; writenode(rv,v); //writedoc(rv,v);
			if (!v.empty()) {
				u_str nodeval = pt->getNodeValue();
				v.append(nodeval);
				pt->setNodeValue(nodeval.c_str());
			}
		}
	}
	
	void Parser::do_mustbetext_set(DOMNode*& pt, DOMNode* rv) {
		if ( rv != NULL ) {
			u_str v; writenode(rv,v); //writedoc(rv,v);
			pt->setNodeValue(v.c_str());
		}
	}
	
	void Parser::do_maybenode_child_gap(DOMDocument*& doc,DOMNode*& pt,const DOMNode* vnod) {
		if (vnod != NULL) {
			DOMNode* inod = doc->importNode(vnod,true);
			do_attr_namespace_kludge(doc,inod);
			pt->appendChild(inod);
			doc->normalize();
		}
	}
	
	void Parser::do_maybenode_preceding_gap(DOMDocument*& doc,DOMNode*& pt,const DOMNode* vnod) {
		if (vnod != NULL) {
			DOMNode* inod = doc->importNode(vnod,true);
			do_attr_namespace_kludge(doc,inod);
			pt->getParentNode()->insertBefore(inod,pt); 
			doc->normalize();
		}
	}
	
	void Parser::do_maybenode_following_gap(DOMDocument*& doc,DOMNode*& pt,const DOMNode* vnod) {
		DOMNode* ptf = pt->getNextSibling();
		if (vnod != NULL) {
			DOMNode* inod = doc->importNode(vnod,true);
			do_attr_namespace_kludge(doc,inod);
			if (ptf == NULL) {
				pt->getParentNode()->appendChild(inod);
			} else {
				ptf->getParentNode()->insertBefore(inod,ptf); //check if pt is a descendant of doc, rather than a child.
			}
			doc->normalize();
		}
	}
	
	void Parser::do_maybenode_set(DOMDocument*& doc,DOMNode*& pt,const DOMNode* vnod) {
		DOMNode *xr = pt->getParentNode();
		if (vnod != NULL) {
			DOMNode* inod = doc->importNode(vnod,true);
			do_attr_namespace_kludge(doc,inod);
			if (doc->getDocumentElement() == pt) {
				DOMNode *xn = xr->removeChild(pt); 
				xn->release();
				xr->appendChild(inod); 
			} else {
				xr->insertBefore(inod,pt); 
				DOMNode *xn = xr->removeChild(pt);	
				xn->release();
			}
			doc->normalize();
		} else {
			DOMNode *xn = xr->removeChild(pt);	
			xn->release();
			doc->normalize();
		}
	}
	
	void Parser::do_attr_namespace_kludge(DOMDocument*& doc,DOMNode*& inod) {
		const XMLCh xmlns[]= {'x','m','l','n','s',chNull}; // "xmlns"
		if ( inod->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)inod;
			DOMAttr* enoda = enod->getAttributeNode(xmlns);
			if (enoda != NULL) {
				DOMElement* dnod = doc->getDocumentElement();
				if (dnod != NULL) {
					DOMAttr* dnoda = dnod->getAttributeNode(xmlns);
					if (dnoda != NULL && dnoda->isEqualNode(enoda)) {
						enoda = enod->removeAttributeNode(enoda);
						enoda->release();
					}
				}
			}
		}
	}
}
