/* 
 * xercesmanager.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xercesmanager.cpp is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#include <cassert>
#include <ostream>
//#include <codecvt>


#include "commons/environment/environment.h"
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
	
	XQilla*					Manager::xqilla = nullptr;
	XercesConfiguration* 	Manager::xc = nullptr;
	Parser*					Manager::xparser = nullptr;

	ostream*				Manager::serial_ostream = nullptr;
	Manager::su_map_type 	Manager::tsu;	//transcode (str to ustr) cache.
	Manager::us_map_type 	Manager::tus;	//transcode (ustr to str) cache.
	
	//u_str	to std::string.
	void Manager::transcode(const u_str& source, std::string& result) {
		if (!source.empty()) {
			if (source.size() == 1) {
				result = (char)(source[0]); 
			} else {
				
				char* buff = nullptr;
				const XMLCh* src=(const XMLCh*)(source.c_str());
				try {
					buff = (char*)(TranscodeToStr(src,source.size(),"UTF-8").adopt());
				} catch (...) { //bad UTF-8
					*Logger::log << error << Log::LI << "Error Bad UTF-8 String found. Replaced with '[NOT UTF-8]'" << Log::LO << Log::blockend;
					buff = nullptr;
					result="[NOT UTF-8]";
				}
				if (buff != nullptr) { 
					result = buff; 
					XMLString::release(&buff);
				}
			}
		}
	}
	
	void Manager::transcode(const std::string& source,u_str& result,const std::string encoding) {
		if (!source.empty()) {
			XMLCh* buf = nullptr;
			const XMLByte* src=(const XMLByte*)(source.c_str());
			buf=TranscodeFromStr(src,source.size(),encoding.c_str()).adopt();	
			if (buf != nullptr) { 
				result = pcu(buf);
				XMLString::release(&buf);
			}
		}
	}
	
	void Manager::to_ustr(const long num,u_str& repo) {
		ostringstream ost;
		ost << num;
		transcode(ost.str(),repo);
	}

	bool Manager::attribute(const DOMNode* n,const u_str attrname, std::string& attrval) {
		bool result=false;
		if (n != nullptr && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(pcx(attrname.c_str()));
			if (enoda != nullptr) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != nullptr && x_attrval[0] != 0 ) {
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
		if (n != nullptr && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(pcx(attrname.c_str()));
			if (enoda != nullptr) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != nullptr && x_attrval[0] != 0 ) {
					attrval = pcu(x_attrval);
					result = true; //only true if result is not empty.
				}
			}
		}
		return result;
	}
	
	bool Manager::attribute(const DOMNode* n,const u_str attrname) {
		bool result=false;
		if (n != nullptr && !attrname.empty() && n->getNodeType() == DOMNode::ELEMENT_NODE ) {
			DOMElement* enod = (DOMElement*)n;
			DOMAttr* enoda = enod->getAttributeNode(pcx(attrname.c_str()));
			if (enoda != nullptr) {
				const XMLCh* x_attrval = enoda->getNodeValue();
				if (x_attrval != nullptr && x_attrval[0] != 0 ) {
					result = true; //only true if result is not empty.
				}
			}
		}
		return result;
	}
	
	void Manager::resetDocPool() { //static, used to release memory in eg. fcgi.
		xparser->parser->resetDocumentPool();
	}
	
	//instantiated by Document.startup().
	Manager::Manager() {
		try {
//			XMLPlatformUtils::Initialize();
			XQillaPlatformUtils::initialize(); //if using XQilla..
			xqilla = new XQilla();
			xc = new XercesConfiguration();
			xparser = new Parser();
			xparser->makerw();
		}
		catch (const XMLException&) {
			*Logger::log << error << Log::LI << "Error during Xerces initialization! Message unavailable" << Log::LO << Log::blockend;
			throw XercesInitExn(); // pass on the error
		}
	}
	
	void Manager::init() {
		xparser->validation_set(Environment::service()->getenvtf("OBYX_VALIDATE_ALWAYS"));
	}
	
	void Manager::finalise() {
		xparser->validation_set(xparser->srv_validation);
	}
	
	Manager::~Manager() {
		delete xc; xc=nullptr;
		delete xparser; xparser=nullptr;
		delete xqilla; xqilla = nullptr;
		XQillaPlatformUtils::terminate();
//		XMLPlatformUtils::Terminate();

	}
}
