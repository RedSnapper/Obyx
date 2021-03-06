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

#include <list>
#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "dataitem.h"
#include "xmlobject.h"
#include "strobject.h"
#include "fragmentobject.h"
#include "document.h"

XMLObject::u_str_map_type XMLObject::object_ns_map;
unsigned long XMLObject::ns_map_version = 0; //used to indicate if the namespace has changed.
//xercesc::DOMXPathNSResolver* 	XMLObject::xpnsr = nullptr;		//namespace resolver.
unsigned long 					XMLObject::xpnsr_v = 0;			//used to indicate if the namespace has changed.
DynamicContext*					XMLObject::xpather = nullptr;		//XPath2 context.
bool							XMLObject::modload = false;
XMLObject::xpp_map_type			XMLObject::xpp_map;
unsigned long long int 			XMLObject::xp_count = 0;


/* ==================== NON virtual methods. =========== */
/* protected */
XMLObject::XMLObject(const XMLObject* s) : DataItem(),dxx(0),x_doc(nullptr) {
	x_doc = (xercesc::DOMDocument*)(s); 
}
XMLObject::XMLObject(const xercesc::DOMDocument* s) : DataItem(),dxx(1),x_doc(nullptr) {
	if  ( s != nullptr ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}	
}
XMLObject::XMLObject(xercesc::DOMDocument*& s) : DataItem(),dxx(2),x_doc(s) {}
XMLObject::XMLObject(xercesc::DOMNode* s) : DataItem(),dxx(3),x_doc(nullptr) {
	if  ( s != nullptr ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}
}
/* Public Errors need to be caught higher up! */
XMLObject::XMLObject(const std::string s) : DataItem(),dxx(4),x_doc(nullptr) {
	x_doc = XML::Manager::parser()->loadDoc(s);
}
XMLObject::XMLObject(u_str s) : DataItem(),dxx(7),x_doc(nullptr) {
	x_doc = XML::Manager::parser()->loadDoc(s);
}
XMLObject::XMLObject(const XMLObject& src) : DataItem(),dxx(5),x_doc(src) {}
XMLObject::XMLObject(const DataItem& src) : DataItem(),dxx(6),x_doc(src) {}
void XMLObject::copy(XMLObject*& container) const {
	if  ( x_doc != nullptr ) {
		DOMDocument* doc = XML::Manager::parser()->newDoc(x_doc);
		container = new XMLObject(doc);
	}
}
void XMLObject::copy(DOMDocument*& container) const {
	if  ( x_doc != nullptr ) { // should really check for nullptr here.
		container = XML::Manager::parser()->newDoc(x_doc);
	}		
}
void XMLObject::take(DOMDocument*& container) {
//	del_cache(); // Delete the pnsr and the xpe cache.
	container = x_doc;
	x_doc = nullptr;
}
void XMLObject::take(DOMNode*& container) {
//	del_cache(); // Delete the pnsr and the xpe cache.
	container = x_doc;
	x_doc = nullptr;
}
/* ====================  VIRTUAL methods. =========== */
XMLObject::operator XMLObject*() { return this; }	
XMLObject::operator u_str() const {
	u_str doc; string errs;
	u_str no_empties = u"//*[not(node())][not(contains('[area|base|br|col|hr|img|input|link|meta|param|command|keygen|source]',local-name()))]";
	DOMText* et = x_doc->createTextNode(pcx("")); //emptystring
	XMLObject dit(et);
	const_cast<XMLObject*>(this)->xp(&dit,no_empties,DOMLSParser::ACTION_APPEND_AS_CHILDREN,false,errs);
	XML::Manager::parser()->writedoc(x_doc,doc);
	return doc;
}
XMLObject::operator std::string() const {
	string doc,errs;
	u_str no_empties = u"//*[not(node())][not(contains('[area|base|br|col|hr|img|input|link|meta|param|command|keygen|source]',local-name()))]";
	DOMText* et = x_doc->createTextNode(pcx("")); //emptystring
	XMLObject dit(et);
	const_cast<XMLObject*>(this)->xp(&dit,no_empties,DOMLSParser::ACTION_APPEND_AS_CHILDREN,false,errs);
	XML::Manager::parser()->writedoc(x_doc,doc);
	return doc;
}
XMLObject::operator xercesc::DOMDocument*() const {
	return x_doc;
}
void XMLObject::copy(DataItem*& container) const {
	if  ( x_doc != nullptr ) {
		DOMDocument* doc = XML::Manager::parser()->newDoc(x_doc);
		container = DataItem::factory(doc); 
	}		
}
XMLObject::operator xercesc::DOMNode*() const {
	return x_doc;	//check how this is being used?
}
long long XMLObject::size() const {
	if (x_doc != nullptr) {
		return 1;
	} else {
		return 0;
	}
}
bool XMLObject::empty() const {
	return x_doc == nullptr;
}
bool XMLObject::same(const DataItem* xtst) const {
	bool retval = false;
	const XMLObject* ox = dynamic_cast<const XMLObject*>(xtst);
	if ( x_doc != nullptr && ox != nullptr ) {
		retval = x_doc->isEqualNode(*ox);
	} else {
		if (x_doc == nullptr && xtst == nullptr) retval = true;
	}
	return retval;
}
void XMLObject::clear() {
	if (x_doc != nullptr) {
		x_doc->release();
		x_doc = nullptr;
	}
}
void XMLObject::trim() {
	//do nothing.
}
bool XMLObject::find(const DataItem* o,std::string& error_msg) {
	//find with an xpath.
	bool retval = false;
	u_str xpath;
	if (o != nullptr) xpath = *o;
	Sequence pt;
	DynamicContext* context = nullptr;
	retval = xp_result(xpath,pt,context,false,error_msg);
	if (retval && pt.getLength() > 0 ) {
		retval = true;
	}
	delete context;
	return retval;
}
bool XMLObject::find(const char* o,std::string& error_msg) {
	bool retval = false;
	u_str xpath;
	if (o != nullptr)  xpath = *o;
	Sequence pt;
	DynamicContext* context = nullptr;
	retval = xp_result(xpath,pt,context,false,error_msg);
	if (retval && pt.getLength()  > 0 ) {
		retval = true;
	}
	delete context;
	return retval;
}

bool XMLObject::find(const XMLCh* srch,std::string& error_msg) {
	bool retval = false;
	if (srch != nullptr && x_doc != nullptr) {
		u_str xpath(pcu(srch));
		Sequence pt;
		DynamicContext* context = nullptr;
		retval = xp_result(xpath,pt,context,false,error_msg);
		if (retval && pt.getLength()  > 0 ) {
			retval = true;
		}
		delete context;
	} else {
		retval = true;
	}
	return retval;
}

//Uses a global pnsr instead of a per document one.
void XMLObject::set_pnsr() { /* Set the pnsr with the latest list of namespaces if it needs it. */
	if (xpnsr_v != ns_map_version) {
		for (XMLObject::u_str_map_type::const_iterator s = object_ns_map.begin(); s != object_ns_map.end(); s++) {
			const u_str& nssig = s->first; const u_str& nsurl = s->second;
			xpather->setNamespaceBinding(pcx(nssig.c_str()),pcx(nsurl.c_str()));
		}
		xpnsr_v = ns_map_version;
	}
}

void XMLObject::rm_cache() {
	if (!xpp_map.empty()) {
		xpp_map_type::iterator it = xpp_map.begin();
		while ( it != xpp_map.end()) {
			delete (*it).second;
			it++;
		}
		xpp_map.clear();
	}
}

//void addFunction(XQUserFunction* fnDef); Maybe useful one day for implementing (or ignoring!) gap().
XQQuery* XMLObject::xpp(const u_str& xpath) {
	XQQuery* retval = nullptr;
	if (! xpath.empty() ) {
		xpp_map_type::iterator it = xpp_map.find(xpath);
		if (it != xpp_map.end()) {
			retval = (*it).second;
		} else {
			int flags = 0x12; //x02: NO_ADOPT_CONTEXT|x10 NO_DEFAULT_MODULES .
			set_pnsr();	//This only does something if it needs to.
			if (!modload) {
				modload = true;
				flags = 0x02; //load modules once.
			}
			retval = Manager::xqilla->parse(pcx(xpath.c_str()),xpather,nullptr,flags);
			xpp_map.insert(xpp_map_type::value_type(xpath,retval));
		}
	}
	return retval;
}

/* -- more non-virtual methods -- */
/* private */
bool XMLObject::xp_result(const u_str& xpath,Sequence& result,DynamicContext*& context,bool expected,std::string& err_message) {
	bool retval = true;
	if (x_doc && x_doc->getDocumentElement() != nullptr) {
		try {
			XQQuery* query = xpp(xpath);
			if (query != nullptr) {
				xp_count++;	//this is an xpath evaluation.
				context = query->createDynamicContext();
				context->setContextItem(((XercesConfiguration*)context->getConfiguration())->createNode(x_doc->getDocumentElement(),context));
				result = query->execute(context)->toSequence(context);
			} else {
				err_message = "Unknown xpath error."; retval = false;
			}
		}
		catch (XQException &e) { XML::Manager::transcode(pcu(e.getError()),err_message); retval = false; }
		catch (XQillaException &e) { XML::Manager::transcode(pcu(e.getString()),err_message); retval = false; }
		catch (DOMXPathException &e) { XML::Manager::transcode(pcu(e.getMessage()),err_message); retval = false; }
		catch (DOMException &e) {XML::Manager::transcode(pcu(e.getMessage()),err_message); retval = false;}
		catch (...) { err_message = "Unknown xpath error."; retval = false; }
	} else {
		if (expected) {
			XML::Manager::transcode(xpath,err_message);
			if (x_doc) {
				err_message.append(";  Document has no document (root) element");
			} else {
				err_message.append(";  Document is null.");
			}
		}
		retval = false;
	}
	return retval;
}

bool XMLObject::xp(const u_str& path,DataItem*& container,bool node_expected,std::string& error_str) {
	// GET a value at the xpath given
	bool retval = true; //retval represents existence not failure.
	if (!path.empty() ) {
		if (path.rfind(u"-gap()",path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			bool want_value = false;
			u_str xp_path(path);
			if (xp_path.rfind(u"/value()",xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
				want_value = true;
			}
			Sequence result;
			DynamicContext* context = nullptr;
			retval = xp_result(xp_path,result,context,node_expected,error_str);
			if (retval) { //otherwise return empty.
				XMLSize_t sslena = result.getLength();
				if (sslena == 0) retval = false;
				for ( XMLSize_t ai = 0; ai < sslena; ai++) {
					const Item::Ptr item = result.item(ai);
					if (item.notNull()) {
						DataItem* ditem = nullptr;
						if ( item->isNode() ) {
							DOMNode *xn = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
							DOMNode::NodeType xnt = xn->getNodeType();
							switch (xnt) {
								case DOMNode::PROCESSING_INSTRUCTION_NODE:
								case DOMNode::COMMENT_NODE: {
									if (want_value) {
										u_str xs = pcu(xn->getNodeValue());
										ditem = DataItem::factory(xs,di_utext);
									} else {
										ditem = DataItem::factory(xn);
									}
								} break;
								case DOMNode::TEXT_NODE:
								case DOMNode::CDATA_SECTION_NODE:
								case DOMNode::ATTRIBUTE_NODE: {
									u_str xs = pcu(xn->getNodeValue());
									ditem = DataItem::factory(xs,di_utext);
								} break;
								default: {
									ditem = DataItem::factory(xn);
								} break;
							}
						} else {
							u_str xs;
							const XMLCh* ias=item->asString(context); //item may be null. in which case the string constructor will fail.
							if (ias != nullptr) {
								xs= pcu(ias);
							}
							ditem = DataItem::factory(xs);
						}
						DataItem::append(container,ditem); //either/both could be nullptr. and we may need to convert from one type to another.
					}
				}
				if (sslena == 0 && node_expected) {
					std::string xpath; XML::Manager::transcode(path,xpath);
					error_str = "While attempting a get, the xpath " + xpath + " returned no nodes.";
					retval=false;
				}
			}
			result.clear();
			if (context != nullptr && retval) {
				delete context;
				context=nullptr;
			}
		} else {
			if (node_expected) {
				std::string xpath; XML::Manager::transcode(path,xpath);
				error_str = "While attempting a get, the xpath " + xpath + " included an insertion point.";												
				retval=false;
			}
		}
	} else {
		copy(container);
	}
	return retval;
}
bool XMLObject::xp(const DataItem* ins,const u_str& path,DOMLSParser::ActionType action,bool node_expected,std::string& error_str) {
	// SET a value at the xpath given
	bool retval = true; //retval is existence, not failure.
	Sequence xpr;
	DynamicContext* context = nullptr;
	if( (kind() != di_object) && node_expected) {
		error_str.append(" Inappropriate for an xpath.");
		std::string xpath; XML::Manager::transcode(path,xpath);
		std::string me = *this;
		error_str = "When attempting an xpath " + xpath + " the object '" + me + "' was not recognised as XML." ;
		retval=false;
	} else {
		retval = xp_result(path,xpr,context,node_expected,error_str);
		if (retval && error_str.empty()) {
			XMLSize_t sslen = xpr.getLength();
			if ( sslen > 0 ) { //node is found.
				for ( XMLSize_t i = 0; i < sslen; i++) {
					const Item::Ptr item = xpr.item(i);
					if (item->isNode()) {
						DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
						u_str insval;
						if (ins != nullptr) {
							const XMLObject* ox = dynamic_cast<const XMLObject*>(ins);
							if ( ox == nullptr ) {  //ins is a string.
								const FragmentObject* fg = dynamic_cast<const FragmentObject*>(ins);
								if ( fg == nullptr ) {  //ins is a string.
									insval = *ins; 
									XML::Manager::parser()->insertContext(x_doc,pt,insval,action);
								} else { //fragment.
									XML::Manager::parser()->insertContext(x_doc,pt,fg->fragment,action);					
								}
							} else {
								XML::Manager::parser()->insertContext(x_doc,pt,ox->x_doc,action);					
							}
						} else {
							XML::Manager::parser()->insertContext(x_doc,pt,insval,action);
						}
					} else {
						if (node_expected) {
							if (error_str.empty()) {
								std::string xpath; XML::Manager::transcode(path,xpath);
								error_str = "When attempting a set, the xpath " + xpath + " result was not a node.";
							}
						}
						retval	= false;					
					}
				}
			} else {
				string::size_type slpoint = path.rfind('/');
				string::size_type apoint = path.rfind('@');
				if ( apoint == slpoint+1 ) {
					u_str apath=path.substr(0,slpoint);
					u_str aname=path.substr(apoint+1);
					Sequence xpra;
					DynamicContext* xcontext = nullptr;
					retval = xp_result(apath,xpra,xcontext,node_expected,error_str);
					if (retval) {
						XMLSize_t sslena = xpra.getLength();
						if (sslena > 0) {
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = xpra.item(ai);
								if (item->isNode()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									if ( pt->getNodeType() == DOMNode::ELEMENT_NODE ) {
										DOMElement* enod = (DOMElement*)pt;
										if ( ins != nullptr && ! ins->empty() ) {
											u_str xvalue = *ins;
											DOMAttr* dnoda = x_doc->createAttribute(pcx(aname.c_str()));
											dnoda->setNodeValue(pcx(xvalue.c_str()));
											enod->setAttributeNode(dnoda);
										} else { //in obyx, setting an attribute to nothing deletes it.								
											enod->removeAttribute(pcx(aname.c_str()));
										}
									}
								}
							}
						} else {
							if (node_expected) {
								if (error_str.empty()) {
									std::string epath; XML::Manager::transcode(apath,epath);
									error_str = "When attempting an attribute set, the xpath " + epath + " did not return a node position.";
								}
								retval=false;
							}
						}
					} else {
						if (node_expected) {
							if (error_str.empty()) {
								std::string epath; XML::Manager::transcode(apath,epath);
								error_str = "When attempting an attribute set, the xpath " + epath + " did not return a node position.";
							}
							retval=false;
						}
					}
					delete xcontext;
				} else {
					string::size_type pathlen = path.size();
					string::size_type com_pos = path.rfind(u"/comment()",pathlen-10);
					if (com_pos != string::npos) {
						u_str apath=path.substr(0,com_pos);
						Sequence xpra;
						DynamicContext* xcontext = nullptr;
						retval = xp_result(apath,xpra,xcontext,node_expected,error_str);
						if (retval) { //retval = existence, not failure.
							XMLSize_t sslena = xpra.getLength();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = xpra.item(ai);
								if (item->isNode()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									if ( pt->getNodeType() == DOMNode::ELEMENT_NODE ) {
										DOMElement* enod = (DOMElement*)pt;
										if ( ins != nullptr && ! ins->empty() ) {
											u_str xvalue  = *ins;
											DOMNode* dnoda = x_doc->createComment(pcx(xvalue.c_str()));
											dnoda->setNodeValue(pcx(xvalue.c_str()));
											enod->appendChild(dnoda);
										} 
									}
								}
							}
						} else {
							if (node_expected) {
								if (error_str.empty()) {
									std::string epath; XML::Manager::transcode(apath,epath);
									error_str = "When attempting to set a comment, the xpath " + epath + " did not return a node position.";
								}
								retval=false;
							}
						}
						delete xcontext;
					} else {
						if (node_expected) {
							if (error_str.empty()) {
								std::string epath; XML::Manager::transcode(path,epath);
								error_str = "When attempting a set, the xpath " + epath + " did not return a node position.";
							}
							retval=false;
						}
					}
				}			
			}
			if (x_doc->getDocumentElement() == nullptr) {
				x_doc->release(); x_doc = nullptr;
			}
		} else {
			if (node_expected) {
				if (error_str.empty()) {
					std::string epath; XML::Manager::transcode(path,epath);
					error_str = "When attempting a set, the xpath " + epath + " failed.";
				}
				retval=false;
			}
		}
		delete context;
	}
	return retval;
}
bool XMLObject::sort(const u_str& path,const u_str& sortpath,bool ascending,bool node_expected,std::string& error_str) {
	bool retval = true;
	if (!path.empty() ) { //currently, this is a redundant check.
		if (path.rfind(u"-gap()",path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			u_str xp_path(path);
			if (xp_path.rfind(u"/value()",xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
			}
			Sequence result;
			DynamicContext* context = nullptr;
			retval = xp_result(xp_path,result,context,node_expected,error_str);
			if (retval) { //otherwise return empty.
				XMLSize_t sslena = result.getLength();
				if (sslena > 0) {
					std::list< pair<u_str,XMLObject*> > lex_results_for_sorting;
					std::list< pair<double,XMLObject*> > num_results_for_sorting;
					bool using_lex = true, tested_lex=false; 
					//if sslena < 1 then life is easy for a sort... but here let's assume not first.
					for ( XMLSize_t ai = 0; ai < sslena; ai++) {
						const Item::Ptr item = result.item(ai);
						if (item.notNull()) {
							XMLObject* ditem = nullptr;
							if ( item->isNode() ) {
								ditem = new XMLObject((DOMNode*)item->getInterface(XercesConfiguration::gXerces));
								DataItem* sortitem = nullptr;
								ditem->xp(sortpath,sortitem,true,error_str);
								u_str sstr; double dnum = NAN;
								if (sortitem != nullptr) {
									sstr = *sortitem; delete sortitem;
								} else {
									if (!error_str.empty()) {
										error_str.append(*ditem);
									}
								}
								if (!tested_lex) { //work out based on the first value if this is a string or a number.
									if (sstr.empty()) { //empty is nan. ie, there's no value set for num results yet.
										num_results_for_sorting.push_back( pair<double,XMLObject*>(dnum,ditem) );
									} else {
										dnum = XMLObject::real(sstr);
										using_lex = (dnum == NAN);
										tested_lex = true;
									}
								} else {
									if (!using_lex) {
										dnum = XMLObject::real(sstr);
									}
								} //dnum is now converted for all iterations.
								if (using_lex) {
									lex_results_for_sorting.push_back( pair<u_str,XMLObject*>(sstr,ditem) );
								} else {
									num_results_for_sorting.push_back( pair<double,XMLObject*>(dnum,ditem) );
								}
							} else {
								error_str = "While expecting node results for a search, the xpath returned text.";												
								retval=false;
							}
						}
					}
					if (using_lex) {
						lex_results_for_sorting.sort();
						if (ascending) {						
							std::list< pair<u_str,XMLObject*> >::iterator i = lex_results_for_sorting.begin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = result.item(ai);
								if (item.notNull()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						} else {
							std::list< pair<u_str,XMLObject*> >::reverse_iterator i = lex_results_for_sorting.rbegin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = result.item(ai);
								if (item.notNull()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						}
						while (!lex_results_for_sorting.empty()) {
							delete lex_results_for_sorting.front().second;
							lex_results_for_sorting.pop_front(); 
						}
						lex_results_for_sorting.clear();
					} else {
						num_results_for_sorting.sort();
						if (ascending) {						
							std::list< pair<double,XMLObject*> >::iterator i = num_results_for_sorting.begin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = result.item(ai);
								if (item.notNull()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						} else {
							std::list< pair<double,XMLObject*> >::reverse_iterator i = num_results_for_sorting.rbegin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								const Item::Ptr item = result.item(ai);
								if (item.notNull()) {
									DOMNode* pt = (DOMNode*)item->getInterface(XercesConfiguration::gXerces);
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						}
						while (!num_results_for_sorting.empty()) {
							delete num_results_for_sorting.front().second;
							num_results_for_sorting.pop_front(); 
						}
					}
				} else {
					if (node_expected) {
						std::string epath; XML::Manager::transcode(path,epath);
						error_str = "While attempting a get, the xpath " + epath + " returned no nodes.";												
						retval=false;
					}
				}
			} else {
				if (node_expected) {
					std::string epath; XML::Manager::transcode(path,epath);
					error_str = "While attempting a get, the xpath " + epath + " returned an empty result.";												
					retval=false;
				}
			}
			delete context;
		} else {
			if (node_expected) {
				std::string epath; XML::Manager::transcode(path,epath);
				error_str = "While attempting a sort, the xpath " + epath + " included an insertion point.";												
				retval=false;
			}
		}
	} 
	return retval;
}
bool XMLObject::setns(const u_str& code, const u_str& signature) {
	if ( signature.empty() ) {
		u_str_map_type::iterator it = object_ns_map.find(code);
		object_ns_map.erase(it);
	} else {
		XML::Manager::parser()->resourceHandler->installGrammar(signature); //still need this for reciprocal grammars.
		pair<u_str_map_type::iterator, bool> ins = object_ns_map.insert(u_str_map_type::value_type(code, signature));
		if (!ins.second) {
			object_ns_map.erase(ins.first);
			object_ns_map.insert(u_str_map_type::value_type(code, signature));
		}
	}
	ns_map_version++;
	return true;
}
bool XMLObject::getns(const u_str& code, u_str& result,bool release) {
	bool retval = false;
	u_str_map_type::iterator it = object_ns_map.find(code);
	if (it != object_ns_map.end()) {
		result = ((*it).second);
		retval = true;
		if (release) {
			object_ns_map.erase(it);
			ns_map_version++;
		}
	}
	return retval;
}

void XMLObject::listns() {
	*Logger::log << Log::subhead << Log::LI << "Namespaces" << Log::LO << Log::LI ;
	for (XMLObject::u_str_map_type::const_iterator s = object_ns_map.begin(); s != object_ns_map.end(); s++) {
		string nsn,nsu;
		XML::Manager::transcode(s->first,nsn);
		XML::Manager::transcode(s->second,nsu);
		if ( ! nsn.empty() ) {
			*Logger::log << Log::LI << Log::II << nsn << Log::IO;
			if ( nsu.empty() ) {
				*Logger::log << Log::II << "--empty--" << Log::IO;
			} else {
				*Logger::log << Log::II << nsu << Log::IO;
			}
			*Logger::log << Log::LO;
		}
	}
	*Logger::log << Log::LO << Log::blockend;
}
void XMLObject::init() {
	modload = false;
	xp_count = 0;
	xpather = Manager::xqilla->createContext((XQilla::Language)(XQilla::XPATH2_FULLTEXT),Manager::xc); //XQilla::XPATH3 is available, maybe.
	setns(u"xs",u"http://www.w3.org/2001/XMLSchema");
}

void XMLObject::finalise() {
	for( xpp_map_type::iterator i = xpp_map.begin(); i != xpp_map.end(); i++) {
		delete i->second;
	}
	xpp_map.clear();
	delete xpather; xpather=nullptr;
	modload= false;
	object_ns_map.clear();
}

XMLObject::~XMLObject() {
	if (x_doc != nullptr) {
		x_doc->release();
		x_doc = nullptr;
	}	
}
#pragma mark static utility
//---------------------------------------------------------------------------
void XMLObject::trim(u_str& str) {
	u_str delims = u" \t\r\n";
	u_str::size_type notwhite = str.find_first_not_of(delims);
	str.erase(0,notwhite);
	notwhite = str.find_last_not_of(delims);
	str.erase(notwhite+1);	
}

void XMLObject::rtrim(u_str& str) {
	u_str delims = u" \t\r\n";
	u_str::size_type notwhite = str.find_last_not_of(delims);
	str.erase(notwhite+1);	
}

//-----------------------------------------------------------------------------
pair<unsigned long long,bool> XMLObject::hex(const u_str& s) { //Given a string, returns a natural from any hex that it STARTS with.
	bool isnumber = false;
	u_str::const_iterator in = s.begin();
	u_str::const_iterator out = s.end();
	if ( in != out ) isnumber = true;
	unsigned long long val = 0;
	bool ended = false;
	while (!ended && in != out) {
		switch (*in++) {
			case '0': val = val << 4; break;
			case '1': val = (val << 4) + 1; break;
			case '2': val = (val << 4) + 2; break;
			case '3': val = (val << 4) + 3; break;
			case '4': val = (val << 4) + 4; break;
			case '5': val = (val << 4) + 5; break;
			case '6': val = (val << 4) + 6; break;
			case '7': val = (val << 4) + 7; break;
			case '8': val = (val << 4) + 8; break;
			case '9': val = (val << 4) + 9; break;
			case 'a': val = (val << 4) + 10; break;
			case 'b': val = (val << 4) + 11; break;
			case 'c': val = (val << 4) + 12; break;
			case 'd': val = (val << 4) + 13; break;
			case 'e': val = (val << 4) + 14; break;
			case 'f': val = (val << 4) + 15; break;
			case 'A': val = (val << 4) + 10; break;
			case 'B': val = (val << 4) + 11; break;
			case 'C': val = (val << 4) + 12; break;
			case 'D': val = (val << 4) + 13; break;
			case 'E': val = (val << 4) + 14; break;
			case 'F': val = (val << 4) + 15; break;
			default: {
				isnumber = false;
				ended=true; 
				in--;
			} break;
		}
	}
	return pair<unsigned long long,bool>(val,isnumber);
}

double XMLObject::real(const u_str& s) {
	double retval = NAN;
	if (!s.empty()) {
		if (s.size() > 2 && s[0]=='0' && ( s[1]=='x' || s[1]=='X' )) {
			pair<unsigned long long,bool> rsp = hex(s.substr(2));
			if (rsp.second) {
				retval = rsp.first;
			}
		} else {
			try {
				retval = XMLDouble(pcx(s.c_str())).getValue();
			} catch (...) {
				retval = NAN;
			}
		}
	}
	return retval;
}

pair<unsigned long long,bool> XMLObject::znatural(const u_str& s) { //Given a u_str, returns a natural from any digits that it STARTS with.
	bool isnumber = false;
	unsigned long long val = 0;
	if (!s.empty()) {
		if (s.size() > 2 && s[0]=='0' && ( s[1]=='x' || s[1]=='X' )) {
			pair<unsigned long long,bool> rsp = hex(s.substr(2));
			if (rsp.second) {
				isnumber= true;
				val = rsp.first;
			}
		} else {
			u_str::const_iterator in = s.begin();
			u_str::const_iterator out = s.end();
			while(in != out && isdigit(*in)) {
				isnumber= true;
				val = val * 10 + *in - '0';
				in++;
			}
		}
	}
	return pair<unsigned long long,bool>(val,isnumber);
}


bool XMLObject::npsplit(const u_str& basis,pair<u_str,u_str>& result,bool& expected) {
	expected = false; 
	string::size_type pos = basis.find_first_of('#');
	if (pos == string::npos) { //
		result.first = basis;
		return false;
	} else {
		result.first = basis.substr(0, pos);
		result.second = basis.substr(pos+1, string::npos);
		if (! result.first.empty()) {
			if (*(result.first.rbegin()) == '!') { 
				expected = true; 
				result.first.resize(result.first.size()-1);  //remove the final character
			}
			if (!result.second.empty() && result.second[0] == '#') { result.second.erase(0,1); } //legacy double #
			return ! result.first.empty(); //ie, "!#wibble" is an invalid namepath.
		} else {
			return false;
		}
	}
}







