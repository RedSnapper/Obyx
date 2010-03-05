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
using namespace qxml;

unsigned long long int		ObyxElement::eval_count =0;
unsigned long long int		ObyxElement::break_point =0;

long_map				ObyxElement::ce_map;
nametype_map			ObyxElement::ntmap;
Vdb::ServiceFactory*	ObyxElement::dbsf = NULL;			//this is managed by main.
Vdb::Service*			ObyxElement::dbs = NULL;				
Vdb::Connection*		ObyxElement::dbc = NULL;

//------------------- XMLNode -----------------------
XMLNode::XMLNode(DOMNode* const& n,ObyxElement *par) : ObyxElement(par,xmlnode,other,n),doneit(false) {
	if ( n->getNodeType() == DOMNode::CDATA_SECTION_NODE  ) { 
		std::string nodevalue;
		u_str u_nodevalue = n->getNodeValue();
		if ( Document::prefix_length == 0 ) {
			if ( u_nodevalue[0] == ':' ) {  // [[: identification.
				transcode(u_nodevalue.c_str(),nodevalue);
				p->results.append(nodevalue.substr(1,string::npos),di_text); 
				doneit = true;
			}
		} else {
			u_str masterprefix;
			if ( !Document::prefix(masterprefix) ) {
				*Logger::log << Log::error << Log::LI << "Error. Master Namespace prefix problem." << Log::LO << Log::blockend;	
			} else {
				if ( u_nodevalue.compare(0,Document::prefix_length,masterprefix) == 0) { // [[o: identification.
					transcode(u_nodevalue.c_str(),nodevalue);
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
	transcode(n->getLocalName(),nodename);
	p->results.append( elnode ); //this TAKES elnode.
} 

bool XMLElement::evaluate(size_t,size_t) {
	return true;
}

XMLElement::~XMLElement() {
}
*/
//------------------- ObyxElement -----------------------

void ObyxElement::do_alloc() {
	//----------------for testing allocation ---
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
		if (t_node->wottype == flowfunction) { //grab note, if there is one..
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
	//--------------------------------------------------------------
}

void ObyxElement::do_dealloc() {
	unsigned long addr = (unsigned long)(this);
	long_map::iterator it = ce_map.find(addr);
	if ( it == ce_map.end() ) {
		*Logger::log << Log::error << Log::LI << "Error. ce was already deleted."  << Log::LO << Log::blockend;	
		trace();
	} else {
		ce_map.erase(it);
	}
}

ObyxElement::ObyxElement(ObyxElement* par,const ObyxElement* orig) : 
	owner(orig->owner),p(par),node(orig->node),results(false),wottype(orig->wottype),wotzit(orig->wotzit) { 
	results.copy(this,orig->results);
//	do_alloc(); 
}

ObyxElement::ObyxElement(ObyxElement* parent,const qxml::elemtype et,const qxml::elemclass tp,DOMNode* n) : 
	owner(NULL),p(parent),node(n),results(),wottype(tp),wotzit(et) {
	if ( p != NULL ) { owner = p->owner; }
}

void ObyxElement::do_breakpoint() {
	eval_count++;		//global..
	if (eval_count == break_point) {
		std::stack<std::ostream*> tmp_stack; //error streams from Logger.
		while (Logger::depth() > 1) {
			std::ostream* container;
			Logger::get_stream(container);
			tmp_stack.push(container);
			Logger::unset_stream();
		}
		xercesc::DOMDocument* src = node->getOwnerDocument();
		u_str pi_name;
		XML::transcode("bp",pi_name);
		DOMProcessingInstruction* pi_nodi = src->createProcessingInstruction(pi_name.c_str(),NULL);
		DOMProcessingInstruction* pi_nodo = src->createProcessingInstruction(pi_name.c_str(),NULL);
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
		*Logger::log << Log::warn << Log::LI << "BREAKPOINT " << (unsigned int)eval_count << Log::LO;
		*Logger::log << Log::LI << Log::info ;
		trace();
		*Logger::log << Log::blockend << Log::LO;
		*Logger::log << Log::LI << src_doc_str << Log::LO;
		*Logger::log << Log::LI ;
		Environment::listEnv();					//for debugging
		Environment::listParms();
		Environment::listReqCookies();
		Environment::listResCookies();
		owner->list();
		ItemStore::list();
		*Logger::log << Log::LO << Log::blockend;
		while (! tmp_stack.empty()) {
			Logger::set_stream(tmp_stack.top());
			tmp_stack.pop();
		}
	}
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
	vector<pair<string,string> > fps;
	while (t_node != NULL) {
		string filesys = Environment::getpathforroot(); //need to remove this from filepath.
		string filepath;
		string xpath;
		if (t_node->owner != NULL) {
			filepath = t_node->owner->own_filepath(); //shouldn't include the filesystem part.
			if (filepath.find(filesys) == 0) {
				filepath.erase(0,filesys.size());
			}
		}
		if (t_node->node != NULL) { 
			basic_string<XMLCh> xp = Manager::parser()->xpath(t_node->node);
			transcode(xp.c_str(),xpath);
		}
		fps.push_back(pair<string,string>(filepath,xpath));
		if (t_node->owner != NULL) {
			t_node = t_node->owner->p;
		} else {
			t_node = NULL;
		}
	}
	while (fps.size() > 0) {
		string fp= fps.back().first;
		string xp= fps.back().second;
		*Logger::log << Log::LI; 
		if (!fp.empty()) {
			*Logger::log << Log::LIFP << fp << Log::LOFP;
		}
		if (!xp.empty()) {
			*Logger::log << Log::LIXP << xp << Log::LOXP;
		}
		*Logger::log << Log::LIPP << (unsigned int)eval_count << Log::LOPP;
		*Logger::log << Log::LO; 
		fps.pop_back();
	}
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
							case output:		result=new Output(n,parent,output);					break; //parmtype
								
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
							transcode(elname.c_str(),ele_name);
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
			if (parent->wottype != flowfunction) {
				std::string nodevalue;
				transcode(n->getNodeValue(),nodevalue);
				if (! parent->results.final()) {
					IKO* iko = dynamic_cast<IKO*>(parent);
					if (iko != NULL) {
						if (iko->wsstrip) {
							String::trim(nodevalue);
						}
					} else {
						String::rtrim(nodevalue);
					}
					if ( !nodevalue.empty()) {
						parent->results.append(nodevalue,di_auto); 
					}
				} else {
					String::trim(nodevalue); 
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
					transcode(u_nodevalue.c_str(),nodevalue);
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
						transcode(u_nodevalue.c_str(),nodevalue);
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
//	do_dealloc();
}

void ObyxElement::finalise() {
	Function::finalise();
	FragmentObject::finalise();
	
	if ( ! ce_map.empty() ) {
		*Logger::log << Log::error << Log::LI << "Error. Not all ObyxElements were deleted."  << Log::LO << Log::blockend;	
		for( long_map::iterator imt = ce_map.begin(); imt != ce_map.end(); imt++) {
			*Logger::log << Log::info << imt->second << Log::LO << Log::blockend;				
		}
	}

	ntmap.clear();
	String::Regex::shutdown();
	Fetch::HTTPFetch::shutdown();
	if (dbc != NULL)  delete dbc;  //dbs is managed by vdb. Just look after Connections.
}

void ObyxElement::init(Vdb::ServiceFactory*& dbsf_i) {
	eval_count = 0;
	break_point=0;
	
	string BREAK_COUNT_val,sa,ra;
	Environment::getcookie_req("BREAK_COUNT",BREAK_COUNT_val);
	Environment::getenv("REMOTE_ADDR",sa);
	Environment::getenv("SERVER_ADDR",ra);
	if (!BREAK_COUNT_val.empty() && sa.compare(ra) == 0 ) {
		pair<unsigned long long,bool> bp_value = String::znatural(BREAK_COUNT_val);
		if  ( bp_value.second ) {
			break_point = bp_value.first;
		} else {
			*Logger::log << Log::error << Log::LI << "Error. BREAK_COUNT: must be a natural." << Log::LO << Log::blockend; 
		}
	}	
	
	dbsf= dbsf_i;		//Now we have a service factory for database services.
	string dbservice="none";
	if (!Environment::getenv("OBYX_SQLSERVICE",dbservice)) {
#ifdef ALLOW_POSTGRESQL
		dbservice="postgresql";		//currently hard-coded here by default.
#endif
#ifdef ALLOW_MYSQL
		dbservice="mysql";			//currently hard-coded here by default.
#endif
	}
	dbs = dbsf->getService(dbservice);
	if (dbs != NULL)  {
		dbc = dbs->instance();
		dbc->open( Environment::SQLhost(),Environment::SQLuser(),Environment::SQLport(),Environment::SQLuserPW() );
		if (dbc->isopen())  {
			dbc->database(Environment::Database());
		} else {
			*Logger::log << Log::error << Log::LI << "SQL Service '" << dbservice << "' was fine, but connection to the database failed." << Log::LO << Log::blockend; 					
		}
	} 
	FragmentObject::init();
	Function::init();
	InputType::init();
	Output::init();
	Fetch::HTTPFetch::startup();
	String::Regex::startup();
	IKO::init();
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