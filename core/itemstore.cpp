/* 
 * itemstore.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * itemstore.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "commons/logger/logger.h"
#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include "commons/xml/xml.h"

#include "itemstore.h"
#include "document.h"
#include "xmlobject.h"
#include "iteration.h"

using namespace obyx;
using namespace std;

item_map_type*				ItemStore::the_item_map = NULL;					
item_map_stack_type*		ItemStore::the_item_map_stack = NULL;	//stack of itemmap controlled by isolated attr.	
item_map_stack_map_type		ItemStore::the_item_map_stack_map;	

void ItemStore::startup() {}
void ItemStore::shutdown() {}
void ItemStore::init() {
	//	the_item_map_stack = new item_map_stack_type();
	//	the_item_map = new item_map_type();
	//	the_item_map_stack->push(the_item_map);
}
void ItemStore::finalise() {
	item_map_stack_map_type::iterator it = the_item_map_stack_map.begin();
	while ( it != the_item_map_stack_map.end()) {
		item_map_stack_type* sstack = (*it).second;
		while ( sstack->size() > 0) {
			delete sstack->top();
			sstack->pop();
		}
		delete (*it).second;
		it++;
	}
	the_item_map_stack_map.clear();
}
#pragma mark GRAMMAR FUNCTIONS
bool ItemStore::setgrammar(const DataItem* sig, DataItem*& document) {
	bool retval=false;
	if (sig != NULL) {
		u_str signature = *sig;
		if ( !signature.empty() && document != NULL &&!document->empty() ) {
			switch (document->kind()) {
				case di_object: {
					XML::Manager::parser()->setGrammar(*document,signature,Grammar::SchemaGrammarType);
				} break;
				case di_text: {
					string grmtst = *document;
					//IN order to identify a schema, we look for just one root element, after comments.
					//because comments may be hiding internal markup, we need to strip them first.
					String::Regex::replace("\\s*<!--(?>-(?!->)|[^-]+)*-->\\s*","",grmtst,true); //remove comments
					size_t count = String::Regex::replace("<([:\\w]+)\\b[^>]*>(?>(?:[^<]++|<(?!\\/?\\1\\b[^>]*>))+|(?R))*<\\/\\1>","",grmtst,true);
					if ( count != 1 ) {
						XML::Manager::parser()->setGrammar(*document,signature,Grammar::DTDGrammarType);
					} else {
						XML::Manager::parser()->setGrammar(*document,signature,Grammar::SchemaGrammarType);
					}
				} break;
				default: {
					//report error here?
				} break;
			}
			retval=true;
		}
	}
	return retval;
}
bool ItemStore::getgrammar(const string& signature, DataItem*& document,kind_type kind,bool release) {
	bool exist_result=false;
	if (!signature.empty()) {
		string grammarfile;
		XML::Manager::parser()->getGrammar(grammarfile,signature,release);
		document = DataItem::factory(grammarfile,kind);
		exist_result=true;
	}
	return exist_result;
}
bool ItemStore::grammarexists(const string& signature,bool release) {
	bool retval=false;
	if ( !signature.empty() ) {
		retval = XML::Manager::parser()->existsGrammar(signature,release);
	}
	return retval;
}
#pragma mark NAMESPACE FUNCTIONS
bool ItemStore::setns(const DataItem* c, DataItem*& sig) {
	bool retval=false;
	if (c != NULL && sig != NULL) {
		u_str signature = *sig; 
		u_str code = *c; 
		retval= XMLObject::setns(code,signature);
	}
	return retval;
}
bool ItemStore::getns(const string& c, DataItem*& container,bool release) {
	bool exist_result=false;
	u_str cont,code;
	XML::transcode(c,code);
	exist_result = XMLObject::getns(code,cont,release);
	container = DataItem::factory(cont,di_text);
	return exist_result;
}
bool ItemStore::nsexists(const string& c,bool release) {
	bool exist_result=false;
	u_str cont,code;
	XML::transcode(c,code);
	exist_result = XMLObject::getns(code,cont,release);
	return exist_result;
}
#pragma mark STORE FUNCTIONS
bool ItemStore::exists(const std::string& obj_id,bool release,std::string& errorstr) {
	bool retval=false;
	if (obj_id.find("#") != string::npos) {
		DataItem* xp = NULL;
		retval = get(obj_id, xp, release, errorstr);
		delete xp; 
	} else {
		item_map_type::iterator it = the_item_map->find(obj_id);
		if (it != the_item_map->end()) {
			retval = true;
			if (release) {
				the_item_map->erase(it);
			}
		}
	}
	return retval;
}
bool ItemStore::find(const std::string& pattern,bool release,std::string& errorstr) {
	bool retval=false;
	if (pattern.find("#") != string::npos) {
		return exists(pattern,release,errorstr);
	} else {
		if ( String::Regex::available() ) {
			ostringstream* suppressor = new ostringstream();
			Logger::set_stream(suppressor);
			for(item_map_type::iterator imt = the_item_map->begin(); !retval && imt != the_item_map->end(); imt++) {
				retval= String::Regex::match(pattern,imt->first);
				if (retval && release) { the_item_map->erase(imt); }
			}
			Logger::unset_stream();
			if (!suppressor->str().empty()) {
				retval = false;
				errorstr = suppressor->str();
			}
			delete suppressor;
		} else {
			item_map_type::iterator it = the_item_map->find(pattern);
			retval = (it != the_item_map->end());
			if (retval && release) { the_item_map->erase(it); }
		}
	}
	return retval;
}
void ItemStore::storekeys(const std::string& pattern,vector<string>& keylist,std::string& errorstr) {
	if (pattern.find("#") != string::npos) {
		errorstr="to iterate over multiple xpaths, use xpath syntax count()";
	} else {
		if ( String::Regex::available() ) {
			for(item_map_type::iterator imt = the_item_map->begin(); imt != the_item_map->end(); imt++) {
				if (String::Regex::match(pattern,imt->first)) {
					keylist.push_back(imt->first);
				}
			}
		} else {
			item_map_type::iterator it = the_item_map->find(pattern);
			if (it != the_item_map->end()) {
				keylist.push_back(pattern);
			}
		}
	}
}

void ItemStore::release(const DataItem* obj_id) {
	string obj_name; if (obj_id != NULL) { obj_name = *obj_id; }
	item_map_type::iterator it = the_item_map->find(obj_name);
	if (it != the_item_map->end()) {
		it->second = NULL;
		the_item_map->erase(it);
	} else {
		*Logger::log << Log::error << Log::LI << "Error. The object '" << obj_name << "' could not be released." << Log::LO << Log::blockend;
	}
}
void ItemStore::list() {
	if ( ! the_item_map->empty() ) {
		item_map_type::iterator it = the_item_map->begin();
		*Logger::log << Log::subhead << Log::LI << "Current store items" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		while (it != the_item_map->end() ) {
			if ( ! it->first.empty() ) {
				*Logger::log << Log::LI << Log::II << it->first << Log::IO;
				if ( it->second == NULL) {
					*Logger::log << Log::II << "--empty--" << Log::IO;
				} else {
					DataItem* x= it->second;
					string result_doc = *x;
					*Logger::log << Log::II << result_doc << Log::IO;
				}
				*Logger::log << Log::LO;				
			}
			it++;
		}
		*Logger::log << Log::blockend << Log::LO ; 	//even
		*Logger::log << Log::blockend; //subhead
	}
}
bool ItemStore::set(const DataItem* namepath_di, DataItem*& item,kind_type kind,std::string& errorstr) {
	//we need to test that the kind being asked for is the same kind as item.
	//if not, we must attempt to cast it.  If we fail, we post an error, but KEEP the item set
	//as it is.
	bool retval = false;
	if (namepath_di != NULL)  { 
		bool node_expected = false;
		pair<string,string> np;
		string namepath,name,path; 
		namepath = *namepath_di; 
		String::split('#',namepath,np);		 // eg foobar#/BOOK[0]      -- foobar       /BOOK[0]
		name=np.first;
		path=np.second;
		if (!path.empty() && path[0] == '#') { path.erase(0,1); }
		if (*name.rbegin() == '!') { node_expected = true; name.resize(name.size()-1); }  //remove the final character
		if (!retval) {	
			if ( String::nametest(name)) {
				if ( (kind != di_auto) && (item != NULL) && (kind != item->kind())) {
					//need to upcast/downcast
					DataItem* nitem = DataItem::factory(item,kind);
					delete item; item=nitem;
				}
				if (path.empty()) {
					item_map_type::iterator it = the_item_map->find(name);
					if (it != the_item_map->end()) {
						DataItem*& basis = it->second;
						delete basis;
						the_item_map->erase(it); //and if there is any item then insert it.
					}
					pair<item_map_type::iterator, bool> ins = the_item_map->insert(item_map_type::value_type(name, item));
					item = NULL; //this was already a document!
					retval = true;
				} else {
					item_map_type::iterator it = the_item_map->find(name);
					if (it != the_item_map->end()) {
						DataItem* basis = it->second; //we will use the path on the basis.
						if (basis != NULL && basis->kind() != di_text) {
							XMLObject* xbase = (XMLObject *)basis;
							if (! xbase->empty() ) {
								insertion_type i_type=DOMLSParser::ACTION_REPLACE;				
								size_t pathlen = path.size();
								if (path.rfind("-gap()",pathlen-6) != string::npos) {
									string::size_type pspoint = path.rfind("/preceding",pathlen-16);
									if ( pspoint != string::npos) { //we want to insert before
										i_type = DOMLSParser::ACTION_INSERT_BEFORE; path.resize(pspoint);
									} else {
										string::size_type fspoint = path.rfind("/following",pathlen-16);
										if ( fspoint != string::npos) { //we want to insert after
											i_type = DOMLSParser::ACTION_INSERT_AFTER; path.resize(fspoint);
										} else {
											string::size_type dpoint = path.rfind("/child",pathlen-12);
											if ( dpoint != string::npos) { //we want to insert below!
												i_type = DOMLSParser::ACTION_APPEND_AS_CHILDREN; path.resize(dpoint);
											}
										}
									}
								} else {
									string::size_type dpoint = path.rfind("/value()",pathlen-8);
									if ( dpoint != string::npos) { //we want to insert below!
										i_type = DOMLSParser::ACTION_REPLACE_CHILDREN; path.resize(dpoint);
									}
								}
								try {
									xbase->xp(item,path,i_type,node_expected,errorstr);
								} 
								//------------------------------								
								catch (XQException &e) {
									XML::transcode(e.getError(),errorstr);
								}
								catch (XQillaException &e) {
									XML::transcode(e.getString(),errorstr);
								}					
								catch (DOMXPathException &e) { 
									XML::transcode(e.getMessage(),errorstr);
								}
								catch (DOMException &e) {
									XML::transcode(e.getMessage(),errorstr);
								}
								catch (...) {
									errorstr = "unknown xpath error";
								}
								//-----------------------------								
							} else {
								the_item_map->erase(it);
								errorstr = "There was no store " + name + " for the path " + path;
								retval= true; //bad name/path				
							}
						} else {
							if (basis == NULL) {
								errorstr = "The item is empty. Maybe a parse or previous xpath failed?";
							} else {
								string bvalue = *basis;
								errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + path + "' cannot be used.";
								if (bvalue.empty()) {
									errorstr = "The item is empty. Maybe a parse or previous xpath failed?";
								} else {
									errorstr = "It's value is '"+ bvalue + "'";
								}
							}
							retval= true; // cannot honour xpath		
						}
					} else { //paths need an object.
						errorstr = "There was no object " + name + " for the path " + path;
						retval= true; //bad name/path				
					}
				}
			} else {
				errorstr = "Store names must be legal. Maybe you missed a # in '" + namepath + "'";
				retval= true; //bad name		
			}
		}
	} else {
		errorstr = "Store space is named. The name is missing.";
		retval= true; //no name				
	}
	return retval;
}
bool ItemStore::get(const string& namepath_di, DataItem*& item, bool release,std::string& errorstr) {
	// bool here represents existence.
	bool retval = false;
	if (!namepath_di.empty())  { 
		bool node_expected = false;
		pair<string,string> np;
		string namepath,name,path; 
		namepath = namepath_di;
		String::split('#',namepath,np);		 // eg foobar#/BOOK[0]      -- foobar       /BOOK[0]
		name=np.first; path=np.second;
		if (*name.rbegin() == '!') { node_expected = true; name.resize(name.size()-1); }  //remove the final character
		if ( String::nametest(name)) {
			retval = true; 
			item_map_type::iterator it = the_item_map->find(name);
			if (it != the_item_map->end()) {
				DataItem*& basis = it->second;
				if (release) { 
					if (! path.empty() && basis != NULL ) {
						if (path.rfind("-gap()",path.length()-6) != string::npos) { //eg h:div/child-gap()
							item = NULL;
						} else {
							kind_type basis_kind = basis->kind();
							if (basis_kind == di_object) {
								XMLObject* xml_document = (XMLObject*)basis;
								if (xml_document != NULL) {
									retval = xml_document->xp(path,item,node_expected,errorstr); //will make a copy.
									if (retval) {
										DataItem* mt = NULL;
										xml_document->xp(mt,path,DOMLSParser::ACTION_REPLACE,node_expected,errorstr);
									}
								} else {
									if (node_expected) {
										errorstr = "Released item '" + name + "' was empty, so the xpath '" + path + "' was unused";
									}
								}
							} else {
								if (node_expected) {
									errorstr = "Released item '" + name + "' was of kind 'text', so the xpath '" + path + "' was unused.";
								}
							}
						}
					} else {
						item = basis; 
						basis = NULL;
						the_item_map->erase(it);
					}
					
				} else { //not release!
					if (! path.empty() && basis != NULL ) {
						if (path.rfind("-gap()",path.length()-6) != string::npos) { //eg h:div/child-gap()
							item = NULL;
						} else {
							kind_type basis_kind = basis->kind();
							switch(basis_kind) {
								case di_object: {
									XMLObject* xml_document = (XMLObject*)basis;
									if (!xml_document->empty()) {
										retval = xml_document->xp(path,item,node_expected,errorstr); //will make a copy.
									} else {
										if (node_expected) {
											errorstr = "Item " + name + " exists but is empty.";
										}
									}
								} break;
								case di_text: {
									if (path.compare("text()") == 0) {
										item = basis; 
									} else {
										if (node_expected) {
											errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + path + "' was unused.";
											string bvalue = *basis;	
											if (bvalue.empty()) {
												errorstr = "The item is empty. Maybe a previous xpath failed?";
											} else {
												errorstr =  "It's value is '" + bvalue + "'";
											}
										}
									}
								} break;
								case di_fragment: {
									if (path.compare("text()") == 0) {
										item = basis; 
									} else {
										if (node_expected) {
											errorstr = "Item '" + name + "' is of kind 'fragment'. so xpath '" + path + "' was unused.";
											string bvalue = *basis;	
											if (bvalue.empty()) {
												errorstr = " The item is empty. Maybe a previous xpath failed?";
											} else {
												errorstr +=  "It's value is '" + bvalue + "'";
											}
										}
									}
								} break;
								case di_null: 
								case di_auto: {
									if (node_expected) {
										errorstr = "Item " + name + " is null.";
									}
								} break;
							}
						}
					} else {
						if (basis != NULL) {
							basis->copy(item);
						}
					}
				}
			} else {
				retval = false; 
				//				retval returning false indicates lack of existence. Store handler above should deal with the msgs.
				//				errorstr << Log::error << Log::LI <<  "Store '" << name << "' does not exist." << Log::LO << Log::blockend;
			}
		} else {
			errorstr = "Store names must be legal. Maybe you missed a # in '" + namepath + "'" ;
			retval= true; //bad name		
		}
	} else {
		errorstr = "stores must have a name.";
		retval= true; //no name				
	}
	return retval;
}
bool ItemStore::get(const string& name, string& container) {	//name container (quick hack)
	//This is always used to get settings values, such as REDIRECT_BREAK_COUNT
	bool retval = false;
	item_map_type::iterator it = the_item_map->find(name);
	if (it != the_item_map->end()) {
		retval = true;
		container = *(it->second);
	} 
	return retval;
}
void ItemStore::prefixpushed(const u_str& prefix) {
	item_map_stack_map_type::iterator it = the_item_map_stack_map.find(prefix);
	if (it != the_item_map_stack_map.end()) {
		the_item_map_stack = ((*it).second);
		the_item_map = the_item_map_stack->top();
	} else {
		the_item_map = new item_map_type();
		the_item_map_stack = new item_map_stack_type();
		the_item_map_stack->push(the_item_map);
		the_item_map_stack_map.insert(item_map_stack_map_type::value_type(prefix,the_item_map_stack));
	}
}
void ItemStore::prefixpopped(const u_str& prefix) {
	item_map_stack_map_type::iterator it = the_item_map_stack_map.find(prefix);
	if (it != the_item_map_stack_map.end()) {
		the_item_map_stack = ((*it).second);
		the_item_map = the_item_map_stack->top();
	} 		
}
