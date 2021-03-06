/* 
 * inputtype.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * inputtype.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <deque>
#include <string>
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"
#include "commons/xml/xml.h"
#include "commons/filing/filing.h"
#include "commons/httpfetch/httpfetch.h"

#include "iteration.h"
#include "obyxelement.h"
#include "iko.h"
#include "function.h"
#include "comparison.h"
#include "instruction.h"
#include "mapping.h"
#include "inputtype.h"
#include "document.h"

using namespace Vdb;
//using namespace Log;
using namespace XML;
using namespace Fetch;
using namespace obyx;

IKO::inp_space_map  InputType::inp_spaces;
InputType::InputType(xercesc::DOMNode* const& n,ObyxElement* par, elemtype el) : 
IKO(n,par,el),eval(false),release(false),ascending(true),type(immediate),parm_name() {
	u_str str_type,eval_str,release_str;
	if ( Manager::attribute(n,u"type",str_type)  ) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << name() << ": attribute 'type' should be 'space'" << Log::LO;
		trace();
		*Logger::log  << Log::blockend;
	}
	if ( Manager::attribute(n,u"space",str_type) ) {
		inp_space_map::const_iterator j = inp_spaces.find(str_type);
		if( j != inp_spaces.end() ) {
			type = j->second;
		} else {
			string err_type; Manager::transcode(str_type.c_str(),err_type);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << name() << ": " <<  err_type << " is not a legal space. It should be one of " ;
			*Logger::log << "immediate, none, store, field, sysparm, sysenv, cookie, file, url, parm, namespace, grammar" << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
	switch (type) {
		case immediate:
		case none:  
		case error: {			
			exists = true; 
		} break; //not reference types
		case url:
		case cookie:
		case sysparm:
		case sysenv: {
			exists = false;
		} break; //reference types
		case store:
		case xmlnamespace:
		case xmlgrammar:
		case field: 
		case file: {
			exists = false; 
		} break; //reference types
		case fnparm: {
			exists = false; 
		} break; //reference types
	}
	if (type == fnparm ) {
		exists = false; 
	}
	if ((type == immediate || type == none ) && context != immediate) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << name() << ": context attribute cannot be used with immediate or empty spaces." << Log::LO ;
		trace();
		*Logger::log << Log::blockend;
	}
	if ( Manager::attribute(n,u"eval",eval_str) ) {
		if (eval_str.compare(u"true") == 0) eval = true;
	}
	if ( Manager::attribute(n,u"release",release_str) ) {
		if (release_str.compare(u"true") == 0) release = true;
		switch (type) {
				//			case url:	//cache
				//			case file:	//cache
			case xmlgrammar:	
			case xmlnamespace:	
			case store: {
			} break;
			default: {
				if (release == true) {
					*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << name() << ":release can only be declared true for store,url,file,grammar,namespace spaces."  << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
			} break; //reference types
		}
	}
	Function* i = dynamic_cast<Function *>(p);
	if (i == nullptr) {
		if ( wotzit == obyx::key) {
			DefInpType* m = dynamic_cast<DefInpType *>(p);
			if (m != nullptr && m->wotzit == obyx::match) {
				m->key = this;
				
				//break="false"
				u_str attr_val,format_str;
				Manager::attribute(n,u"break",attr_val);
				if ( (! attr_val.empty()) && (attr_val.compare(u"true") == 0)) {
					m->k_break = true; 
				}
				attr_val.clear();
				//scope="all|first"
				Manager::attribute(n,u"scope",attr_val);
				if ( !attr_val.empty()) {
					Mapping* mp = dynamic_cast<Mapping *>(m->p);
					if (mp != nullptr) {
						if (mp->op() == m_substitute) {
							if ( attr_val.compare(u"first") == 0 ) {
								m->k_scope = false; 
							} else {
								if ( attr_val.compare(u"all") == 0 ) {
									m->k_scope = true; 
								} else {
									*Logger::log << Log::syntax << Log::LI << "Syntax Error. The scope attribute of a key must have the value 'first' or 'all' "  << Log::LO;
									trace();
									*Logger::log << Log::blockend;
								}
							}
						} else {
							if ( attr_val.compare(u"first") == 0 ) {
								*Logger::log << Log::syntax << Log::LI << "Syntax Error. The scope attribute of a key is only meaningful within a substitute mapping.  ";
								*Logger::log << "As keys within 'switch' mappings match the entire domain, the scope attribute can only be 'all'."  << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							} else {
								//Ignored. XML Parser may add in default attribute values, so we must ignore it if we are not in substitute.
							}
						}
					} else {
						*Logger::log << Log::syntax << Log::LI << "Syntax Error. match with a key can only be a child of mapping."  << Log::LO;
						trace();
						*Logger::log << Log::blockend;
					}
				}
				
				//format="regex" // or literal... 'l' or 'r' 
				Manager::attribute(n,u"format",format_str);
				if ( ! format_str.empty() ) { m->k_format = format_str[0]; }
			} else {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << name() << " can only be a child of match."  << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log <<  Log::syntax << Log::LI << "Syntax Error. Input types can only belong to iteration, include, instruction, comparison, substitution, mapping, match" << Log::LO;	
			trace();
			*Logger::log << Log::blockend;
		}
	} else {
		switch (wotzit) { //don't push back the definputs here..
			case input: {
				Instruction * ix = dynamic_cast<Instruction *>(i);
				if (ix != nullptr) {
					switch (ix->op()) {
						case obyx::function:
						case obyx::arithmetic:
						case obyx::bitwise: {
							u_str tmp_v;
							Manager::attribute(n,u"name",tmp_v);
							XML::Manager::transcode(tmp_v,parm_name);
						} break;
						case obyx::sort: {
							u_str order_s;
							Manager::attribute(n,u"order",order_s);
							ascending = (order_s[0] == 'a');
						} break;
						default: break;
					}
				}
			}
			case control:
			case comparate:
				i->addInputType(this);
				break;
			default: break;
		}
	}
}
InputType::InputType(ObyxElement* par,const InputType* orig) : IKO(par,orig),
eval(orig->eval),release(orig->release),ascending(orig->ascending),type(orig->type),parm_name(orig->parm_name) {
}
InputType::~InputType() {
	//higher stuff is deleted by ~IKO
}

void InputType::evalfind(std::set<std::string>& keylist) {
	if (type != error && type != none) {
		inp_space space = type;		//we need to evaluate this as immediate.
		type = immediate;
		kind = di_text;
		evaluate();
		DataItem* iresult= nullptr; results.takeresult(iresult);
		u_str key = *iresult;
		if (!key.empty()) {
			keysinspace(key,space,keylist);	//gather them keys.
		}
		delete iresult; 
	} else {
		evaluate(); 
		keylist.clear();
	}
}
void InputType::evaluate(size_t /*item_num*/,size_t /*item_count*/) {
	prep_breakpoint();
	prepcatch();
	if (!results.final()) {	//This can happen when the value is not an attribute.
		results.evaluate(wsstrip);				//small leak here.
	}
	if (results.final()) {
		DataItem* name_part = nullptr;				  //name_part is what it normally is, unless type=immediate!
		switch ( context ) {					  //do the context, including none/immediate.
			case error:
			case none:
			case immediate: {
				results.takeresult(name_part); //this is evaluating the context. we probably want the name.
			} break;
			default: {
				DataItem* context_part = nullptr; 
				results.takeresult(context_part);
				//          type  release eval  context kind  name/ref container 
				evaltype(context, false, false, true, di_text, context_part,name_part);
//				if (!xpath.empty()) {
					//was this used already?
//					xpath.clear();
//				}
				delete context_part; 
				context_part=nullptr;
				context = immediate;
			} break;
		}
		DataItem* value_part = nullptr;  //remember - for nearly everything other than immediate, the input value is a name.
		if (encoder != e_none) {
			if (process == encode) {
				evaltype(type, release, eval, false, kind, name_part, value_part);    //Use the kind before the encoding.
				kind = di_auto;														  //Kind should be used once.
			} else {
				evaltype(type, release, eval, false, di_auto, name_part, value_part); //Use the kind after the decoding
			}
			process_encoding(value_part); //This will use the kind.
		} else {
			evaltype(type, release, eval, false, kind, name_part, value_part); //Big memory leak here.
		}
		results.setresult(value_part); //name_part is generated by evaltype, so we don't need to copy.
		if (name_part != nullptr) {
			delete name_part; name_part=nullptr;
		}
	}
	dropcatch();
	do_breakpoint();
}
void InputType::startup() {
	//static methods - once only thank-you very much..
	inp_spaces.insert(inp_space_map::value_type(u"immediate", immediate));
	inp_spaces.insert(inp_space_map::value_type(u"none", none));
	inp_spaces.insert(inp_space_map::value_type(u"store", store));
	inp_spaces.insert(inp_space_map::value_type(u"field", field ));
	inp_spaces.insert(inp_space_map::value_type(u"url", url ));
	inp_spaces.insert(inp_space_map::value_type(u"file", file ));
	inp_spaces.insert(inp_space_map::value_type(u"sysparm", sysparm));
	inp_spaces.insert(inp_space_map::value_type(u"sysenv", sysenv));
	inp_spaces.insert(inp_space_map::value_type(u"cookie", cookie));
	inp_spaces.insert(inp_space_map::value_type(u"error", error));
	inp_spaces.insert(inp_space_map::value_type(u"parm", fnparm));
	inp_spaces.insert(inp_space_map::value_type(u"namespace", xmlnamespace));
	inp_spaces.insert(inp_space_map::value_type(u"grammar", xmlgrammar));
}
void InputType::shutdown() {
	inp_spaces.clear();
}
DefInpType::DefInpType(xercesc::DOMNode* const& n,ObyxElement* par,elemtype el) : 
InputType(n,par,el),k_break(false),k_scope(true),k_format('l'),key(nullptr) {
	//this includes match and domain but not key.
	wotspace=defparm;
	Function* i = dynamic_cast<Function *>(par);
	if (i != nullptr) {
		i->addDefInpType(this);
	} else {
		*Logger::log << Log::error << Log::LI << "Error. " << name() << " Input types must be inside flow-functions!" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
}
DefInpType::DefInpType(ObyxElement* par,const DefInpType* orig) : 
InputType(par,orig),k_break(orig->k_break),k_scope(orig->k_scope),k_format(orig->k_format),key(nullptr) { 
	if (wotzit == obyx::match && orig->key != nullptr) {
		key = new InputType(p,orig->key);
	} else {
		key = nullptr;
	}
}
void DefInpType::evaluate_key() { //result = if key is evaluated.
	if (wotzit == obyx::match && key != nullptr) {
		key->results.undefer();
		key->evaluate(0,0);
		//There MAY have been a reason for this, but it is beyond me what. So it's been disabled.
		/*		
		 if (key->encoder != e_none) {
		 DataItem* pe = nullptr;
		 results.takeresult(pe);
		 key->process_encoding(pe);
		 results.setresult(pe);
		 }
		 */
	}
}
void DefInpType::evaluate(size_t,size_t) {
	results.undefer();
	InputType::evaluate();
}
DefInpType::~DefInpType() {
	if ( key != nullptr) delete key;
}

