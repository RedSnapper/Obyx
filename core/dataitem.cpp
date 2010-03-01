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
#include "strobject.h"
#include "xmlobject.h"
#include "fragmentobject.h"

#include "commons/environment/environment.h"
#include "commons/logger/logger.h"

DataItem::~DataItem() {
}

//results of b go into a, b is set to null. Both may start off as NULL!
void DataItem::append(DataItem*& a,DataItem*& b) { //this is static
	if (a != NULL) {
		if (b != NULL) { //if b is null, then we are appending nothing.
			XMLObject* x = dynamic_cast<XMLObject*>(a);
			if (x != NULL) {
				DOMNode* n;
				x->take(n);
				delete a;
				a = new FragmentObject(n);
			} 
			a->append(b);
		} 
	} else {
		a = b;
	}
	b = NULL;	
}

void DataItem::append(DataItem*&) { //Do nothing
}

DataItem* DataItem::factory(std::string& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {
			if (! s.empty() ) {
				string::size_type c = s.find_first_not_of(" \r\n\t" );
				if(s[c] == '<') { 
					retval=new XMLObject(s);
				} else {
					retval=new StrObject(s);
				}
			} else {
				retval=NULL;
			}
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
	return retval;
}
DataItem* DataItem::factory(const string& s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {
			if (! s.empty() ) {
				string::size_type c = s.find_first_not_of(" \r\n\t" );
				if(s[c] == '<') { 
					retval=new XMLObject(s);
				} else {
					retval=new StrObject(s);
				}
			} else {
				retval=NULL;
			}
		} break;
		case di_text: {
			retval= new StrObject(s);
		} break;
		case di_fragment: {
			retval= new FragmentObject(s);
		} break;			
		case di_object: {
			if (Environment::UseDeprecated) {
				if ( s.compare(0,10,"NAMESPACE#") == 0 ) {
					retval= new StrObject(s);
				} else if ( s.compare(0,8,"GRAMMAR#") == 0 ) {
					retval= new StrObject(s);
				} else {
					retval= new XMLObject(s);
				}
			} else {
				retval= new XMLObject(s);
			}
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
		case di_text: {
			string doc; 
			XML::Manager::parser()->writenode(s,doc);
			retval = new StrObject(doc);
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
		case di_text: {
			string doc; 
			XML::Manager::parser()->writenode(s,doc);
			retval = new StrObject(doc);
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
		case di_text: {
			if ( s != NULL) {
				std::string x = *s;
				retval = new StrObject(x);
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
						retval = new XMLObject((xercesc::DOMDocument*&)pt);
					} break;
					case DOMNode::ELEMENT_NODE: {
						retval = new XMLObject(pt);
					} break;
					case DOMNode::DOCUMENT_TYPE_NODE: 
					case DOMNode::DOCUMENT_FRAGMENT_NODE: 
					case DOMNode::NOTATION_NODE: 
					case DOMNode::TEXT_NODE: 
					case DOMNode::COMMENT_NODE: 
					case DOMNode::PROCESSING_INSTRUCTION_NODE:
					case DOMNode::ENTITY_NODE:
					case DOMNode::ENTITY_REFERENCE_NODE:
					case DOMNode::CDATA_SECTION_NODE: 
					case DOMNode::ATTRIBUTE_NODE: {
						retval = new FragmentObject(pt);
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
			case di_text: {
				string snode; XML::Manager::parser()->writenode(pt,snode);
				retval = new StrObject(snode);
			} break;
			case di_null: break;
		}
	}
	return retval;
}
//static DataItem* factory(std::string&,kind_type = di_auto);
DataItem* DataItem::factory(u_str s,kind_type kind_it_is) {
	DataItem* retval = NULL;
	switch (kind_it_is) {
		case di_auto: {			
			if (! s.empty() ) {
				retval= new FragmentObject(s);
			} else {
				retval=NULL;
			}
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
	return retval;
}

