/* 
 * iko.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * iko.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <deque>
#include <string>
#include <xercesc/dom/DOMNode.hpp>

#include "commons/date/date.h"
#include "commons/environment/environment.h"
#include "commons/filing/filing.h"
#include "commons/httpfetch/httpfetch.h"
#include "commons/httphead/httphead.h"
#include "commons/logger/logger.h"
#include "commons/vdb/vdb.h"
#include "commons/xml/xml.h"

#include "comparison.h"
#include "obyxelement.h"
#include "document.h"
#include "function.h"
#include "iko.h"
#include "inputtype.h"
#include "iteration.h"
#include "mapping.h"
#include "output.h"
#include "strobject.h"
#include "xmlobject.h"
#include "itemstore.h"
#include "osiapp.h"

using namespace Log;
using namespace qxml;
using namespace Fetch;

enc_type_map IKO::enc_types;
inp_type_map IKO::ctx_types;
kind_type_map IKO::kind_types;
current_type_map IKO::current_types;

IKO::IKO(xercesc::DOMNode* const& n,ObyxElement* par, elemtype el) : 
	ObyxElement(par,el,parm,n),kind(di_auto),encoder(e_none),context(immediate),
	process(qxml::encode),wsstrip(true),exists(false),name_v() {
	u_str str_context;
	if ( XML::Manager::attribute(n,UCS2(L"context"),str_context) ) {
		inp_type_map::const_iterator j = ctx_types.find(str_context);
		if( j != ctx_types.end() ) {
			context = j->second; 
		} else {
			string err_msg; transcode(str_context.c_str(),err_msg);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " <<  err_msg << " is not a legal context space. It should be one of " ;
			*Logger::log << "none, store, field, sysparm, sysenv, cookie, file, url, parm." << Log::LO; 
			trace();
			*Logger::log << Log::blockend;
		}
	}
	if (context == immediate) exists=true;

	u_str str_encoder,str_process,str_kind;
	XML::Manager::attribute(n,UCS2(L"kind"),str_kind);
	XML::Manager::attribute(n,UCS2(L"encoder"),str_encoder);
	XML::Manager::attribute(n,UCS2(L"process"),str_process);

	if ( ! str_kind.empty() ) {
		kind_type_map::const_iterator j = kind_types.find(str_kind);
		if( j != kind_types.end() ) {
			kind = j->second; 
		} else {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Kind value not recognised: needs to be one of:auto text object" << Log::LO;	
			trace();
			*Logger::log << Log::blockend;
		}
	}
		
	if ( ! str_encoder.empty() ) {
		enc_type_map::const_iterator j = enc_types.find(str_encoder);
		if( j != enc_types.end() ) {
			encoder = j->second; 
		} else {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Encoder value not recognised: needs to be one of: none, name, digits, sql, xml, url, base64" << Log::LO;	
			trace();
			*Logger::log << Log::blockend;
		}
	}

	if ( ! str_process.empty() ) {
		if( str_process.compare(UCS2(L"decode")) == 0 ) {
			switch (encoder) {
				case e_name:
				case e_digits:
				case e_none: 
				case e_sql: {
					encoder = e_none;
					string err_msg; transcode(str_encoder.c_str(),err_msg);
					*Logger::log << Log::syntax << Log::LI << "Syntax Error. process='decode' is not supported for encoder='" << err_msg << "'." << Log::LO;	
					trace();
					*Logger::log << Log::blockend;
				} break;
				case e_message:
				case e_qp: 
				case e_xml:
				case e_url: 
				case e_base64: 
				case e_hex:	{
					process = qxml::decode; 					
				} break;
			}
		} else {
			if (str_process.compare(UCS2(L"encode")) !=0) {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. process not recognised: needs to be one of: encode,decode" << Log::LO;	
				trace();
				*Logger::log << Log::blockend;
			} else {
				if  (encoder == e_qp || encoder == e_message ) {
					encoder = e_none;
					string err_msg; transcode(str_encoder.c_str(),err_msg);
					*Logger::log << Log::syntax << Log::LI << "Syntax Error. process='encode' is not supported for encoder='" << err_msg << "'." << Log::LO;	
					trace();
					*Logger::log << Log::blockend;
				} 
			}
		}
	} 
	u_str svalue,wsstrp;
	bool has_val_attr = Manager::attribute(n,UCS2(L"value"),svalue);
	if (XML::Manager::attribute(n,UCS2(L"wsstrip"),wsstrp)) {
		if ( wsstrp.compare(UCS2(L"false")) == 0 ) { 
			wsstrip = false;
		} else {
			if ( wsstrp.compare(UCS2(L"true")) == 0 ) { 
				wsstrip = true;
			} else {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. wsstrip must be either 'true' or 'false'." << Log::LO;	
				trace();
				*Logger::log << Log::blockend;
			}
		}
	}
	if ( has_val_attr ) {	//wsstrip is ignored when value attribute is used.
		wsstrip = false;
	}
	if ( has_val_attr ) {
		DataItem* attrval = NULL;
		attrval = DataItem::factory(svalue,di_text);
		results.setresult(attrval, wsstrip);
	}
}

IKO::IKO(ObyxElement* par,const IKO* orig) : ObyxElement(par,orig),
	kind(orig->kind),encoder(orig->encoder),context(orig->context),
	process(orig->process),wsstrip(orig->wsstrip),exists(orig->exists),name_v(orig->name_v) {
}

bool IKO::currentenv(const string& req,const usage_tests exist_test, const IKO* iko,DataItem*& container) {
	bool exists = false;
	string result;
	container = NULL;
	current_type_map::const_iterator j = current_types.find(req);
	if( j != current_types.end() ) {
		switch (j->second) {
			case c_vnumber: {
				exists = Environment::getenv("OBYX_VERSION_NUMBER",result);
				container = DataItem::factory(result,di_text);
			} break;
			case c_version: {
				exists = Environment::getenv("OBYX_VERSION",result);
				container = DataItem::factory(result,di_text);
			} break;
			case c_object: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						if (iko != NULL) {
							const xercesc::DOMDocument* self = iko->owner->doc();
							container = DataItem::factory(self,di_object);
						} else {
							container = DataItem::factory(req,di_text); //hmm
						}
					} break;
				}
			} break;
			case c_name: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						pair <string,string> namepair;
						string name;
						if (iko != NULL) {
							name = iko->owner->currentname();
						} else {
							name = Document::currentname();
						}
						String::split('#',name,namepair);
						container = DataItem::factory(namepair.first,di_text);
					} break;
				}
			} break;
			case c_time: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						DateUtils::Date::getUTCTimeOfDay(result);
						container = DataItem::factory(result,di_text);
					} break;
				}
			} break;
			case c_timing: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						Environment::gettiming(result);
						container = DataItem::factory(result,di_text);
					} break;
				}
			} break;
			case c_point: {
				exists = true;
				result = String::tostring(ObyxElement::eval_count);
				container = DataItem::factory(result,di_text);
			} break;
			case c_request: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						result = Document::currenthttpreq();
						container = DataItem::factory(result,di_object);
					} break;
				}
			} break;
			case c_response: {
				result = OsiAPP::last_osi_response();
				exists = ! result.empty();
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: 
					case ut_value: {
						container = DataItem::factory(result,di_object);
					} break;
				}
			} break;
			case c_cookies: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						Environment::do_response_cookies(result);
						container = DataItem::factory(result,di_object);
					} break;
				}
			} break;
			case c_http: {
				exists = true;
				switch (exist_test) {
					case ut_existence: break;
					case ut_significant: {
						container = DataItem::factory(req,di_text);
					} break;
					case ut_value: {
						Httphead::explain(result);
						container = DataItem::factory(result,di_object);
					} break;
				}
			} break;
		}
	} else {
		exists = Environment::getenv("CURRENT_" + req,result);		
		switch (exist_test) {
			case ut_existence: break;
			case ut_significant: 
			case ut_value: {
				container = DataItem::factory(req,di_text);
			} break;
		}
	}
	return exists;	
}

void IKO::process_encoding(DataItem*& basis) {
	if (basis != NULL && encoder != e_none) {
		string encoded = *basis;		//xml cannot survive an encoding.
		delete basis;					//now it is no longer.
		basis = NULL;					//default for non-implemented encodings.
		switch ( encoder ) {
			case e_message: {
				if ( process == encode) {
					*Logger::log << Log::error << Log::LI << "Error. message encoding is not yet explicitly implemented." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {
					ostringstream msgres;
					OsiAPP::compile_message(encoded,msgres,true);
					basis = DataItem::factory(msgres.str(),di_object); //always xml..
				}
			} break;
			case e_qp: {
				if ( process == encode) {
					*Logger::log << Log::error << Log::LI << "Error. qp (quoted-printable) encoding is not yet implemented." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {
					String::qpdecode(encoded);
					basis = DataItem::factory(encoded,kind); //cannot be xml..
				}
			} break;
			case e_url: {
				if ( process == encode) {
					String::urlencode(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml..
				} else {
					String::urldecode(encoded);
					basis = DataItem::factory(encoded,kind); //MAY be XML - maybe not.
				}
			} break;
			case e_xml: {
				if ( process == encode) {
					XMLChar::encode(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml..
				} else {
					String::xmldecode(encoded);
					basis = DataItem::factory(encoded,kind); //MAY be XML - maybe not.
				}
			} break;
			case e_base64: {
				if ( process == encode) {
					String::base64encode(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml..
				} else {
					//rfc2045:  "All line breaks or other characters not found in Table 1 must be ignored by decoding software"
					String::base64decode(encoded);
					basis = DataItem::factory(encoded,kind); //MAY be XML - maybe not.
				}
			} break;
			case e_hex: {
				if ( process == encode) {
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml..
				} else {
					string errstr;
					if (String::fromhex(encoded,errstr)) { //false = failed to decode.
						basis = DataItem::factory(encoded,kind); //MAY be XML - maybe not.
					} else {
						*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', hex decoding failed. " << errstr << Log::LO;
						trace();
						*Logger::log << Log::blockend;
						basis = NULL;
					}
				}
			} break;
			case e_sql: {
				if (dbc != NULL)  {
					dbc->escape(encoded);
					basis = DataItem::factory(encoded,di_text); //MAY be XML - maybe not.
				} else {
					*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', sql encoding depends on there being an sql connection, and there isn't one." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
					basis = NULL;
				}
			} break;
			case e_name: {
				String::nameencode(encoded);
				basis = DataItem::factory(encoded,di_text); //cannot be xml.
			} break;
			case e_digits: { //strip out anything that isn't a digit...
				String::todigits(encoded);
				basis = DataItem::factory(encoded,di_text); //cannot be xml.
			} break;
			default: break;
		}
	}
}

//evaltype() is used for evaluating BOTH inputs proper and also contexts for inputs and outputs.
//exists is evaluated. significant is tested by comparision and will be looking for a value.
//the basic logic for existence / significance is..
/*
exists = type.exists();
if (exist_test != ut_existence) {
	if (exists) { 
		if (exist_test == significant) {
			return a text value.
		} else {
			return value (ikind)
		}
	} else {
	 if (exist_test == value) {
		 post an error.
	}
}

*/
bool IKO::evaltype(inp_type the_space, bool release, bool eval,kind_type ikind,DataItem*& name_item, DataItem*& container) {
	exists = false; 
	if (container != NULL) {
		*Logger::log << Log::error << Log::LI << "Internal Error. container should be empty!" << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	}
	bool finished = true;
	switch ( the_space ) { //two switches. to make things a bit faster.
		case immediate: { 
			//everything exists in space 'immediate'
			exists = true; 
			container = name_item;
			name_item = NULL; 
		} break;
		case none: {
			// nothing exists in space 'none'
			exists = false;
			delete name_item;
			name_item = NULL; 
		} break;
		default: { //all the remaining input spaces have a name to access their value.
			u_str input_name;  
			if (name_item != NULL) {
				input_name = *name_item;
			}
			//Also, it maybe that this is an existence test.
			usage_tests exist_test = ut_value; 
			if (the_space != context) { //if evaluating this IKO's context, then don't worry about exist_test. yet!
				name_v = input_name;
				Comparison* cmp = dynamic_cast<Comparison *>(p);
				if ((cmp != NULL) && (wotzit == qxml::comparate)) {
					switch (cmp->op()) {
						case qxml::exists: exist_test=ut_existence; break;
						case qxml::significant: exist_test= ut_significant; break;
						default: break; // already value. 
					}
				} else {
					if (wotzit == control) {
						Iteration* ite = dynamic_cast<Iteration *>(p);
						if ( (ite != NULL) && ((ite->op() == qxml::it_while) || (ite->op() == qxml::it_while_not))) {
							exist_test = ut_existence;
						}
					}
				}
			}
			switch ( the_space ) { //now do all the named input_spaces!
				case field: {
					if ( input_name.empty() ) {  
						finished = true;
						*Logger::log << Log::error << Log::LI << "Error. Field instructions need a field reference." << Log::LO; 
						trace();
						*Logger::log << Log::blockend;
					} else {
						std::string errstring; 
						const ObyxElement* par = p;
						const Iteration* ite = dynamic_cast<const Iteration *>(par);
						const Mapping* mpp = dynamic_cast<const Mapping *>(par);
						finished = false;
						while (par != NULL && !finished ) {
//							errstring = "";
							if (ite != NULL && !ite->active()) {
								ite = NULL;
							}
							if (mpp != NULL && !mpp->active()) {
								mpp = NULL;
							}
							if (ite != NULL) {
								std::string fresult; 
								std::string fname; transcode(input_name.c_str(),fname);
								exists = ite->fieldexists(fname,errstring); //errstring=empty = ok.
								if (exists && exist_test != ut_existence) {
									finished = true;
									if (ite->field(fname,fresult,errstring)) {
										if (exist_test == ut_significant) {
											container = DataItem::factory(fresult,di_text);
										} else {
											container = DataItem::factory(fresult,ikind);
										}
									} else {
										if (errstring.empty()) {
											errstring="Unknown problem while retrieving field.";
										}
										*Logger::log << Log::error << Log::LI << "Error. Field " << fname << " " << errstring << Log::LO;
										trace();
										*Logger::log << Log::blockend;
									}
								} else { //doesn't exist - move up - if we can, and there's no error string..
									if (par != NULL) {
										ite = NULL; // try the next iteration up...
									}
								}
							} else {
								if (mpp != NULL) {
									std::string fname; transcode(input_name.c_str(),fname);
									std::string fresult;
									exists = mpp->field(fname,fresult);
									if (exists) {
										finished = true;
										if (exist_test == ut_significant) {
											container = DataItem::factory(fresult,di_text);
										} else {
											container = DataItem::factory(fresult,ikind);
										}
									} else { //doesn't exist - move up - if we can, and there's no error string..
										if (par != NULL) {
											mpp = NULL; // try the next iteration up...
										}
									}
								} 
							}
							if (par != NULL && !finished) {
								par = par->p;
								ite = dynamic_cast<const Iteration *>(par);
								mpp = dynamic_cast<const Mapping *>(par);
							}														
						}
						if ((!finished && exist_test == ut_value) || (par == NULL && !errstring.empty())) {
							*Logger::log  << Log::error;
							if (!errstring.empty()) {
								std::string err_msg; transcode(input_name.c_str(),err_msg);
								*Logger::log << Log::LI << "Error. Field " << err_msg << " has an error. " << errstring << Log::LO;
							} else {
								std::string err_msg; transcode(input_name.c_str(),err_msg);
								*Logger::log << Log::LI << "Error. Field " << err_msg << " does not exist or is not available." << Log::LO;
							}
							trace();
							*Logger::log << Log::blockend;
						}
						finished = true;
					}
				} break;
				case xmlnamespace: { 
					if ( exist_test != ut_existence ) {
						exists = ItemStore::nsexists(name_item,false);
						if (exists) {
							ItemStore::getns(name_item,container,release);
						} else {
							if (exist_test == ut_value) {
								std::string err_msg; transcode(input_name.c_str(),err_msg);
								*Logger::log << Log::error << Log::LI << "Error. Namespace " << err_msg <<  " does not exist " << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					} else {
						exists = ItemStore::nsexists(name_item,release);
					}
				} break;
				case xmlgrammar: {
					if ( exist_test != ut_existence ) {
						exists = ItemStore::grammarexists(name_item,false);
						if (exists) {
							ItemStore::getgrammar(name_item,container,kind,release);
						} else {
							if (exist_test == ut_value) {
								std::string err_msg; transcode(input_name.c_str(),err_msg);
								*Logger::log << Log::error << Log::LI << "Error. " <<  name() << " Grammar " << err_msg <<  " does not exist " << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					} else {
						exists = ItemStore::grammarexists(name_item,release);
					}
				} break;
				case store: {
					string errstring;
					if ( exist_test != ut_existence ) {
						exists = ItemStore::get(name_item,container,release,errstring);
						if (!errstring.empty()) {
							std::string err_msg; transcode(input_name.c_str(),err_msg);
							*Logger::log << Log::error << Log::LI << "Error. Store " << err_msg  << " "  << errstring << Log::LO;	
							trace();
							*Logger::log << Log::blockend;
						} else {
							if (exist_test == ut_value && !exists) {
								std::string err_msg; transcode(input_name.c_str(),err_msg);
								*Logger::log << Log::error << Log::LI << "Error. Store " << err_msg  << " does not exist."  << Log::LO;	
								trace();
								*Logger::log << Log::blockend;
							}		
						}
					} else {
						exists = ItemStore::exists(name_item,release,errstring); //looking for a name!
						if (!errstring.empty()) {
							std::string err_msg; transcode(input_name.c_str(),err_msg);
							*Logger::log << Log::error << Log::LI << "Error. Store " << err_msg  << " "  << errstring << Log::LO;	
							trace();
							*Logger::log << Log::blockend;
						}
					}
				} break;
				case fnparm: {
					const DataItem* ires = NULL;
					if (! owner->getparm(input_name,ires) ) {
						exists  = false;
						if (exist_test == ut_value) {
							std::string err_msg; transcode(input_name.c_str(),err_msg);
							*Logger::log << Log::error << Log::LI << "Error. Parm " << err_msg  << " does not exist in this context."  << Log::LO;	
							trace();
							*Logger::log << Log::blockend;
						}
					} else {
						exists  = true;	//prob this one
						if (exist_test != ut_existence && ires != NULL) { //empty parms are existing parms..
							ires->copy(container);
						}
					}
				} break;					
				case file: {
					std::string file_path; transcode(input_name.c_str(),file_path);
					if (file_path[0] == '/') { //we don't want to use file root, but site root.
						file_path = Environment::getpathforroot()  + file_path;
					} else {
						string opath = owner->filepath;
						size_t pathpos = opath.find_last_of('/');
						if (pathpos != string::npos) {
							file_path = opath.substr(0,pathpos + 1) + file_path;
						} else {
							file_path = FileUtils::Path::wd() + "/" + file_path;
						}
					}
					string orig_wd(FileUtils::Path::wd());
					FileUtils::Path destination; 
					destination.cd(file_path);
					std::string dest_out = destination.output(true);
					transcode(dest_out,name_v);			//Really not sure what is going on here.
					FileUtils::File file(dest_out);
					exists = file.exists();				//used by comparison to test...
					if ( exist_test != ut_existence ) {
						if (exists) {
							off_t flen = file.getSize();
							if ( flen != 0 ) {
								string file_content;
								file.readFile(file_content);
								if ( !file_content.empty() ) { //errors should be caught outside of this.
									if (exist_test == ut_significant) {
										container = DataItem::factory(file_content,di_text); //no test for xml
									} else {
										ostringstream* docerrs = NULL;
										docerrs = new ostringstream();
										Logger::set_stream(docerrs);
										container = DataItem::factory(file_content,ikind);   //test for xml!!
										Logger::unset_stream();
										string errs = docerrs->str();
										delete docerrs; docerrs=0;
										if ( ! errs.empty() ) {
											*Logger::log << Log::error << Log::LI << "Error with file " << dest_out << Log::LO;
											trace();
											*Logger::log << Log::LI << Log::RI << errs << Log::RO << Log::LO;
											*Logger::log << Log::blockend;
										}
									}
								}
							} // else container remains null.
						} else {
							if (exist_test == ut_value) {
								string root(Environment::getpathforroot());
								string wd(FileUtils::Path::wd());
								if ( wd.find(root) == 0 ) {
									wd.erase(0,root.length());
									if ( wd.empty() ) wd = "/";
								}
								std::string err_msg; transcode(name_v.c_str(),err_msg);
								*Logger::log << Log::error << Log::LI << "Error. File " << err_msg << " does not exist. wd:" << wd << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					}
					destination.cd(orig_wd);
				} break;
				case url: {
					if (HTTPFetch::available()) {
						std::string input_url; transcode(input_name.c_str(),input_url);
						string fresult, errstr;
						HTTPFetch pr(errstr);
						HTTPFetchHeader header;
						std::vector<std::string> redirects;
						exists = pr.fetchPage(input_url, header, redirects, fresult, errstr);
						if ( exists ) {
							if (! fresult.empty()) {
								if (exist_test == ut_significant) {
									container = DataItem::factory(fresult,di_text); //don't want to parse for sig.
								} else {
									ostringstream* docerrs = NULL;
									docerrs = new ostringstream();
									Logger::set_stream(docerrs);
									container = DataItem::factory(fresult,ikind);   //test for xml!!
									Logger::unset_stream();
									string errs = docerrs->str();
									delete docerrs; docerrs=0;
									if ( ! errs.empty() ) {
										*Logger::log << Log::error << Log::LI << "Error with url " << input_url << Log::LO;
										trace();
										*Logger::log << Log::LI << Log::RI << errs << Log::RO << Log::LO;
										*Logger::log << Log::blockend;
									}
								}
							}
						} else {
							if (exist_test == ut_value) {
								if (errstr.empty()) errstr = " failed.";
								*Logger::log << Log::error << Log::LI << "Error. Url " << input_url << " " << errstr << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					} else {
						*Logger::log << Log::error << Log::LI << "Error. Url requires libcurl and this was not found. ";
						if (Environment::envexists("OBYX_LIBCURLSO")) {
							string val; Environment::getenv("OBYX_LIBCURLSO",val); 
							*Logger::log << "The environment 'OBYX_LIBCURLSO' of '" << val << "' is not working.";
						} else {
							*Logger::log << "Set the environment 'OBYX_LIBCURLSO'.";
						}
						*Logger::log << Log::LO;
						trace();
						*Logger::log << Log::blockend;
						finished = false;
					}
				} break;
				case cookie: {
					std::string cookie_name; transcode(input_name.c_str(),cookie_name); //This really should be converted - being internal.
					string cookie_value;
					exists = Environment::getcookie_req(cookie_name,cookie_value);
					if( exist_test != ut_existence  ) {
						if( exists ) {
							if (exist_test == ut_significant) {
								container = DataItem::factory(cookie_value,di_text); //no test for xml!!
							} else {
								container = DataItem::factory(cookie_value,ikind); //test for xml if needs be.	
							}
						} else { // it doesn't exist...
							if( exist_test == ut_value) {
								*Logger::log << Log::error << Log::LI << "Error. Cookie " << cookie_name << " does not exist." << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							} // else it's significant - and we are ok.
						}
					}
				} break;
				case sysparm: {
					string fresult;
					std::string sysparm_name; transcode(input_name.c_str(),sysparm_name); //This really should be converted - being internal.
					exists = Environment::getparm(sysparm_name,fresult);
					if ( exist_test != ut_existence ) {
						if ( exists ) {
							if (! fresult.empty()) {
								if (exist_test == ut_significant) {
									container = DataItem::factory(fresult,di_text); //no test for xml!!
								} else {
									container = DataItem::factory(fresult,ikind); //test for xml if needs be.	
								}
							}
						} else { 
							if (exist_test == ut_value) {
								*Logger::log << Log::error << Log::LI << "Error. Sysparm " << sysparm_name << " does not exist." << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					}
				} break;
				case sysenv: {
					std::string sysenv_name; transcode(input_name.c_str(),sysenv_name); //This really should be converted - being internal.
					string fresult;
					string errmsg = "does not exist.";
					if ( sysenv_name.find("OBYX_",0,5) == string::npos) {
						if ( sysenv_name.find("CURRENT_",0,8) == string::npos) { //it's something.
							exists = Environment::getenv(sysenv_name,fresult);
						} else { //it's a CURRENT_
							exists = currentenv(sysenv_name.substr(8,string::npos),exist_test,this,container);
						}
					} else { //it's an OBYX_
						if ( sysenv_name.compare("OBYX_VERSION") == 0 ) {
							errmsg = "is restricted. Use CURRENT_VERSION instead";
						} else {
							errmsg = "is restricted.";
						}
					} 
					if (exist_test != ut_existence) {
						if (exists) { 
							if (!fresult.empty() && container == NULL) { //container test because of CURRENT_OBJECT.
								if (exist_test == ut_significant) {
									container = DataItem::factory(fresult,di_text);
								} else {
									container = DataItem::factory(fresult,ikind);
								}
							}
						} else {
							if (exist_test == ut_value) {
								*Logger::log << Log::error << Log::LI << "Error. Sysenv " << sysenv_name << " " << errmsg << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						}
					}
				} break;
				case qxml::error: {
					exists = true;
					std::string err_msg; transcode(input_name.c_str(),err_msg); //This really should be converted - being internal.
					*Logger::log << Log::warn << Log::LI << "Break  '" << err_msg << "'" << Log::LO;
					*Logger::log << Log::LI << Log::notify ;
					trace();
					*Logger::log << Log::blockend << Log::LO;
					*Logger::log << Log::LI ;
					Environment::list();
					owner->list();
					ItemStore::list();
					Iteration::list(this);		//available fields from here.
					*Logger::log << Log::LO << Log::blockend;
				} break;
				default: { //uncaught input_spaces will hit here.
					exists = false;
					*Logger::log << Log::error << Log::LI << "Error. Internal Error. Should not reach here." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} break;
			}			
		}
	}
	if (finished && eval) {
		if (container != NULL ) {
			string filestring,dirstring;
			if (! name_v.empty() ) { 
				transcode(name_v.c_str(),filestring);
				dirstring = filestring;
			}
			if (the_space == file) { //push working directory.
				if ( dirstring.rfind('/') != string::npos ) {
					dirstring = dirstring.substr(0,dirstring.rfind('/'));
					if ( !dirstring.empty() ) {
						FileUtils::Path::push_wd(dirstring);
					}
				}
			}
			DataItem* doc_to_eval = container; //We don't want to write over ourselves!
			container = NULL;
			Document eval_doc(doc_to_eval,Document::File,filestring,this); //evaluate immediately!
			if (eval_doc.results.final()) { 
				eval_doc.results.takeresult(container); //
			} else {
				*Logger::log << Log::error << Log::LI << "Error. File " << filestring << " was not evaluated " << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
			if (the_space == file && !dirstring.empty() ) { FileUtils::Path::pop_wd(); }
		} else {
			*Logger::log << Log::error << Log::LI << "Error. eval cannot be applied to an empty value." << Log::LO;
			trace();
			*Logger::log << Log::blockend;
		}
	}
	return finished;
}

void IKO::init() {
	current_types.insert(current_type_map::value_type("OBJECT",c_object));
	current_types.insert(current_type_map::value_type("NAME",c_name));
	current_types.insert(current_type_map::value_type("REQUEST",c_request));
	current_types.insert(current_type_map::value_type("OSI_RESPONSE",c_response));
	current_types.insert(current_type_map::value_type("TIMING",c_timing));
	current_types.insert(current_type_map::value_type("TIME",c_time));
	current_types.insert(current_type_map::value_type("VERSION",c_version));
	current_types.insert(current_type_map::value_type("VERSION_NUMBER",c_vnumber));
	current_types.insert(current_type_map::value_type("HTTP",c_http));
	current_types.insert(current_type_map::value_type("POINT",c_point));
	current_types.insert(current_type_map::value_type("COOKIES",c_cookies));
	
	kind_types.insert(kind_type_map::value_type(UCS2(L"auto"), di_auto));
	kind_types.insert(kind_type_map::value_type(UCS2(L"text"), di_text));
	kind_types.insert(kind_type_map::value_type(UCS2(L"object"), di_object));

	enc_types.insert(enc_type_map::value_type(UCS2(L"none"), e_none));
	enc_types.insert(enc_type_map::value_type(UCS2(L"qp"), e_qp));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sql"), e_sql));
	enc_types.insert(enc_type_map::value_type(UCS2(L"xml"), e_xml));
	enc_types.insert(enc_type_map::value_type(UCS2(L"url"), e_url));
	enc_types.insert(enc_type_map::value_type(UCS2(L"name"), e_name));
	enc_types.insert(enc_type_map::value_type(UCS2(L"digits"), e_digits));
	enc_types.insert(enc_type_map::value_type(UCS2(L"base64"), e_base64));
	enc_types.insert(enc_type_map::value_type(UCS2(L"hex"), e_hex));
	enc_types.insert(enc_type_map::value_type(UCS2(L"message"), e_message));

	ctx_types.insert(inp_type_map::value_type(UCS2(L"none"), immediate));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"field"), field ));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"url"), url ));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"file"), file ));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"parm"), fnparm));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"sysparm"), sysparm));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"sysenv"), sysenv));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"cookie"), cookie)); 
	ctx_types.insert(inp_type_map::value_type(UCS2(L"store"), store));
	ctx_types.insert(inp_type_map::value_type(UCS2(L"namespace"), xmlnamespace));
	
	if (Environment::UseDeprecated) {
		ctx_types.insert(inp_type_map::value_type(UCS2(L"object"), store)); 
	}		
}
