/* 
 * mapping.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mapping.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <ctype.h>
#include <math.h>
#include <xercesc/dom/DOMNode.hpp>

#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/string/strings.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"

#include "xmlobject.h"
#include "strobject.h"
#include "pairqueue.h"
#include "pairqueue.h"
#include "iteration.h"
#include "obyxelement.h"
#include "function.h"
#include "inputtype.h"
#include "output.h"
#include "document.h"
#include "mapping.h"
#include "itemstore.h"

using namespace Log;
using namespace obyx;

map_type_map Mapping::map_types;

Mapping::Mapping(xercesc::DOMNode* const& n,ObyxElement* par) : 
Function(n,mapping,par),operation(m_switch),repeated(false),keys_evaluated(false),dom_evaluated(false),mat_evaluated(false),matched(false),sdom(""),skey("") {
	
	u_str do_repeat,op_string;
	XML::Manager::attribute(n,UCS2(L"operation"),op_string);
	map_type_map::const_iterator i = map_types.find(op_string);
	if( i != map_types.end() ) {
		operation = i->second; 
	} else {
		if ( ! op_string.empty() ) {
			string err_msg; transcode(op_string.c_str(),err_msg);
			*Logger::log <<  Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal operation. It should be one of switch, substitute" << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
	bool had_rpt = XML::Manager::attribute(n,UCS2(L"repeat"),do_repeat);
	if ( had_rpt ) {
		if (do_repeat.compare(UCS2(L"true")) == 0) { 
			if (operation != m_switch) {
				repeated = true;
			} else {
				*Logger::log <<  Log::syntax << Log::LI << "Syntax Error. repeat is not legal for switch operations, as both the keys and the domain won't change." << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		} else {
			if (do_repeat.compare(UCS2(L"false")) != 0) {
				*Logger::log <<  Log::syntax << Log::LI << "Syntax Error. repeat attribute must be either 'true' or 'false'" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
}
Mapping::Mapping(ObyxElement* par,const Mapping* orig) : Function(par,orig),operation(orig->operation),
repeated(orig->repeated),keys_evaluated(orig->keys_evaluated),
dom_evaluated(orig->dom_evaluated),mat_evaluated(orig->mat_evaluated),
matched(orig->matched),sdom(orig->sdom),skey(orig->skey) {
}
bool Mapping::evaluate_this() {
	size_t inpsize = definputs.size();
	if (inpsize < 2) {
		*Logger::log <<  Log::error << Log::LI << "Error. Mapping must have one domain and at least one match." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
		return true;
	}
	//1. make sure all our keys evaluate. 
	if (!keys_evaluated ) {
		keys_evaluated = true;
		for ( unsigned int i = 1; i < inpsize; i++ ) {  // first definput is the domain.
			bool wasfinal = definputs[i]->evaluate_key();
			keys_evaluated = keys_evaluated && wasfinal;
		}
	} 
	
	//2. once the keys are evaluated, we need to evaluate the domain.
	if (keys_evaluated && !dom_evaluated ) {
		dom_evaluated = definputs[0]->evaluate();
	}
	
    //3. for each match, we need to find it in the domain, and then evaluate it, iff it is found.
	if (keys_evaluated && dom_evaluated && definputs.size() > 1) {
		bool error_sent = false;
		unsigned long long forced_break = 100;
		DataItem* the_domain = NULL;
		definputs[0]->results.takeresult(the_domain);
		if (repeated) {
			string regbrk;
			if (ItemStore::get("REPEAT_BREAK_COUNT",regbrk)) {
				pair<unsigned long long,bool> forced_break_setting = String::znatural(regbrk);
				if ( forced_break_setting.second ) forced_break = forced_break_setting.first;
			}
		}
		bool t_deferred=false;
		DefInpType *def_match = NULL;
		do {
			forced_break--;
			matched = false;
			def_match = NULL;
			for ( unsigned int j = 1; j < definputs.size() && !t_deferred; j++ ) {
				t_deferred = false;
				DefInpType *match = definputs[j];
				InputType *key = match->key;
				if ( key == NULL) { // this is a default value - so just replace 
					if (def_match == NULL) {
						def_match = match;
					} else {
						if (!error_sent) {
							*Logger::log << Log::error << Log::LI << "Error. Mapping accepts one match with no key to act as the default." << Log::LO; 
							trace();
							*Logger::log << Log::blockend;	
							error_sent = true;
						}
					}
					break;  //do not continue processing this set.
				} else {
					if ( the_domain != NULL ) {
						kind_type dom_kind = the_domain->kind();
						DefInpType* tmp_input = NULL;
						switch (match->k_format) { 
							case 'l': { //literal
								switch (operation) {
									case m_switch: { // m_switch uses full-matches without changing the domain (repeat illegal).
										if ( the_domain->same( key->results.result()) ) {
											tmp_input = match;
											if ( tmp_input->evaluate() ) {
												matched = true;
												DataItem* tmp = NULL;
												tmp_input->results.takeresult(tmp);
												results.setresult(tmp,match->wsstrip);
											} else { t_deferred=true; }
										}
									} break;
									case m_state: { // m_switch uses full-matches and changes the domain.
										if ( the_domain->same( key->results.result()) ) {
											if (repeated) {
												tmp_input = new DefInpType(this,match);
											} else {
												tmp_input = match;
											}									
											if ( tmp_input->evaluate() ) {
												matched = true;
												delete the_domain;
												tmp_input->results.takeresult(the_domain);
											} else { t_deferred=true; }
										}
									} break;
									case m_substitute: { // m_substitute uses partial-matching and changes the domain.
										sdom = *the_domain;
										if (key->results.result() != NULL) {
											skey = *key->results.result();
										}
										if ( (key->results.result() == NULL) || (sdom.find(skey) != string::npos) ) { // do the match
											if (repeated) {
												tmp_input = new DefInpType(this,match);
											} else {
												tmp_input = match;
											}									
											if ( tmp_input->evaluate() ) {
												std::string insert="";
												if (tmp_input->results.result() != NULL) {
													insert = *tmp_input->results.result();
												}
												if (key->results.result() == NULL) {
													matched = true;
													sdom = insert;
												} else {
													matched = String::fandr(sdom,skey,insert,tmp_input->k_scope);
												}
												the_domain->clear();											
												the_domain = DataItem::factory(sdom,dom_kind);
											} else { 
												t_deferred=true; 
											}
										}
									} break;
								}
							} break;
							case 'r': {
								if ( String::Regex::available() ) {
									if (key->results.result() != NULL) { //no worries about this one.
										skey = *key->results.result();
									}
									sdom = *the_domain;
									switch (operation) {
										case m_switch: { // m_switch uses full-matches without changing the domain (repeat illegal).
											if ( String::Regex::fullmatch(skey, sdom) ) {
												tmp_input = match;
												if ( tmp_input->evaluate() ) {
													matched = true;
													DataItem* tmp = NULL;
													tmp_input->results.takeresult(tmp);
													results.setresult(tmp,match->wsstrip);
												} else { 
													t_deferred=true; 
												}
											}
										} break;
										case m_state: { // m_switch uses full-matches and changes the domain.
											if ( String::Regex::fullmatch(skey, sdom) ) {
												if (repeated) {
													tmp_input = new DefInpType(this,match);
												} else {
													tmp_input = match;
												}									
												if ( tmp_input->evaluate() ) {
													matched = true;
													tmp_input->results.takeresult(the_domain);
												} else { 
													t_deferred=true; 
												}
											}
										} break;
										case m_substitute: { // m_substitute uses partial-matching and changes the domain.
											sdom = *the_domain;
											if ( String::Regex::match( skey, sdom ) ) {
												if (repeated) {
													tmp_input = new DefInpType(this,match);
												} else {
													tmp_input = match;
												}											
												if ( tmp_input->evaluate() ) {
													matched = true;
													string matchresult="";
													if (tmp_input->results.result() != NULL) {
														matchresult = *(tmp_input->results.result());
													}
													String::Regex::replace( skey,matchresult, sdom, tmp_input->k_scope);
													the_domain->clear();											
													the_domain = DataItem::factory(sdom,dom_kind);
												} else { 
													t_deferred=true; 
												}
											}
										} break;
									}
								} else {
									*Logger::log << Log::error << Log::LI << "Error. Key with regex format depend on a regex (pcre) library loaded, and there isn't one." << Log::LO;
									trace();
									*Logger::log << Log::blockend;
								}
							} break;	
							default: {
								*Logger::log << Log::error << Log::LI << "Error. Key format must be either regex or literal." << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						} //end switch
						if (repeated && tmp_input != NULL) {
							delete tmp_input; 
							tmp_input =NULL;
						}						
					} else { //the format doesn't really matter for NULL matching.
						if ( key->results.result() == NULL ) {
							if ( match->evaluate() ) {
								matched = true;
								match->results.takeresult(the_domain);
							} else { t_deferred=true; }
						}
					}
					if ( matched && match->k_break ) break; 
				} //end if null...
			} //end for
		} while ( !t_deferred && repeated && matched && forced_break > 0 );
		if (!matched && !t_deferred && def_match != NULL) {
			if ( def_match->evaluate() ) {
				matched = true;
				if (operation == m_switch) {
					DataItem* tmp = NULL;
					def_match->results.takeresult(tmp);
					results.setresult(tmp,def_match->wsstrip);
				} else {
					def_match->results.takeresult(the_domain);
				}
			} else { 
				t_deferred=true; 
			}					
		}
		if ( t_deferred ) {
			definputs[0]->results.setresult(the_domain);
		} else {
			mat_evaluated = true;
			for ( unsigned int k = 0; k < definputs.size(); k++ ) { 
				delete definputs[k]; 
			}
			definputs.clear();
			if (operation == m_switch) { 
				delete the_domain;
				the_domain = NULL;
			} else {
				results.setresult(the_domain);
			}
		}
	}
	return mat_evaluated;
}
void Mapping::addInputType(InputType*) {
	*Logger::log << Log::error << Log::LI << "Error. Mapping accepts one domain and any number of match." << Log::LO; 
	trace();
	*Logger::log << Log::blockend;
}
void Mapping::addDefInpType(DefInpType* i) {
	if (!definputs.empty()) {
		if (i->wotzit == match) { 
			definputs.push_back(i);
		} else {
			*Logger::log << Log::error << Log::LI << "The subsequent itypes for mapping must be match" << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	} else {
		if (i->wotzit == domain) { 
			definputs.push_back(i);
		} else {
			*Logger::log << Log::error << Log::LI << "The first itype for mapping must be domain" << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
}
bool Mapping::field(const string& field_name,string& container) const {
	pair<long long, bool> i_res = String::integer(field_name);
	bool retval=i_res.second;
	if (retval) {
		retval = String::Regex::field(skey,sdom,(int)i_res.first,container);
	}
	return retval;
}
void Mapping::init() {
	//static methods - once only (either per main doc, or per process) thank-you very much..
}
void Mapping::finalise() {
}
void Mapping::startup() {
	map_types.insert(map_type_map::value_type(UCS2(L"switch"), m_switch));
	map_types.insert(map_type_map::value_type(UCS2(L"substitute"), m_substitute));
	map_types.insert(map_type_map::value_type(UCS2(L"state"), m_state));
}
void Mapping::shutdown() {
	map_types.clear();
}	

