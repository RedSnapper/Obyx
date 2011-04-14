/* 
 * dataitem.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * dataitem.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "dataitem.h"
#include "rawitem.h"
#include "strobject.h"
#include "ustritem.h"
#include "xmlobject.h"
#include "fragmentobject.h"

#include "commons/environment/environment.h"
#include "commons/logger/logger.h"

/*
 DataItem::long_map			DataItem::ce_map;
 
 void DataItem::do_alloc(const std::string v) {
 unsigned long addr = (unsigned long)(this);
 ce_map.insert(long_map::value_type(addr,v));
 }
 
 void DataItem::do_dealloc() {
 unsigned long addr = (unsigned long)(this);
 long_map::iterator it = ce_map.find(addr);
 if ( it == ce_map.end() ) {
 *Logger::log << Log::error << Log::LI << "Error. ce was already deleted."  << Log::LO << Log::blockend;	
 } else {
 ce_map.erase(it);
 }
 }
 */

DataItem::~DataItem() {}
DataItem::DataItem() {}

//results of b go into a, b is set to null. Both may start off as NULL!
void DataItem::append(DataItem*& a,DataItem*& b) { //this is static
	if (a != NULL) {
		if (b != NULL) { //if b is null, then we are appending nothing.
			if (a->kind() == di_raw || b->kind() == di_raw) { //raw + xxx == raw.
				std::string x = *a; delete a;
				a = new RawItem(x);
			} else {
				XMLObject* x = dynamic_cast<XMLObject*>(a);
				if (x != NULL) {
					DOMNode* n;
					x->take(n); delete a;
					a = new FragmentObject(n);
				} 
			}
			a->append(b); //does the delete.
		} 
	} else {
		a = b;
	}
	b = NULL;	
}

void DataItem::append(DataItem*&) { //Do nothing
}

DataItem* DataItem::autoItem(const std::string& s) {
	DataItem* retval = NULL;
	if ( s.empty() ) {
		retval=NULL;
	} else {
		if (XMLChar::isutf8(s)) {
			string::size_type b = s.find_first_not_of(" \r\n\t" ); //Test for first non-ws is a '<'
			if(s[b] != '<') {  
				retval=new StrObject(s);
			} else {
				string::size_type e = s.find_last_not_of(" \r\n\t" ); //Test for last non-ws is a '>'
				if(s[e] != '>') { 
					retval=new StrObject(s);
				} else {
					bool xml_dt_prolog_found = false;
					if ( String::Regex::available() ) {
						if (String::Regex::match(String::Regex::xml_doctype_prolog,s)) {
							xml_dt_prolog_found = true;
							retval=new XMLObject(s);
						} 
					} else {
						if(s.substr(b,6).compare("<?xml ")==0) { 
							xml_dt_prolog_found = true;
							retval=new XMLObject(s);
						}
					}
					if(!xml_dt_prolog_found) {  
						bool xmlns_found = false;
						if ( String::Regex::available() ) {
							if (String::Regex::match(String::Regex::xml_namespace_prolog,s)) {
								xmlns_found = true;
								retval=new XMLObject(s);
							} 
						} else {
							if (s.find("xmlns") != string::npos) {
								xmlns_found = true;
								retval=new XMLObject(s);
							}
						}
						if (!xmlns_found) {  //no xml prolog, no xmlns declaration, but < and > so try a suppressed xmlobject.
							ostringstream* suppressor = NULL;
							suppressor = new ostringstream();
							Logger::set_stream(suppressor);
							retval=new XMLObject(s);
							Logger::unset_stream();
							if (!suppressor->str().empty()) {
								if (retval != NULL) { delete retval; }
								retval=new StrObject(s);
							}
							delete suppressor;
						}
					}
				} 
			}
		} else {
			retval=new RawItem(s);
		}
	}
	return retval;
}
DataItem* DataItem::autoItem(const u_str& s) {
	DataItem* retval = NULL;
	if ( s.empty() ) {
		retval=NULL;
	} else {
		u_str::size_type b = s.find_first_not_of(UCS2(L" \r\n\t" )); //Test for first non-ws is a '<'
		if(s[b] != '<') {  
			retval=new UStrItem(s);
		} else {
			string::size_type e = s.find_last_not_of(UCS2(L" \r\n\t" )); //Test for last non-ws is a '>'
			if(s[e] != '>') { 
				retval=new UStrItem(s);
			} else {
				bool xml_dt_prolog_found = false;
				if(s.substr(b,6).compare(UCS2(L"<?xml "))==0) { 
					xml_dt_prolog_found = true;
					retval=new XMLObject(s);
				}
				if(!xml_dt_prolog_found) {  
					bool xmlns_found = false;
					if (s.find(UCS2(L"xmlns")) != string::npos) {
						xmlns_found = true;
						retval=new XMLObject(s);
					}
					if (!xmlns_found) {  //no xml prolog, no xmlns declaration, but < and > so try a suppressed xmlobject.
						ostringstream* suppressor = NULL;
						suppressor = new ostringstream();
						Logger::set_stream(suppressor);
						retval=new XMLObject(s);
						Logger::unset_stream();
						if (!suppressor->str().empty()) {
							if (retval != NULL) { delete retval; }
							retval=new UStrItem(s);
						}
						delete suppressor;
					}
				}
			} 
		}
	}
	return retval;
}

DataItem* DataItem::factory(std::string& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {
			retval= autoItem(s);
		} break;
		case di_raw: {
			retval= new RawItem(s);
		} break;
		case di_text: {
			retval= new StrObject(s);
		} break;
		case di_utext: {
			retval= new UStrItem(s);
		} break;
		case di_fragment: {
			retval= new FragmentObject(s);
		} break;
		case di_object: {
			retval= new XMLObject(s);
		} break;
		case di_null: {
			retval=NULL;
		}
	}
	return retval;
}
DataItem* DataItem::factory(const string& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {
			retval= autoItem(s);
		} break;
		case di_raw: {
			retval= new RawItem(s);
		} break;
		case di_text: {
			retval= new StrObject(s);
		} break;
		case di_utext: {
			retval= new UStrItem(s);
		} break;
		case di_fragment: {
			retval= new FragmentObject(s);
		} break;			
		case di_object: {
			retval= new XMLObject(s);
		} break;
		case di_null: {
			retval=NULL;
		}
	}
	return retval;
}
DataItem* DataItem::factory(const char* s,kind_type kind_it_is) {
	if (s != NULL) {
		string test(s);
		return factory(test,kind_it_is);
	} else {
		return NULL;
	}
}
DataItem* DataItem::factory(const XMLCh* s,kind_type kind_it_is) {
	if (s != NULL) {
		u_str test(s);
		return factory(test,kind_it_is);
	} else {
		return NULL;
	}
}

DataItem* DataItem::factory(xercesc::DOMDocument*& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_object:
		case di_auto: {
			retval =  new XMLObject(s);
		} break;
		case di_fragment: {
			retval= new FragmentObject(s);
		} break;
		case di_utext: {
			retval= new UStrItem(s);
		} break;
		case di_text: {
			retval= new StrObject(s);
		} break;
		case di_raw: { //we don't want to lose the utf-8 of this if possible.
			retval= new StrObject(s);
		} break;
		case di_null: {
			retval=NULL;
		}
	}
	return retval;
}
DataItem* DataItem::factory(const xercesc::DOMDocument*& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_object:
		case di_auto: {
			retval =  new XMLObject(s);
		} break;
		case di_fragment: {
			retval= new FragmentObject(s);
		} break;
		case di_utext: {
			retval= new UStrItem(s);
		} break;
		case di_text: {
			retval= new StrObject(s);
		} break;
		case di_raw: { //we don't want to lose the utf-8 of this if possible.
			retval= new StrObject(s);
		} break;
		case di_null: {
			retval=NULL;
		}
	}
	return retval;
}
DataItem* DataItem::factory(DataItem* s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {
			if ( s != NULL) { s->copy(retval); }
		} break;
		case di_fragment: {
			retval= new FragmentObject(*s);
		} break;
		case di_object: {
			if ( s != NULL) { 
				if (s->kind() == di_object) {
					s->copy(retval);
				} else { //upcast
					retval = new XMLObject(*s);	
					if (retval->empty()) {
						delete retval;
						s->copy(retval);
					}
				}
			}			
		} break;
		case di_utext: {
			if ( s != NULL) {
				u_str x = *s;
				retval = new UStrItem(x);
			}
		} break;
		case di_text: {
			if ( s != NULL) {
				std::string x = *s;
				retval = new StrObject(x);
			}
		} break;
		case di_raw: { 
			if ( s != NULL) {
				std::string x = *s;
				if (s->kind() == di_raw) {
					retval = new RawItem(x);
				} else { //crosscast, keep the utf-8
					retval = new StrObject(x);
				}
			}
		} break;
		case di_null: {
			retval=NULL;
		}
	}
	return retval;
}
DataItem* DataItem::factory(xercesc::DOMNode* const& pt,kind_type kind_it_is) {
	DataItem* retval = NULL;
	if (pt != NULL) {
		switch( kind_it_is ) {
			case di_auto: {
				switch ( pt->getNodeType() ) {
					case DOMNode::DOCUMENT_NODE: {
						retval = new XMLObject((const xercesc::DOMDocument*&)pt);
					} break;
					case DOMNode::ELEMENT_NODE: {
						retval = new XMLObject((const xercesc::DOMElement*&)pt);
					} break;
					case DOMNode::DOCUMENT_TYPE_NODE: 
					case DOMNode::DOCUMENT_FRAGMENT_NODE: 
					case DOMNode::NOTATION_NODE: 
					case DOMNode::ENTITY_NODE:
					case DOMNode::ENTITY_REFERENCE_NODE: {
						retval = new FragmentObject(pt);
					} break;
					case DOMNode::CDATA_SECTION_NODE: 
					case DOMNode::ATTRIBUTE_NODE:
					case DOMNode::PROCESSING_INSTRUCTION_NODE:
					case DOMNode::COMMENT_NODE: 
					case DOMNode::TEXT_NODE: {
						retval = new UStrItem(pt);
					} break;
					default: {
						*Logger::log << Log::fatal << Log::LI << "Node Type not recognised. Probably corrupt data." << Log::LO << Log::blockend;
					} break;
				} 
			} break;
			case di_fragment: {
				retval = new FragmentObject(pt);
			} break;
			case di_object: {
				retval = new XMLObject(pt);
			} break;
			case di_utext: {
				retval = new UStrItem(pt);
			} break;				
			case di_text: {
				retval = new StrObject(pt);
			} break;
			case di_raw: { //we don't want to lose the utf-8 of this if possible.
				retval= new StrObject(pt);
			} break;
			case di_null: break;
		}
	}
	return retval;
}
DataItem* DataItem::factory(u_str s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	if (! s.empty() ) {
		switch (kind_it_is) {
			case di_auto: {			
				if (! s.empty() ) {
					retval= autoItem(s);

//					retval= new UStrItem(s); //why are we not doing xml detection here?
				} else {
					retval=NULL;
				}
			} break;
			case di_raw: { //we don't want to lose the utf-8
				retval= new StrObject(s);
			} break;
			case di_utext: {
				retval = new UStrItem(s);
			} break;
			case di_text: {
				retval= new StrObject(s);
			} break;
			case di_fragment: {
				retval= new FragmentObject(s);
			} break;
			case di_object: {
				retval= new XMLObject(s);
			} break;
			case di_null: {
				retval=NULL;
			}
		}
	} else {
		retval=NULL;
	}
	return retval;
}
void DataItem::startup() {
	FragmentObject::startup();
}
void DataItem::shutdown() {
	FragmentObject::shutdown();
}
void DataItem::init() {
	FragmentObject::init();
}
void DataItem::finalise() {
	/*	
	 if ( ! ce_map.empty() ) {
	 *Logger::log << Log::error << Log::LI << "Error. Not all ObyxElements were deleted."  << Log::LO << Log::blockend;	
	 for( long_map::iterator imt = ce_map.begin(); imt != ce_map.end(); imt++) {
	 *Logger::log << Log::info << imt->second << Log::LO << Log::blockend;				
	 }
	 }
	 */	
	FragmentObject::finalise();
}

