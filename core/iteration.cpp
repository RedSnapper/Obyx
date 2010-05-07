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

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
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

it_type_map Iteration::it_types;

Iteration::Iteration(xercesc::DOMNode* const& n,ObyxElement* par) : 
Function(n,iteration,par),ctlevaluated(false),evaluated(false),query(NULL),
operation(it_repeat),lastrow(false),expanded(false),currentrow(1),numreps(1) {
	u_str op_string;
	Manager::attribute(n,UCS2(L"operation"),op_string);
	it_type_map::const_iterator j = it_types.find(op_string);
	if( j != it_types.end() ) {
		operation = j->second; 
	} else {
		if ( ! op_string.empty() ) {
			string err_op; transcode(op_string.c_str(),err_op);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_op << " is not a legal iteration operation. It should be one of sql, repeat, while." << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
}

//deal with anything specific to this function
Iteration::Iteration(ObyxElement* par,const Iteration* orig) : Function(par,orig),
ctlevaluated(orig->ctlevaluated),evaluated(orig->evaluated),query(orig->query),
operation(orig->operation),lastrow(orig->lastrow),expanded(orig->expanded),
currentrow(orig->currentrow),numreps(orig->numreps) {
}

// evaluated returns true only if the control is successful, the body is expanded, and the expansion is successful.
bool Iteration::evaluate_this() { //This can be run as an evaluated iteration within the current iteration.
	//IF this isn't finished and there is a control (then nothing else has happened so far)
	//first evaluate the control (not run it, just evaluate it!)
	if (!evaluated && ! ctlevaluated) { //legal inputs are control
		ctlevaluated = true;
		if ( ! inputs.empty() && (operation==it_sql || operation==it_repeat)) {
			ctlevaluated = inputs[0]->evaluate();
		}
		if ( ! ctlevaluated ) {
			return false;
		} else {
			if ( definputs.size() == 0 ) { //This is just a control string, but we must run it if it is a query.
				if ( operation==it_sql ) {
					evaluated = operation_sql();
				} else {				
					evaluated = true; 
				}
				expanded=true;
			}
		}
	}
	///now deal with the body. the actual body exist to the end of the expansion - see output type=error.
	if ( ctlevaluated && !evaluated && !expanded ) {  //1 body.
		switch (operation) { 
			case it_sql: { evaluated = operation_sql(); } break;
			case it_repeat: { evaluated = operation_repeat(); } break; 
			case it_while: { evaluated = operation_while(true); } break; 
			case it_while_not: { evaluated = operation_while(false); } break; 
		}
		expanded = true;
	} 
	// But we need to do it as a part of the evaluation. See test t00093.qxml
	if (ctlevaluated && !evaluated && expanded ) { //this is the body 
		size_t n = definputs.size();
		bool definputs_evaluated = true;
		for ( size_t i = 0; i < n; i++ ) {
			bool exp_evaluated = definputs[i]->evaluate();
			definputs_evaluated = definputs_evaluated && exp_evaluated;
		}
		evaluated = definputs_evaluated;
		if (!evaluated) { //maybe we have to wait
			if (p->wotzit == xmldocument) {
				for ( size_t i = 0; i < n; i++ ) {
					DefInpType*& x = definputs[i];
					*Logger::log << Log::error << Log::LI << "Error. SQL Evaluation failed." << Log::LO; 
					trace();
					if (x != NULL) {
						*Logger::log << Log::error << Log::LI;
						x->explain();
						*Logger::log << Log::LO;
					}
					*Logger::log << Log::blockend;
				}
			}
			for ( size_t i = 0; i < n; i++ ) {
				delete definputs[i];
			}
			evaluated = true;
		}
		definputs.clear();
	}
	if ( ctlevaluated && evaluated  && expanded ) {
		size_t n = definputs.size();
		for ( size_t i = 0; i < n; i++ ) {
			DataItem* di = NULL; 
			definputs[i]->results.takeresult(di);
			results.append(di);
			delete definputs[i];
			definputs[i] = NULL;
		}
		definputs.clear();
	}
	return evaluated;
}
bool Iteration::fieldexists(const string& fname,string& errstring) const {
	bool retval = false,rcfound = false,rfound=false,fcfound=false;
	bool hashfound = fname.find('#') != string::npos;
	if (hashfound) {
		rcfound = fname.find("#rowcount") != string::npos;
		rfound = fname.find("#row") != string::npos;
		fcfound = fname.find("#fieldcount") != string::npos;
	}
	switch (operation) {
		case it_sql: {
			if (rcfound || rfound || fcfound) {
				retval = true;
			} else {
				if (query != NULL) {
					retval = query->hasfield(fname);
				} else {
					errstring = "The field was not found.";
				}
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
bool Iteration::field(const string& fname,string& container,string& errstring) const {
	bool retval = false;
	if (query != NULL && operation == it_sql) {
		retval = query->readfield(currentrow,fname,container,errstring);
	} else {
		string tmpval;
		container = fname;
		size_t hashpos = container.find('#');
		while (hashpos != string::npos) {
			if (container.compare(hashpos,9,"#rowcount") == 0) {
				if ( operation == it_repeat) {
					String::tostring(tmpval,numreps);
					container.replace(hashpos,9,tmpval);
					hashpos--;
					retval = true; 
				} 
			} else {
				if ( container.compare(hashpos,4,"#row") == 0 ) {
					String::tostring(tmpval,currentrow);
					container.replace(hashpos,4,tmpval);
					hashpos--;
					retval = true;
				}
			}
			hashpos = container.find('#',++hashpos);
		}
		if (!retval) {
			container = "";
			errstring = "Error. Field " + fname + " not allowed here. In while and repeat only the fields #rowcount and #row are valid";
		}
	}
	return retval;
}
void Iteration::list(const ObyxElement* base) { //static.
	*Logger::log << Log::subhead << Log::LI << "Current SQL Queries" << Log::LO;
	for (ObyxElement* curr = base->p; curr != NULL; curr = curr->p) {
		const Iteration* i = dynamic_cast<const Iteration *>(curr);
		if ( (i != NULL) &&  (i->query != NULL) && (i->operation == it_sql) ) {
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
bool Iteration::operation_sql() {
	bool inputsfinal = true;
	string tmp_var,controlstring;
	if ( ! inputs.empty() && inputs[0]->results.result() != NULL ) { 
		controlstring = *(inputs[0]->results.result());
	}
	if ( ! controlstring.empty() ) {
		if (dbs != NULL)  {
			query = NULL;	 //reset the reference..  (used for iko type="field")
			if ( dbc->query(query,controlstring) ) {
				if ( query->execute() ) {
					if (definputs.size() > 0) {
						DefInpType* base_template = definputs[0];
						definputs.clear();
						unsigned long long forced_break = INT_MAX;
						long long queryrows = query->getnumrows();
						numreps = queryrows < 1 ? 0 :  queryrows;
						if ( ItemStore::get("ITERATION_BREAK_COUNT",tmp_var) ) {
							pair<unsigned long long,bool> forced_break_setting = String::znatural(tmp_var);
							if ( forced_break_setting.second ) { 
								forced_break = forced_break_setting.first;
							} else {
								*Logger::log << Log::error << Log::LI << "Error. ITERATION_BREAK_COUNT expected a natural number, but found [" << tmp_var << "]." << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}						
						if (numreps > forced_break) numreps = forced_break;
						if ( numreps == 0 ) {
							delete base_template; 
						} else {
							for (currentrow = 1; currentrow <= numreps; currentrow++) {
								DefInpType* iter_input = NULL;	
								if (currentrow != numreps) {	 //now can delete original 
									iter_input = new DefInpType(this,base_template);
								} else {
									iter_input = base_template;	
									lastrow = true;
								}
								bool row_result = iter_input->evaluate();
								inputsfinal = inputsfinal && row_result; //stick this on the same line as above and the optimizer won't run evaluate!!
								definputs.push_back(iter_input);
							}					
						}
					}
				} else {
					*Logger::log << Log::error;
					trace();
					*Logger::log << Log::blockend;
					inputsfinal = false; //Just give up.
				}
				delete query; query = NULL;	 //reset the reference..  (used for iko type="field")
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Iteration operation sql needs a database selected. An sql service was found, but the sql connection failed." << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. The iteration sql operation must have an sql service available." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. The value of an SQL control must contain an SQL statement" << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
	return inputsfinal;
}
bool Iteration::operation_repeat() {
	bool fully_evaluated = true;
	string tmp_var;
	string controlstring;
	unsigned long long forced_break = INT_MAX;
	if ( ! inputs.empty() ) { 
		const DataItem* res = inputs[0]->results.result();
		if (res != NULL) {
			controlstring = *res;
		}
	}
	numreps = 1;
	if ( ItemStore::get("ITERATION_BREAK_COUNT",tmp_var) ) {
		pair<unsigned long long,bool> forced_break_setting = String::znatural(tmp_var);
		if ( forced_break_setting.second ) { 
			forced_break = forced_break_setting.first;
		} else {
			*Logger::log << Log::error << Log::LI << "Error. ITERATION_BREAK_COUNT expected a natural number, but found [" << tmp_var << "]." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
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
		definputs.clear();
		if ( numreps == 0 ) {
			delete base_template; //just let it be deleted.
		} else {
			DefInpType* iter_input = NULL;
			for (currentrow = 1; currentrow <= numreps; currentrow++) {
				if (currentrow != numreps) {
					iter_input = new DefInpType(this,base_template);
				} else {
					iter_input = base_template;		
					lastrow = true;
				}
				bool inp_evaluated = iter_input->evaluate();
				fully_evaluated = fully_evaluated && inp_evaluated;
				definputs.push_back(iter_input);
				iter_input = NULL;
			}
		}
	}
	lastrow = true;
	return fully_evaluated;
}
bool Iteration::operation_while(bool existence) {
	bool inputsfinal = true;
	unsigned long long forced_break = 100;
	numreps = 0;
	string tmp_var;
	if ( ItemStore::get("ITERATION_BREAK_COUNT",tmp_var) ) {
		pair<unsigned long long,bool> forced_break_setting = String::znatural(tmp_var);
		if ( forced_break_setting.second ) { 
			forced_break = forced_break_setting.first;
		} else {
			*Logger::log << Log::error << Log::LI << "Error. ITERATION_BREAK_COUNT expected a natural number, but found [" << tmp_var << "]." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
	if (definputs.size() > 0) {
		DefInpType* base_template = definputs[0];
		definputs.clear();
		DefInpType* iter_input = NULL;
		bool loopdone=false;
		if (inputs.size() == 1) {
			for (currentrow = 1; currentrow < forced_break && !loopdone ; currentrow++ ) {
				InputType* it_input = new InputType(this,inputs[0]);
				if ( it_input->evaluate() ) {
					if ( existence != it_input->getexists() ) {  //eg for while (true) then when !true we break.
						inputsfinal=true;
						loopdone = true;
					} else {
						numreps = currentrow;
						iter_input = new DefInpType(this,base_template);
						inputsfinal = inputsfinal && iter_input->evaluate();
						definputs.push_back(iter_input);
						iter_input = NULL;
					}
				} else {
					inputsfinal=false;
					loopdone = true;
				}
				delete it_input;
			}
		} else {
			inputsfinal=true; loopdone = true;
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

//static methods - once only (either per main doc, or per process) thank-you very much..
void Iteration::init() {
}

void Iteration::finalise() {
}

void Iteration::startup() {
	it_types.insert(it_type_map::value_type(UCS2(L"sql"),it_sql));
	it_types.insert(it_type_map::value_type(UCS2(L"repeat"),it_repeat));
	it_types.insert(it_type_map::value_type(UCS2(L"while"),it_while));
	it_types.insert(it_type_map::value_type(UCS2(L"while_not"),it_while_not));
}

void Iteration::shutdown() {
	it_types.clear();
}	
