/* 
 * comparison.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * comparison.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <ctype.h>
//#include <math.h>

#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/string/strings.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"

#include "document.h"
#include "dataitem.h"
#include "pairqueue.h"
#include "iteration.h"
#include "obyxelement.h"
#include "function.h"
#include "inputtype.h"
#include "output.h"
#include "comparison.h"

using namespace Log;
using namespace XML;
using namespace obyx;
using namespace std;

cmp_type_map Comparison::cmp_types;

Comparison::Comparison(ObyxElement* par,const Comparison* orig) : Function(par,orig),
operation(orig->operation),cbreak(orig->cbreak),invert(orig->invert),logic(orig->logic),
eval_found(orig->eval_found),cmp_evaluated(orig->cmp_evaluated),
def_evaluated(orig->def_evaluated),operation_result(orig->operation_result) {}

Comparison::Comparison(xercesc::DOMNode* const& n,ObyxElement* par) :
Function(n,comparison,par),operation(equivalent_to),cbreak(true),invert(false),logic(obyx::all),eval_found(false),
cmp_evaluated(false),def_evaluated(false),operation_result('X') {
	u_str op_string,invert_str,break_str,logic_str; 
	if ( Manager::attribute(n,u"operation",op_string) ) {
		cmp_type_map::const_iterator i = cmp_types.find(op_string);
		if( i != cmp_types.end() ) {
			operation = i->second.first; 
			invert = i->second.second;
		} else {
			if ( ! op_string.empty() ) {
				string err_type,name; Manager::transcode(op_string.c_str(),err_type);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_type << " is not a legal comparison operation. It should be one of ";
				cmp_type_map::iterator op_ti = cmp_types.begin();
				while ( op_ti != cmp_types.end() ) {
					XML::Manager::transcode(op_ti->first,name);
					op_ti++;
					if (op_ti == cmp_types.end()) {
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
	if ( Manager::attribute(n,u"invert",invert_str) ) {
		if (invert_str.compare(u"true") == 0) {
			invert = !invert;
		} else {
			if (invert_str.compare(u"false") != 0) {
				string err_msg; Manager::transcode(invert_str.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal invert. It should be one of: true,false" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
	if (owner->version() < 1.120000) { // break used to default to false. but it now defaults to true.
		cbreak	= false;
	} else {
		cbreak	= true;
	}	
	if ( Manager::attribute(n,u"break",break_str) ) {
		if (break_str.compare(u"true") == 0) {
			cbreak = true;
		} else {
			if (break_str.compare(u"false") != 0) {
				string err_msg; Manager::transcode(break_str.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal break. It should be one of: true,false" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			} else {
				cbreak = false;
			}
		}
	}
	bool logic_found = false;
	if (owner->version() < 1.110208) { // 'scope' changed to 'logic'.
		logic_found		= Manager::attribute(n,u"scope",logic_str);
	} else {
		logic_found		= Manager::attribute(n,u"logic",logic_str);
		if (Manager::attribute(n,u"scope")) {
			Environment* env = Environment::service();
			if (env->envexists("OBYX_ALLOW_LEGACY_COMPARISON_SCOPE")) {
				logic_found		= Manager::attribute(n,u"scope",logic_str);
			} else {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. 'scope' is not legal for the version (" << owner->version_str() <<  ") of this document.  For versions 1.110208 or above use 'logic'." << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
	if ( logic_found ) {
		if (logic_str.compare(u"any") == 0) {
			logic = obyx::any;
		} else {
			if (logic_str.compare(u"all") != 0) {
				string err_msg; Manager::transcode(logic_str.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << err_msg << " is not a legal logic attribute. It should be one of: any,all" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
}
bool Comparison::evaluate_this() {
	bool firstval = true;
	bool finished = false;
	bool unary_op = false;
	std::string saccumulator;
	double daccumulator = 0;
	pair<long long,bool> naccumulator;
	cmp_evaluated = true;
	if (cmp_evaluated && operation_result=='X') {	//all the comparators are evaluated but the operation is not
		DataItem* acc = nullptr;
		bool compare_bool = (logic == obyx::all ? true : false);	//if invert, then bool must be false.
		for ( unsigned int i = 0; (!finished  && i < inputs.size()); i++ ) {
			if (operation != found || inputs[i]->wotzit != comparate) {
				inputs[i]->evaluate();
			}
			if ( inputs[i]->wotzit == comparate ) {
				if (firstval) {
					switch(operation) {
						case obyx::found: {
							unary_op = true;
							std::set<std::string,less<std::string> > spacekeys;
							inputs[i]->evalfind(spacekeys);
							compare_bool = !spacekeys.empty();
						} break;
						case obyx::equivalent_to: {
							inputs[i]->results.takeresult(acc);
						} break;
						case obyx::exists: {
							inputs[i]->results.takeresult(acc);
							unary_op = true;
							compare_bool = inputs[i]->getexists();
						} break;	// compare_bool is true if it does exist.
						case obyx::is_empty: {
							inputs[i]->results.takeresult(acc);
							unary_op = true;
							compare_bool = (acc == nullptr || acc->empty());  
						} break; 
						case obyx::less_than: {
							inputs[i]->results.takeresult(acc);
							string sacc; if (acc != nullptr) { sacc = *acc; }
							daccumulator = String::real(sacc);
						} break; 
						case obyx::greater_than: {
							inputs[i]->results.takeresult(acc);
							string sacc; if (acc != nullptr) { sacc = *acc; }
							daccumulator = String::real(sacc);
						} break;
						case obyx::significant: {
							inputs[i]->results.takeresult(acc);
							unary_op = true;
							compare_bool = inputs[i]->getexists();  
							if (compare_bool) {
								compare_bool = (acc != nullptr && ! acc->empty());
							}
						} break;
						case obyx::cmp_true: { 
							inputs[i]->results.takeresult(acc);
							unary_op = true;
							string sacc; if (acc != nullptr) { sacc = *acc; }
							if (sacc.compare("true") == 0) { 
								compare_bool = true;
							} else {
								compare_bool = false;
								if (sacc.compare("false") != 0) { 
									*Logger::log << Log::error << Log::LI << "Error. Boolean mismatch. [" << sacc << "] found instead of true or false." << Log::LO;
									trace();
									*Logger::log << Log::blockend;									
								}
							}
						} break;
					}
				} else {
					DataItem* inpval= nullptr;
					if ( compare_bool || logic==obyx::any ) {
						bool compare_test = false;
						switch(operation) {
							case obyx::found: {
								std::set<std::string,less<std::string> > spacekeys;
								inputs[i]->evalfind(spacekeys);
								compare_test = !spacekeys.empty();
							} break;
							case obyx::exists: {
								inputs[i]->results.takeresult(inpval);
								compare_test = inputs[i]->getexists(); 
							} break;
							case obyx::is_empty: {
								inputs[i]->results.takeresult(inpval);
								compare_test = ( inpval == nullptr || inpval->empty() ); 
							} break;
							case obyx::significant: {
								inputs[i]->results.takeresult(inpval);
								compare_test = inputs[i]->getexists(); 
								if (compare_test) {
									compare_test = (inpval != nullptr && ! inpval->empty());
								}
							} break;
							case obyx::equivalent_to: {
								inputs[i]->results.takeresult(inpval);
								if (acc == nullptr) {
									compare_test = (inpval == nullptr);
								} else {
									if (inpval != nullptr) {
										compare_test = acc->same(inpval);
									} else {
										compare_test = false;
									}
								}
							} break;
							case obyx::less_than: {
								inputs[i]->results.takeresult(inpval);
								string sinp; if (inpval != nullptr) { sinp = *inpval; }
								double dinput = String::real(sinp);
								if (!invert) {
									compare_test = daccumulator < dinput; 
								} else {
									compare_test = daccumulator >= dinput; 
								}
								daccumulator = dinput;
							} break;
							case obyx::greater_than: { //LTE = !GT for 773  (7 !gt 7)=T && (7 !gt 3)=F
								inputs[i]->results.takeresult(inpval);
								string sinp; if (inpval != nullptr) { sinp = *inpval; }
								double dinput = String::real(sinp);   // (7 !gt 7) = true = ok
								if (!invert) {
									compare_test = daccumulator > dinput; 
								} else {
									compare_test = daccumulator <= dinput; 
								}
								daccumulator = dinput;
							} break;
							case obyx::cmp_true: {
								inputs[i]->results.takeresult(inpval);
								string sinp; if (inpval != nullptr) { sinp = *inpval; }
								if (sinp.compare("true") == 0) { 
									compare_test = true; // baccumulator = baccumulator || true => baccumulator=true;
								} else {
									compare_test = false;
									if (sinp.compare("false") != 0) {
										*Logger::log << Log::error << Log::LI << "Error. Boolean mismatch. [" << sinp << "] found instead of true or false." << Log::LO;
										trace();
										*Logger::log << Log::blockend;									
									}
								}
							} break;
						}
						if (logic == obyx::all) {
							compare_bool = compare_bool && compare_test; 
						} else {
							compare_bool = compare_bool || compare_test; 
						}
						
					}
					delete inpval;
				}
			}
			if (cbreak && (unary_op || !firstval)) {
				if (logic == obyx::all) {
					if (!invert) {
						finished = !compare_bool;
					} else {
						finished = compare_bool;
					}
				} else { //logic = obyx::any
					if (!invert) {
						finished = compare_bool;
					} else {
						finished = !compare_bool;
					}
				}
			}
			firstval = false;
		}
		delete acc;		
		if (invert && operation != greater_than && operation !=less_than ) compare_bool = !compare_bool;
		if (compare_bool) {
			operation_result = 'T';
		} else {
			operation_result = 'F';
		}
	}
	if (cmp_evaluated && operation_result!='X' && !def_evaluated ) { 
		def_evaluated = true; 
		for ( unsigned int i = 0; i < definputs.size(); i++ ) {		//only one of the definputs will be evaluated here, if it exists.
			if ( (operation_result=='T' && definputs[i]->wotzit == ontrue  ) 
				||   (operation_result=='F' && definputs[i]->wotzit == onfalse ) ) {
				eval_found = true;
				definputs[i]->evaluate();
				DataItem* defev = nullptr;
				definputs[i]->results.takeresult(defev); 
				results.setresult(defev); 
			}
		}	
		if ( def_evaluated) {
			if ( ! eval_found ) {
				results.clearresult();
			}
		} else {
			results.clearresult();
		}
	}
	return cmp_evaluated && def_evaluated;
}
bool Comparison::may_eval_outputs() {
	return results.final() && def_evaluated && eval_found;
}
void Comparison::addInputType(InputType* i) {
	if (i->wotzit == comparate) {
		inputs.push_back(i);
	} else {
		*Logger::log << Log::error << Log::LI << "Error. Only comparate, ontrue, onfalse are legal for comparison." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	}
}
void Comparison::addDefInpType(DefInpType* i) {
	if (definputs.size() == 2) {
		*Logger::log << Log::error << Log::LI << "Error. There can only be one ontrue and one onfalse for each comparison." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	} else {
		switch (i->wotzit) {
			case ontrue: {
				if (!definputs.empty()) {
					if (definputs[0]->wotzit == ontrue) {
						*Logger::log << Log::error << Log::LI << "Error. Only one ontrue is allowed per comparison." << Log::LO; 
						trace();
						*Logger::log << Log::blockend;
					} else {
						*Logger::log << Log::error << Log::LI << "Error. Ontrue should be placed before onfalse." << Log::LO; 
						trace();
						*Logger::log << Log::blockend;
					}
				} else {
					definputs.push_back(i);
				}
			} break;
			case onfalse: {
				if (!definputs.empty()) {
					if (definputs[0]->wotzit == onfalse) {
						*Logger::log << Log::error << Log::LI << "Error. Only one onfalse is allowed per comparison." << Log::LO; 
						trace();
						*Logger::log << Log::blockend;
					} else {
						definputs.push_back(i);
					}
				} else {
					definputs.push_back(i);
				}
			} break;
			default: {
				*Logger::log << Log::error << Log::LI << "Error. Only comparate, ontrue, onfalse are legal for comparison." << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			} break;
		}
	}
}
Comparison::~Comparison() {
	//outputs/inputs are deleted by Function.
}
void Comparison::init() {
}
void Comparison::finalise() {
}
void Comparison::startup() {
	cmp_types.insert(cmp_type_map::value_type(u"empty", std::pair<cmp_type,bool>(obyx::is_empty,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"equal", std::pair<cmp_type,bool>(obyx::equivalent_to,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"existent", std::pair<cmp_type,bool>(obyx::exists,false)));
	cmp_types.insert(cmp_type_map::value_type(u"found", std::pair<cmp_type,bool>(obyx::found,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"greater", std::pair<cmp_type,bool>(obyx::greater_than,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"lesser", std::pair<cmp_type,bool>(obyx::less_than,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"significant", std::pair<cmp_type,bool>(obyx::significant,false) ));
	cmp_types.insert(cmp_type_map::value_type(u"true", std::pair<cmp_type,bool>(obyx::cmp_true,false) ));
}
void Comparison::shutdown() {
	cmp_types.clear();
}	


