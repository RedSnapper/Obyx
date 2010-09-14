/* 
 * instruction.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * instruction.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <cfloat>
#include <xercesc/dom/DOMNode.hpp>

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

using namespace Log;
using namespace XML;
using namespace obyx;

op_type_map Instruction::op_types;

Instruction::Instruction(xercesc::DOMNode* const& n,ObyxElement* par) :
Function(n,instruction,par),operation(move),precision(0),bitpadding(0),base_convert(false),inputsfinal(false) {
	u_str op_string;
	Manager::attribute(n,UCS2(L"operation"),op_string);
	op_type_map::const_iterator i = op_types.find(op_string);
	if( i != op_types.end() ) {
		operation = i->second; 
	} else {
		if ( ! op_string.empty() ) {
			string err_type; transcode(op_string.c_str(),err_type);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_type << " is not a legal instruction operation. It should be one of assign, append, position, substring, length, left, right, upper, lower, add, subtract, multiply, divide, remainder, quotient, shell, query, function." << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
	if ( 
		(operation == obyx::divide) || (operation == obyx::multiply) || 
		(operation == obyx::add) || (operation == obyx::subtract) 
		) {
		std::string str_prec;
		Manager::attribute(n,"precision",str_prec);
		if ( ! str_prec.empty() ) {
			if (str_prec[0] == 'B' || str_prec[0] == 'b') { //2,8,16 (B is for base)
				base_convert = true;
				str_prec.erase(0,1);
				size_t bcpt = str_prec.find('.');
				if (bcpt != string::npos) {
					string bits=str_prec.substr(bcpt+1,string::npos);
					pair<unsigned long long,bool> bits_value = String::znatural(bits);
					if  ( bits_value.second && bits_value.first < 64) {
						bitpadding = (unsigned int)bits_value.first;
					} else {
						*Logger::log << Log::syntax << Log::LI << "Syntax Error. Instruction: bits component of precision must be a number between 1 and 64." << Log::LO;
						trace();
						*Logger::log << Log::blockend;
					}
					str_prec.erase(bcpt,string::npos);
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
operation(orig->operation),precision(orig->precision),bitpadding(orig->bitpadding),base_convert(orig->base_convert),inputsfinal(orig->inputsfinal) {
}
void Instruction::do_function() {
	DataItem* document = NULL;
	inputs[0]->results.takeresult(document);
	Document::type_parm_map* function_instance = new Document::type_parm_map();
	size_t n = inputs.size();
	for ( unsigned int i = 1; i < n; i++ ) {
		u_str pname=inputs[i]->parm_name;
		if ( ! pname.empty()) {
			DataItem* rslt = NULL;
			string parm_key;
			XML::transcode(pname.c_str(),parm_key);

			if ( ! inputs[i]->results.final() ) {
				inputs[i]->evaluate();
				if ( ! inputs[i]->results.final() ) {
					*Logger::log <<  Log::error << Log::LI << "Error. Parameter " << parm_key << " must be able to be evaluated before function is called." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {
					if (inputs[i]->results.result() != NULL) {
						inputs[i]->results.result()->copy(rslt);
					}
				}
			} else {
				if (inputs[i]->results.result() != NULL) {
					inputs[i]->results.result()->copy(rslt);
				}
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
		transcode(inputs[0]->name_v.c_str(),fn_filename);
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
	Document eval_doc(document,Document::File,fn_filename,this,false);
	eval_doc.setparms(function_instance);
	eval_doc.doc_par = owner;
	eval_doc.owner = &eval_doc;
	eval_doc.eval();
	if ( ! eval_doc.results.final()  ) {
		eval_doc.results.normalise();	
		if ( ! eval_doc.results.final()  ) { //if it is STILL not final..
			*Logger::log << Log::error << Log::LI << "Error. Function was not fully evaluated." << Log::LO ;
			trace();
			//			string troubled_doc;
			//			XML::Manager::parser()->writedoc(eval_doc,troubled_doc);
			*Logger::log << Log::LI << "The document that failed is:" << Log::LO;
			*Logger::log << Log::LI << Log::info << Log::LI << document << Log::LO << Log::blockend << Log::LO; 
			*Logger::log << Log::blockend; //Error
			results.clear();
		}
		DataItem* nowt = NULL;
		results.setresult(nowt);
	} else {
		DataItem* doc_result = NULL;
		eval_doc.results.takeresult(doc_result);
		results.setresult(doc_result);
	}
	if(has_own_directory) {
		FileUtils::Path::pop_wd();
	}	
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
		switch (operation) {
			case function: {
				do_function();
			} break;
			case move: {
				if ( n > 1 ) {
					*Logger::log << Log::error << Log::LI << "Error. Operation 'assign' accepts only the first input. Use operation 'append' for multiple input instructions." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
				if ( n > 0 ) {	
					DataItem* srr = NULL;
					inputs[0]->results.takeresult(srr);
					results.setresult(srr);
				}
			} break;
			case kind: {
				if ( n > 1 ) {
					*Logger::log << Log::error << Log::LI << "Error. Operation 'kind' accepts only one input." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
				if ( n > 0 ) {	
					DataItem* srr = NULL;
					string value;
					inputs[0]->results.takeresult(srr);
					if (srr != NULL) {
						if (srr->kind() == di_text) {
							value="text";
						} else {
							value="object";
						}
						delete srr;
						srr=NULL;
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
				DataItem* first_value = NULL;	//used by eg left to hold the initial parameter, against which everything else is evaluated.
				DataItem* srcval = NULL;
				long long iaccumulator = 0;
				std::string accumulator;
				unsigned long long naccumulator = 0;
				double daccumulator = 0;
				for ( size_t i = 0; i < n; i++ ) {
					if ( inputs[i]->wotzit == input ) {					
						inputs[i]->results.takeresult(srcval); //final stuff here - this is always right - see above
						if (firstval) {
							first_value = srcval;
							firstval = false;
							switch ( operation ) {
								case move:
								case kind:
								case function:
									break; //operations handled outside of this switch.
									
								case query_command:		// call_sql(first_value); break;
								case shell_command:	{	// call_system(first_value); break;
									if (first_value != NULL) {
										accumulator.append(*first_value);
									}
								} break;
								case obyx::append: {	//
									results.append(first_value);
								} break;
								case substring:		  // we don't want the first value to be stuck out.
								case position:
								case obyx::left:
								case obyx::right:
									if (first_value != NULL) {
										accumulator = *first_value;
									}
									break;
								case maximum: {
									string fv; if (first_value != NULL) { fv = *first_value; }
									daccumulator = String::real(fv);
									if ( isnan(daccumulator) ) {
										daccumulator = - DBL_MAX;
									}
								} break;
								case minimum: { 
									string fv; if (first_value != NULL) { fv = *first_value; }
									daccumulator = String::real(fv);
									if ( isnan(daccumulator) ) {
										daccumulator =   DBL_MAX;
									}
								} break;
								case divide: 
								case multiply: 
								case obyx::add: 
								case subtract: { 
									string fv; if (first_value != NULL) { fv = *first_value; }
									daccumulator = String::real(fv);
								} break;
								case obyx::upper: {
									string fv; if (first_value != NULL) { fv = *first_value; }
									String::toupper(fv);
									accumulator = fv;
								} break;
								case obyx::lower: {
									string fv; if (first_value != NULL) { fv = *first_value; }
									String::tolower(fv);
									accumulator = fv;
								} break;
								case obyx::reverse: {
									string fv; if (first_value != NULL) { fv = *first_value; }
									String::reverse(fv);
									accumulator = fv;
								} break;
								case obyx::length: {
									string fv; if (first_value != NULL) { fv = *first_value; }
									if (! String::length(fv,naccumulator) ) {
										*Logger::log <<  Log::error << Log::LI << "Error. " << "'" << fv << "' is not a legal UTF-8 string." << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} break;
								case quotient: 
								case obyx::remainder: {
									string fv; if (first_value != NULL) { fv = *first_value; }
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
								case move:
								case kind:
								case function: 
									break; //operations handled outside of this switch.
									
								case query_command: 
								case shell_command:	{	// call_system(first_value); break;
									if (srcval != NULL) {
										accumulator.append(*srcval);
									}
								} break;
								case obyx::append: {
									results.append(srcval);
								} break; //done differently.
								case obyx::upper: {
									string fv; if (srcval != NULL) { fv = *srcval; }
									String::toupper(fv);
									accumulator.append(fv);
								} break;
								case obyx::lower: {
									string fv; if (srcval != NULL) { fv = *srcval; }
									String::tolower(fv);
									accumulator.append(fv);
								} break;
								case obyx::reverse: {
									string fv; if (srcval != NULL) { fv = *srcval; }
									String::reverse(fv);
									accumulator.insert(0,fv);
								} break;
								case substring: {
									string fv; if (srcval != NULL) { fv = *srcval; }
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
											if (first_value != NULL) {
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
									string fv; if (srcval != NULL) { fv = *srcval; }
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
									string fv; if (srcval != NULL) { fv = *srcval; }
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
								case position: {
									unsigned long long strposition=0;
									string fv;
									if (srcval != NULL) {
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
									if (srcval != NULL) {
										fv = *srcval;
										if (! String::length(fv,bacc) ) {
											*Logger::log <<  Log::error << Log::LI << "Error. " << "'" << fv << "' is not a legal UTF-8 string." << Log::LO;
											trace();
											*Logger::log << Log::blockend;
										}
										naccumulator += bacc;
									}
								} break;
								case obyx::add: {
									string rstring;
									if (srcval != NULL) {
										rstring = *srcval;
										daccumulator += String::real(rstring); 
									}
								} break;
								case maximum: {
									string sv; if (srcval != NULL) { sv = *srcval; }
									double tst = String::real(sv);
									if ( ! isnan(tst) ) { //testing for nan..
										daccumulator = daccumulator > tst ? daccumulator : tst; 
									}
								} break;
								case minimum: {
									string sv; if (srcval != NULL) { sv = *srcval; }
									double tst = String::real(sv);
									if ( ! isnan(tst) ) { //testing for nan..
										daccumulator = daccumulator < tst ? daccumulator : tst; 
									}
								} break;
								case subtract: {
									string sv; if (srcval != NULL) { sv = *srcval; }
									daccumulator -= String::real(sv);
								} break;
								case multiply: {
									string sv; if (srcval != NULL) { sv = *srcval; }
									daccumulator *= String::real(sv);
								} break;
								case divide: {
									string sv; if (srcval != NULL) { sv = *srcval; }
									daccumulator /= String::real(sv);
								} break;
								case quotient: {
									if (!failed) {
										long long nsrc = 0;
										string sv; if (srcval != NULL) { sv = *srcval; }
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
										string fv; if (srcval != NULL) { fv = *srcval; }
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
							}
						}
					} else {
						*Logger::log << Log::error << Log::LI << "Error. Instructions may only use Inputs." << Log::LO;
						trace();
						*Logger::log << Log::blockend;
					}
				}
				
				switch ( operation ) {
					case obyx::append:
					case function:
					case move: 
					case kind:
						break;
					case query_command: {
						call_sql(accumulator);
					} break;
					case shell_command: { 
						call_system(accumulator);  
					} break;
					case position: {
						if (n < 2) {
							*Logger::log << Log::error << Log::LI << "Error. Operation 'position' needs two inputs." << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							results.append("NaN",di_text);
						} else {
							results.append(accumulator,di_text);
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
					case obyx::add: 
					case subtract: 
					case multiply: 
					case maximum: 
					case minimum: 
					case divide: {
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
					case quotient: {
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
				}
			} break;
		}
	}
	return inputsfinal;
}
void Instruction::call_sql(std::string& querystring) {
	if ( ! querystring.empty() ) {
		if ( dbs != NULL && dbc != NULL ) {
			Vdb::Query *query = NULL;
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
				*Logger::log << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. Instruction operation query needs an sql service, and there is none." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
}
void Instruction::call_system(std::string& cmd) {
	Environment* env = Environment::service();
	string command(env->ScriptsDir());
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
				//base 64 with spaces plus _
				const char pm[]="_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789=+/ ";
				size_t endprmpos = command_parms.second.find_first_not_of(pm);
				if (endprmpos != string::npos) {
					*Logger::log << Log::error << Log::LI << "Error. Instruction operation shell.";
					*Logger::log << "The shell parameter list must only be base64 characters and spaces." << Log::LO;
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
						if (!command_parms.second.empty()) {
							if ( command_parms.second.size() + command.size() <= 200 ) {
								command.append(" ");
								command.append(command_parms.second);
								command.append(" > ");
								command.append(resultfile);
								res = system(command.c_str());
							} else {
								string sourcefile;
								ostringstream cmd;
								sourcefile = env->ScratchDir();
								sourcefile.append( env->ScratchName());
								sourcefile.append("obyx_srce");
								FileUtils::File srce(sourcefile);
								srce.writeFile(command_parms.second);
								cmd << "cat " << sourcefile << " | " << command << " > " << resultfile;
								command = cmd.str();
								res = system(command.c_str());
								if (srce.exists()) {
									srce.removeFile();
								}
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
							*Logger::log << "the script " << command_parms.first << " returned error " << res << Log::LO;
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
void Instruction::init() {
	//static methods - once only (either per main doc, or per process) thank-you very much..
}
void Instruction::finalise() {
}
void Instruction::startup() {
	op_types.insert(op_type_map::value_type(UCS2(L"add"), obyx::add));
	op_types.insert(op_type_map::value_type(UCS2(L"append"), obyx::append));
	op_types.insert(op_type_map::value_type(UCS2(L"assign"), move));
	op_types.insert(op_type_map::value_type(UCS2(L"divide"), divide));
	op_types.insert(op_type_map::value_type(UCS2(L"function"), function));
	op_types.insert(op_type_map::value_type(UCS2(L"kind"), obyx::kind));
	op_types.insert(op_type_map::value_type(UCS2(L"left"), obyx::left));
	op_types.insert(op_type_map::value_type(UCS2(L"length"), obyx::length));
	op_types.insert(op_type_map::value_type(UCS2(L"lower"), obyx::lower));
	op_types.insert(op_type_map::value_type(UCS2(L"max"), maximum ));
	op_types.insert(op_type_map::value_type(UCS2(L"min"), minimum ));
	op_types.insert(op_type_map::value_type(UCS2(L"multiply"), multiply));
	op_types.insert(op_type_map::value_type(UCS2(L"position"), position));
	op_types.insert(op_type_map::value_type(UCS2(L"query"), query_command));
	op_types.insert(op_type_map::value_type(UCS2(L"quotient"), quotient ));
	op_types.insert(op_type_map::value_type(UCS2(L"remainder"), obyx::remainder ));
	op_types.insert(op_type_map::value_type(UCS2(L"reverse"), obyx::reverse));
	op_types.insert(op_type_map::value_type(UCS2(L"right"), obyx::right));
	op_types.insert(op_type_map::value_type(UCS2(L"shell"), shell_command));
	op_types.insert(op_type_map::value_type(UCS2(L"substring"), substring));
	op_types.insert(op_type_map::value_type(UCS2(L"subtract"), subtract));
	op_types.insert(op_type_map::value_type(UCS2(L"upper"), obyx::upper));	
}
void Instruction::shutdown() {
	op_types.clear();
}	

