/* 
 * iteration.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * iteration.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <utility>

#include "commons/vdb/vdb.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"
#include "commons/string/strings.h"

#include "pairqueue.h"
#include "obyxelement.h"
#include "iteration.h"
#include "inputtype.h"
#include "function.h"
#include "document.h"
#include "itemstore.h"

using namespace Log;
using namespace XML;
using namespace Vdb;

#define DEFAULT_MAX_ITERATIONS 100000

it_type_map Iteration::it_types;

Iteration::Iteration(xercesc::DOMNode* const& n,ObyxElement* par) : 
Function(n,iteration,par),ctlevaluated(false),evaluated(false),query(nullptr),
operation(it_repeat),lastrow(false),expanded(false),currentrow(1),numreps(1),currentkey("") {
	u_str op_string;
	Manager::attribute(n,u"operation",op_string);
	it_type_map::const_iterator j = it_types.find(op_string);
	if( j != it_types.end() ) {
		operation = j->second; 
	} else {
		if ( ! op_string.empty() ) {
			string err_type,name; Manager::transcode(op_string.c_str(),err_type);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_type << " is not a legal iteration operation. It should be one of ";
			it_type_map::iterator op_ti = it_types.begin();
			while ( op_ti != it_types.end() ) {
				XML::Manager::transcode(op_ti->first,name);
				op_ti++;
				if (op_ti == it_types.end()) {
					*Logger::log << name << ".";
				} else {
					*Logger::log << name << ", ";
				}
			}
			*Logger::log << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
}

//deal with anything specific to this function
Iteration::Iteration(ObyxElement* par,const Iteration* orig) : Function(par,orig),
ctlevaluated(orig->ctlevaluated),evaluated(orig->evaluated),query(orig->query),
operation(orig->operation),lastrow(orig->lastrow),expanded(orig->expanded),
currentrow(orig->currentrow),numreps(orig->numreps),currentkey(orig->currentkey) {
}
bool Iteration::evaluate_this() { //This can be run as an evaluated iteration within the current iteration.
	// evaluated returns true only if the control is successful, the body is expanded, and the expansion is successful.
	//IF this isn't finished and there is a control (then nothing else has happened so far)
	//first evaluate the control (not run it, just evaluate it!)
	if (!evaluated && ! ctlevaluated) { //legal inputs are control
		ctlevaluated = true;
		if ( ! inputs.empty() && (operation==it_sql || operation==it_repeat || operation == it_search)) {
			inputs[0]->evaluate();
		}
		if ( definputs.size() == 0 ) { //This is just a control string, but we must run it if it is a query.
			if ( operation==it_sql ) {
				evaluated = operation_sql();
			} else {				
				evaluated = true; 
			}
			expanded=true;
		}
	}
	///now deal with the body. the actual body exist to the end of the expansion - see output type=error.
	if ( ctlevaluated && !evaluated && !expanded ) {  //1 body.
		switch (operation) { 
			case it_each: { evaluated = operation_each(); } break; 
			case it_repeat: { evaluated = operation_repeat(); } break; 
			case it_search: { evaluated = operation_search(); } break;
			case it_sql: { evaluated = operation_sql(); } break;
			case it_while: { evaluated = operation_while(true); } break; 
			case it_while_not: { evaluated = operation_while(false); } break; 
		}
		expanded = true;
	} 
	// But we need to do it as a part of the evaluation. See test t00093.obyx
	if (ctlevaluated && !evaluated && expanded ) { //this is the body 
		size_t n = definputs.size();
		bool definputs_evaluated = true;
		for ( size_t i = 0; i < n; i++ ) {
			definputs[i]->evaluate();
		}
		evaluated = definputs_evaluated;
		if (!evaluated) { //maybe we have to wait
			if (p->wotzit == xmldocument) {
				for ( size_t i = 0; i < n; i++ ) {
					DefInpType*& x = definputs[i];
					*Logger::log << Log::error << Log::LI << "Error. SQL Evaluation failed." << Log::LO; 
					trace();
					if (x != nullptr) {
						*Logger::log << Log::error << Log::LI;
						x->explain();
						*Logger::log << Log::LO;
					}
					*Logger::log << Log::blockend;
				}
			}
//deleted by ~Function			
//			for ( size_t i = 0; i < n; i++ ) {
//				delete definputs[i];
//			}
			evaluated = true;
		}
//		definputs.clear();
	}
	if ( ctlevaluated && evaluated  && expanded ) {
		size_t n = definputs.size();
		for ( size_t i = 0; i < n; i++ ) {
			results.append(definputs[i]->results,this);
		}
	}
	return evaluated;
}
bool Iteration::fieldfind(const u_str& pattern) const { //regex..
	bool retval = false;
	if ( String::Regex::available() ) {
		if (query != nullptr) {
			string rexpr; XML::Manager::transcode(pattern,rexpr);		
			retval = query->findfield(rexpr);
		}
	} else {
		string dummy;
		retval = fieldexists(pattern,dummy);
	}
	return retval;
}
void Iteration::fieldkeys(const u_str& pattern,set<string>& keylist) const { //regex..
	if (query != nullptr) {
		string rexpr; XML::Manager::transcode(pattern,rexpr);
		query->fieldkeys(rexpr,keylist);
	}
}
bool Iteration::fieldexists(const u_str& fname,string& errstring) const {
	bool retval = false,rcfound = false,rfound=false,fcfound=false,kyfound=false;
	size_t hashpt = fname.find('#');
	if (hashpt != string::npos) {
		if (hashpt > 0 ) { hashpt--; }
		rcfound = fname.find(u"#rowcount",hashpt) != string::npos;
		rfound = fname.find(u"#row",hashpt) != string::npos;
		fcfound = fname.find(u"#fieldcount",hashpt) != string::npos;
		kyfound = fname.find(u"#key",hashpt) != string::npos;
	}
	switch (operation) {
		case it_search:
		case it_sql: {
			if (rcfound || rfound || fcfound) {
				retval = true;
			} else {
				if (!kyfound && query != nullptr) {
					string qkey; XML::Manager::transcode(fname,qkey);
					retval = query->hasfield(qkey);
				} else {
					errstring = "The field was not found.";
				}
			}
		} break;
		case it_each: {
			if (rcfound || rfound || kyfound ) {
				retval = true;
			}
		} break;
		case it_repeat: {
			if (rcfound || rfound) {
				retval = true;
			}
		} break;
		case it_while_not:
		case it_while: {
			if (rcfound || fcfound) {
				errstring = "Field #rowcount and #fieldcount not allowed in while and while_not. Only the field #row is valid";
			} else {
				retval = true;
			}
		} break;
	}
	return retval;
}
bool Iteration::while_repeat_metafield(u_str& metafield) const {
	bool retval = false;
	size_t hashpos = metafield.find('#');
	while (hashpos != string::npos) {
		if (metafield.compare(hashpos,9,u"#rowcount") == 0) {
			if ( operation == it_repeat || operation == it_each ) {
				u_str rowstr;
//				XML::Manager::to_ustr(numreps,rowstr);
				metafield.replace(hashpos,9,rowstr);
				hashpos += rowstr.size()-1 ;
				retval = true;
			} 
		} else {
			if ( metafield.compare(hashpos,4,u"#row") == 0 ) {
				u_str rowstr;
//				XML::Manager::to_ustr(currentrow,rowstr);
				metafield.replace(hashpos,4,rowstr);
				hashpos += rowstr.size()-1 ;
				retval = true;
			}
			else {
				if ( operation==it_each && metafield.compare(hashpos,4,u"#key") == 0 ) {
					u_str ckey; XML::Manager::transcode(currentkey,ckey);
					metafield.replace(hashpos,4,ckey);
					hashpos += ckey.size()-1; //#key
					retval = true;
				}
			}
		}
		hashpos = metafield.find('#',++hashpos);
	}
	return retval;
}
bool Iteration::field(const u_str& fname,DataItem*& container,const kind_type& kind,string& errstring) const { 
	bool retval = false;
	if (query != nullptr && (operation == it_sql || operation == it_search)) {
		//This is where we could do an xpath split.
		string qkey,srepo; XML::Manager::transcode(fname,qkey);
		retval = query->readfield(currentrow,qkey,srepo,errstring);
		if (retval) {
			container = DataItem::factory(srepo,kind);
		}
	} else {
		u_str metafield = fname;
		retval = while_repeat_metafield(metafield);
		if (!retval) {
//			string erv; XML::Manager::transcode(fname,erv);
//			errstring = "Error. Field " + erv + " not allowed here. In while and repeat only the fields #rowcount, #row and #key are valid";
		}  else {
			container = DataItem::factory(metafield,kind);
		}
	}
	return retval;
}
void Iteration::list(const ObyxElement* base) { //static.
	*Logger::log << Log::subhead << Log::LI << "Current SQL Queries" << Log::LO;
	for (ObyxElement* curr = base->p; curr != nullptr; curr = curr->p) {
		const Iteration* i = dynamic_cast<const Iteration *>(curr);
		if ( (i != nullptr) &&  (i->query != nullptr) && (i->operation == it_sql || i->operation == it_search) ) {
			const std::vector<std::string>& fl = i->query->fieldlist();
			if (!fl.empty()) {
				string qery = i->query->getquery();
				string note = i->note();
				*Logger::log << Log::LI << Log::II << qery << Log::IO;
				*Logger::log << Log::even;
				for (vector<string>::const_iterator it = fl.begin(); it != fl.end(); it++) {
					string container,errstring,fname;
					fname = *it;
					i->query->readfield(i->currentrow,fname,container,errstring);
					*Logger::log << Log::LI << Log::II << fname << Log::IO << Log::II << container << Log::IO << Log::LO;
				}
				*Logger::log << Log::blockend << Log::LO;   //even
			}
		}
	}
	*Logger::log << Log::blockend; //subhead
}
bool Iteration::operation_each() {
	std::set<std::string,less<std::string> > spacekeys;
	if ( ! inputs.empty() ) { 
		inputs[0]->evalfind(spacekeys);			//now we have the search string	
		if (definputs.size() > 0) {				//body.
			DefInpType* base_template = definputs[0]; definputs.clear(); //We are about to re-construct the list. 
			numreps = spacekeys.size();
			unsigned long long forced_break = DEFAULT_MAX_ITERATIONS;
			if(!owner->metastore("ITERATION_BREAK_COUNT",forced_break)) {
				*Logger::log << Log::error << Log::LI << "Error. each with bad ITERATION_BREAK_COUNT" << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
			if ( forced_break < numreps) { numreps = forced_break;}
			if ( numreps == 0 ) {
				delete base_template; //just let it be deleted.
			} else {
				DefInpType* iter_input = nullptr;
				set<string>::const_iterator ski = spacekeys.begin();
				for (currentrow = 1; currentrow <= numreps; currentrow++) {
					currentkey = *(ski++);
					if (currentrow != numreps) {
						iter_input = new DefInpType(this,base_template);
					} else {
						iter_input = base_template;		
						lastrow = true;
					}
					iter_input->evaluate();
					definputs.push_back(iter_input);
					iter_input = nullptr;
				}
			}
		}
	} 
	lastrow = true;
	return true;
}
bool Iteration::operation_repeat() {
	bool fully_evaluated = true;
	string tmp_var;
	string controlstring;
	if ( ! inputs.empty() ) { 
		const DataItem* res = inputs[0]->results.result();
		if (res != nullptr) {
			controlstring = *res;
		}
	}
	numreps = 1;
	unsigned long long forced_break = DEFAULT_MAX_ITERATIONS;
	if(!owner->metastore("ITERATION_BREAK_COUNT",forced_break)) {
		*Logger::log << Log::error << Log::LI << "Error. repeat with bad ITERATION_BREAK_COUNT" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
	pair<unsigned long long,bool> repeat_count = String::znatural(controlstring);
	if  ( repeat_count.second ) {
		numreps = repeat_count.first;
	} else {
		if ( ! controlstring.empty() ) {
			*Logger::log << Log::error << Log::LI << "Error. iteration operation='repeat' expected a natural number, but found [" << controlstring << "]. If this is sql, you may want to add operation='sql' to the iteration." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
	if (forced_break < numreps) { 
		numreps = forced_break;
	}
	if (definputs.size() > 0) {
		DefInpType* base_template = definputs[0];
		definputs.clear();		 //we are going to recomposit this.
		if ( numreps == 0 ) {
			delete base_template; //just let it be deleted.
		} else {
			DefInpType* iter_input = nullptr;
			for (currentrow = 1; currentrow <= numreps; currentrow++) {
				if (currentrow != numreps) {
					iter_input = new DefInpType(this,base_template);
				} else {
					iter_input = base_template;		
					lastrow = true;
				}
				iter_input->evaluate();
				definputs.push_back(iter_input);
				iter_input = nullptr;
			}
		}
	}
	lastrow = true;
	return fully_evaluated;
}
bool Iteration::operation_search() {
	bool searchdone = true;
	string tmp_var,controlstring;
	if ( ! inputs.empty() && inputs[0]->results.result() != nullptr ) {
		controlstring = *(inputs[0]->results.result());
	}
	if ( ! controlstring.empty() ) {
		if (scc != nullptr)  {
			query = nullptr;	 //reset the reference..  (used for iko type="field") search
			if (scc->query(query,controlstring) ) {
				if ( query->execute() ) {
					if (definputs.size() > 0) {
						DefInpType* base_template = definputs[0];
						definputs.clear();		//because we are recompositing this list.
						long long queryrows = query->getnumrows();
						numreps = queryrows < 1 ? 0 :  queryrows;
						unsigned long long forced_break = DEFAULT_MAX_ITERATIONS;
						if(!owner->metastore("ITERATION_BREAK_COUNT",forced_break)) {
							*Logger::log << Log::error << Log::LI << "Error. sql with bad ITERATION_BREAK_COUNT" << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
						if (numreps > forced_break) numreps = forced_break;
						if ( numreps == 0 ) {
							delete base_template;
						} else {
							for (currentrow = 1; currentrow <= numreps; currentrow++) {
								DefInpType* iter_input = nullptr;
								if (currentrow != numreps) {	 //now can delete original
									iter_input = new DefInpType(this,base_template);
								} else {
									iter_input = base_template;
									lastrow = true;
								}
								iter_input->evaluate();
								definputs.push_back(iter_input);
							}
						}
					}
				} else {
					*Logger::log << Log::error;
					trace();
					*Logger::log << Log::blockend;
					results.clear();
					searchdone = false; //Just give up.
				}
				delete query; query = nullptr;	 //reset the reference..  (used for iko type="field")
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Iteration operation search service was found, but the connection failed to provide a query object." << Log::LO;
				trace();
				scc->list();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. The iteration search operation must have a search service available." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
			results.clear();
			searchdone = false;
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. The value of a search control must contain a search statement" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
		results.clear();
		searchdone = false;
	}
	return searchdone;
}
bool Iteration::operation_sql() {
	bool sqldone = true;
	string tmp_var,controlstring;
	if ( ! inputs.empty() && inputs[0]->results.result() != nullptr ) { 
		controlstring = *(inputs[0]->results.result());
	}
	if ( ! controlstring.empty() ) {
		if (dbs != nullptr)  {
			query = nullptr;	 //reset the reference..  (used for iko type="field")
			if ( dbc != nullptr && dbc->query(query,controlstring) ) {
				if ( query->execute() ) {
					if (definputs.size() > 0) {
						DefInpType* base_template = definputs[0];
						definputs.clear();		//because we are recompositing this list.
						long long queryrows = query->getnumrows();
						numreps = queryrows < 1 ? 0 :  queryrows;
						unsigned long long forced_break = DEFAULT_MAX_ITERATIONS;
						if(!owner->metastore("ITERATION_BREAK_COUNT",forced_break)) {
							*Logger::log << Log::error << Log::LI << "Error. sql with bad ITERATION_BREAK_COUNT" << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
						if (numreps > forced_break) numreps = forced_break;
						if ( numreps == 0 ) {
							delete base_template;
						} else {
							for (currentrow = 1; currentrow <= numreps; currentrow++) {
								DefInpType* iter_input = nullptr;
								if (currentrow != numreps) {	 //now can delete original
									iter_input = new DefInpType(this,base_template);
								} else {
									iter_input = base_template;
									lastrow = true;
								}
								iter_input->evaluate();
								definputs.push_back(iter_input);
							}
						}
					}
				} else {
					results.clear();
					sqldone = false; //Just give up.
				}
				delete query; query = nullptr;	 //reset the reference..  (used for iko type="field")
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Iteration operation sql needs a database selected. An sql service was found, but the sql connection failed." << Log::LO;
				trace();
				dbc->list();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. The iteration sql operation must have an sql service available." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
			results.clear();
			sqldone = false;
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. The value of an SQL control must contain an SQL statement" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
		results.clear();
		sqldone = false;
	}
	return sqldone;
}
bool Iteration::operation_while(bool existence) {
	bool inputsfinal = true;
	numreps = 0;
	string tmp_var;
	unsigned long long forced_break = DEFAULT_MAX_ITERATIONS;
	if(!owner->metastore("ITERATION_BREAK_COUNT",forced_break)) {
		*Logger::log << Log::error << Log::LI << "Error. while with bad ITERATION_BREAK_COUNT" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
	if (definputs.size() > 0) {
		DefInpType* base_template = definputs[0];
		definputs.clear();
		DefInpType* iter_input = nullptr;
		bool loopdone=false;
		if (inputs.size() == 1) {
			for (currentrow = 1; currentrow < forced_break && !loopdone ; currentrow++ ) {
				InputType* it_input = new InputType(this,inputs[0]);
				it_input->evaluate();
				if ( existence != it_input->getexists() ) {  //eg for while (true) then when !true we break.
					inputsfinal=true;
					loopdone = true;
				} else {
					numreps = currentrow;
					iter_input = new DefInpType(this,base_template);
					iter_input->evaluate();
					definputs.push_back(iter_input);
					iter_input = nullptr;
				}
				delete it_input;
			}
		} else {
			inputsfinal=true;
			*Logger::log << Log::error << Log::LI << "Error. Iteration operations 'while' and 'while_not' must have a control." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
		if ( currentrow == forced_break ) {
			*Logger::log << Log::error << Log::LI << "Error. The while iteration issued a force break due to " << (unsigned int)forced_break << " iterations.";
			*Logger::log << "If you need a larger number of iterations, set the store variable ITERATION_BREAK_COUNT with a higher maximum." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
		delete base_template; //never know which is the last row - so better always use unique..
	}
	lastrow = true;
	return inputsfinal;
}
Iteration::~Iteration() { 
	//outputs/inputs are deleted by Function.
}
void Iteration::addInputType(InputType* i) {
	if (inputs.size() > 0) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. A maximum of one control is legal for iteration." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	} else {
		if (i->wotzit == control) {
			inputs.push_back(i);
		} else {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Only control and body are legal for iteration." << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
}
void Iteration::addDefInpType(DefInpType* i) {
	if (definputs.size() > 0) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. A maximum of one body is legal for iteration." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	} else {
		if (i->wotzit == body) {
			definputs.push_back(i);
		} else {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Only control and body are legal for iteration." << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
}
void Iteration::init() {
	//static methods - once only (either per main doc, or per process) thank-you very much..
}
void Iteration::finalise() {
}
void Iteration::startup() {
	it_types.insert(it_type_map::value_type(u"each",it_each));
	it_types.insert(it_type_map::value_type(u"repeat",it_repeat));
	it_types.insert(it_type_map::value_type(u"search",it_search));
	it_types.insert(it_type_map::value_type(u"sql",it_sql));
	it_types.insert(it_type_map::value_type(u"while",it_while));
	it_types.insert(it_type_map::value_type(u"while_not",it_while_not));
}
void Iteration::shutdown() {
	it_types.clear();
}	
