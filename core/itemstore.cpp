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
#include <set>

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

ItemStore::ItemStore() : owner(),the_item_map() {
}
void ItemStore::setowner(const std::string p) {
	owner=p;
}
ItemStore::ItemStore(const ItemStore* orig) : the_item_map() {
	if (orig != NULL && ! orig->the_item_map.empty() ) {
		item_map_type::const_iterator it = orig->the_item_map.begin();
		while ( it != orig->the_item_map.end()) {
			the_item_map.insert(*it);
			it++;
		}
	} 
}
ItemStore::~ItemStore() {
	item_map_type::iterator it = the_item_map.begin();
	while ( it != the_item_map.end()) {
#ifdef PROFILING
		string x= it->first;
		DataItem*& y=it->second;
		delete y; y=NULL;
#else
		delete (*it).second; 
		(*it).second = NULL;
#endif
		it++;
	}
	the_item_map.clear();
}

void ItemStore::startup() {}
void ItemStore::shutdown() {}
void ItemStore::init() {}
void ItemStore::finalise() {}

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
	XML::Manager::transcode(c,code);
	exist_result = XMLObject::getns(code,cont,release);
	container = DataItem::factory(cont,di_text);
	return exist_result;
}
bool ItemStore::nsexists(const string& c,bool release) {
	bool exist_result=false;
	u_str cont,code;
	XML::Manager::transcode(c,code);
	exist_result = XMLObject::getns(code,cont,release);
	return exist_result;
}
#pragma mark STORE FUNCTIONS
bool ItemStore::exists(const u_str& namepath,bool release,std::string& errorstr) {
	bool retval=false;
	if (namepath.find('#') != string::npos) {
		DataItem* xp = NULL;
		bool node_expected = false;
		pair<u_str,u_str> np;
		XMLObject::npsplit(namepath,np,node_expected);	// eg foobar#/BOOK[0]      -- foobar       /BOOK[0]
		retval = sget(np.first, np.second, node_expected, xp, release, errorstr);
		delete xp; 
	} else {
		std::string skey;
		XML::Manager::transcode(namepath,skey);	//Regex works on strings!	
		item_map_type::iterator it = the_item_map.find(skey);
		if (it != the_item_map.end()) {
			retval = true;
			if (release) {
				delete it->second;
				the_item_map.erase(it);
			}
		}
	}
	return retval;
}
bool ItemStore::exists(const u_str& name,const u_str& path,bool release,string& errorstr) { // name, path.
	bool retval=false;
	DataItem* xp = NULL;
	retval = sget(name, path, false, xp, release, errorstr);
	delete xp; 
	return retval;
}

bool ItemStore::find(const string& pattern,bool release,std::string& errorstr) {
	bool retval=false;
	if (pattern.find('#') != string::npos) { //this is a namepath
		u_str uxpr; XML::Manager::transcode(pattern,uxpr);	
		return exists(uxpr,release,errorstr);
	} else {
		if ( String::Regex::available() ) {
			ostringstream* suppressor = new ostringstream();
			Logger::set_stream(suppressor);
			for(item_map_type::iterator imt = the_item_map.begin(); !retval && imt != the_item_map.end(); imt++) {
				retval= String::Regex::match(pattern,imt->first);
				if (retval && release) { 
					delete imt->second;
					the_item_map.erase(imt); 
				}
			}
			Logger::unset_stream();
			if (!suppressor->str().empty()) {
				retval = false;
				errorstr = suppressor->str();
			}
			delete suppressor;
		} else {
			item_map_type::iterator it = the_item_map.find(pattern);
			retval = (it != the_item_map.end());
			if (retval && release) { 
				delete it->second;
				the_item_map.erase(it); 
			}
		}
	}
	return retval;
}

bool ItemStore::find(const u_str& pattern,const u_str& xpath,bool release,std::string& errorstr) {
	bool retval=false;
	if (release && !xpath.empty() && !pattern.empty()) {
		retval=false;
	}
	errorstr ="Sorry, separated xpath is not yet supported for find.";
	return retval;
}

bool ItemStore::find(const u_str& pattern,bool release,std::string& errorstr) {
	bool retval=false;
	if (pattern.find('#') != string::npos) { //this is a namepath
		return exists(pattern,release,errorstr);
	} else {
		string rxpr; XML::Manager::transcode(pattern,rxpr);	//Regex works on strings!	
		if ( String::Regex::available() ) {
			ostringstream* suppressor = new ostringstream();
			Logger::set_stream(suppressor);
			for(item_map_type::iterator imt = the_item_map.begin(); !retval && imt != the_item_map.end(); imt++) {
				retval= String::Regex::match(rxpr,imt->first);
				if (retval && release) { 
					delete imt->second;
					the_item_map.erase(imt);
				}
			}
			Logger::unset_stream();
			if (!suppressor->str().empty()) {
				retval = false;
				errorstr = suppressor->str();
			}
			delete suppressor;
		} else {
			item_map_type::iterator it = the_item_map.find(rxpr);
			retval = (it != the_item_map.end());
			if (retval && release) { 
				delete it->second;
				the_item_map.erase(it); 
			}
		}
	}
	return retval;
}

void ItemStore::keys(const u_str& pattern,std::set<string>& keylist,std::string& errorstr) {
	if (pattern.find('#') != string::npos) {
		errorstr="to iterate over multiple xpaths, use xpath function 'count()'";
	} else {
		std::string rpattern; XML::Manager::transcode(pattern,rpattern);	//Regex works on strings!	
		if ( String::Regex::available() ) {
			for(item_map_type::iterator imt = the_item_map.begin(); imt != the_item_map.end(); imt++) {
				if (String::Regex::match(rpattern,imt->first)) {
					keylist.insert(imt->first);
				}
			}
		} else {
			item_map_type::iterator it = the_item_map.find(rpattern);
			if (it != the_item_map.end()) {
				keylist.insert(rpattern);
			}
		}
	}
}
bool ItemStore::release(const u_str& obj_name) {
	bool retval = false;
	std::string name; XML::Manager::transcode(obj_name,name); //Our keys are actually std::strings	
	item_map_type::iterator it = the_item_map.find(name);
	if (it != the_item_map.end()) {
		delete it->second;
		the_item_map.erase(it);
		retval = true;
	} 
	return retval;
}
void ItemStore::list() const {
	if ( ! the_item_map.empty() ) {
		item_map_type::const_iterator it = the_item_map.begin();
//		*Logger::log << Log::subhead << Log::LI << "Stores (" << Log::II << owner << Log::IO << ")" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		while (it != the_item_map.end() ) {
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
//		*Logger::log << Log::blockend; //subhead
	}
	XMLObject::listns();
}
bool ItemStore::sset(const u_str& sname,const u_str& tpath,bool node_expected, DataItem*& item,kind_type kind,std::string& errorstr) {
	//we need to test that the kind being asked for is the same kind as item.
	//if not, we must attempt to cast it.  If we fail, we post an error, but KEEP the item set
	//as it is.
	bool retval = false;
	std::string name; XML::Manager::transcode(sname,name); //Our keys are actually std::strings	
	u_str path(tpath); //we may want to manipulate the path.
	if ( String::nametest(name)) {
		if ( (kind != di_auto) && (item != NULL) && (kind != item->kind())) {
			//need to upcast/downcast
			DataItem* nitem = DataItem::factory(item,kind);
			delete item; item=nitem;
		}
		if (path.empty()) {
			item_map_type::iterator it = the_item_map.find(name);
			if (it != the_item_map.end()) {
				DataItem*& basis = it->second;
				delete basis;
				the_item_map.erase(it); //and if there is any item then insert it.
			}
			the_item_map.insert(item_map_type::value_type(name, item));
			item = NULL; //this was already a document!
			retval = true;
		} else {
			item_map_type::iterator it = the_item_map.find(name);
			if (it != the_item_map.end()) {
				DataItem* basis = it->second; //we will use the path on the basis.
				if (basis != NULL && basis->kind() != di_text) {
					XMLObject* xbase = (XMLObject *)basis;
					if (! xbase->empty() ) {
						insertion_type i_type=DOMLSParser::ACTION_REPLACE;				
						size_t pathlen = path.size();
						if (path.rfind(UCS2(L"-gap()"),pathlen-6) != string::npos) {
							string::size_type pspoint = path.rfind(UCS2(L"/preceding"),pathlen-16);
							if ( pspoint != string::npos) { //we want to insert before
								i_type = DOMLSParser::ACTION_INSERT_BEFORE; path.resize(pspoint);
							} else {
								string::size_type fspoint = path.rfind(UCS2(L"/following"),pathlen-16);
								if ( fspoint != string::npos) { //we want to insert after
									i_type = DOMLSParser::ACTION_INSERT_AFTER; path.resize(fspoint);
								} else {
									string::size_type dpoint = path.rfind(UCS2(L"/child"),pathlen-12);
									if ( dpoint != string::npos) { //we want to insert below!
										i_type = DOMLSParser::ACTION_APPEND_AS_CHILDREN; path.resize(dpoint);
									}
								}
							}
						} else {
							string::size_type dpoint = path.rfind(UCS2(L"/value()"),pathlen-8);
							if ( dpoint != string::npos) { //we want to insert below!
								i_type = DOMLSParser::ACTION_REPLACE_CHILDREN; path.resize(dpoint);
							}
						}
						try {
							xbase->xp(item,path,i_type,node_expected,errorstr);
							if (item!=NULL) {
								delete item; item= NULL; //item here is a const.
							}
						}
						//------------------------------								
						catch (XQException &e) {
							XML::Manager::transcode(e.getError(),errorstr);
						}
						catch (XQillaException &e) {
							XML::Manager::transcode(e.getString(),errorstr);
						}					
						catch (DOMXPathException &e) { 
							XML::Manager::transcode(e.getMessage(),errorstr);
						}
						catch (DOMException &e) {
							XML::Manager::transcode(e.getMessage(),errorstr);
						}
						catch (...) {
							errorstr = "unknown xpath error";
							retval= true; //bad name/path
						}
						//-----------------------------								
					} else {
						the_item_map.erase(it);
						std::string erv; XML::Manager::transcode(path,erv);	
						errorstr = "There was no store " + name + " for the path " + erv;
						retval= true; //bad name/path				
					}
				} else {
					if (basis == NULL) {
						errorstr = "The item is empty. Maybe a parse or previous xpath failed?";
					} else {
						string bvalue = *basis;
						std::string erv; XML::Manager::transcode(path,erv);	
						errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + erv + "' cannot be used.";
						if (bvalue.empty()) {
							errorstr = "The item is empty. Maybe a parse or previous xpath failed?";
						} else {
							errorstr = "It's value is '"+ bvalue + "'";
						}
					}
					retval= true; // cannot honour xpath		
				}
			} else { //paths need an object.
				std::string erv; XML::Manager::transcode(path,erv);	
				errorstr = "There was no existing store " + name + " for the path " + erv;
				retval= true; //bad name/path				
			}
		}
	} else {
		errorstr = "Store names must be legal. Maybe you missed a # in '" + name + "'";
		retval= true; //bad name		
	}
	return retval;
} 
bool ItemStore::sget(const u_str& sname,const u_str& path,bool node_expected, DataItem*& item, bool release,std::string& errorstr) {
	// bool here represents existence.
	bool retval = false;
	std::string name; XML::Manager::transcode(sname,name); //Our keys are actually std::strings	
	if ( String::nametest(name)) {
		retval = true; 
		item_map_type::iterator it = the_item_map.find(name);
		if (it != the_item_map.end()) {
			DataItem*& basis = it->second;
			if (release) { 
				if (! path.empty() && basis != NULL ) {
					if (path.rfind(UCS2(L"-gap()"),path.length()-6) != string::npos) { //eg h:div/child-gap()
						item = NULL;
					} else {
						kind_type basis_kind = basis->kind();
						if (basis_kind == di_object) {
							XMLObject* xml_document = (XMLObject*)basis;
							if (xml_document != NULL) {
								retval = xml_document->xp(path,item,node_expected,errorstr); //will put a copy into item.
								if (retval) {
									DataItem* mt = NULL;
									xml_document->xp(mt,path,DOMLSParser::ACTION_REPLACE,node_expected,errorstr);
								}
							} else {
								if (node_expected) {
									std::string erv; XML::Manager::transcode(path,erv);	
									errorstr = "Released item '" + name + "' was empty, so the xpath '" + erv + "' was unused";
								}
							}
						} else {
							if (node_expected) {
								std::string erv; XML::Manager::transcode(path,erv);	
								errorstr = "Released item '" + name + "' was of kind 'text', so the xpath '" + erv + "' was unused.";
							}
						}
					}
				} else {
					item = basis; 
					basis = NULL;
					the_item_map.erase(it);
				}
				
			} else { //not release!
				if (! path.empty() && basis != NULL ) {
					if (path.rfind(UCS2(L"-gap()"),path.length()-6) != string::npos) { //eg h:div/child-gap()
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
							case di_utext: {
								if (path.compare(UCS2(L"text()")) == 0) {
									item = basis; 
								} else {
									if (node_expected) {
										std::string erv; XML::Manager::transcode(path,erv);	
										errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + erv + "' was unused.";
										string bvalue = *basis;	
										if (bvalue.empty()) {
											errorstr.append(" The item is empty. Maybe a previous xpath failed?");
										} else {
											errorstr.append(" It's value is '" + bvalue + "'");
										}
									}
								}
							} break;
							case di_raw: {
								if (path.compare(UCS2(L"text()")) == 0) {
									item = basis; 
								} else {
									if (node_expected) {
										std::string erv; XML::Manager::transcode(path,erv);	
										errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + erv + "' was unused.";
										string bvalue = *basis;	
										if (bvalue.empty()) {
											errorstr = "The item is empty. Maybe a previous xpath failed?";
										} else {
											errorstr =  "It's value is raw binary.";
										}
									}
								}
							} break;
							case di_text: {
								if (path.compare(UCS2(L"text()")) == 0) {
									item = basis; 
								} else {
									if (node_expected) {
										std::string erv; XML::Manager::transcode(path,erv);	
										errorstr = "Item '" + name + "' is of kind 'text'. so xpath '" + erv + "' was unused.";
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
								if (path.compare(UCS2(L"text()")) == 0) {
									item = basis; 
								} else {
									if (node_expected) {
										std::string erv; XML::Manager::transcode(path,erv);	
										errorstr = "Item '" + name + "' is of kind 'fragment'. so xpath '" + erv + "' was unused.";
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
		errorstr = "Store names must be legal. Maybe you missed a # in '" + name + "'" ;
		retval= true; //bad name		
	}
	return retval;
}

//This returns true if there is NO value set, but false if the value is NOT a number!
bool ItemStore::meta(const string& name,pair<bool,unsigned long long>& result) { //used for meta settings.
	bool retval = true;
	if (! the_item_map.empty() ) { //this happens when eg. using an OSI as root document
		item_map_type::iterator it = the_item_map.find(name);
		if (it != the_item_map.end()) {
			u_str stored = *(it->second);
			pair<unsigned long long,bool> my_ull = 	XMLObject::znatural(stored);
			retval = my_ull.second;
			if ( retval ) {  //Otherwise don't touch it!!
				result.first = my_ull.second;
				result.second = my_ull.first;
			}
		} 
	}
	return retval;
}
