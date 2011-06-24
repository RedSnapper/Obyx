/* 
 * document.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * document.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <iostream>
#include <sstream>
#include <string>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"

#include "document.h"
#include "obyxelement.h"
#include "iteration.h"
#include "filer.h"
#include "xmlobject.h"
#include "dataitem.h"
#include "itemstore.h"
#include "osiapp.h"

using namespace Log;
using namespace xercesc;
using namespace XML;

XML::Manager* Document::xmlmanager = NULL;

Document*					Document::root = NULL; //This is the opening document.
std::stack<u_str>			Document::prefix_stack;
std::stack<std::string>		Document::filepath_stack;
Document::type_store_map 	Document::docstores; //Set of stores against digests.
size_t						Document::prefix_length=0;		//Document wide specified	

string Document::curr_http_req;

void Document::startup() {
	xmlmanager = new XML::Manager();
}
void Document::init() {
	prefix_length = 0;
}
void Document::finalise() {
	prefix_length = 0;
	curr_http_req.clear();
//	type_store_map::iterator it = docstores.find(signature_str);
	
	while (! prefix_stack.empty()) {
		prefix_stack.pop();
	}
	while (! filepath_stack.empty()) {
		filepath_stack.pop();
	}
	for(type_store_map::iterator i = docstores.begin(); i != docstores.end(); i++) {
		delete i->second;
	}
	xmlmanager->resetDocPool(); //This resets the cache of the entire set of all docs
}
void Document::shutdown() {
	delete xmlmanager;
}
const std::string Document::currentname() {
	Environment* env = Environment::service();
	string rootpath = env->getpathforroot();
	string fp = filepath_stack.top();
	if ( fp.find(rootpath) == 0 ) {
		fp.erase(0,rootpath.length());
	}
	return fp;
}
const std::string Document::name() const { 
	Environment* env = Environment::service();
	string rootpath = env->getpathforroot();
	string fp = filepath;
	if ( fp.find(rootpath) == 0 ) {
		fp.erase(0,rootpath.length());
	}
	return fp;
}
const xercesc::DOMDocument* Document::doc() const {
	return xdoc;
}				
bool Document::setstore(const DataItem* namepath_di, DataItem*& item,kind_type kind,Output::scope_type scope,std::string& errorstr) {
	bool retval = true;
	if (namepath_di != NULL) {
		bool node_expected = false;
		u_str namepath = *namepath_di; pair<u_str,u_str> np;
		XMLObject::npsplit(namepath,np,node_expected);
		if (doc_version < 1.110208 ) {
			retval = root->store.sset(np.first,np.second,node_expected,item,kind,errorstr);
		} else {
			switch (scope) {
				case Output::ancestor: {
					bool found = false;
					Document* doc = this;
					do {
						if (doc->p != NULL) {
							doc = doc->p->owner;
						} else {
							doc = NULL;
						}
						if (doc != NULL) {
							found = doc->store.exists(np.first,false,errorstr);
						}
					} while (doc != NULL && !found);
					if (found && doc!=NULL) {
						retval = doc->store.sset(np.first,np.second,node_expected,item,kind,errorstr);
					} else {
						retval = false;
						errorstr.append("Ancestor was not found for output.");
					}
				} break;
				case Output::branch: {
					if ( np.second.empty()) {
						retval = store.sset(np.first,np.second,node_expected,item,kind,errorstr);
					} else {
						bool found = false;
						Document* doc = this;
						while (doc != NULL && !found) {
							found = doc->store.exists(np.first,false,errorstr);
							if (!found) {
								if (doc->p != NULL) {
									doc = doc->p->owner;
								} else {
									doc = NULL;
								}
							}
						}
						if (found && doc!=NULL) {
							retval = doc->store.sset(np.first,np.second,node_expected,item,kind,errorstr);
						} else {
							string ervn,ervp; 
							XML::Manager::transcode(np.first,ervn); 
							XML::Manager::transcode(np.second,ervp);	
							errorstr = "There was no existing store " + ervn + " for the path " + ervp;
						}
					}
				} break;
				case Output::global: {
					retval = root->store.sset(np.first,np.second,node_expected,item,kind,errorstr);
				} break;
				case Output::document: {
					if (doc_store == NULL) {
						errorstr = "The document signature [" + signature + "] is empty, so output to document scope does nothing.";
					} else {
						retval = doc_store->sset(np.first,np.second,node_expected,item,kind,errorstr);
					}
				} break;
			}
		}
	}
	return retval;
}
bool Document::setstore(u_str& name,u_str& path, DataItem*& item,kind_type kind,Output::scope_type scope,std::string& errorstr) {
	bool retval = true;
    bool node_expected = false;
	if (doc_version < 1.110208) {
		retval = root->store.sset(name,path,node_expected,item,kind,errorstr);
	} else {
		switch (scope) {
			case Output::ancestor: {
				bool found = false;
				Document* doc = this;
				do {
					if (doc->p != NULL) {
						doc = doc->p->owner;
					} else {
						doc = NULL;
					}
					if (doc != NULL) {
						found = doc->store.exists(name,false,errorstr);
					}
				} while (doc != NULL && !found);
				if (found && doc!=NULL) {
					retval = doc->store.sset(name,path,node_expected,item,kind,errorstr);
				} else {
					retval = false;
					errorstr.append("Ancestor was not found for output.");
				}
			} break;
			case Output::branch: {
				if ( path.empty()) {
					retval = store.sset(name,path,node_expected,item,kind,errorstr);
				} else {
					bool found = false;
					Document* doc = this;
					while (doc != NULL && !found) {
						found = doc->store.exists(name,false,errorstr);
						if (!found) {
							if (doc->p != NULL) {
								doc = doc->p->owner;
							} else {
								doc = NULL;
							}
						}
					}
					if (found && doc!=NULL) {
						retval = doc->store.sset(name,path,node_expected,item,kind,errorstr);
					} else {
						string ervn,ervp; 
						XML::Manager::transcode(name,ervn); 
						XML::Manager::transcode(name,ervp);		
						errorstr = "There was no existing store " + ervn + " for the path " + ervp;
					}
				}
			} break;
			case Output::global: {
				retval = root->store.sset(name,path,node_expected,item,kind,errorstr);
			} break;
			case Output::document: {
				if (doc_store == NULL) {
					errorstr = "The document signature [" + signature + "] is empty, so output to document scope does nothing.";
				} else {
					retval = doc_store->sset(name,path,node_expected,item,kind,errorstr);
				}
			} break;
		}
	}
	return retval;
}

bool Document::metastore(const string key,unsigned long long& container) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc->store.meta(key,container);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}

bool Document::storeexists(const u_str& name,const u_str& xpath,bool release,std::string& errorstr) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc->store.exists(name,xpath,release,errorstr);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}

bool Document::storeexists(const u_str& obj_id,bool release,std::string& errorstr) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc->store.exists(obj_id,release,errorstr);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}
bool Document::storefind(const string& pattern,bool release,std::string& errorstr) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc->store.find(pattern,release,errorstr);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}
bool Document::storefind(const u_str& pattern,bool release,std::string& errorstr) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc->store.find(pattern,release,errorstr);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}
bool Document::storefind(const u_str& pattern,const u_str& xpath,bool release,std::string& errorstr) {
	bool retval = false;
	Document* doc = this;
	while (doc != NULL && !retval) {
		retval = doc_store->find(pattern,xpath,release,errorstr);
		if (!retval) {
			if (doc->p != NULL) {
				doc = doc->p->owner;
			} else {
				doc = NULL;
			}
		}
	}
	return retval;
}
void Document::storekeys(const u_str& pattern,std::set<std::string>& keylist,std::string& errorstr) {
	Document* doc = this;
	while (doc != NULL) {
		doc->store.keys(pattern,keylist,errorstr);
		if (doc->p != NULL) {
			doc = doc->p->owner;
		} else {
			doc = NULL;
		}
	}
}
bool Document::getstore(const u_str& namepath, DataItem*& item, bool release,std::string& errorstr) {
	bool retval = false;
	bool node_expected = false;
	pair<u_str,u_str> np;
	XMLObject::npsplit(namepath,np,node_expected);
	if (doc_store != NULL && doc_store->exists(np.first,false,errorstr)) {
		retval = doc_store->sget(np.first,np.second,node_expected,item,release,errorstr);
	} else {
		bool found = false;
		Document* doc = this;
		while (doc != NULL && !found) {
			found = doc->store.exists(np.first,false,errorstr);
			if (!found) {
				if (doc->p != NULL) {
					doc = doc->p->owner;
				} else {
					doc = NULL;
				}
			}
		}
		if (found && doc!=NULL) {
			retval = doc->store.sget(np.first,np.second,node_expected,item,release,errorstr);
		}
	}
	return retval;
}
bool Document::getstore(const u_str& name,const u_str& path,DataItem*& item, bool release,std::string& errorstr) {
	bool retval = false;
	bool node_expected = false;
	if (doc_store != NULL && doc_store->exists(name,false,errorstr)) {
		retval = doc_store->sget(name,path,node_expected,item,release,errorstr);
	} else {
		bool found = false;
		Document* doc = this;
		while (doc != NULL && !found) {
			found = doc->store.exists(name,false,errorstr);
			if (!found) {
				if (doc->p != NULL) {
					doc = doc->p->owner;
				} else {
					doc = NULL;
				}
			}
		}
		if (found && doc!=NULL) {
			retval = doc->store.sget(name,path,node_expected,item,release,errorstr);
		}
	}
	return retval;
}

void Document::liststore() {
	store.list();
	if (doc_store != NULL) {
		doc_store->list();
	}
	if (p != NULL) {
		p->owner->liststore(); //now get the stuff above me.
	}
}

//private, reached when going through higher owners.
bool Document::getparm(const string& parmkey,const DataItem*& container) const {
	bool retval = false;
	if (parm_map != NULL) {
		container = NULL;
		type_parm_map::const_iterator it = parm_map->find(parmkey);
		if (it != parm_map->end()) {
			container = ((*it).second);
			retval = true;
		}
	} 
	if (!retval && p != NULL && doc_version > 1.110120) {
		retval = p->owner->getparm(parmkey,container); //now get the stuff above me.
	}
	return retval; //if we are outside of a function there is no parm.
}

bool Document::getparm(const u_str& parmkey,const DataItem*& container) const {
	bool retval = false;
	if (parm_map != NULL) {
		container = NULL;
		string pkey; XML::Manager::transcode(parmkey,pkey);		
		type_parm_map::const_iterator it = parm_map->find(pkey);
		if (it != parm_map->end()) {
			container = ((*it).second);
			retval = true;
		} else {
			if (p != NULL && doc_version > 1.110120) {
				retval = p->owner->getparm(pkey,container); //now get the stuff above me.
			}
		}
	} 
	if (!retval && p != NULL && doc_version > 1.110120) {
		retval = p->owner->getparm(parmkey,container); //now get the stuff above me.
	}
	return retval; //if we are outside of a function there is no parm.
}
bool Document::parmexists(const u_str& parmkey) const {
	bool existent = false;
	if (parm_map != NULL) {
		string pkey; XML::Manager::transcode(parmkey,pkey);		
		type_parm_map::const_iterator it = parm_map->find(pkey);
		existent = (it != parm_map->end());
	} 
	if (!existent && p != NULL && doc_version > 1.110120) {
		existent = p->owner->parmexists(parmkey); //now get the stuff above me.
	}
	return existent; //if we are outside of a function there is no parm.
}
bool Document::parmfind(const u_str& pattern) const {
	bool retval = false;
	if ( String::Regex::available() ) {
		string rexpr; XML::Manager::transcode(pattern,rexpr);		
		for(type_parm_map::const_iterator imt = parm_map->begin(); !retval && imt != parm_map->end(); imt++) {
			retval= String::Regex::match(rexpr,imt->first);
		}
	} else {
		retval = parmexists(pattern);
	}
	if (!retval && p != NULL && doc_version > 1.110120) {
		retval = p->owner->parmfind(pattern); //now get the stuff above me.
	}
	return retval;
}
void Document::parmkeys(const u_str& pattern,set<string>& keylist) const {
	if ( String::Regex::available() ) {
		string rexpr; XML::Manager::transcode(pattern,rexpr);		
		for(type_parm_map::const_iterator imt = parm_map->begin(); imt != parm_map->end(); imt++) {
			if (String::Regex::match(rexpr,imt->first)) {
				keylist.insert(imt->first);
			}
		}
	} else {
		if (parmexists(pattern)) {
			string rexpr; XML::Manager::transcode(pattern,rexpr);		
			keylist.insert(rexpr);
		}
	}
	if (p != NULL && doc_version > 1.110120) {
		p->owner->parmkeys(pattern,keylist); //now get the stuff above me.
	}
}
void Document::inner_list() const {
 	if (parm_map != NULL && ! parm_map->empty() ) {
        *Logger::log << Log::LI << Log::II << name() << Log::IO << Log::LO;
		type_parm_map::iterator it = parm_map->begin();
		*Logger::log << Log::LI << Log::even;
		while (it != parm_map->end() ) {
			if ( ! it->first.empty() ) {
				string value;
				DataItem* x= it->second;
				if ( x == NULL) {
					value = "[NULL]"; 
				} else {
					value = *x;
				}
				*Logger::log << Log::LI << Log::II << it->first << Log::IO << Log::II << value << Log::IO << Log::LO;
			}
			it++;
		}
		*Logger::log << Log::blockend << Log::LO;
	}
	if (p != NULL && doc_version > 1.110120) {
		p->owner->inner_list(); //now get the stuff above me.
	}
}
void Document::list() const {
    *Logger::log << Log::subhead << Log::LI << Log::II << "fnparms" << Log::IO << Log::LO;
    inner_list();
    *Logger::log << Log::blockend ; //subhead.
}
Document::Document(ObyxElement* par,const Document* orig) :
ObyxElement(par,orig), xdoc(NULL),root_node(NULL),filepath(),signature(orig->signature),doc_version(HUGE_VALF),ownprefix(),parm_map(NULL),store(orig->store),doc_store(orig->doc_store),doc_par(NULL) { 
	xdoc = XML::Manager::parser()->newDoc(orig->xdoc);
	root_node = xdoc->getDocumentElement();
	filepath  = orig->filepath;
	doc_version = orig->doc_version;	
	ownprefix = orig->ownprefix;	
	if (orig->parm_map != NULL) {
		Document::type_parm_map* pm_instance = new Document::type_parm_map();
		parm_map = pm_instance;
		type_parm_map::iterator it = parm_map->begin();
		while ( it != parm_map->end()) {
			pair<Document::type_parm_map::iterator, bool> ins = pm_instance->insert(*it);
			it++;
		}
	}
	doc_par = orig->doc_par;
}
Document::Document(DataItem* inputfile,load_type use_loader, std::string fp, ObyxElement* par, bool evaluate_it) : 
ObyxElement(par,xmldocument,other,NULL), xdoc(NULL),root_node(NULL),filepath(fp),signature(),ownprefix(),parm_map(NULL),store(),doc_store(NULL),doc_par(NULL) { 
	bool loaded=false;
	ostringstream* docerrs = NULL;
	if (use_loader == Main) { //otherwise this is handled by output type = error.
		docerrs = new ostringstream();
		Logger::set_stream(docerrs);
	}
	if (inputfile == NULL) {
		*Logger::log << Log::error << Log::LI << "Error. Document " << filepath << " failed to be parsed or did not exist. " << Log::LO ;
		trace();
		*Logger::log << Log::blockend ;
		results.clear();
	} else {
		switch ( use_loader ) {
			case Main:
			case File: {
				const XMLObject* xif = dynamic_cast<const XMLObject*>(inputfile);
				if (xif != NULL) {
					xif->copy(xdoc);
				} else {
					xdoc = XML::Manager::parser()->loadDoc((string)*inputfile); 
				}
				if (xdoc != NULL) loaded=true;
			} break; //don't delete xdoc - it's owned by the parser -- hmm but get the parser to release it!!!
			case URL:
			case URLText:	
				loaded = XML::Manager::parser()->loadURI(*inputfile,xdoc); 
				break;
		}
	}
	store.setowner(name());
	if (use_loader == Main) { //otherwise this is handled by output type = error.
		root = this;
		Logger::unset_stream();
		string errs = docerrs->str(); // XMLChar::encode(errs);
		delete docerrs; docerrs=0;
		if ( ! errs.empty() ) {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Document parse error." << Log::LO;
			*Logger::log << Log::LI << Log::RI << errs << Log::RO << Log::LO;
			if (inputfile != NULL) {
				*Logger::log << Log::LI ;
				*Logger::log << Log::notify;
				*Logger::log << Log::LI << "The file that could not be parsed as xml" << Log::LO;
				*Logger::log << Log::LI << Log::info << Log::LI << string(*inputfile) << Log::LO << Log::blockend << Log::LO;
				*Logger::log << Log::blockend;
				*Logger::log << Log::LO;
			}
			*Logger::log << Log::blockend;
		}
	}
	if ( loaded ) {
		filepath_stack.push(fp);
		root_node = xdoc->getDocumentElement();
		if ( root_node != NULL ) {
			if (use_loader == URLText) {
				string textstuff;
				XML::Manager::transcode(root_node->getTextContent(),textstuff);
				String::trim(textstuff);
				results.append(textstuff,di_text);
			} else {
				if (evaluate_it) {
					if (par != NULL) {
						doc_par = par->owner;
					}
					owner = this;
					eval();
				}
			}
		}  else {
			*Logger::log << Log::debug << Log::LI << "Parser returned XML First Node as a NULL." << Log::LO << Log::blockend;	
			results.append(inputfile);
		}
		filepath_stack.pop();
	} 
	if ( evaluate_it && !results.final()  ) {
		results.normalise();	
		if ( ! results.final()  ) { //if it is STILL not final..
			*Logger::log << Log::error << Log::LI << "Error. Document " << filepath << " was not fully evaluated." << Log::LO ;
			trace();
			string troubled_doc;
			XML::Manager::parser()->writedoc(xdoc,troubled_doc);
			*Logger::log << Log::LI << "The document that failed is:" << Log::LO;
			*Logger::log << Log::LI << Log::info << Log::LI << troubled_doc << Log::LO << Log::blockend << Log::LO; 
			*Logger::log << Log::blockend; //Error
			results.clear();
		}
	}
}

Document::~Document() {
	if (parm_map != NULL) {
		type_parm_map::iterator it = parm_map->begin();
		while ( it != parm_map->end()) {
			delete (*it).second;
			it++;
		}
		delete parm_map;
	}
	if (xdoc != NULL) {
		xdoc->release();
	}
}
std::string const Document::currenthttpreq() {
	if ( curr_http_req.empty() ) {
		string head,body;
		Environment* env = Environment::service();
		env->getrequesthttp(head,body);
		OsiAPP osi;
		osi.compile_http_request(head,body,curr_http_req);
	}
	return curr_http_req;
}
bool Document::eval() {
	bool retval = false;
	if ( xdoc != NULL && root_node != NULL ) {
		u_str doc_ns; 
		const XMLCh* doc_nsp = root_node->getNamespaceURI();
		if (doc_nsp != NULL) { 
			doc_ns = doc_nsp;
			const XMLCh* rn = root_node->getPrefix();
			if (rn != NULL) {ownprefix = rn;}
			u_str doc_prefix;
			prefix(doc_prefix); //discard the test.
			if ( (doc_ns.compare(UCS2(L"http://www.obyx.org")) == 0) ) {
				retval = true;
				doc_version = Environment::version();
				std::string version_str,err_string;
				XML::Manager::attribute(root_node,UCS2(L"version"),version_str);
				XML::Manager::attribute(root_node,UCS2(L"signature"),signature);
				if (!version_str.empty()) {
					double version_val=String::real(version_str); 
					if (!isnan(version_val) && version_val > 0) {
						doc_version = version_val;
					}
				}
				if (!signature.empty()) {
					type_store_map::iterator it = docstores.find(signature);
					if (it != docstores.end()) {
						doc_store = it->second;
					} else {
						doc_store = new ItemStore();
						doc_store->setowner(signature);
						docstores.insert(type_store_map::value_type(signature, doc_store));
					}
				}
				if (prefix_stack.empty() || (ownprefix.compare(doc_prefix) != 0) ) {
					pushprefix(ownprefix);
					process(root_node,this);
					popprefix();
				} else {
 					process(root_node,this);
				}
			} else { //NOT obyx...
				if ( doc_ns.compare(UCS2(L"http://www.obyx.org/osi-application-layer")) == 0 ) {
					OsiAPP do_osi;
					DataItem* osi_result = NULL;
					int max_redirects = 33, timeout_secs = 30; //GRAB FROM STORE!!
					
					if ( do_osi.request(root_node,max_redirects,timeout_secs,osi_result)) {
						results.setresult(osi_result);
					} else {
						*Logger::log << Log::error << Log::LI << "Error. eval of osi-application-layer object failed." << Log::LO;	
						string troubled_doc;
						XML::Manager::parser()->writedoc(xdoc,troubled_doc);
						*Logger::log << Log::LI << "The request was:" << Log::LO;
						*Logger::log << Log::LI << Log::info << Log::LI << troubled_doc << Log::LO << Log::blockend << Log::LO; 
						
						troubled_doc = OsiAPP::last_osi_response();
						*Logger::log << Log::LI << "The response was:" << Log::LO;
						*Logger::log << Log::LI << Log::info << Log::LI << troubled_doc << Log::LO << Log::blockend << Log::LO; 
						
						*Logger::log << Log::blockend; //Error
					}
				} else {
					process(root_node,this);
				}
			}
		} else {
			process(root_node,this);
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. eval of non-object failed." << Log::LO;	
		trace();
		*Logger::log << Log::blockend;
	}
	return retval;
}
void Document::process( xercesc::DOMNode*& n,ObyxElement* par) {
	if (par == NULL) par = this; //owner_document
	if (!ObyxElement::break_happened) {
		ObyxElement* ce = ObyxElement::Factory(n,par);	//create the new obyxelement here
		if ( ce != NULL ) {	
			for (DOMNode* child=n->getFirstChild(); child != NULL; child=child->getNextSibling()) {				
				process(child,ce);  //carry on down..
			}
			if (!ObyxElement::break_happened) {			
				Function* fn = dynamic_cast<Function*>(ce);
				if (fn != NULL) {
					string errs;
					if (fn->pre_evaluate(errs)) {
						delete ce; ce = NULL;
					} // else it's deferred.
					if (!errs.empty()) {
						*Logger::log << Log::syntax << Log::LI << "Syntax Error. While loading flow-function. " << errs << Log::LO;	
						par->trace();
						*Logger::log << Log::blockend;
						delete ce; ce = NULL;
					}
					fn = NULL;
				} 
				if ( (ce != NULL) && (par == doc_par || (par == this) )) { //par = this then use results!
					results.evaluate(false);
				}
			}
		}
	}
}
bool const Document::prefix(u_str& container) {
	bool retval;
	if ( prefix_stack.empty() ) { //This can be reached if dom/sax validation is turned off.
		retval=false;
	} else {
		container = prefix_stack.top(); 
		retval=true;
	}
	return retval;
}
void Document::pushprefix(const u_str the_prefix) {
	prefix_stack.push(the_prefix); 
	prefix_length=the_prefix.length();
}
void Document::popprefix() { 
	prefix_stack.pop();
	if (! prefix_stack.empty() ) {
		prefix_length=prefix_stack.top().length();	
	}
}

