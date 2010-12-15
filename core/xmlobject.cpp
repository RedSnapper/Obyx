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

#include <algorithm>
#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "dataitem.h"
#include "xmlobject.h"
#include "strobject.h"
#include "fragmentobject.h"

XMLObject::u_str_map_type XMLObject::object_ns_map;
/* ==================== NON virtual methods. =========== */
/* protected */
XMLObject::XMLObject(const XMLObject* s) : DataItem(),x(0),x_doc(NULL) { 
	x_doc = (xercesc::DOMDocument*)(s); 
}
XMLObject::XMLObject(const xercesc::DOMDocument* s) : DataItem(),x(1),x_doc(NULL) {
	if  ( s != NULL ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}	
}
XMLObject::XMLObject(xercesc::DOMDocument*& s) : DataItem(),x(2),x_doc(s) {}
XMLObject::XMLObject(const xercesc::DOMNode* s) : DataItem(),x(3),x_doc(NULL) {
	if  ( s != NULL ) {
		x_doc = XML::Manager::parser()->newDoc(s);
	}	
}
/* Public Errors need to be caught higher up! */
XMLObject::XMLObject(const std::string s) : DataItem(),x(4),x_doc(NULL) { 
	x_doc = XML::Manager::parser()->loadDoc(s);
	if (x_doc != NULL) {
		x_doc->normalize();
	}
}
XMLObject::XMLObject(u_str s) : DataItem(),x(7),x_doc(NULL) { 
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
inline bool XMLObject::a_compare(pair<string,XMLObject*> a,pair<string,XMLObject*> b) {
	if ((a.first[0] >= '0' && a.first[0] <= '9') || a.first[0] == '+' || a.first[0] == '-' ) {
		return (String::real(a.first) <  String::real(b.first));
	} else {
		return ( a.first.compare(b.first) < 0);
	}
}
inline bool XMLObject::d_compare(pair<string,XMLObject*> a,pair<string,XMLObject*> b) {
	return a_compare(b,a);
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
	if (x_doc != NULL) {
		x_doc->release();
		x_doc = NULL;
		XML::Manager::resetDocPool();
	}
}
void XMLObject::trim() {
	//do nothing.
}
bool XMLObject::find(const DataItem* o,std::string& error_msg) const {
	//find with an xpath.
	bool retval = false;
	string xpath;
	if (o != NULL)  xpath = *o;
	DOMXPathResult* pt = NULL;
	retval = xp_result(xpath,pt,error_msg);
	if (retval && pt != NULL && pt->getSnapshotLength()  > 0 ) {
		pt->release();
		retval = true;
	}
	return retval;
}
bool XMLObject::find(const char* o,std::string& error_msg) const {
	bool retval = false;
	string xpath;
	if (o != NULL)  xpath = *o;
	DOMXPathResult* pt = NULL;
	retval = xp_result(xpath,pt,error_msg);
	if (retval && pt != NULL && pt->getSnapshotLength()  > 0 ) {
		pt->release();
		retval = true;
	}
	return retval;
}
/* -- more non-virtual methods -- */
/* private */
bool XMLObject::xp_result(const string& path,DOMXPathResult*& result,std::string& err_message) const {
	bool retval = true;
	AutoRelease<DOMXPathNSResolver> pnsr(x_doc->createNSResolver(NULL));	
	if (pnsr != NULL) {
		for (XMLObject::u_str_map_type::const_iterator s = object_ns_map.begin(); s != object_ns_map.end(); s++) {
			const u_str& nssig = s->first; const u_str& nsurl = s->second;
			pnsr->addNamespaceBinding(nssig.c_str(),nsurl.c_str());
		}
		u_str xpath;
		XML::transcode(path,xpath);		
		try {
			AutoRelease<DOMXPathExpression>parsedExpression(x_doc->createExpression(xpath.c_str(), pnsr));
			result = parsedExpression->evaluate(x_doc->getDocumentElement(),DOMXPathResult::SNAPSHOT_RESULT_TYPE, NULL);
		} 
		catch (XQException &e) {
			XML::transcode(e.getError(),err_message);
			retval = false;
		}
		catch (XQillaException &e) {
			XML::transcode(e.getString(),err_message);
			retval = false;
		}					
		catch (DOMXPathException &e) { 
			XML::transcode(e.getMessage(),err_message);
			retval = false;
		}
		catch (DOMException &e) {
			XML::transcode(e.getMessage(),err_message);
			retval = false;
		}
		catch (...) {
			err_message = "unknown xpath error.";
			retval = false;
		}
	} 
	return retval;
}
//with get the value of this at xpath set by input 
//although the results of an xpath may be multiple, here we must glob them together.
//Container must be empty when we get here.
bool XMLObject::xp(const std::string& path,DataItem*& container,bool node_expected,std::string& error_str) const {
	bool retval = true;
	if (!path.empty() ) { //currently, this is a redundant check.
		if (path.rfind("-gap()",path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			std::string xp_path(path);
			DOMXPathResult* result = NULL;
			bool want_value = false;
			if (xp_path.rfind("/value()",xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
				want_value = true;
			}
			retval = xp_result(xp_path,result,error_str);
			if (retval && result != NULL) { //otherwise return empty.			
				XMLSize_t sslena = result->getSnapshotLength();
				for ( XMLSize_t ai = 0; ai < sslena; ai++) {
					if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
						DataItem* item = NULL;
						if ( result->isNode() ) {
							DOMNode* xn = result->getNodeValue();
							DOMNode::NodeType xnt = xn->getNodeType();
							string itemx;
							switch (xnt) {
								case DOMNode::PROCESSING_INSTRUCTION_NODE: 
								case DOMNode::COMMENT_NODE: {
									if (want_value) {
										string xs;
										XML::transcode(xn->getNodeValue(),xs);
										item = DataItem::factory(xs,di_text);
									} else {
										item = DataItem::factory(xn);
									}
								} break;
								case DOMNode::TEXT_NODE:
								case DOMNode::CDATA_SECTION_NODE:
								case DOMNode::ATTRIBUTE_NODE: {
									string xs;
									XML::transcode(xn->getNodeValue(),xs);
									item = DataItem::factory(xs,di_text);
								} break;
								default: {
									item = DataItem::factory(xn);
								} break;
							}
						} else {
							string xs;
							XML::transcode(result->getStringValue(),xs);
							item = DataItem::factory(xs);
						}
						DataItem::append(container,item); //either/both could be NULL. and we may need to convert from one type to another.
					}
				}
				delete result; result = NULL;
				if (sslena == 0 && node_expected) {
					error_str = "While attempting a get, the xpath " + path + " returned no nodes.";												
					retval=false;
				}
			} else {
				if (node_expected) {
					error_str = "While attempting a get, the xpath " + path + " returned an empty result.";												
					retval=false;
				}
			}
		} else {
			if (node_expected) {
				error_str = "While attempting a get, the xpath " + path + " included an insertion point.";												
				retval=false;
			}
		}
	} else {
		copy(container);
	}
	return retval;
}
bool XMLObject::xp(const DataItem* ins,const std::string& path,DOMLSParser::ActionType action,bool node_expected,std::string& error_str) {
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
									std::string insstr = *ins;								
									XML::transcode(insstr,insval);
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
								error_str = "When attempting a set, the xpath " + path + " result was not a node.";
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
				string apath=path.substr(0,slpoint);
				string aname=path.substr(apoint+1);
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
										u_str xaname,xvalue;
										XML::transcode(aname,xaname);		
										if ( ins != NULL && ! ins->empty() ) {
											string value = *ins;
											XML::transcode(value,xvalue);		
											DOMAttr* dnoda = x_doc->createAttribute(xaname.c_str());
											dnoda->setNodeValue(xvalue.c_str());
											enod->setAttributeNode(dnoda);
										} else { //in obyx, setting an attribute to nothing deletes it.								
											enod->removeAttribute(xaname.c_str());
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
								error_str = "When attempting an attribute set, the xpath " + apath + " did not return a node position.";
							}
							retval=false;
						}
					}
					xpra->release();
					xpra = NULL;
				} else {
					if (node_expected) {
						if (error_str.empty()) {
							error_str = "When attempting an attribute set, the xpath " + apath + " did not return a node position.";
						}
						retval=false;
					}
				}
			} else {
				string::size_type pathlen = path.size();
				string::size_type com_pos = path.rfind("/comment()",pathlen-10);
				if (com_pos != string::npos) {
					string apath=path.substr(0,com_pos);
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
											string value = *ins;
											u_str xvalue;
											XML::transcode(value,xvalue);		
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
								error_str = "When attempting to set a comment, the xpath " + apath + " did not return a node position.";
							}
							retval=false;
						}
					}
				} else {
					if (node_expected) {
						if (error_str.empty()) {
							error_str = "When attempting a set, the xpath " + path + " did not return a node position.";
						}
						retval=false;
					}
				}
			}			
		}
		xpr->release();
		xpr = NULL;
	} else {
		if (node_expected) {
			if (error_str.empty()) {
				error_str = "When attempting a set, the xpath " + path + " failed.";
			}
			retval=false;
		}
	}
	return retval;
}
bool XMLObject::sort(const std::string& path,const std::string& sortpath,bool ascending,bool node_expected,std::string& error_str) {
	bool retval = true;
	if (!path.empty() ) { //currently, this is a redundant check.
		if (path.rfind("-gap()",path.length()-6) == string::npos) { //  eg //BOOK[0]/child-gap() will always return empty.
			std::string xp_path(path);
			DOMXPathResult* result = NULL;
			bool want_value = false;
			if (xp_path.rfind("/value()",xp_path.length()-8) != string::npos) { //eg comment()/value()
				xp_path.resize(xp_path.length()-8);
				want_value = true;
			}
			retval = xp_result(xp_path,result,error_str);
			if (retval && result != NULL) { //otherwise return empty.	
				vector< pair<string,XMLObject*> > results_for_sorting;
				XMLSize_t sslena = result->getSnapshotLength();
				//if sslena < 1 then life is easy for a sort... but here let's assume not first.
				for ( XMLSize_t ai = 0; ai < sslena; ai++) {
					if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
						XMLObject* item = NULL;
						if ( result->isNode() ) {
							item = new XMLObject(result->getNodeValue());
							DataItem* sortitem = NULL;
							item->xp(sortpath,sortitem,true,error_str);
							string sstr("");
							if (sortitem != NULL) {
								sstr = *sortitem; delete sortitem;
							} else {
								if (!error_str.empty()) {
									error_str.append(*item);
								}
							}
							results_for_sorting.push_back( pair<string,XMLObject*>(sstr,item) );
						} else {
							error_str = "While expecting node results for a search, the xpath returned text.";												
							retval=false;
						}
					}
				}
				if (ascending) {
					std::stable_sort(results_for_sorting.begin(),results_for_sorting.end(), a_compare );
				} else {
					std::stable_sort(results_for_sorting.begin(),results_for_sorting.end(), d_compare );
				}
				for ( XMLSize_t ai = 0; ai < sslena; ai++) {
					if (result->snapshotItem(ai)) { //should always return true - but best to do this, eh?
						DOMNode* pt = result->getNodeValue();
						DOMNode* vv = results_for_sorting[ai].second->x_doc;
						XML::Manager::parser()->insertContext(x_doc,pt,vv,DOMLSParser::ACTION_REPLACE);
						delete results_for_sorting[ai].second;
					}
				}
				delete result; result = NULL;
				if (sslena == 0 && node_expected) {
					error_str = "While attempting a get, the xpath " + path + " returned no nodes.";												
					retval=false;
				}
			} else {
				if (node_expected) {
					error_str = "While attempting a get, the xpath " + path + " returned an empty result.";												
					retval=false;
				}
			}
		} else {
			if (node_expected) {
				error_str = "While attempting a sort, the xpath " + path + " included an insertion point.";												
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
		}
	}
	return retval;
}
XMLObject::~XMLObject() {
	if (x_doc != NULL) {
		x_doc->release();
		x_doc = NULL;
	}	
}
