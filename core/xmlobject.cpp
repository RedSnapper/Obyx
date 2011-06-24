/*
 * xmlobject.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlobject.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <list>
#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "dataitem.h"
#include "xmlobject.h"
#include "strobject.h"
#include "fragmentobject.h"

XMLObject::u_str_map_type XMLObject::object_ns_map;
unsigned long XMLObject::ns_map_version = 0; //used to indicate if the namespace has changed.

/* ==================== NON virtual methods. =========== */
/* protected */
XMLObject::XMLObject(const XMLObject* s) : DataItem(),x(0),xpnsr_v(0),xpnsr(NULL),x_doc(NULL) { 
	x_doc = (xercesc::DOMDocument*)(s); 
}
XMLObject::XMLObject(const xercesc::DOMDocument* s) : DataItem(),x(1),xpnsr_v(0),xpnsr(NULL),x_doc(NULL) {
	if  ( s != NULL ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}	
}
XMLObject::XMLObject(xercesc::DOMDocument*& s) : DataItem(),x(2),xpnsr_v(0),xpnsr(NULL),x_doc(s) {}
XMLObject::XMLObject(const xercesc::DOMNode* s) : DataItem(),x(3),xpnsr_v(0),xpnsr(NULL),x_doc(NULL) {
	if  ( s != NULL ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}	
}
/* Public Errors need to be caught higher up! */
XMLObject::XMLObject(const std::string s) : DataItem(),x(4),xpnsr_v(0),xpnsr(NULL),x_doc(NULL) { 
	x_doc = XML::Manager::parser()->loadDoc(s);
	if (x_doc != NULL) {
		x_doc->normalize();
	}
}
XMLObject::XMLObject(u_str s) : DataItem(),x(7),xpnsr_v(0),xpnsr(NULL),x_doc(NULL) { 
	x_doc = XML::Manager::parser()->loadDoc(s);
}
XMLObject::XMLObject(const XMLObject& src) : DataItem(),x(5),x_doc(src) {}
XMLObject::XMLObject(const DataItem& src) : DataItem(),x(6),x_doc(src) {}
void XMLObject::copy(XMLObject*& container) const {
	if  ( x_doc != NULL ) {
		DOMDocument* doc = XML::Manager::parser()->newDoc(x_doc);
		container = new XMLObject(doc);
	}
}
void XMLObject::copy(DOMDocument*& container) const {
	if  ( x_doc != NULL ) { // should really check for NULL here.
		container = XML::Manager::parser()->newDoc(x_doc);
	}		
}
void XMLObject::take(DOMDocument*& container) {
	container = x_doc;
	x_doc = NULL;
}
void XMLObject::take(DOMNode*& container) {
	container = x_doc;
	x_doc = NULL;
}

/* ====================  VIRTUAL methods. =========== */
XMLObject::operator XMLObject*() { return this; }	
XMLObject::operator u_str() const {
	u_str doc; 
	XML::Manager::parser()->writedoc(x_doc,doc);
	return doc;
}
XMLObject::operator std::string() const {
	string doc; 
	XML::Manager::parser()->writedoc(x_doc,doc);
	return doc;
}
XMLObject::operator xercesc::DOMDocument*() const {
	return x_doc;
}
void XMLObject::copy(DataItem*& container) const {
	if  ( x_doc != NULL ) {
		DOMDocument* doc = XML::Manager::parser()->newDoc(x_doc);
		container = DataItem::factory(doc); 
	}		
}
XMLObject::operator xercesc::DOMNode*() const {
	return x_doc;	//check how this is being used?
}
long long XMLObject::size() const {
	if (x_doc != NULL) {
		return 1;
	} else {
		return 0;
	}
}
bool XMLObject::empty() const {
	return x_doc == NULL;
}
bool XMLObject::same(const DataItem* xtst) const {
	bool retval = false;
	const XMLObject* ox = dynamic_cast<const XMLObject*>(xtst);
	if ( x_doc != NULL && ox != NULL ) {
		retval = x_doc->isEqualNode(*ox);
	} else {
		if (x_doc == NULL && xtst == NULL) retval = true;
	}
	return retval;
}
void XMLObject::clear() {
	del_pnsr(); // Delete the pnsr and the xpe cache.
	if (x_doc != NULL) {
		x_doc->release();
		x_doc = NULL;
//		XML::Manager::resetDocPool(); //This resets the cache of the entire set of all docs
	}
}
void XMLObject::trim() {
	//do nothing.
}
bool XMLObject::find(const DataItem* o,std::string& error_msg) {
	//find with an xpath.
	bool retval = false;
	u_str xpath;
	if (o != NULL)  xpath = *o;
	DOMXPathResult* pt = NULL;
	retval = xp_result(xpath,pt,error_msg);
	if (retval && pt != NULL && pt->getSnapshotLength()  > 0 ) {
		pt->release();
		retval = true;
	}
	return retval;
}
bool XMLObject::find(const char* o,std::string& error_msg) {
	bool retval = false;
	u_str xpath;
	if (o != NULL)  xpath = *o;
	DOMXPathResult* pt = NULL;
	retval = xp_result(xpath,pt,error_msg);
	if (retval && pt != NULL && pt->getSnapshotLength()  > 0 ) {
		pt->release();
		retval = true;
	}
	return retval;
}

bool XMLObject::find(const XMLCh* srch,std::string& error_msg) {
	bool retval = false;
	if (srch != NULL && x_doc != NULL) {
		u_str xpath(srch);
		DOMXPathResult* pt = NULL;
		retval = xp_result(xpath,pt,error_msg);
		if (retval && pt != NULL && pt->getSnapshotLength()  > 0 ) {
			pt->release();
			retval = true;
		}
	} else {
		retval = true;
	}
	return retval;
}

void XMLObject::set_pnsr() { /* Set the pnsr with the latest list of namespaces if it needs it. */
	if (xpnsr_v != ns_map_version) {
		if ( xpnsr != NULL ) { 
			xpnsr->release(); xpnsr= NULL;
		}		
	}
	if ( x_doc != NULL && xpnsr == NULL ) { 
		xpnsr = x_doc->createNSResolver(NULL); 
	}
	if (xpnsr_v != ns_map_version && xpnsr != NULL) {
		for (XMLObject::u_str_map_type::const_iterator s = object_ns_map.begin(); s != object_ns_map.end(); s++) {
			const u_str& nssig = s->first; const u_str& nsurl = s->second;
			xpnsr->addNamespaceBinding(nssig.c_str(),nsurl.c_str());
		}
		xpnsr_v = ns_map_version;
	}
}

void XMLObject::del_pnsr() { /* Set the pnsr with the latest list of namespaces if it needs it. */
	if ( xpnsr != NULL ) { 
		xpnsr->release(); xpnsr= NULL;
		if (!xpe_map.empty()) {
			xpe_map_type::iterator it = xpe_map.begin();
			while ( it != xpe_map.end()) {
				DOMXPathExpression*& x = (*it).second;
				x->release();
				it++;
			}
			xpe_map.clear();
		}
	}		
}

DOMXPathExpression* XMLObject::xpe(const u_str& xpath) {
	// cached xpath expressions.
	DOMXPathExpression* retval = NULL;
	if (! xpath.empty() ) {
		xpe_map_type::iterator it = xpe_map.find(xpath);
		if (it != xpe_map.end()) {
			retval = (*it).second;
		} else {
			retval = x_doc->createExpression(xpath.c_str(),xpnsr);
			xpe_map.insert(xpe_map_type::value_type(xpath, retval));
		}
	}
	return retval;
}


/* -- more non-virtual methods -- */
/* private */
bool XMLObject::xp_result(const u_str& xpath,DOMXPathResult*& result,std::string& err_message) {
	bool retval = true;
	set_pnsr();	//This only does something if it needs to.
	try {
		// DOMXPathExpression*
		DOMXPathExpression* parsedExpression = xpe(xpath);
		result = parsedExpression->evaluate(x_doc->getDocumentElement(),DOMXPathResult::SNAPSHOT_RESULT_TYPE, NULL);
	} 
	catch (XQException &e) {
		XML::Manager::transcode(e.getError(),err_message);
		retval = false;
	}
	catch (XQillaException &e) {
		XML::Manager::transcode(e.getString(),err_message);
		retval = false;
	}					
	catch (DOMXPathException &e) { 
		XML::Manager::transcode(e.getMessage(),err_message);
		retval = false;
	}
	catch (DOMException &e) {
		XML::Manager::transcode(e.getMessage(),err_message);
		retval = false;
	}
	catch (...) {
		err_message = "unknown xpath error.";
		retval = false;
	}
	return retval;
}
//with get the value of this at xpath set by input 
//although the results of an xpath may be multiple, here we must glob them together.
//Container must be empty when we get here.
bool XMLObject::xp(const u_str& path,DataItem*& container,bool node_expected,std::string& error_str) {
	bool retval = true; //retval represents existence not failure.
	if (!path.empty() ) { //currently, this is a redundant check.
		if (path.rfind(UCS2(L"-gap()"),path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			u_str xp_path(path);
			DOMXPathResult* result = NULL;
			bool want_value = false;
			if (xp_path.rfind(UCS2(L"/value()"),xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
				want_value = true;
			}
			retval = xp_result(xp_path,result,error_str);
			if (retval && result != NULL) { //otherwise return empty.			
				XMLSize_t sslena = result->getSnapshotLength();
				if (sslena == 0) retval = false;
				for ( XMLSize_t ai = 0; ai < sslena; ai++) {
					if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
						DataItem* item = NULL;
						if ( result->isNode() ) {
							DOMNode* xn = result->getNodeValue();
							DOMNode::NodeType xnt = xn->getNodeType();
							switch (xnt) {
								case DOMNode::PROCESSING_INSTRUCTION_NODE: 
								case DOMNode::COMMENT_NODE: {
									if (want_value) {
										u_str xs(xn->getNodeValue());
										item = DataItem::factory(xs,di_utext);
									} else {
										item = DataItem::factory(xn);
									}
								} break;
								case DOMNode::TEXT_NODE:
								case DOMNode::CDATA_SECTION_NODE:
								case DOMNode::ATTRIBUTE_NODE: {
									u_str xs(xn->getNodeValue());
									item = DataItem::factory(xs,di_utext);
								} break;
								default: {
									item = DataItem::factory(xn);
								} break;
							}
						} else {
							u_str xs(result->getStringValue());
							item = DataItem::factory(xs);
						}
						DataItem::append(container,item); //either/both could be NULL. and we may need to convert from one type to another.
					}
				}
				delete result; result = NULL;
				if (sslena == 0 && node_expected) {
					std::string xpath; XML::Manager::transcode(path,xpath);
					error_str = "While attempting a get, the xpath " + xpath + " returned no nodes.";												
					retval=false;
				}
			} else {
				if (node_expected) {
					std::string xpath; XML::Manager::transcode(path,xpath);
					error_str = "While attempting a get, the xpath " + xpath + " returned an empty result.";												
					retval=false;
				}
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
	bool retval = true;
	DOMXPathResult* xpr = NULL;
	retval = xp_result(path,xpr,error_str);	
	if (retval && xpr != NULL) {
		XMLSize_t sslen = xpr->getSnapshotLength();
		if ( sslen > 0 ) { //node is found.
			for ( XMLSize_t i = 0; i < sslen; i++) {
				if (xpr->snapshotItem(i)) {
					DOMNode* pt = xpr->getNodeValue();
					if (pt != NULL) {
						const XMLObject* ox = dynamic_cast<const XMLObject*>(ins);
						if ( ox == NULL ) {  //ins is a string.
							const FragmentObject* fg = dynamic_cast<const FragmentObject*>(ins);
							if ( fg == NULL ) {  //ins is a string.
								u_str insval;
								if (ins != NULL) {
									insval = *ins; 
								}
								XML::Manager::parser()->insertContext(x_doc,pt,insval,action);
							} else { //fragment.
								XML::Manager::parser()->insertContext(x_doc,pt,fg->fragment,action);					
							}
						} else {
							XML::Manager::parser()->insertContext(x_doc,pt,ox->x_doc,action);					
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
				} else {
					// should never get here!
				}
			}
		} else {
			string::size_type slpoint = path.rfind('/');
			string::size_type apoint = path.rfind('@');
			if ( apoint == slpoint+1 ) {
				u_str apath=path.substr(0,slpoint);
				u_str aname=path.substr(apoint+1);
				DOMXPathResult* xpra = NULL;
				retval = xp_result(apath,xpra,error_str);
				if (retval && xpra != NULL) {
					XMLSize_t sslena = xpra->getSnapshotLength();
					if (sslena > 0) {
						for ( XMLSize_t ai = 0; ai < sslena; ai++) {
							if (xpra->snapshotItem(ai)) {
								DOMNode* pt = xpra->getNodeValue();
								if ( pt != NULL ) {
									if ( pt->getNodeType() == DOMNode::ELEMENT_NODE ) {
										DOMElement* enod = (DOMElement*)pt;
										if ( ins != NULL && ! ins->empty() ) {
											u_str xvalue = *ins;
											DOMAttr* dnoda = x_doc->createAttribute(aname.c_str());
											dnoda->setNodeValue(xvalue.c_str());
											enod->setAttributeNode(dnoda);
										} else { //in obyx, setting an attribute to nothing deletes it.								
											enod->removeAttribute(aname.c_str());
										}
									}
								} else {
									// should never get here!
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
					xpra->release();
					xpra = NULL;
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
				string::size_type pathlen = path.size();
				string::size_type com_pos = path.rfind(UCS2(L"/comment()"),pathlen-10);
				if (com_pos != string::npos) {
					u_str apath=path.substr(0,com_pos);
					DOMXPathResult* xpra = NULL;
					retval = xp_result(apath,xpra,error_str);
					if (retval && xpra != NULL) {
						XMLSize_t sslena = xpra->getSnapshotLength();
						for ( XMLSize_t ai = 0; ai < sslena; ai++) {
							if (xpra->snapshotItem(ai)) {
								DOMNode* pt = xpra->getNodeValue();
								if ( pt != NULL ) {
									if ( pt->getNodeType() == DOMNode::ELEMENT_NODE ) {
										DOMElement* enod = (DOMElement*)pt;
										if ( ins != NULL && ! ins->empty() ) {
											u_str xvalue  = *ins;
											DOMNode* dnoda = x_doc->createComment(xvalue.c_str());
											dnoda->setNodeValue(xvalue.c_str());
											enod->appendChild(dnoda);
										} 
									}
								} else {
									// should never get here!
								}
							}
						}
						xpra->release();
						xpra = NULL;
					} else {
						if (node_expected) {
							if (error_str.empty()) {
								std::string epath; XML::Manager::transcode(apath,epath);
								error_str = "When attempting to set a comment, the xpath " + epath + " did not return a node position.";
							}
							retval=false;
						}
					}
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
		xpr->release();
		xpr = NULL;
		if (x_doc->getDocumentElement() == NULL) {
			del_pnsr(); // Delete the pnsr and the xpe cache.
			x_doc->release(); x_doc = NULL;
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
	return retval;
}
bool XMLObject::sort(const u_str& path,const u_str& sortpath,bool ascending,bool node_expected,std::string& error_str) {
	bool retval = true;
	if (!path.empty() ) { //currently, this is a redundant check.
		if (path.rfind(UCS2(L"-gap()"),path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			u_str xp_path(path);
			DOMXPathResult* result = NULL;
//			bool want_value = false;
			if (xp_path.rfind(UCS2(L"/value()"),xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
//				want_value = true;
			}
			retval = xp_result(xp_path,result,error_str);
			if (retval && result != NULL) { //otherwise return empty.
				XMLSize_t sslena = result->getSnapshotLength();
				if (sslena > 0) {
					std::list< pair<u_str,XMLObject*> > lex_results_for_sorting;
					std::list< pair<double,XMLObject*> > num_results_for_sorting;
					bool using_lex = true, tested_lex=false; 
					//if sslena < 1 then life is easy for a sort... but here let's assume not first.
					for ( XMLSize_t ai = 0; ai < sslena; ai++) {
						if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
							XMLObject* item = NULL;
							if ( result->isNode() ) {
								item = new XMLObject(result->getNodeValue());
								DataItem* sortitem = NULL;
								item->xp(sortpath,sortitem,true,error_str);
								u_str sstr; double dnum = NAN;
								if (sortitem != NULL) {
									sstr = *sortitem; delete sortitem;
								} else {
									if (!error_str.empty()) {
										error_str.append(*item);
									}
								}
								if (!tested_lex) { //work out based on the first value if this is a string or a number.
									if (sstr.empty()) { //empty is nan. ie, there's no value set for num results yet.
										num_results_for_sorting.push_back( pair<double,XMLObject*>(dnum,item) );
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
									lex_results_for_sorting.push_back( pair<u_str,XMLObject*>(sstr,item) );
								} else {
									num_results_for_sorting.push_back( pair<double,XMLObject*>(dnum,item) );
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
								if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
									DOMNode* pt = result->getNodeValue();
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						} else {
							std::list< pair<u_str,XMLObject*> >::reverse_iterator i = lex_results_for_sorting.rbegin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
									DOMNode* pt = result->getNodeValue();
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						}
						lex_results_for_sorting.clear();

					} else {
						num_results_for_sorting.sort();
						if (ascending) {						
							std::list< pair<double,XMLObject*> >::iterator i = num_results_for_sorting.begin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
									DOMNode* pt = result->getNodeValue();
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						} else {
							std::list< pair<double,XMLObject*> >::reverse_iterator i = num_results_for_sorting.rbegin();
							for ( XMLSize_t ai = 0; ai < sslena; ai++) {
								if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
									DOMNode* pt = result->getNodeValue();
									DOMNode* vv = i->second->x_doc; i++;
									XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
								}
							}
						}
						num_results_for_sorting.clear();
					}
					delete result; result = NULL;
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
XMLObject::~XMLObject() {
	del_pnsr(); // Delete the pnsr and the xpe cache.
	if (x_doc != NULL) {
		x_doc->release();
		x_doc = NULL;
	}	
}
#pragma mark static utility
//---------------------------------------------------------------------------
void XMLObject::trim(u_str& str) {
	XMLCh const* delims = UCS2(L" \t\r\n");
	u_str::size_type notwhite = str.find_first_not_of(delims);
	str.erase(0,notwhite);
	notwhite = str.find_last_not_of(delims);
	str.erase(notwhite+1);	
}

void XMLObject::rtrim(u_str& str) {
	XMLCh const* delims = UCS2(L" \t\r\n");
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
				retval = XMLDouble(s.c_str()).getValue();
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







