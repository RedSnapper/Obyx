/*
 * xmlobject.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlobject.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "dataitem.h"
#include "fragmentobject.h"
#include "xmlobject.h"
#include "strobject.h"

xercesc::DOMDocument* FragmentObject::frag_doc = nullptr;

/* ==================== NON virtual methods. =========== */
/* public static */
void FragmentObject::init() {
	frag_doc = XML::Manager::parser()->newDoc(nullptr);
}
void FragmentObject::finalise() {
	if (frag_doc != nullptr) {
		frag_doc->release();
		frag_doc = nullptr;
	}	
}
void FragmentObject::startup() {}
void FragmentObject::shutdown() {}

FragmentObject::FragmentObject(const std::string s) : DataItem(),fragment(nullptr) { 
	fragment = frag_doc->createDocumentFragment();
	if ( ! s.empty() ) {
		u_str tt;
		XML::Manager::transcode(s,tt);
		DOMText* vt = frag_doc->createTextNode(pcx(tt.c_str()));
		fragment->appendChild(vt);
	}
}
FragmentObject::FragmentObject(u_str s) : DataItem(),fragment(nullptr) { 
	fragment = frag_doc->createDocumentFragment();
	if ( ! s.empty() ) {
		DOMText* vt = frag_doc->createTextNode(pcx(s.c_str()));
		fragment->appendChild(vt);
	}
}
//DOMDocumentImpl
FragmentObject::FragmentObject(const xercesc::DOMNode* n) : DataItem(),fragment(nullptr) {
	fragment = frag_doc->createDocumentFragment();
	DOMNode* newnode = nullptr;
	DOMNode::NodeType nt = n->getNodeType();
	switch (nt) {
		case DOMNode::DOCUMENT_NODE: {
			const DOMDocument* d = static_cast<const DOMDocument*>(n);
			if (d != nullptr) {
				const DOMNode* de = d->getDocumentElement();
				if (de != nullptr) {
					newnode = frag_doc->importNode(de,true);	 //importNode always takes a copy - returns DOMNode* inod =  new node pointer.
				}
			}
		} break;
		default: {
			newnode = frag_doc->importNode(n,true);	 //importNode always takes a copy - returns DOMNode* inod =  new node pointer.
		} break;
	}
	if (newnode != nullptr) {
		fragment->appendChild(newnode);
	}
}
FragmentObject::FragmentObject(const DataItem& s):DataItem(),fragment(nullptr) {
	fragment = frag_doc->createDocumentFragment();
	DOMNode* n = s;
	fragment->appendChild(n);
}
/* ====================  VIRTUAL methods. =========== */
void FragmentObject::append(DataItem*& s) {
	if (s != nullptr) {
		DOMNode* n = nullptr;
		XMLObject* x = dynamic_cast<XMLObject*>(s);
		if (x != nullptr) {
			DOMNode* nbase;
			x->take(nbase);
			DOMNode::NodeType nt = nbase->getNodeType();
			switch (nt) {
				case DOMNode::DOCUMENT_NODE: {
					const DOMDocument* d = static_cast<const DOMDocument*>(nbase);
					if (d != nullptr) {
						const DOMNode* de = d->getDocumentElement();
						if (de != nullptr) {
							n = frag_doc->importNode(de,true);	 //importNode always takes a copy - returns DOMNode* inod =  new node pointer.
						}
						delete d;
					}
				} break;
				default: {
					n = frag_doc->importNode(nbase,true);	 //importNode always takes a copy - returns DOMNode* inod =  new node pointer.
					nbase->release();
				} break;
			}
		} else {
			FragmentObject* y = dynamic_cast<FragmentObject*>(s);
			if (y != nullptr) {
				y->take(n);
			} else {
				std::string str = *s;
				if ( ! str.empty() ) {
					u_str tt;
					XML::Manager::transcode(str,tt);
					n = frag_doc->createTextNode(pcx(tt.c_str()));
				}
			}
		}
		fragment->appendChild(n);
		delete s;
	}
}
FragmentObject::operator u_str() const {
	u_str doc; 
	XML::Manager::parser()->writenode(fragment,doc);
	return doc;
}
FragmentObject::operator FragmentObject*() { return this; }	

FragmentObject::operator std::string() const {
	string doc; 
	XML::Manager::parser()->writenode(fragment,doc);
	return doc;
}
FragmentObject::operator XMLObject*() {
	return nullptr;
}
FragmentObject::operator xercesc::DOMDocument*() const { //Fragments do not hold onto doctypes.
	return XML::Manager::parser()->newDoc(fragment);
}
FragmentObject::operator xercesc::DOMNode*() const {
	return fragment;	//check how this is being used?
}
void FragmentObject::copy(DataItem*& container) const {
	if  ( fragment != nullptr ) {
		DOMNode* tmp = fragment->cloneNode(true);
		container = DataItem::factory(tmp,di_fragment);
	}		
}
long long FragmentObject::size() const {
	if (fragment != nullptr) {
		return 1;
	} else {
		return 0;
	}
}
bool FragmentObject::empty() const {
	return fragment == nullptr;
}
bool FragmentObject::same(const DataItem* xtst) const {
	bool retval = false;
	if (fragment == nullptr && xtst == nullptr) {
		retval = true;	
	} else {
		if (fragment != nullptr) {
			const FragmentObject* ox = dynamic_cast<const FragmentObject*>(xtst);
			if (ox != nullptr ) {
				retval = fragment->isEqualNode(*ox);
			} else {
				const StrObject* ox = dynamic_cast<const StrObject*>(xtst);
				if (ox != nullptr) {
					string frag,text = *xtst;
					XML::Manager::parser()->writenode(fragment,frag);
					retval = (frag.compare(text) == 0);
				} else {
					u_str frag,text = *xtst;
					XML::Manager::parser()->writenode(fragment,frag);
					retval = (frag.compare(text) == 0);
				}
			}
		}
	}
	return retval;
}
void FragmentObject::clear() {
	if (fragment != nullptr) {
		delete fragment;
		fragment = nullptr;
	}
}
bool FragmentObject::find(const DataItem* di,std::string&)  {
	bool retval=false;
	if (fragment != nullptr && di != nullptr) {
		string doc; 
		string srch = *di;
		XML::Manager::parser()->writenode(fragment,doc);
		retval = doc.find(srch) != string::npos;
	}
	return retval;
}
bool FragmentObject::find(const char* str,std::string&)  {
	bool retval=false;
	if (fragment != nullptr && str != nullptr) {
		string doc; 
		XML::Manager::parser()->writenode(fragment,doc);
		retval = doc.find(str) != string::npos;
	}
	return retval;
}
bool FragmentObject::find(const XMLCh* s,std::string&)  {
	bool retval = false;
	if (s != nullptr && fragment != nullptr) {
		u_str srch(pcu(s));
		u_str doc; 
		XML::Manager::parser()->writenode(fragment,doc);
		retval = doc.find(srch) != string::npos;
	} else {
		retval = true;
	}
	return retval;
}

void FragmentObject::trim() {
	//do nothing.
}
void FragmentObject::take(DOMNode*& container) {
	container = fragment;
	fragment = nullptr;
}
FragmentObject::~FragmentObject() {
	if (fragment != nullptr) {
		fragment->release();
		fragment = nullptr;
	}	
}
