/* 
 * obyxelement.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * obyxelement.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>

#include "commons/httpfetch/httpfetch.h"
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"

#include "function.h"
#include "instruction.h"
#include "comparison.h"
#include "mapping.h"
#include "itemstore.h"

#include "obyxelement.h"
#include "iteration.h"
#include "output.h"
#include "inputtype.h"
#include "document.h"
#include "xmlobject.h"
#include "fragmentobject.h"

using namespace xercesc;

using namespace Log;
using namespace XML;
using namespace obyx;

unsigned long long int	ObyxElement::eval_count =0;
unsigned long long int	ObyxElement::break_point =0;
std::stack<elemtype>	ObyxElement::eval_type; 

bool					ObyxElement::break_happened = false;
nametype_map			ObyxElement::ntmap;
Vdb::Service*			ObyxElement::dbs = NULL;				
Vdb::Connection*		ObyxElement::dbc = NULL;
#ifdef PROFILING
long_map				ObyxElement::ce_map;
#endif

//------------------- XMLNode -----------------------
XMLNode::XMLNode(DOMNode* const& n,ObyxElement *par) : ObyxElement(par,xmlnode,other,n),doneit(false) {
	if ( n->getNodeType() == DOMNode::CDATA_SECTION_NODE  ) { 
		std::string nodevalue;
		u_str u_nodevalue = n->getNodeValue();
		if ( Document::prefix_length == 0 ) {
			if ( u_nodevalue[0] == ':' ) {  // [[: identification.
				Manager::transcode(u_nodevalue,nodevalue);
				p->results.append(nodevalue.substr(1,string::npos),di_text); 
				doneit = true;
			}
		} else {
			u_str masterprefix;
			if ( !Document::prefix(masterprefix) ) {
				*Logger::log << Log::error << Log::LI << "Error. Master Namespace prefix problem." << Log::LO << Log::blockend;	
			} else {
				if ( u_nodevalue.compare(0,Document::prefix_length,masterprefix) == 0) { // [[o: identification.
					Manager::transcode(u_nodevalue,nodevalue);
					p->results.append(nodevalue.substr(Document::prefix_length+1,string::npos),di_text); 				
					doneit = true;
				}
			}
		}
	} 
} 
XMLNode::~XMLNode() {}
/*
 //------------------- XMLElement -----------------------
 XMLElement::XMLElement(DOMNode* const& n,ObyxElement *par,unsigned long sib) : 
 ObyxElement(par,xmlelement,other),nodename() {
 DataItem* elnode = DataItem::factory(n,di_object);
 Manager::transcode(n->getLocalName(),nodename);
 p->results.append( elnode ); //this TAKES elnode.
 } 
 
 bool XMLElement::evaluate(size_t,size_t) {
 return true;
 }
 
 XMLElement::~XMLElement() {
 }
 */
//------------------- ObyxElement -----------------------
#ifdef PROFILING
void ObyxElement::do_alloc() {
	if (wotzit != obyx::endqueue) {
		ostringstream oss;
		const ObyxElement* t_node = this;
		string el_name;
		switch ( wotzit ) {
			case comment:		el_name= "[comment]"; break;
			case xmlnode:		el_name= "[xmlnode]"; break;
			case xmldocument:	el_name= "[xmldocument]"; break;
			case iteration:		el_name= "[iteration]"; break;
			case instruction:	el_name= "[instruction]"; break;
			case comparison:	el_name= "[comparison]"; break;
			case mapping:		el_name= "[mapping]"; break;
			case endqueue:		el_name= "[endqueue]"; break;
			case output:		el_name= "[output]"; break;
			case control:		el_name= "[control]"; break;
			case body:			el_name= "[body]"; break;
			case input:			el_name= "[input]"; break;
			case comparate:		el_name= "[comparate]"; break;
			case ontrue:		el_name= "[ontrue]"; break;
			case onfalse:		el_name= "[onfalse]"; break;
			case key:			el_name= "[key]"; break;
			case match:			el_name= "[match]"; break;
			case domain:		el_name= "[domain]"; break;
			case shortsequence:	el_name= "[s]"; break;
		}
		oss << el_name;
		t_node = t_node->p;		
		while (t_node != NULL) {
			oss << "[" << t_node->name();
			if (t_node->wotspace == flowfunction) { //grab note, if there is one..
				const Function* i = dynamic_cast<const Function *>(t_node);
				const string note =  i->note();
				if (!note.empty())	oss << " '" << i->note() << "' "; 
			}
			oss << "]";
			if (t_node->p == NULL) {
				if (t_node->wotzit == xmldocument) {
					const Document* i = dynamic_cast<const Document *>(t_node);
					t_node = i->doc_par;
				} else {
					t_node = t_node->p;
				}
			} else {
				t_node = t_node->p;
			}
		}
		string mytrace = oss.str();
		unsigned long addr = (unsigned long)(this);
		ce_map.insert(long_map::value_type(addr,mytrace));
	}
}
void ObyxElement::do_dealloc() {
	if (wotzit != obyx::endqueue) {
		unsigned long addr = (unsigned long)(this);
		long_map::iterator it = ce_map.find(addr);
		if ( it == ce_map.end() ) {
			*Logger::log << Log::error << Log::LI << "Error. ce was already deleted."  << Log::LO << Log::blockend;	
			trace();
		} else {
			ce_map.erase(it);
		}
	}
}
#endif

ObyxElement::ObyxElement(ObyxElement* par,const ObyxElement* orig) : 
owner(orig->owner),p(par),node(orig->node),results(false),wotspace(orig->wotspace),wotzit(orig->wotzit) { 
	results.copy(this,orig->results);
#ifdef PROFILING
	do_alloc(); 
#endif
}
ObyxElement::ObyxElement(ObyxElement* parent,const obyx::elemtype et,const obyx::elemclass tp,DOMNode* n) : 
owner(NULL),p(parent),node(n),results(),wotspace(tp),wotzit(et) {
	if ( p != NULL ) { owner = p->owner; }
#ifdef PROFILING
	do_alloc(); 
#endif
}
void ObyxElement::do_breakpoint() {
	eval_count++;		//global..
	if (eval_count == break_point) {
		Environment* env = Environment::service();
		unsigned int the_bp = (unsigned int)breakpoint();
		break_happened = true;
		Logger::set_syslogging(false);
		std::stack<std::ostream*> tmp_stack; //error streams from Logger.
		while (Logger::depth() > 1) {
			std::ostream* container;
			Logger::get_stream(container);
			tmp_stack.push(container);
			Logger::unset_stream();
		}
		xercesc::DOMDocument* src = node->getOwnerDocument();
		const u_str pi_name = UCS2(L"bpi"),po_name = UCS2(L"bpo");
		DOMProcessingInstruction* pi_nodi = src->createProcessingInstruction(pi_name.c_str(),NULL);
		DOMProcessingInstruction* pi_nodo = src->createProcessingInstruction(po_name.c_str(),NULL);
		if (node->getParentNode() != NULL) {
			node->getParentNode()->insertBefore(pi_nodi,node);
			if (node->getNextSibling() != NULL) {
				node->getParentNode()->insertBefore(pi_nodo,node->getNextSibling());
			} else {
				node->getParentNode()->appendChild(pi_nodo);
			}
		}
		string src_doc_str;
		XML::Manager::parser()->writedoc(src,src_doc_str);
		*Logger::log << Log::fatal << Log::LI << Log::II << "BREAKPOINT" << Log::IO << Log::II << the_bp << Log::IO << Log::LO;
		*Logger::log << Log::LI << Log::info ;
		trace();
		*Logger::log << Log::blockend << Log::LO;
		*Logger::log << Log::LI << src_doc_str << Log::LO;
		*Logger::log << Log::LI;
		results.explain(); 
		*Logger::log << Log::LO;
		*Logger::log << Log::LI ;
		env->listEnv();					//for debugging
		env->listParms();
		env->listReqCookies();
		env->listResCookies();
		owner->list();
		owner->liststore();
		Iteration::list(this);		
//		Mapping::list(this);		
		*Logger::log << Log::LO << Log::blockend;
		while (! tmp_stack.empty()) {
			Logger::set_stream(tmp_stack.top());
			tmp_stack.pop();
		}
	}
	eval_type.pop();
}
void ObyxElement::prep_breakpoint() {
	eval_type.push(wotzit);
}
unsigned long long int ObyxElement::breakpoint() {
	return eval_count + 1;
}
std::string ObyxElement::breakpoint_str() {
	return String::tostring(eval_count + 1);
}


const string ObyxElement::name() const {
	switch ( wotzit ) {
		case iteration:		return "iteration"; break;
		case control:		return "control"; break;
		case body:			return "body"; break;
		case input:			return "input"; break;
		case comparate:		return "comparate"; break;
		case ontrue:		return "ontrue"; break;
		case onfalse:		return "onfalse"; break;
		case domain:		return "domain"; break;
		case match:			return "match"; break;
		case key:			return "key"; break;
		case output:		return "output"; break;
		case instruction:	return "instruction"; break;
		case comparison:	return "comparison"; break;
		case mapping:		return "mapping"; break;
		case endqueue:		return "endqueue"; break;
		case xmldocument:	return "document"; break;
			//		case xmlelement:	return "xmlelement"; break;
		case xmlnode:		return "xmlnode"; break;
		case comment:		return "comment"; break;
		case shortsequence:	return "s"; break;
	}
	return "[unknown]";
}

void ObyxElement::trace() const { //always called within a block
	const ObyxElement* t_node = this;
	Environment* env = Environment::service();
	vector<pair<string, pair<string, string> > > fps;
	while (t_node != NULL) {
		string filesys = env->getpathforroot(); //need to remove this from filepath.
		string filepath;
		string xpath;
        string language_version;
		if (t_node->owner != NULL) {
            language_version = String::tostring(t_node->owner->version(),6);
			filepath = t_node->owner->own_filepath(); //shouldn't include the filesystem part.
			if (filepath.find(filesys) == 0) {
				filepath.erase(0,filesys.size());
			} else {
				filepath.erase(0,1+filepath.rfind('/'));
			}
		}
		if (t_node->node != NULL) { 
			basic_string<XMLCh> xp = Manager::parser()->xpath(t_node->node);
			Manager::transcode(xp,xpath);
		}
		fps.push_back(pair<string,pair<string, string> >(filepath,pair<string, string>(xpath,language_version)));
		if (t_node->owner != NULL) {
			t_node = t_node->owner->p;
		} else {
			t_node = NULL;
		}
	}
	*Logger::log << Log::LI << Log::even ;
	while (fps.size() > 0) {
		string fp= fps.back().first;
		string xp= fps.back().second.first;
        string lv= fps.back().second.second;
		*Logger::log << Log::LI << Log::II << fp << Log::IO << Log::II << xp << Log::IO << Log::II << lv << Log::IO << Log::LO;
		fps.pop_back();
	}
	*Logger::log << Log::blockend << Log::LO;
}
//------------------- static methods - once only thank-you very much -----------------------
ObyxElement* ObyxElement::Factory(DOMNode* const& n,ObyxElement* parent) {
	ObyxElement* result = NULL;
	switch (n->getNodeType()) {
		case DOMNode::ELEMENT_NODE: {
			u_str elname = n->getLocalName();
			u_str docpfx,elpfx;
			if (!Document::prefix(docpfx)) { //This is an xml document not inside an obyx container.
				DataItem* elnode = DataItem::factory(n,di_object);
				parent->results.append( elnode );
			} else {
				const XMLCh* px = n->getPrefix();
				if (px != NULL) { elpfx = px; }
				if ( (Document::prefix_length == 0 && elpfx.empty() ) || (Document::prefix_length != 0 && (docpfx.compare(elpfx) == 0) ) ) { //this is in the namespace.
					nametype_map::const_iterator i = ntmap.find(elname);	//It SHOULD be here..
					if(i != ntmap.end()) {
						elemtype cetype = i->second;
						switch ( cetype ) {
							case output: { result=new Output(n,parent,output); } break; //parmtype
								
							case iteration:		result=new Iteration(n,parent);						break; //instructiontype
							case control:		result=new InputType(n,parent,control);				break; //inputtype - parmtype
							case body:			result=new DefInpType(n,parent,body);				break; //inputtype - parmtype
								
							case instruction:	result=new Instruction(n,parent);					break; //instructiontype
							case input:			result=new InputType(n,parent,input);				break; //inputtype - parmtype
								
							case comparison:	result=new Comparison(n,parent);					break; //instructiontype
							case comparate:		result=new InputType(n,parent,comparate);			break; //inputtype - parmtype
							case ontrue:		result=new DefInpType(n,parent,ontrue);				break; //inputtype - parmtype
							case onfalse:		result=new DefInpType(n,parent,onfalse);			break; //inputtype - parmtype
								
							case mapping:		result=new Mapping(n,parent);						break; //instructiontype
							case key:			result=new InputType(n,parent,key);					break; //pattern - parmtype
							case match:			result=new DefInpType(n,parent,match);				break; //inputtype - parmtype
							case domain:		result=new DefInpType(n,parent,domain);				break; //inputtype - parmtype
								
							case comment:		break; //Use NULL
							case shortsequence: {
								Instruction *instr=new Instruction(n,parent);
								new InputType(n,instr,input);		//this is added to instr.
								result=instr;
							} break;
							default: {
								DataItem* elnode = DataItem::factory(n,di_object);
								parent->results.append( elnode );
							} break;
						}
					} else {
						if (Document::prefix_length > 0) { 
							std::string ele_name;
							Manager::transcode(elname,ele_name);
							*Logger::log << Log::error << Log::LI << "Error. " << ele_name << " is not a valid obyx element." << Log::LO;	
							parent->trace();
							*Logger::log << Log::blockend;
						}
					}
				} else {
					DataItem* elnode = DataItem::factory(n,di_object);
					parent->results.append( elnode );			
				}	
			}
		} break;
		case DOMNode::TEXT_NODE: {
			if (parent->wotspace != flowfunction) {
				u_str nodevalue;
				const XMLCh* nv = n->getNodeValue();
				if (nv != NULL) {
					nodevalue = nv;
				}
				if (! parent->results.final()) {
					IKO* iko = dynamic_cast<IKO*>(parent);
					if (iko != NULL) {
						if (iko->wsstrip) {
							XMLObject::trim(nodevalue);
						}
					} else {
						XMLObject::rtrim(nodevalue);
					}
					if ( !nodevalue.empty()) {
						parent->results.append(nodevalue,di_auto); 
					}
				} else {
					XMLObject::trim(nodevalue); 
					if (!nodevalue.empty()) {
						*Logger::log << Log::syntax << Log::LI << "Syntax Error. Value attribute is already set. Content is disallowed if there is a value attribute." << Log::LO;
						ObyxElement* oe = dynamic_cast<ObyxElement*>(parent);
						if (oe != NULL) { oe->trace(); } //pretty hard to see how this would be non-null - but best to check, i guess.
						*Logger::log << Log::blockend;
					}
				}
			}
		} break;
		case DOMNode::CDATA_SECTION_NODE: {
			std::string nodevalue;
			u_str u_nodevalue = n->getNodeValue();
			if ( Document::prefix_length == 0 ) {
				if ( u_nodevalue[0] == ':' ) {  // [[: identification.
					Manager::transcode(u_nodevalue,nodevalue);
					parent->results.append(nodevalue.substr(1,string::npos),di_text); 
				} else {
					DataItem* elcdata = DataItem::factory(n);
					parent->results.append( elcdata ); //this TAKES elnode.
				}
			} else {
				u_str masterprefix;
				if ( !Document::prefix(masterprefix) ) {
					*Logger::log << Log::error << Log::LI << "Error. Namespace prefix problem in XMLCData." << Log::LO << Log::blockend;	
				} else {
					if ( u_nodevalue.compare(0,Document::prefix_length,masterprefix) == 0) { // [[o: identification.
						Manager::transcode(u_nodevalue,nodevalue);
						parent->results.append(nodevalue.substr(Document::prefix_length+1,string::npos),di_text); 				
					} else {
						DataItem* elcdata = DataItem::factory(n);
						parent->results.append( elcdata ); //this TAKES elnode.
					}
				}
			}
		} break;
		case DOMNode::ENTITY_REFERENCE_NODE:
		case DOMNode::ATTRIBUTE_NODE:
		case DOMNode::ENTITY_NODE:
		case DOMNode::PROCESSING_INSTRUCTION_NODE:
		case DOMNode::COMMENT_NODE:
		case DOMNode::DOCUMENT_NODE: 
		case DOMNode::DOCUMENT_TYPE_NODE: 
		case DOMNode::DOCUMENT_FRAGMENT_NODE: 
		case DOMNode::NOTATION_NODE: 
			break;
	}
	return result;
}
ObyxElement::~ObyxElement() {
//results are deleted when going out of scope.
#ifdef PROFILING
	do_dealloc();
#endif
}

void ObyxElement::get_sql_service() {
	string dbservice="none";
	if (!Environment::getbenv("OBYX_SQLSERVICE",dbservice)) {
#ifdef ALLOW_POSTGRESQL
		dbservice="postgresql";		//currently hard-coded here by default.
#endif
#ifdef ALLOW_MYSQL
		dbservice="mysql";			//currently hard-coded here by default.
#endif
	}
	dbs = Vdb::ServiceFactory::getService(dbservice);
}
void ObyxElement::drop_sql_service() {
		dbs = NULL; //vdb deletes this
}
void ObyxElement::get_sql_connection() {
	if (dbs != NULL)  {
		dbc = dbs->instance();
		if (dbc != NULL) {
			dbc->open( Environment::SQLhost(),Environment::SQLuser(),Environment::SQLport(),Environment::SQLuserPW() );
			if (dbc->isopen())  {
				dbc->database(Environment::Database());
			} else {
				if (Environment::getbenvtf("OBYX_SQLSERVICE_REQ") && Logger::log != NULL) {
					*Logger::log << Log::error << Log::LI << "SQL Service. Service library was loaded but the host connection failed." << Log::LO;
					*Logger::log << Log::LI << Log::notify << Log::LI;
					*Logger::log << "If the host is on another box, check the database client configuration or host that networking is enabled. " << Log::LO;
					*Logger::log << Log::LI << "mysql -D" << Environment::Database() << " -h" << Environment::SQLhost() << " -u" << Environment::SQLuser();
					int pt = Environment::SQLport(); if (pt != 0) {
						*Logger::log << " -P" << pt << Log::LO;
					}
					string up = Environment::SQLuserPW(); if (!up.empty()) {
						*Logger::log << " -p";
						string px(up.size(),'*');
						px[0] = up[0];
						px[px.size()-1] = up[up.size()-1];
						*Logger::log << " -p" << px << Log::LO;
					}
					*Logger::log << Log::blockend << Log::LO << Log::blockend;
				}
			}
		}
	} 
}
void ObyxElement::drop_sql_connection() {
	if (dbc != NULL ) {
		if (dbc->isopen())  { //dbs is managed by vdb. Just look after Connections.
			dbc->close();
			delete dbc; 
		}
		dbc = NULL;
	} 
}
void ObyxElement::shutdown() {
	Function::shutdown();
	IKO::shutdown();
#ifdef FAST
	string tmp;
	if (!Environment::getbenvtf("OBYX_SQLPER_REQUEST")) {
		drop_sql_connection();
	}
#else
	drop_sql_connection();
#endif
	drop_sql_service();
	ntmap.clear();
}
void ObyxElement::startup() {
	get_sql_service();
#ifdef FAST
	string tmp;
	if (!Environment::getbenvtf("OBYX_SQLPER_REQUEST")) {
		get_sql_connection();
	}
#endif
	Function::startup();
	IKO::startup();
	ntmap.insert(nametype_map::value_type(UCS2(L"iteration"), iteration));
	ntmap.insert(nametype_map::value_type(UCS2(L"context"), iteration));
	ntmap.insert(nametype_map::value_type(UCS2(L"control"), control));
	ntmap.insert(nametype_map::value_type(UCS2(L"body"), body));
	ntmap.insert(nametype_map::value_type(UCS2(L"instruction"), instruction));
	ntmap.insert(nametype_map::value_type(UCS2(L"comparison"), comparison));
	ntmap.insert(nametype_map::value_type(UCS2(L"output"), output));
	ntmap.insert(nametype_map::value_type(UCS2(L"input"), input));
	ntmap.insert(nametype_map::value_type(UCS2(L"comparate"), comparate));
	ntmap.insert(nametype_map::value_type(UCS2(L"ontrue"), ontrue));
	ntmap.insert(nametype_map::value_type(UCS2(L"onfalse"), onfalse));
	ntmap.insert(nametype_map::value_type(UCS2(L"mapping"), mapping));
	ntmap.insert(nametype_map::value_type(UCS2(L"key"), key));	
	ntmap.insert(nametype_map::value_type(UCS2(L"match"), match));
	ntmap.insert(nametype_map::value_type(UCS2(L"domain"), domain));
	ntmap.insert(nametype_map::value_type(UCS2(L"s"), shortsequence));
	ntmap.insert(nametype_map::value_type(UCS2(L"comment"), comment));
}
void ObyxElement::init() {
#ifndef FAST
	get_sql_connection();
#else
	string tmp;
	if (Environment::getbenvtf("OBYX_SQLPER_REQUEST")) {
		get_sql_connection();
	}
#endif
	Environment* env = Environment::service();
	Function::init();
	eval_count = 0;
	break_point = 0;
	break_happened = false;
	string envtmp,BREAK_COUNT_val;
	bool ok_to_break = env->getenvtf("OBYX_DEVELOPMENT");
	env->getcookie_req("BREAK_COUNT",BREAK_COUNT_val);
	if (!BREAK_COUNT_val.empty() && ok_to_break ) {
		pair<unsigned long long,bool> bp_value = String::znatural(BREAK_COUNT_val);
		if  ( bp_value.second ) {
			break_point = bp_value.first;
		} else {
			*Logger::log << Log::error << Log::LI << "Error. BREAK_COUNT: must be a natural." << Log::LO << Log::blockend; 
		}
	}	
}
void ObyxElement::finalise() {
	Function::finalise();
	FragmentObject::finalise();
	while (!eval_type.empty()) { eval_type.pop();}
#ifdef FAST
	string tmp;
	if (Environment::getbenvtf("OBYX_SQLPER_REQUEST")) {
		drop_sql_connection();
	}
#endif
#ifdef PROFILING
	if ( ! ce_map.empty() ) {
		*Logger::log << Log::error << Log::LI << "Error. Not all ObyxElements were deleted."  << Log::LO << Log::blockend;	
		for( long_map::iterator imt = ce_map.begin(); imt != ce_map.end(); imt++) {
			*Logger::log << Log::info << imt->second << Log::LO << Log::blockend;				
		}
		ce_map.clear();
	}
#endif
}
