/* 
 * comparison.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * comparison.cpp is a part of Obyx - see http://www.obyx.org .
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
using namespace qxml;

cmp_type_map Comparison::cmp_types;

Comparison::Comparison(ObyxElement* par,const Comparison* orig) : Function(par,orig),
operation(orig->operation),invert(orig->invert),scope(orig->scope),
eval_found(orig->eval_found),cmp_evaluated(orig->cmp_evaluated),
def_evaluated(orig->def_evaluated),operation_result(orig->operation_result) {}

Comparison::Comparison(xercesc::DOMNode* const& n,ObyxElement* par) :
Function(n,comparison,par),operation(equivalent_to),invert(false),scope(qxml::all),eval_found(false),
cmp_evaluated(false),def_evaluated(false),operation_result('X') {
	u_str op_string,invert_str,scope_str; 
	if ( Manager::attribute(n,UCS2(L"operation"),op_string) ) {
		cmp_type_map::const_iterator i = cmp_types.find(op_string);
		if( i != cmp_types.end() ) {
			operation = i->second.first; 
			invert = i->second.second;
		} else {
			if ( ! op_string.empty() ) {
				string err_msg; transcode(op_string.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal comparison operation. It should be one of: equal,existent,empty,significant,greater,lesser,true" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
	if ( Manager::attribute(n,UCS2(L"invert"),invert_str) ) {
		if (invert_str.compare(UCS2(L"true")) == 0) {
			invert = !invert;
		} else {
			if (invert_str.compare(UCS2(L"false")) != 0) {
				string err_msg; transcode(invert_str.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal invert. It should be one of: true,false" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
	if ( Manager::attribute(n,UCS2(L"scope"),scope_str) ) {
		if (scope_str.compare(UCS2(L"any")) == 0) {
			scope = qxml::any;
		} else {
			if (scope_str.compare(UCS2(L"all")) != 0) {
				string err_msg; transcode(scope_str.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal scope. It should be one of: any,all" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
}

bool Comparison::evaluate_this() {
	bool firstval = true;
	std::string saccumulator;
	double daccumulator = 0;
	pair<long long,bool> naccumulator;
	if (!cmp_evaluated ) {			//are all the comparators evaluated?
		cmp_evaluated = true;
		for ( size_t i = 0; i < inputs.size(); i++ ) {
			bool wasfinal = inputs[i]->evaluate();
			cmp_evaluated = cmp_evaluated && wasfinal;
		}
	}
	if (cmp_evaluated && operation_result=='X') {	//all the comparators are evaluated but the operation is not
		bool baccumulator = true;
		DataItem* acc = NULL;
		bool compare_bool = (scope == qxml::all ? true : false);	//if invert, then bool must be false.
		for ( unsigned int i = 0; i < inputs.size(); i++ ) {
			if ( inputs[i]->wotzit == comparate ) {
				if (firstval) {
					inputs[i]->results.takeresult(acc);
					firstval = false;
					switch(operation) {
						case exists: {   
							compare_bool = inputs[i]->getexists();  
						} break;	// compare_bool is true if it does exist.
						case significant: {
							compare_bool = inputs[i]->getexists();  
							if (compare_bool) {
								compare_bool = (acc != NULL && ! acc->empty());
							}
						} break;
						case is_empty: { 
							compare_bool = (acc == NULL || acc->empty());  
						} break; 
						case cmp_true:
						case cmp_and: 
						case cmp_xor:
						case cmp_or:   { 
							string sacc; if (acc != NULL) { sacc = *acc; }
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
						case less_than: {
							string sacc; if (acc != NULL) { sacc = *acc; }
							daccumulator = String::real(sacc);
						} break; 
						case greater_than: {
							string sacc; if (acc != NULL) { sacc = *acc; }
							daccumulator = String::real(sacc);
						} break;
						case natural: {
							string sacc; if (acc != NULL) { sacc = *acc; }
							pair<unsigned long long,bool> isnat = String::znatural(sacc);
							compare_bool = isnat.second;
						} break;
						case email: {
							string sacc; if (acc != NULL) { sacc = *acc; }
							compare_bool = String::mailencode(sacc);
						} break;
						case substring_of: {
							if (acc != NULL) { saccumulator = *acc; };
						} break;
						default: break;
					}
				} else {
					DataItem* inpval= NULL;
					inputs[i]->results.takeresult(inpval);
					if ( compare_bool || scope==qxml::any ) {
						bool compare_test = false;
						switch(operation) {
							case natural: {
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								pair<unsigned long long,bool> isnat = String::znatural(sinp);
								compare_test = isnat.second;							
							} break;
							case substring_of: {
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								compare_test = (sinp.find(saccumulator) != string::npos) ;
								if (inpval != NULL) { saccumulator = *inpval; }
							} break;
							case exists: { 
								compare_test = inputs[i]->getexists(); 
							} break;
							case is_empty: { 
								compare_test = ( inpval == NULL || inpval->empty() ); 
							} break;
							case significant: {
								compare_test = inputs[i]->getexists(); 
								if (compare_test) {
									compare_test = (inpval != NULL && ! inpval->empty());
								}
							} break;
								
							case equivalent_to: { 
								if (acc == NULL) {
									compare_test = (inpval == NULL);
								} else {
									if (inpval != NULL) {
										compare_test = acc->same(inpval);
									} else {
										compare_test = false;
									}
								}
							} break;
							case email: { 
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								compare_test = String::mailencode(sinp);
							} break;							
							case less_than: { 
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								double dinput = String::real(sinp);
								if (!invert) {
									compare_test = daccumulator < dinput; 
								} else {
									compare_test = daccumulator >= dinput; 
								}
								daccumulator = dinput;
							} break;
							case greater_than: { //LTE = !GT for 773  (7 !gt 7)=T && (7 !gt 3)=F
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								double dinput = String::real(sinp);   // (7 !gt 7) = true = ok
								if (!invert) {
									compare_test = daccumulator > dinput; 
								} else {
									compare_test = daccumulator <= dinput; 
								}
								daccumulator = dinput;
							} break;
							case cmp_or:
							case cmp_true: {
								string sinp; if (inpval != NULL) { sinp = *inpval; }
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
							case cmp_and: {
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								if (sinp.compare("true") != 0) {
									if (sinp.compare("false") == 0) { 
										compare_test = false; // baccumulator = baccumulator && false => false;
									} else {
										*Logger::log << Log::error << Log::LI << "Error. [" << sinp << "] found instead of true or false." << Log::LO;
										trace();
										*Logger::log << Log::blockend;									
									}
								}								
							} break;
							case cmp_xor: {
								string sinp; if (inpval != NULL) { sinp = *inpval; }
								if (sinp.compare("true") == 0) { 
									compare_test = ! baccumulator;
								} else {
									if (sinp.compare("false") != 0) {
										*Logger::log << Log::error << Log::LI << "Error. [" << sinp << "] found instead of true or false." << Log::LO;
										trace();
										*Logger::log << Log::blockend;									
									}
								}
							} break;
						}
						if (scope == qxml::all) {
							compare_bool = compare_bool && compare_test; 
						} else {
							compare_bool = compare_bool || compare_test; 
						}
						
					}
				}
			}
		}
		if (invert && operation != greater_than && operation !=less_than ) compare_bool = !compare_bool;
		if (compare_bool) {
			operation_result = 'T';
		} else {
			operation_result = 'F';
		}
		while ( inputs.size() > 0) {
			delete inputs.front();
			inputs.pop_front(); 
		}
	}
	if (cmp_evaluated && operation_result!='X' && !def_evaluated ) { 
		def_evaluated = true; 
		for ( unsigned int i = 0; i < definputs.size(); i++ ) {		//only one of the definputs will be evaluated here, if it exists.
			if ( (operation_result=='T' && definputs[i]->wotzit == ontrue  ) 
				||   (operation_result=='F' && definputs[i]->wotzit == onfalse ) ) {
				eval_found = true;
				def_evaluated = definputs[i]->evaluate();
				if (def_evaluated) { 
					DataItem* defev = NULL;
					definputs[i]->results.takeresult(defev); 
					results.setresult(defev); 
				}
			}
		}	
		if ( def_evaluated) {
			while ( definputs.size() > 0) {
				delete definputs.front();
				definputs.pop_front(); 
			}
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

//static methods - once only (either per main doc, or per process) thank-you very much..
void Comparison::init() {
}

void Comparison::finalise() {
}

void Comparison::startup() {
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"equal"), std::pair<cmp_type,bool>::pair(equivalent_to,false) ));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"existent"), std::pair<cmp_type,bool>::pair(exists,false)));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"empty"), std::pair<cmp_type,bool>::pair(is_empty,false) ));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"significant"), std::pair<cmp_type,bool>::pair(significant,false) ));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"greater"), std::pair<cmp_type,bool>::pair(greater_than,false) ));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"lesser"), std::pair<cmp_type,bool>::pair(less_than,false) ));
	cmp_types.insert(cmp_type_map::value_type(UCS2(L"true"), std::pair<cmp_type,bool>::pair(cmp_true,false) ));
}

void Comparison::shutdown() {
	cmp_types.clear();
}	


