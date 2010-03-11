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
#include "osiapp.h"
#include "xmlobject.h"
#include "dataitem.h"
#include "itemstore.h"


using namespace Log;
using namespace xercesc;
using namespace XML;

XML::Manager* Document::xmlmanager = NULL;

std::stack<u_str>			Document::prefix_stack;
std::stack<std::string>		Document::filepath_stack;
size_t						Document::prefix_length=0;		//Document wide specified	

string Document::curr_http_req;

void Document::init() {
	xmlmanager = new XML::Manager();
	ItemStore::init();  //needs xerces
}

void Document::shutdown() {
	ItemStore::shutdown();  //needs xerces
	delete xmlmanager;
}

const std::string Document::currentname() { 
	string root = Environment::getpathforroot();
	string fp = filepath_stack.top();
	if ( fp.find(root) == 0 ) {
		fp.erase(0,root.length());
	}
	return fp;
}

const std::string Document::name() const { 
	string root = Environment::getpathforroot();
	string fp = filepath;
	if ( fp.find(root) == 0 ) {
		fp.erase(0,root.length());
	}
	return fp;
}

const xercesc::DOMDocument* Document::doc() const {
	return xdoc;
}

bool Document::getparm(u_str const docname,const DataItem*& container) const {
	bool retval = false;
	if (parm_map != NULL) {
		container = NULL;
		type_parm_map::const_iterator it = parm_map->find(docname);
		if (it != parm_map->end()) {
			container = ((*it).second);
			retval = true;
		}
	} 
	return retval; //if we are outside of a function there is no parm.
}

void Document::list() const {
	if (parm_map != NULL && ! parm_map->empty() ) {
		type_parm_map::iterator it = parm_map->begin();
		*Logger::log << Log::subhead << Log::LI << "Current parms" << Log::LO << Log::LI;
		while (it != parm_map->end() ) {
			if ( ! it->first.empty() ) {
				string err_msg; transcode(it->first.c_str(),err_msg);
				
				*Logger::log << Log::even << Log::LI << "[" << err_msg << "]"; 
				DataItem* x= it->second;
				if ( x == NULL) {
					*Logger::log <<  " -- which is NULL!"; 
				} else {
					string result_doc = *x;
					*Logger::log << Log::rule << result_doc << Log::rule; 
				}
				*Logger::log << Log::LO << Log::blockend; 	//even
			}
			it++;
		}
		*Logger::log << Log::LO << Log::blockend; //subhead
	}
}

Document::Document(ObyxElement* par,const Document* orig) :
ObyxElement(par,orig), xdoc(NULL),root_node(NULL),filepath(),ownprefix(),
parm_map(NULL),doc_par(NULL) { 
	xdoc = XML::Manager::parser()->newDoc(orig->xdoc);
	root_node = xdoc->getDocumentElement();
	filepath  = orig->filepath;
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
	ObyxElement(par,xmldocument,other,NULL), xdoc(NULL),root_node(NULL),filepath(fp),ownprefix(),
	parm_map(NULL),doc_par(NULL) { 
	bool loaded=false;
	ostringstream* docerrs = NULL;
	if (use_loader == Main) { //otherwise this is handled by output type = error.
		docerrs = new ostringstream();
		Logger::set_stream(docerrs);
	}
	if (inputfile == NULL) {
		*Logger::log << Log::error << Log::LI << "Error. Document " << filepath << " failed to be parsed or did not exist. " << Log::LO ;
		trace();
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
			} break; //don't  delete xdoc - it's owned by the parser -- hmm but get the parser to release it!!!
			case URL:
			case URLText:	
				loaded = XML::Manager::parser()->loadURI(*inputfile,xdoc); 
				break;
		}
	}
	if (use_loader == Main) { //otherwise this is handled by output type = error.	
		Logger::unset_stream();
		string errs = docerrs->str(); // XMLChar::encode(errs);
		delete docerrs; docerrs=0;
		if ( ! errs.empty() ) {
			*Logger::log << Log::error << Log::LI << "Error. Document parse error." << Log::LO;
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
				XML::transcode(root_node->getTextContent(),textstuff);
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
//			results.explain();
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
		Environment::getrequesthttp(head,body);
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
					if ( do_osi.request(root_node, osi_result)) {
						results.setresult(osi_result);
					} else {
						*Logger::log << Log::error << Log::LI << "Error. eval of osi-application-layer object failed." << Log::LO;	
						trace();
						*Logger::log << Log::blockend;
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
	ObyxElement* ce = ObyxElement::Factory(n,par);	//create the new obyxelement here
	if ( ce != NULL ) {	
		for (DOMNode* child=n->getFirstChild(); child != NULL; child=child->getNextSibling()) {				
			process(child,ce);  //carry on down..
		}
		Function* fn = dynamic_cast<Function*>(ce);
		if (fn != NULL) {
			if (fn->pre_evaluate()) {
				delete ce; ce = NULL;
			} // else it's deferred.
		} 
		if ( (ce != NULL) && (par == doc_par || (par == this) )) {
			if ( ce->evaluate() )  {
				DataItem* rs = NULL;
				ce->results.takeresult(rs);
				results.setresult(rs);
			}
//			delete ce;  //YES there's a bug here. It's happening inside functions. somewhere.
//			ce = NULL;
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
	ItemStore::prefixpushed(the_prefix);
}

void Document::popprefix() { 
	prefix_stack.pop();
	if (! prefix_stack.empty() ) {
		ItemStore::prefixpopped(prefix_stack.top());
		prefix_length=prefix_stack.top().length();	
	}
}

