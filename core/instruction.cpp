/* 
 * instruction.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * instruction.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <algorithm>
#include <ctype.h>
//#include <math.h>
#include <cfloat>
#include <errno.h>
#include <float.h>

#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/string/strings.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"
#include "commons/filing/filing.h"

#include "pairqueue.h"
#include "iteration.h"
#include "obyxelement.h"
#include "function.h"
#include "inputtype.h"
#include "output.h"
#include "instruction.h"
#include "document.h"
#include "xmlobject.h"


using namespace Log;
using namespace XML;
using namespace obyx;

op_type_map Instruction::op_types;

Instruction::Instruction(xercesc::DOMNode* const& n,ObyxElement* par) :
Function(n,instruction,par),operation(obyx::move),precision(0),bitpadding(0),bool_convert(false),base_convert(false),inputsfinal(false) {
	u_str op_string;
	Manager::attribute(n,u"operation",op_string);
	op_type_map::const_iterator i = op_types.find(op_string);
	if( i != op_types.end() ) {
		operation = i->second; 
	} else {
		if ( ! op_string.empty() ) {
			string err_type,name; Manager::transcode(op_string.c_str(),err_type);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_type << " is not a legal instruction operation. It should be one of ";
			op_type_map::iterator op_ti = op_types.begin();
			while ( op_ti != op_types.end() ) {
				XML::Manager::transcode(op_ti->first,name);
				op_ti++;
				if (op_ti == op_types.end()) {
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
	if ( 
		(operation == obyx::divide) || (operation == obyx::multiply) || 
		(operation == obyx::add) || (operation == obyx::subtract) || 
		(operation == obyx::random) || (operation == obyx::arithmetic) || (operation == obyx::bitwise)
		) {
		std::string str_prec;
		Manager::attribute(n,u"precision",str_prec);
		if ( ! str_prec.empty() ) {
			if (str_prec[0] == 'B' || str_prec[0] == 'b') { //2,8,10,16 (B is for base), t = truth
				if (!str_prec.compare("bool")) {
					bool_convert = true;
					str_prec="0";
				} else {
					base_convert = true;
					str_prec.erase(0,1);
					size_t bcpt = str_prec.find('.');
					if (bcpt != string::npos) {
						string bits=str_prec.substr(bcpt+1,string::npos);
						pair<unsigned long long,bool> bits_value = String::znatural(bits);
						if  ( bits_value.second && bits_value.first <= 64) {
							bitpadding = (unsigned int)bits_value.first;
						} else {
							*Logger::log << Log::syntax << Log::LI << "Syntax Error. Instruction: bits component of precision must be a number between 1 and 64." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
						str_prec.erase(bcpt,string::npos);
					}
				}
			}
			pair<unsigned long long,bool> prec_value = String::znatural(str_prec);
			if  ( prec_value.second && prec_value.first < 17) {
				precision = (unsigned int)prec_value.first;
			} else {
				*Logger::log << Log::syntax << Log::LI << "Syntax. Instruction: precision must be a number between 0 and 16." << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
}
Instruction::Instruction(ObyxElement* par,const Instruction* orig) : Function(par,orig),
operation(orig->operation),precision(orig->precision),bitpadding(orig->bitpadding),bool_convert(orig->bool_convert),base_convert(orig->base_convert),inputsfinal(orig->inputsfinal) {
}
void Instruction::do_function() {
	DataItem* document = nullptr;
	inputs[0]->results.takeresult(document);
	Document::type_parm_map* function_instance = new Document::type_parm_map();
	size_t n = inputs.size();
	for ( unsigned int i = 1; i < n; i++ ) {
		string parm_key=inputs[i]->parm_name;
		if ( ! parm_key.empty()) {
			DataItem* rslt = nullptr;
			if ( ! inputs[i]->results.final() ) {
				inputs[i]->evaluate();
				if ( ! inputs[i]->results.final() ) {
					*Logger::log <<  Log::error << Log::LI << "Error. Parameter " << parm_key << " must be able to be evaluated before function is called." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {
					inputs[i]->results.takeresult(rslt);
				}
			} else {
				inputs[i]->results.takeresult(rslt);
			}
			pair<Document::type_parm_map::iterator, bool> ins = function_instance->insert(Document::type_parm_map::value_type(parm_key,rslt));
			if (!ins.second) {
				*Logger::log <<  Log::error << Log::LI << "Error. Duplicate name found. All the name attributes of a function call must be unique." << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log <<  Log::error << Log::LI << "Error. All inputs (other than the first) of a function call need a name attribute." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
	string fn_filename;
	//really need to get the value from input 1 value.
	if (! inputs[0]->name_v.empty()) {
		Manager::transcode(inputs[0]->name_v.c_str(),fn_filename);
	} 
	if (fn_filename.empty()) {
		fn_filename="function call"; 
	}
	
	bool has_own_directory = false;
	if ( fn_filename.rfind('/') != string::npos ) {	
		FileUtils::File file(fn_filename);
		if (file.exists()) {
			has_own_directory = true;
			string dirstring = fn_filename.substr(0,fn_filename.rfind('/'));
			FileUtils::Path::push_wd(dirstring);
		}
	}
	Document* eval_doc = new Document(document,Document::File,fn_filename,this,false);
	eval_doc->setparms(function_instance);
	eval_doc->doc_par = owner;
	eval_doc->owner = eval_doc;
	eval_doc->eval();
	if ( ! eval_doc->results.final()  ) {
		eval_doc->results.normalise();	
		if ( ! eval_doc->results.final()  ) { //if it is STILL not final..
			*Logger::log << Log::error << Log::LI << "Error. Function was not fully evaluated." << Log::LO ;
			trace();
			//			string troubled_doc;
			//			XML::Manager::parser()->writedoc(eval_doc,troubled_doc);
			*Logger::log << Log::LI << "The document that failed is:" << Log::LO;
			*Logger::log << Log::LI << Log::info << Log::LI << document << Log::LO << Log::blockend << Log::LO; 
			*Logger::log << Log::blockend; //Error
			results.clear();
		}
		DataItem* nowt = nullptr;
		results.setresult(nowt);
	} else {
		results.setresult(eval_doc->results);
	}
	if(has_own_directory) {
		FileUtils::Path::pop_wd();
	}	
	delete eval_doc;
	delete document;
}
bool Instruction::evaluate_this() {
	size_t n = inputs.size();
	if ( !inputsfinal ) {
		inputsfinal = true;
		if (n == 0) {
			*Logger::log << Log::error << Log::LI << "Error. Instruction requires a minimum of one input." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
		for ( size_t i = 0; i < n; i++ ) {
			inputs[i]->evaluate(i+1,n);
		}
	}
	if (inputsfinal) {
#ifndef DISALLOW_GMP		
		String::Bit::Evaluate* expr_bit_eval = nullptr;	//use this only if op = arithmetic.
#endif
		String::Infix::Evaluate* expr_eval = nullptr;		//use this only if op = arithmetic.
		if (operation == obyx::arithmetic) {
			expr_eval = new String::Infix::Evaluate();
		}
		if (operation == obyx::bitwise) {
#ifndef DISALLOW_GMP		
			expr_bit_eval = new String::Bit::Evaluate();
#else
			*Logger::log << Log::error << Log::LI << "Error. Operation 'bitwise' requires the gmp library from http://gmplib.org." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
#endif
		}
		switch (operation) {
			case obyx::function: {
				do_function();
			} break;
			case obyx::move: {
				if ( n > 1 ) {
					*Logger::log << Log::error << Log::LI << "Error. Operation 'assign' accepts only the first input. Use operation 'append' for multiple input instructions." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
				if ( n > 0 ) {	
					results.setresult(inputs[0]->results);
				}
			} break;
			case obyx::kind: {
				if ( n > 1 ) {
					*Logger::log << Log::error << Log::LI << "Error. Operation 'kind' accepts only one input." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
				if ( n > 0 ) {	
					DataItem* srr = nullptr;
					string value;
					inputs[0]->results.takeresult(srr);
					if (srr != nullptr) {
						switch(srr->kind()) {
							case di_null: 		{ value="empty"; } break;
							case di_object:
							case di_fragment: 	{ value="object"; } break;
							case di_raw: 		{ value="raw"; } break;
							case di_auto:
							case di_text:
							case di_utext: 		{ value="text"; } break;	
						}
						delete srr;
						srr=nullptr;
					} else {
						value="empty";
					}
					srr = DataItem::factory(value,di_text);
					results.setresult(srr);
				}
			} break;				
			default: {
				bool failed = false;		//Used in quotient/modulus for division by zero
				bool firstval = true;
				DataItem* first_value = nullptr;	//used by eg left to hold the initial parameter, against which everything else is evaluated.
				DataItem* srcval = nullptr;
				long long iaccumulator = 0;
				std::string accumulator,special;
				enc_type hmac_enc= e_none;
				u_str uaccumulator;
				unsigned long long naccumulator = 0;
				long double daccumulator = 0;
				for ( size_t i = 0; i < n; i++ ) {
					if ( inputs[i]->wotzit == input ) {					
						inputs[i]->results.takeresult(srcval); //final stuff here - this is always right - see above
						if (firstval) {
							first_value = srcval;
							firstval = false;
							switch ( operation ) {
								case obyx::move:
								case obyx::kind:
								case obyx::sort:
								case obyx::transliterate:
								case obyx::function:
									break; //operations handled outside of this switch, or DataItem is native.
								case obyx::bitwise: {
#ifndef DISALLOW_GMP		
									if (expr_bit_eval != nullptr) {
										string fv; if (first_value != nullptr) { fv = *first_value; }
										expr_bit_eval->set_expression(fv);
									}
#endif
								} break;
								case obyx::hmac: {
									hmac_enc = IKO::str_to_encoder(*first_value);
								} break;
								case obyx::arithmetic: {
									if (expr_eval != nullptr) {
										string fv; if (first_value != nullptr) { fv = *first_value; }
										expr_eval->set_expression(fv);
									}
								} break;
								case obyx::query_command:		// call_sql(first_value); break;
								case obyx::shell_command:	{	// call_system(first_value); break;
									if (first_value != nullptr) {
										accumulator.append(*first_value);
									}
								} break;
								case obyx::unique:	{	// just add to accumulator; break;
									if (first_value != nullptr) {
										uaccumulator.append(*first_value);
									}
								} break;
								case obyx::append: {	//
									results.append(first_value);
								} break;
								case obyx::substring:		  // we don't want the first value to be stuck out.
								case obyx::position:
								case obyx::left:
								case obyx::right:
									if (first_value != nullptr) {
										accumulator = *first_value;
									}
									break;
								case obyx::maximum: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									daccumulator = String::real(fv);
									if ( std::isnan(daccumulator) ) {
										daccumulator = - DBL_MAX;
									}
								} break;
								case obyx::minimum: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									daccumulator = String::real(fv);
									if ( std::isnan(daccumulator) ) {
										daccumulator =   DBL_MAX;
									}
								} break;
								case obyx::random: { 
									string fv; if (first_value != nullptr) { fv = *first_value; }
									daccumulator = String::real(fv);
									if (n == 1) { // this is from  lowbound .. .dacc.
										do_random(daccumulator,0,daccumulator);
									}
								} break;
								case obyx::divide:
								case obyx::multiply:
								case obyx::add: 
								case subtract: { 
									string fv; if (first_value != nullptr) { fv = *first_value; }
									daccumulator = String::real(fv);
								} break;
								case obyx::upper: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									String::toupper(fv);
									accumulator = fv;
								} break;
								case obyx::lower: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									String::tolower(fv);
									accumulator = fv;
								} break;
								case obyx::reverse: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									String::reverse(fv);
									accumulator = fv;
								} break;
								case obyx::length: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									if (! String::length(fv,naccumulator) ) {
										*Logger::log <<  Log::error << Log::LI << "Error. " << "'" << fv << "' is not a legal UTF-8 string." << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} break;
								case obyx::quotient:
								case obyx::remainder: {
									string fv; if (first_value != nullptr) { fv = *first_value; }
									pair<long long, bool> i_res = String::integer(fv);
									if (i_res.second) {
										iaccumulator = i_res.first;
									} else {
										*Logger::log <<  Log::error << Log::LI << "Error. " << "'" << fv << "' is not a legal initial input value for the remainder operation. It should be an integer." << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} break;
							}
						} else {
							switch ( operation ) {
								case obyx::move:
								case obyx::kind:
								case obyx::function:
									break; //operations handled outside of this switch.
								case obyx::bitwise: {
#ifndef DISALLOW_GMP											
									if (expr_bit_eval != nullptr) {
										string parm_key=inputs[i]->parm_name;
										if ( ! parm_key.empty()) {
											string fv; if (srcval != nullptr) { fv = *srcval; }
											expr_bit_eval->add_parm(parm_key,fv);
										}
									}
#endif
								} break;
								case obyx::hmac: {
									if (srcval != nullptr) {
										if (i == 1) { //this is the key
											special = *srcval;
										} else {
											accumulator.append(*srcval);
										}
									}
								} break;
								case obyx::arithmetic: {
									if (expr_eval != nullptr) {
										string parm_key=inputs[i]->parm_name;
										if ( ! parm_key.empty()) {
											string fv; if (srcval != nullptr) { fv = *srcval; }
											double dble = String::real(fv);
											expr_eval->add_parm(parm_key,dble);
										}
									}
								} break;
								case obyx::query_command:
								case obyx::shell_command:	{	// call_system(first_value); break;
									if (srcval != nullptr) {
										accumulator.append(*srcval);
									}
								} break;
								case obyx::unique:	{	// just add to accumulator; break;
									if (srcval != nullptr) {
										uaccumulator.append(*srcval);
									}
								} break;
								case obyx::sort: {
									if (srcval != nullptr) {
										if (i == 1) { //base
											uaccumulator = *srcval;
										} else {	//xpath
											if (first_value != nullptr) {
												u_str sortstr = *srcval;
												XMLObject* xdoc = *first_value;
												if (xdoc != nullptr) {
													string error_str;
													xdoc->sort(uaccumulator,sortstr,inputs[i]->ascending,false,error_str);
													if (!error_str.empty()) {
														*Logger::log <<  Log::error  << Log::LI << "Error. " << error_str << Log::LO;
														trace();
														*Logger::log << Log::blockend;
													}
												}
											}
										}
									}
								} break;
								case obyx::append: {
									results.append(srcval);
								} break; //done differently.
								case obyx::upper: {
									string fv; if (srcval != nullptr) { fv = *srcval; }
									String::toupper(fv);
									accumulator.append(fv);
								} break;
								case obyx::lower: {
									string fv; if (srcval != nullptr) { fv = *srcval; }
									String::tolower(fv);
									accumulator.append(fv);
								} break;
								case obyx::reverse: {
									string fv; if (srcval != nullptr) { fv = *srcval; }
									String::reverse(fv);
									accumulator.insert(0,fv);
								} break;
								case obyx::substring: {
									string fv; if (srcval != nullptr) { fv = *srcval; }
									if (i == 1) { //left cutting point.
										pair<unsigned long long, bool> i_res = String::znatural(fv);
										//pair<long long, bool> i_res = String::integer(fv);
										if (i_res.second) {
											naccumulator = i_res.first;
										} else {
											*Logger::log <<  Log::error  << Log::LI << "Error. '" << fv << "' is not a legal index value for substring operation. It should be a zero or positive integer." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}
									} else { //right cutting point.
										pair<unsigned long long, bool> i_res = String::znatural(fv);
										if (i_res.second) {
											if (first_value != nullptr) {
												String::substring(string(*first_value),naccumulator,i_res.first,accumulator);
												naccumulator = i_res.first;
											}
										} else {
											*Logger::log <<  Log::error  << Log::LI << "Error. '" << fv << "' is not a legal length value for substring operation. It should be a zero or positive integer." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}
									}
								} break;
								case obyx::left: {
									long long charstocut = 0;
									string fv; if (srcval != nullptr) { fv = *srcval; }
									pair<long long, bool> i_res = String::integer(fv);
									if (i_res.second) {
										charstocut = i_res.first;
										string cutted=fv;
										String::left(accumulator,charstocut,cutted);
										accumulator = cutted;
									} else {
										*Logger::log <<  Log::error << Log::LI << "Error. '" << fv << "' is not a legal subsequent input value for left operation. It should be an integer." << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} break;
								case obyx::right: {
									long long charstocut = 0;
									string fv; if (srcval != nullptr) { fv = *srcval; }
									pair<long long, bool> i_res = String::integer(fv);
									if (i_res.second) {
										charstocut = i_res.first;
										string cutted=fv;
										String::right(accumulator,charstocut,cutted);
										accumulator = cutted;
									} else {
										*Logger::log << Log::error << Log::LI << "Error. '" << fv << "' is not a legal subsequent input value for right operation. It should be an integer." << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} break;
								case obyx::position: {
									unsigned long long strposition=0;
									string fv;
									if (srcval != nullptr) {
										fv = *srcval;
										if ( String::position(accumulator,fv,strposition) ) { 
											String::tostring(accumulator,strposition);
										} else {
											accumulator = "NaN";
										}
									} else {
										accumulator = "NaN";
									}
								} break;
								case obyx::length: {
									string fv;
									unsigned long long bacc;
									if (srcval != nullptr) {
										fv = *srcval;
										if (! String::length(fv,bacc) ) {
											*Logger::log <<  Log::error << Log::LI << "Error. " << "'" << fv << "' is not a legal UTF-8 string." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}
										naccumulator += bacc;
									}
								} break;
								case obyx::random: { 
									if (srcval != nullptr) {
										string sv = *srcval;
										long double lowbound = daccumulator;
										daccumulator = String::real(sv);
										do_random(daccumulator,lowbound,daccumulator);
									}
								} break;
								case obyx::add: {
									string rstring;
									if (srcval != nullptr) {
										rstring = *srcval;
										daccumulator += String::real(rstring); 
									}
								} break;
								case obyx::maximum: {
									string sv; if (srcval != nullptr) { sv = *srcval; }
									double tst = String::real(sv);
									if ( ! std::isnan(tst) ) { //testing for nan..
										daccumulator = daccumulator > tst ? daccumulator : tst; 
									}
								} break;
								case obyx::minimum: {
									string sv; if (srcval != nullptr) { sv = *srcval; }
									double tst = String::real(sv);
									if ( ! std::isnan(tst) ) { //testing for nan..
										daccumulator = daccumulator < tst ? daccumulator : tst; 
									}
								} break;
								case obyx::subtract: {
									string sv; if (srcval != nullptr) { sv = *srcval; }
									daccumulator -= String::real(sv);
								} break;
								case obyx::multiply: {
									string sv; if (srcval != nullptr) { sv = *srcval; }
									daccumulator *= String::real(sv);
								} break;
								case obyx::divide: {
									string sv; if (srcval != nullptr) { sv = *srcval; }
									daccumulator /= String::real(sv);
								} break;
								case obyx::quotient: {
									if (!failed) {
										long long nsrc = 0;
										string sv; if (srcval != nullptr) { sv = *srcval; }
										pair<long long, bool> i_res = String::integer(sv);
										if (i_res.second) {
											nsrc = i_res.first;
											if (nsrc == 0) {
												failed = true;
											} else {
												iaccumulator = (long long)(floor(double(iaccumulator)/double(nsrc)));
											}										
										} else {
											*Logger::log <<  Log::error << Log::LI << "Error. '" << sv << "' is not a legal subsequent input value for quotient operation. It should be an integer." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}									
									}
								} break;
								case obyx::remainder: {
									if (!failed) {
										long long isrc = 0;
										string fv; if (srcval != nullptr) { fv = *srcval; }
										pair<long long, bool> i_res = String::integer(fv);
										if (i_res.second) {
											isrc = i_res.first;
											if (isrc == 0) {
												failed = true;
											} else {
												iaccumulator = iaccumulator % isrc; 
											}										
										} else {
											*Logger::log <<  Log::error << Log::LI << "Error. '" << fv << "' is not a legal subsequent input value for remainder operation. It should be an integer." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}
									}
								} break;
								case obyx::transliterate: {
									if (srcval != nullptr) {
										uaccumulator.append(*srcval);
									}
								} break;
							}
							delete srcval; srcval=nullptr;
						}
					} else {
						*Logger::log << Log::error << Log::LI << "Error. Instructions may only use Inputs." << Log::LO;
						trace();
						*Logger::log << Log::blockend;
					}
				}
				switch ( operation ) {
					case obyx::append:
					case obyx::function:
					case obyx::move:
					case obyx::kind:
						break;
					case obyx::bitwise: { //expr_bit_eval (result is in hexdigits)
#ifndef DISALLOW_GMP	
						if (expr_bit_eval != nullptr) {
							std::string errs,expr_result;
							if (base_convert) {
								expr_result = expr_bit_eval->process(errs,precision);
							} else {
								if (bool_convert) {
									expr_result = expr_bit_eval->process(errs,16);
									if (!expr_result.compare("0")) {
										expr_result="false";
									} else {
										expr_result="true";
									}
								} else {
									expr_result = expr_bit_eval->process(errs,16);
								}
							}
							if (!errs.empty()) {
								*Logger::log << Log::error << Log::LI << errs << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
							results.append(expr_result,di_text);
							delete expr_bit_eval;
						}
#endif
					} break;
					case obyx::arithmetic: {
						if (expr_eval != nullptr) {
							std::string expr_result, errs;
							long double retval = expr_eval->process(errs);
							if (!errs.empty()) {
								*Logger::log << Log::error << Log::LI << errs << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
							if (base_convert) {
								String::tobasestring(retval,precision,bitpadding,expr_result);
							} else {
								if (bool_convert) {
									if (retval == 0) {
										expr_result="false";
									} else {
										expr_result="true";
									}
								} else {
									expr_result = String::tostring(retval,precision);
								}
							}
							results.append(expr_result,di_text);
							delete expr_eval;
						}
					} break;
					case obyx::unique: {
						std::sort(uaccumulator.begin(), uaccumulator.end());
						uaccumulator.erase(std::unique(uaccumulator.begin(), uaccumulator.end()), uaccumulator.end());
						results.append(uaccumulator,di_utext);
					} break;
					case obyx::query_command: {
						call_sql(accumulator);
					} break;
					case obyx::shell_command: {
						call_system(accumulator);  
					} break;
					case obyx::position: {
						if (n < 2) {
							*Logger::log << Log::error << Log::LI << "Error. Operation 'position' needs two inputs." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							results.append("NaN",di_text);
						} else {
							results.append(accumulator,di_text);
						}
					} break;
					case obyx::hmac: {
						string result,errs;
						if (String::Digest::available(errs)) {
							switch (hmac_enc) {
								case e_sha1: String::Digest::hmac(String::Digest::sha1,special,accumulator,result); break;
								case e_sha224: String::Digest::hmac(String::Digest::sha224,special,accumulator,result); break;
								case e_sha256: String::Digest::hmac(String::Digest::sha256,special,accumulator,result); break;
								case e_sha384: String::Digest::hmac(String::Digest::sha384,special,accumulator,result); break;
								case e_sha512:  String::Digest::hmac(String::Digest::sha512,special,accumulator,result); break;
								case e_dss1:  String::Digest::hmac(String::Digest::dss1,special,accumulator,result); break;
								case e_mdc2:  String::Digest::hmac(String::Digest::mdc2,special,accumulator,result); break;
								case e_ripemd160:  String::Digest::hmac(String::Digest::ripemd160,special,accumulator,result); break;
								case e_md5:  String::Digest::hmac(String::Digest::md5,special,accumulator,result); break;
								default: {
									*Logger::log << Log::error << Log::LI << "Error. Operation 'hmac' expects sha1,sha224,sha256,sha384,sha512,dss1,mdc2,ripemd160 or md5 in it's first parameter." << Log::LO;
									trace();
									*Logger::log << Log::blockend;
								} break;
							}
							results.append(result,di_text);						
						} else {
							*Logger::log << Log::error << Log::LI << "Error. Operation 'hmac' relies upon openssl which wasn't found." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
					} break;
					case obyx::substring:
					case obyx::reverse:
					case obyx::left:
					case obyx::right:
					case obyx::upper:
					case obyx::lower: {
						results.append(accumulator,di_text);
					} break;
					case obyx::random: {
						std::string math_result;
						if (base_convert) {
							String::tobasestring(daccumulator,precision,bitpadding,math_result);
						} else {
							math_result = String::tostring(daccumulator,precision);
						}
						results.append(math_result,di_text);
					} break;
					case obyx::add: 
					case obyx::subtract:
					case obyx::multiply:
					case obyx::maximum:
					case obyx::minimum:
					case obyx::divide: {
						if (n < 2) {
							*Logger::log << Log::error << Log::LI << "Error. Arithmetic operations need at least two inputs." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							results.append("NaN",di_text);
						} else {
							std::string math_result;
							if (base_convert) {
								String::tobasestring(daccumulator,precision,bitpadding,math_result);
							} else {
								math_result = String::tostring(daccumulator,precision);
							}
							results.append(math_result,di_text);
						}
					} break;
					case obyx::remainder: {
						if (n < 2) {
							*Logger::log << Log::error << Log::LI << "Error. 'remainder' operation needs at least two inputs." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							results.append("NaN",di_text);
						} else {
							if (!failed) {
								std::string num;
								if (base_convert) {
									String::tobase(iaccumulator,precision,bitpadding,num);
								} else {
									num = String::tostring(iaccumulator);
								}
								results.append(num,di_text);
							} else {
								results.append("NaN",di_text);
							}
						}
					} break;
					case obyx::transliterate: {
#ifndef DISALLOW_ICU
						if (first_value != nullptr) {
							u_str urules = *first_value;
							if (String::TransliterationService::available()) {
								string errs;
								String::TransliterationService::transliterate(uaccumulator,urules,errs);
								results.append(uaccumulator,di_utext);
								if (!errs.empty()) {
									*Logger::log << Log::error << Log::LI << "Error. ICU transliterate. failed:"  << errs << Log::LO;
									trace();
									*Logger::log << Log::blockend;
								} 
							} else {
								string extra = String::TransliterationService::startup_messages;
								*Logger::log << Log::error << Log::LI << "Error. ICU is not available for transliterate. ("  << extra << ")" << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
#else
						*Logger::log << Log::error << Log::LI << "Error. ICU support was disabled at compile time." << Log::LO;
						trace();
						*Logger::log << Log::blockend;				
#endif
					} break;
					case obyx::quotient: {
						if (n < 2) {
							*Logger::log << Log::error << Log::LI << "Error. 'quotient' operation needs at least two inputs." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							results.append("NaN",di_text);
						} else {
							if (!failed) {
								std::string num;
								if (base_convert) {
									String::tobase(iaccumulator,precision,bitpadding,num);
								} else {
									num = String::tostring(iaccumulator);
								}
								results.append(num,di_text);
							} else {
								results.append("inf",di_text);
							}
						}
					} break;
					case obyx::length:{
						std::string num;
						if (base_convert) {
							String::tobase(naccumulator,precision,bitpadding,num);
						} else {
							num = String::tostring(naccumulator);
						}
						results.append(num,di_text);
					} break;
					case obyx::sort: {
						results.setresult(first_value,false);
					} break;
				}
				delete first_value; first_value=nullptr;
			} break;
		}
	}
	return (inputsfinal);
}
void Instruction::call_sql(std::string& querystring) {
	if ( ! querystring.empty() ) {
		if ( dbs != nullptr && dbc != nullptr ) {
			Vdb::Query *query = nullptr;
			if (dbc->query(query,querystring)) {
				if (! query->execute() ) {
					*Logger::log << Log::error << Log::LI << "Error. DB Error: SQL Query:" << querystring << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}	
				delete query;
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Instruction operation query needs a database selected. An sql service was found, but the sql connection failed." << Log::LO;
				trace();
				dbc->list();
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. Instruction operation query needs an sql service, and there is none." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
}
void Instruction::do_random(long double& result,long double low_endpoint,long double high_endpoint) {
	string errs;
	if ( String::Digest::available(errs) ) {
		unsigned long long base = ULLONG_MAX;
		long double dbase = base,calc = 0.0;
		result = 0.0;
		string rnd("");
		int x = sizeof(unsigned long long);
		pair<unsigned long long,bool> num_pr(0,false);
		while (!num_pr.second || rnd.empty()) {
			String::Digest::random(rnd,x);
			String::tohex(rnd);
			string hx = "0x";
			hx.append(rnd);
			num_pr = String::znatural(hx);
		}
		calc = num_pr.first;
		long double lowv = low_endpoint;          //C$14
		long double high = high_endpoint;         //C$15
		long double decp = pow(10,precision);	  //C$17
		long double r = calc/dbase;               //0.84036
//		       = C$14+FLOOR(R*(C$17*(C$15-C$14)+1),1)/C$17
//		       = lowv+floor(r*(decp*(high-lowv)+1.0))/decp
		
		result = lowv+floor(r*(decp*(high-lowv)+1.0))/decp;
	} else {
		*Logger::log <<  Log::error << Log::LI << "Error. Instruction operation random requires ssl service. " << errs << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
}
void Instruction::call_system(std::string& cmd) {
	Environment* env = Environment::service();
	string command(env->ScriptsDir());
	string errmsg("");
	if (!command.empty()) {
		String::trim(cmd);
		if ( ! cmd.empty() ) {
			pair<string,string> command_parms;
			if (! String::split(' ',cmd,command_parms)) { //just a command.
				command_parms.first = cmd;
				command_parms.second = "";
			}
			const char sn[]="_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.";
			size_t endcmdpos = command_parms.first.find_first_not_of(sn);
			if (endcmdpos != string::npos) {
				*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell The shell command must be alphanumeric (and _ or .)" << Log::LO;
				*Logger::log << Log::LI << command_parms.first << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			} else {
				//base 64 with spaces plus _./
				const char pm[]="._ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=+/ ";
				size_t endprmpos = command_parms.second.find_first_not_of(pm);
				if (endprmpos != string::npos) {
					*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell.";
					*Logger::log << "The shell parameter list must only be base64 characters and spaces, periods and slashes." << Log::LO;
					*Logger::log << Log::LI << command_parms.second << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {
					command.append(command_parms.first);
					FileUtils::File cfile(command);
					if (cfile.exists()) {
						int res = 0;
						string resultfile;
						resultfile= env->ScratchDir();
						resultfile.append( env->ScratchName());
						resultfile.append("obyx_rslt");
						if ( command_parms.second.empty() || (command_parms.second.size() <= 128000) ) { //128000 is basically ARG_MAX. Also See MAX_ARG_STRLEN
							if (!command_parms.second.empty()) {
								command.append(" ");
								command.append(command_parms.second);
							}
							command.append(" > ");
							command.append(resultfile);
							res = system(command.c_str());
							if (res != 0) { errmsg = strerror(errno); }
						} else {
							string sourcefile;
							ostringstream cmd;
							sourcefile = env->ScratchDir();
							sourcefile.append( env->ScratchName());
							sourcefile.append("obyx_srce");
							FileUtils::File srce(sourcefile);
							srce.writeFile(command_parms.second);
							cmd << command << "<" << sourcefile << " > " << resultfile;
							command = cmd.str();
							res = system(command.c_str());
							if (res != 0) { errmsg = strerror(errno); }
							if (srce.exists()) {
								srce.removeFile();
							}
						}
						FileUtils::File file(resultfile);
						if (file.exists()) {
							long long flen = file.getSize();
							if ( flen != 0 ) {
								string ffile;
								file.readFile(ffile);
								results.append(ffile,di_auto);
							}
							file.removeFile();
						}
						if (res != 0) {
							*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell.";
							*Logger::log << " The script " << command_parms.first << " returned error " << errmsg << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
					} else {
						*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell.";
						*Logger::log << "the script " << command << " does not exist." << Log::LO;
						trace();
						*Logger::log << Log::blockend;
					}
				}				
			}
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell. The OBYX_SCRIPTS_DIR environment must be set." << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
}
void Instruction::addInputType(InputType* i) {
	if (i->wotzit == input) {
		inputs.push_back(i);
	} else {
		*Logger::log << Log::error << Log::LI << "Error. Instruction only accepts inputs." << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	}
}
void Instruction::addDefInpType(DefInpType*) {
	*Logger::log << Log::error << Log::LI << "Error. Instruction only accepts inputs." << Log::LO; 
	trace();
	*Logger::log << Log::blockend;
}
Instruction::~Instruction() {
	//outputs/inputs are deleted by Function.
}

void Instruction::init() {
	//static methods - once only (either per main doc, or per process) thank-you very much..
}

void Instruction::finalise() {
}
void Instruction::startup() {
	op_types.insert(op_type_map::value_type(u"add", obyx::add));
	op_types.insert(op_type_map::value_type(u"append", obyx::append));
	op_types.insert(op_type_map::value_type(u"arithmetic", obyx::arithmetic));
	op_types.insert(op_type_map::value_type(u"assign", obyx::move));
	op_types.insert(op_type_map::value_type(u"bitwise", obyx::bitwise));
	op_types.insert(op_type_map::value_type(u"divide", obyx::divide));
	op_types.insert(op_type_map::value_type(u"function", obyx::function));
	op_types.insert(op_type_map::value_type(u"hmac", obyx::hmac));
	op_types.insert(op_type_map::value_type(u"kind", obyx::kind));
	op_types.insert(op_type_map::value_type(u"left", obyx::left));
	op_types.insert(op_type_map::value_type(u"length", obyx::length));
	op_types.insert(op_type_map::value_type(u"lower", obyx::lower));
	op_types.insert(op_type_map::value_type(u"max", obyx::maximum ));
	op_types.insert(op_type_map::value_type(u"min", obyx::minimum ));
	op_types.insert(op_type_map::value_type(u"multiply", obyx::multiply));
	op_types.insert(op_type_map::value_type(u"position", obyx::position));
	op_types.insert(op_type_map::value_type(u"query", obyx::query_command));
	op_types.insert(op_type_map::value_type(u"quotient", obyx::quotient ));
	op_types.insert(op_type_map::value_type(u"random", obyx::random));
	op_types.insert(op_type_map::value_type(u"remainder", obyx::remainder ));
	op_types.insert(op_type_map::value_type(u"reverse", obyx::reverse));
	op_types.insert(op_type_map::value_type(u"right", obyx::right));
	op_types.insert(op_type_map::value_type(u"shell", obyx::shell_command));
	op_types.insert(op_type_map::value_type(u"sort", obyx::sort));
	op_types.insert(op_type_map::value_type(u"substring", substring));
	op_types.insert(op_type_map::value_type(u"transliterate", obyx::transliterate));
	op_types.insert(op_type_map::value_type(u"subtract", obyx::subtract));
	op_types.insert(op_type_map::value_type(u"unique", obyx::unique));
	op_types.insert(op_type_map::value_type(u"upper", obyx::upper));
}
void Instruction::shutdown() {
	op_types.clear();
}	

//bool XMLObject::sort(const std::string& path,const std::string& sortpath,DataItem*& container,bool node_expected,std::string& error_str) {
