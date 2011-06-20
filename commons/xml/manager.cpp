/* 
 * xercesmanager.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xercesmanager.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <cassert>
#include <istream>
#include <ostream>

#include "commons/logger/logger.h"
#include "commons/xml/xml.h"

namespace {
	//std
	using std::ostream;
	
	// Xerces-c
	using xercesc::XMLException;
	using xercesc::XMLUni;
}

using namespace Log;
using namespace std;

namespace XML {
	// for information regarding transcoders, see
	// http://xerces.apache.org/xerces-c/faq-parse-3.html#faq-19
	
	Parser*					Manager::xparser = NULL;
	ostream*				Manager::serial_ostream = NULL;
	Manager::su_map_type 	Manager::tsu;	//transcode (str to ustr) cache.
	Manager::us_map_type 	Manager::tus;	//transcode (ustr to str) cache.
	
	//u_str	to std::string.
	void Manager::transcode(const u_str& source, std::string& result) {
		if (!source.empty()) {
			if (source.size() == 1) {
				result = (char)(source[0]); 
			} else {
				
				char* buff = NULL;
				const XMLCh* src=(const XMLCh*)(source.c_str());
				buff = (char*)(TranscodeToStr(src,source.size(),"UTF-8").adopt());
				if (buff != NULL) { 
					result = buff; 
					XMLString::release(&buff);
				}
			}
		}
	}
	
	void Manager::transcode(const std::string& source,u_str& result,const std::string encoding) {
		if (!source.empty()) {
//			tsu.clear();	//cache transcodes.

			XMLCh* buf = NULL;
			const XMLByte* src=(const XMLByte*)(source.c_str());
			buf=TranscodeFromStr(src,source.size(),encoding.c_str()).adopt();	
			if (buf != NULL) { 
				result = buf; 
				XMLString::release(&buf);
			}
		}
	}
	
	void Manager::to_ustr(const long num,u_str& repo) {
		XMLCh buff[64]; //sizeof long should not be more than 64 chars.
		XMLString::binToText(num,buff,63,10);
		repo = buff;
	}

	bool Manager::attribute(const DOMNode* n,const u_str attrname, std::string& attrval) {
		bool result=false;
		if (n != NULL && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(attrname.c_str());
			if (enoda != NULL) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != NULL && x_attrval[0] != 0 ) {
					char* value = (char*)TranscodeToStr(x_attrval,"UTF-8").adopt();
					size_t vl = strlen(value);
					attrval.reserve(vl);
					attrval.assign(value,vl);
					XMLString::release(&value); //delete attr;
					result = true; //only true if result is not empty.
				}
			}
		}
		return result;
	}
	
	bool Manager::attribute(const DOMNode* n,const u_str attrname, u_str& attrval) {
		bool result=false;
		if (n != NULL && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(attrname.c_str());
			if (enoda != NULL) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != NULL && x_attrval[0] != 0 ) {
					attrval = x_attrval;
					result = true; //only true if result is not empty.
				}
			}
		}
		return result;
	}
	
	bool Manager::attribute(const DOMNode* n,const u_str attrname) {
		bool result=false;
		if (n != NULL && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(attrname.c_str());
			if (enoda != NULL) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != NULL && x_attrval[0] != 0 ) {
					result = true; //only true if result is not empty.
				}
			}
		}
		return result;
	}
	
	void Manager::resetDocPool() { //static, used to release memory in eg. fcgi.
		xparser->parser->resetDocumentPool();
		xparser->parser->resetCachedGrammarPool();
	}
	
	Manager::Manager() {
		try {
//			XMLPlatformUtils::Initialize(); 
			XQillaPlatformUtils::initialize(); //if using XQilla..
			xparser = new Parser();
			xparser->makerw();
		}
		catch (const XMLException&) {
			*Logger::log << error << Log::LI << "Error during Xerces initialization! Message unavailable" << Log::LO << Log::blockend;
			throw XercesInitExn(); // pass on the error
		}
	}
	
	Manager::~Manager() {
//		XMLPlatformUtils::Terminate();
		XQillaPlatformUtils::terminate();
	}
}
