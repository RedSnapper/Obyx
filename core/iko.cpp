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

#include "commons/string/strings.h"
#include "commons/date/date.h"
#include "commons/environment/environment.h"
#include "commons/filing/filing.h"
#include "commons/httpfetch/httpfetch.h"
#include "commons/httphead/httphead.h"
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
#include "osimessage.h"

using namespace Log;
using namespace obyx;
using namespace Fetch;

enc_type_map IKO::enc_types;
IKO::inp_space_map IKO::ctx_types;
IKO::current_type_map IKO::current_types;
kind_type_map IKO::kind_types;
Json	IKO::json;


void IKO::log(const Log::msgtype mtype,const std::string msg) const {
	*Logger::log << mtype << Log::LI << msg << Log::LO;
	trace();
	*Logger::log << Log::blockend;
}

enc_type IKO::str_to_encoder(const u_str str_encoder) {
	enc_type result = e_none;
	if ( ! str_encoder.empty() ) {
		enc_type_map::const_iterator j = enc_types.find(str_encoder);
		if( j != enc_types.end() ) {
			result = j->second; 
		}
	}
	return result;
}

IKO::IKO(xercesc::DOMNode* const& n,ObyxElement* par, elemtype el) : ObyxElement(par,el,parm,n),kind(di_auto),encoder(e_none),context(immediate), process(obyx::encode),wsstrip(true),exists(false),name_v(),xpath() {
	u_str str_context;
	if ( XML::Manager::attribute(n,UCS2(L"context"),str_context) ) {
		inp_space_map::const_iterator j = ctx_types.find(str_context);
		if( j != ctx_types.end() ) {
			context = j->second; 
		} else {
			string err_msg; Manager::transcode(str_context.c_str(),err_msg);
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
  	XML::Manager::attribute(n,UCS2(L"xpath"),xpath);
	
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
				case e_md5:
				case e_sha1:
				case e_dss1:
				case e_mdc2:
				case e_ripemd160:
				case e_secret:
				case e_sha256:
				case e_sha224:
				case e_sha384:
				case e_sha512:
				case e_digits:
				case e_none: 
				case e_sql: {
					encoder = e_none;
					string err_msg; Manager::transcode(str_encoder.c_str(),err_msg);
					*Logger::log << Log::syntax << Log::LI << "Syntax Error. process='decode' is not supported for encoder='" << err_msg << "'." << Log::LO;	
					trace();
					*Logger::log << Log::blockend;
				} break;
				case e_json:
				case e_message:
				case e_qp: 
				case e_xml:
				case e_url: 
				case e_10to16: 
				case e_btwoc:					
				case e_base64: 
				case e_deflate:
				case e_hex:	{
					process = obyx::decode; 					
				} break;
			}
		} else {
			if (str_process.compare(UCS2(L"encode")) !=0) {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. process not recognised: needs to be one of: encode,decode" << Log::LO;	
				trace();
				*Logger::log << Log::blockend;
			} else {
				if  (encoder == e_qp ) {
					encoder = e_none;
					string err_msg; Manager::transcode(str_encoder.c_str(),err_msg);
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
		DataItem* attrval = DataItem::factory(svalue,di_utext);
		results.setresult(attrval, wsstrip);
	}
}
IKO::~IKO() { 
//results deleted by ~ObyxElement.
}

IKO::IKO(ObyxElement* par,const IKO* orig) : ObyxElement(par,orig),
	kind(orig->kind),encoder(orig->encoder),context(orig->context), 
	process(orig->process),wsstrip(orig->wsstrip),exists(orig->exists),
	name_v(orig->name_v),xpath(orig->xpath) {}

//bool IKO::legalsysenv(const u_str& envname) const {
bool IKO::currentenv(const u_str& req,const usage_tests exist_test, const IKO* iko,DataItem*& container) {
	Environment* env = Environment::service();
	bool exists = false;
	std::string result;
	container = NULL;
	current_type_map::const_iterator j = current_types.find(req);
	if( j != current_types.end() ) {
		if (exist_test == ut_existence || exist_test == ut_found) {
			if (j->second == c_osi_response) {
				result = OsiAPP::last_osi_response();
				exists = ! result.empty();
			} else {
				exists=true;
			}
		} else {
			exists=true;
			switch (j->second) {
				case c_vnumber: {
					env->getenv("OBYX_VERSION_NUMBER",result);
					container = DataItem::factory(result,di_text);
				} break;
				case c_version: {
					env->getenv("OBYX_VERSION",result);
					container = DataItem::factory(result,di_text);
				} break;
				case c_object: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						if (iko != NULL) {
							const xercesc::DOMDocument* self = iko->owner->doc();
							container = DataItem::factory(self,di_object);
						} else {
							container = DataItem::factory(req,di_text); //hmm
						}
					}
				} break;
				case c_name: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						pair <string,string> namepair;
						string name;
						if (iko != NULL) {
							name = iko->owner->currentname();
						} else {
							name = Document::currentname();
						}
						String::split('#',name,namepair);
						container = DataItem::factory(namepair.first,di_text);
					}
				} break;
				case c_ts: {
					DateUtils::Date::getTS(result);
					container = DataItem::factory(result,di_text);
				} break;
				case c_time: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						DateUtils::Date::getUTCTimeOfDay(result);
						container = DataItem::factory(result,di_text);
					}
				} break;
				case c_timing: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						env->gettiming(result);
						container = DataItem::factory(result,di_text);
					}
				} break;
				case c_point: {
					result = String::tostring(ObyxElement::eval_count);
					container = DataItem::factory(result,di_text);
				} break;
				case c_request: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						result = Document::currenthttpreq();
						container = DataItem::factory(result,di_object);
					}
				} break;
				case c_osi_response: {
					result = OsiAPP::last_osi_response();
					container = DataItem::factory(result,di_object);
					if (container == NULL) {
						log(Log::error,"Error. OSI Response [" + result + "] was malformed.");
						container = DataItem::factory("",di_text);
					}
				} break;
				case c_cookies: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						env->do_response_cookies(result);
						container = DataItem::factory(result,di_object);
					}
				} break;
				case c_response: {
					if (exist_test == ut_significant) {
						container = DataItem::factory(req,di_text);
					} else { //ut_value
						Httphead* http = Httphead::service();	
						http->explain(result);
						container = DataItem::factory(result,di_object);
					}
				} break;
			}
		}
	} else {
		string rqbit; XML::Manager::transcode(req,rqbit);
		string recomp="CURRENT_"+rqbit;
		exists = env->getenv(recomp,result);		
		switch (exist_test) {
			case ut_existence: break;
			case ut_found: {
				if (!exists) {
					string test = recomp;
					exists = env->envfind(recomp); //regex.		
				}
			} break;
			case ut_significant: 
			case ut_value: {
				container = DataItem::factory(req,di_text);
			} break;
		}
	}
	return exists;	
}
void IKO::setfilepath(const string& input_name,string& file_path) const {
	file_path = input_name;
	if (!file_path.empty() && file_path[0] == '/') { //we don't want to use file root, but site root.
		file_path = Environment::service()->getpathforroot()  + file_path;
	} else {
		string opath = owner->filepath;
		size_t pathpos = opath.find_last_of('/');
		if (pathpos != string::npos) {
			file_path = opath.substr(0,pathpos + 1) + file_path;
		} else {
			file_path = FileUtils::Path::wd() + "/" + file_path;
		}
	}
}
bool IKO::legalsysenv(const u_str& envname) const {
	bool legal = false;
	if ( envname.find(UCS2(L"OBYX_"),0,5) != string::npos) {
		string erv; XML::Manager::transcode(envname,erv);		
		if ( envname.compare(UCS2(L"OBYX_VERSION")) == 0 ) {
			log(Log::error,"Error. Sysenv " + erv + " is restricted. Use CURRENT_VERSION instead");
		} else {
			log(Log::error,"Error. Sysenv " + erv + " is restricted.");
		}
	} else { 
		legal = true;
	}
	return legal;
}
bool IKO::httpready() const {
	bool httpgood = true;
	if (!HTTPFetch::available()) {
		httpgood = false;
		*Logger::log << Log::error << Log::LI << "Error. space 'url' requires libcurl and this was not found. ";
		Environment* env = Environment::service();
		if (env->envexists("OBYX_LIBCURLSO")) {
			string val; env->getenv("OBYX_LIBCURLSO",val); 
			*Logger::log << "The environment 'OBYX_LIBCURLSO' of '" << val << "' is not working.";
		} else {
			*Logger::log << "If your libcurl is not in a system library directory, set the environment 'OBYX_LIBCURLSO'.";
		}
		*Logger::log << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
	return httpgood;
}
void IKO::doerrspace(const u_str& input_name) const {
	if (!input_name.empty()) {
		string err_msg; XML::Manager::transcode(input_name,err_msg);		
		if (err_msg.compare(0,6,"fatal#") == 0) {
			break_happened = true;
			err_msg.erase(0,6);
			log(fatal,"Fatal '" + err_msg + "'");
		} else {
			if (err_msg.compare(0,6,"debug#") == 0) {
				Environment* env = Environment::service();
				err_msg.erase(0,6);
				*Logger::log << Log::thrown << Log::LI << "Debug '" << err_msg << "' (" << (unsigned int)ObyxElement::eval_count << ")" << Log::LO;
				trace();
				*Logger::log << Log::LI ;
				env->list();
				owner->list();
				owner->liststore();
				Iteration::list(this);		//available fields from here.
				*Logger::log << Log::LO << Log::blockend;
			} else {
				if (err_msg.compare(0,6,"trace#") == 0) {
					err_msg.erase(0,6);
					*Logger::log << Log::thrown << Log::LI << "Trace '" << err_msg << "'" << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				} else {//we don't want the trace for this.
					*Logger::log << Log::thrown << Log::LI << "Throw '" << err_msg << "'" << Log::LO << Log::blockend;
				}
			}
		}
	} else {
		*Logger::log << Log::thrown << Log::LI << "Throw" << Log::LO << Log::blockend;
	}
}
void IKO::process_encoding(DataItem*& basis) {
	if (basis != NULL && encoder != e_none) {
		string errs,encoded;
		if (!((encoder == e_message && process == encode) || (encoder == e_json && process == decode) )) {
			encoded = *basis;		//xml cannot survive an encoding.
			delete basis;					//now it is no longer.
			basis = NULL;					//default for non-implemented encodings.
		}
		switch ( encoder ) {
			case e_message: {
				if ( process == encode) { //This uses a node rather than a string.
					ostringstream msgres;
					xercesc::DOMDocument* doc = *basis;
					if (doc != NULL) {
						xercesc::DOMNode* node = doc->getDocumentElement();
						OsiMessage::decompile(node,msgres,false,true);
						delete basis; basis = NULL;	doc=NULL;
						basis = DataItem::factory(msgres.str(),di_text); //always xml..
					}
				} else {
					ostringstream msgres;
					OsiMessage msg;
					msg.compile(encoded,msgres,true);
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
			case e_json: {
				string errstr;
				bool ok = false;
				if ( process == encode) { //ie to native format.
					ok = json.encode(&basis,di_text,errstr);
				} else {
					ok = json.decode(&basis,di_object,errstr);
				}
				if (!ok) {
					*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', json failed. " << errstr << Log::LO;
					trace();
					*Logger::log << Log::blockend;
					if (basis != NULL) { 
						delete basis; basis = NULL;
					}
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
			case e_btwoc: {
#ifndef DISALLOW_GMP
				if ( String::Regex::available() ) {
					if ( process == encode) {
						String::Bit::Num e(encoded,10);
						encoded = e.str(16);
						if (encoded.empty()) {
							encoded = "\\x00";
						} else {
							unsigned char first=encoded[0]-'0';
							String::toupper(encoded);
							String::Regex::replace("(..)","\\\\x\\1",encoded,true);
							if (first > 7) {
								encoded = "\\x00" + encoded;
							}
						}
						basis = DataItem::factory(encoded,di_text);
					} else {
						String::fandr(encoded,"\\x");
						String::Bit::Num e(encoded,16);
						encoded = e.str(10);
						if (encoded.empty()) encoded = "0";
						basis = DataItem::factory(encoded,di_text);
					}
				}
#endif
			} break;
			case e_10to16: {
#ifndef DISALLOW_GMP		
				if ( process == encode) {
					String::Bit::Num e(encoded,10);
					encoded = e.str(16);
					if (encoded.empty()) encoded = "00";
					basis = DataItem::factory(encoded,di_text);
					
				} else {
					String::Bit::Num e(encoded,16);
					encoded = e.str(10);
					if (encoded.empty()) encoded = "0";
					basis = DataItem::factory(encoded,di_text);
				}
#endif
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
					basis = DataItem::factory(encoded,di_auto); //MAY be XML, maybe RAW.
				} else {
					*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', sql encoding depends on there being an sql connection, and there isn't one." << Log::LO;
					trace();
					*Logger::log << Log::blockend;
					basis = NULL;
				}
			} break;
			case e_sha1: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::sha1,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_sha256: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::sha256,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_sha224: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::sha224,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_sha384: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::sha384,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
				
			case e_sha512: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::sha512,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_dss1: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::dss1,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_mdc2: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::mdc2,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_ripemd160: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::ripemd160,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_secret: {
				if (String::Digest::available(errs)) {
					encoded.append(Environment::Salt()); 
					String::Digest::do_digest(String::Digest::sha512,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;			

			case e_md5: {
				if (String::Digest::available(errs)) {
					String::Digest::do_digest(String::Digest::md5,encoded);
					String::tohex(encoded);
					basis = DataItem::factory(encoded,di_text); //cannot be xml.
				}
			} break;
			case e_deflate: {
				if (String::Deflate::available(errs)) {
					if ( process == encode) {
						String::Deflate::deflate(encoded,errs);
						if (errs.empty()) {
							basis = DataItem::factory(encoded,di_raw); //cannot be xml.
						} else {
							*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', deflate failed. " << errs << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							basis = NULL;
						}
					} else {
						String::Deflate::inflate(encoded,errs);
						if (errs.empty()) {
							basis = DataItem::factory(encoded,kind); //MAY be XML - maybe not.
						} else {
							*Logger::log << Log::error << Log::LI << "Error. In '" << name() << "', inflate failed. " << errs << Log::LO;
							trace();
							*Logger::log << Log::blockend;
							basis = NULL;
						}
					}
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
void IKO::evaltype(inp_space the_space, bool release, bool eval,bool is_context,kind_type ikind,DataItem*& name_item, DataItem*& container) {
	//evaltype() is used for evaluating BOTH inputs proper and also contexts for inputs and outputs.
	//exists is evaluated. significant is tested by comparision and will be looking for a value.
	exists = false; 
	usage_tests exist_test = ut_value; //we need to identify the way in which this is to be evaluated.
	if (container != NULL) {
		*Logger::log << Log::error << Log::LI << "Internal Error. container should be empty!" << Log::LO; 
		trace();
		*Logger::log << Log::blockend;
	}
	if (the_space == immediate || the_space == none) {
		if (the_space == immediate ) {
			exists = true; 
			container = name_item;
			name_item = NULL; 
		} else {
			if (name_item != NULL && !name_item->empty()) {
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. Space none cannot have a value!" << Log::LO; 
				trace();
				*Logger::log << Log::blockend;
			}
		}
	} else {
		u_str input_name;
		if (name_item != NULL && !name_item->empty()) {
			input_name = *name_item;
		}
		if (!is_context) { //if evaluating this IKO's context, then don't worry about exist_test. yet!
			name_v = input_name;
			Comparison* cmp = dynamic_cast<Comparison *>(p);
			if ((cmp != NULL) && (wotzit == obyx::comparate)) {
				switch (cmp->op()) {
					case obyx::exists: exist_test=ut_existence; break;
					case obyx::significant: exist_test= ut_significant; break;
					case obyx::found: exist_test= ut_found; break;
					default: break; // already value. 
				}
			} else {
				if (wotzit == control) {
					Iteration* ite = dynamic_cast<Iteration *>(p);
					if ( ite != NULL) {
						switch (ite->op()) {
							case obyx::it_while: exist_test=ut_existence; break;
							case obyx::it_while_not: exist_test= ut_existence; break;
							case obyx::it_each: exist_test= ut_found; break;
							default: break; // already value. 
						}
					}
				}
			}
		}
		switch (exist_test) {
			case ut_value: { //container is now a new object.
				exists = valuefromspace(input_name,the_space,is_context,release,ikind,container);
			} break;
			case ut_existence: {
				exists = existsinspace(input_name,the_space,is_context,release);
			} break;			
            case ut_significant: {
				if (eval) {
					exists = valuefromspace(input_name,the_space,is_context,release,ikind,container);
				} else {
					exists = sigfromspace(input_name,the_space,release,ikind,container);
				}
			} break;
			case ut_found: {
				exists = foundinspace(input_name,the_space,release);
			} break;
		}
	}
	if (eval) {
		if (exists) {
			if (container != NULL ) {
				string filestring,dirstring;
				if (! name_v.empty() ) { 
					Manager::transcode(name_v.c_str(),filestring);
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
				DataItem* doc_to_eval = container; //This is just a ptr copy.
				container = NULL;
				Document eval_doc(doc_to_eval,Document::File,filestring,this); //evaluate immediately!
				if (eval_doc.results.final()) { 
					if (exist_test == ut_significant && !is_context ) {
						exists = ! eval_doc.results.empty();
					}
					eval_doc.results.takeresult(container); //
				} else {
					*Logger::log << Log::error << Log::LI << "Error. File " << filestring << " was not evaluated " << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
				delete doc_to_eval; doc_to_eval = NULL;
				if (the_space == file && !dirstring.empty() ) { FileUtils::Path::pop_wd(); }
			} else {
				if (exist_test != ut_significant) {
					log(Log::error,"Error. eval cannot be applied to an empty value.");
				}
			}
		} else {
			if (exist_test != ut_significant) {
				if (exist_test == ut_existence) {
					log(Log::error,"Error. eval cannot be applied to a non-existent value.");
				} else {
					log(Log::error,"Error. eval cannot be applied within the context of an existence test.  Use significant for conditional eval.");
				}
			}
		}
	}
	if (name_item != NULL) {
		delete name_item;
		name_item = NULL; 
	}
}
bool IKO::foundinspace(const u_str& input_name,const inp_space the_space,const bool release) {
	Environment* env = Environment::service();
	exists = false;
	string errstring;			
	string discarded_result; //used to hold result for some spaces.
	switch ( the_space ) { //now do all the named input_spaces!
		case immediate: { 
			exists = true;
		} break;
		case none: {
			exists = false;
		} break;
		case field: {
			if ( input_name.empty() ) {
				log(Log::error,"Error. Field name missing.");
			} else {
				const ObyxElement* cur = this;
				const ObyxElement* par = p;
				const Iteration* ite = dynamic_cast<const Iteration *>(par);
				const Mapping* mpp = dynamic_cast<const Mapping *>(par);
				while (par != NULL && !exists) {
					if (ite != NULL && ite->active() && cur->wotzit==body ) {
						exists = ite->fieldfind(input_name);
					}
					if (mpp != NULL && mpp->active() && cur->wotzit==match) {
						exists = mpp->field(input_name,discarded_result); 
					}
					if (par != NULL && !exists) {
						cur = par;
						par = par->p;
						ite = dynamic_cast<const Iteration *>(par);
						mpp = dynamic_cast<const Mapping *>(par);
					}														
				}
			}
		} break;
		case xmlnamespace: { 
			log(Log::error,"Error. find key over namespace space not yet supported. use an existence test.");
		} break;
		case xmlgrammar: {
			log(Log::error,"Error. find key over grammar space not yet supported. use an existence test.");
		} break;
		case store: {
            if (!xpath.empty()) {
                exists = owner->storefind(input_name,xpath,release,errstring);
            } else {
                exists = owner->storefind(input_name,release,errstring);
            }
//			exists = owner->storefind(input_name,release,errstring);
			if (!errstring.empty()) {
				log(Log::error,"Error. Store error: " + errstring);
			} 
		} break;
		case fnparm: {
			exists = owner->parmfind(input_name);
		} break;
		case file: {
			vector<FileUtils::File> list;
			string file_path; setfilepath("",file_path);
			FileUtils::Path basis(file_path);
			XML::Manager::transcode(input_name,file_path);
			basis.listFiles(list,true,file_path);
			exists = ! list.empty();
		} break;
		case url: {
			log(Log::error,"Error. find key over url space not supported. use an existence test.");
		} break;
		case cookie: {
			string iname; XML::Manager::transcode(input_name,iname);
			exists = env->cookiefind(iname);
		} break;
		case sysparm: {
			string iname; XML::Manager::transcode(input_name,iname);
			exists = env->parmfind(iname);
		} break;
		case sysenv: {
			if (legalsysenv(input_name)) {
				if ( input_name.find(UCS2(L"CURRENT_"),0,8) == !string::npos) {
					DataItem* dummy = NULL;
					exists = currentenv(input_name.substr(8,string::npos),ut_found,this,dummy);
				} else {
					string iname; XML::Manager::transcode(input_name,iname);
					exists = env->envfind(iname);
				}
			}
		} break;
		case IKO::error: {
			doerrspace(input_name);
			exists = true;
		} break;
	}	
	return exists;
}
bool IKO::existsinspace(u_str& input_name,const inp_space the_space,const bool is_context,const bool release) {
	Environment* env = Environment::service();
	exists = false;
	string errstring;			
	string discarded_result; //used to hold result for some spaces.
	switch ( the_space ) { //now do all the named input_spaces!
		case immediate: { 
			exists = true;
		} break;
		case none: {
			exists = false;
		} break;
		case field: {
			if ( input_name.empty() ) {
				log(Log::error,"Error. Field name missing.");
			} else {
				const ObyxElement* par = p;
				const Iteration* ite = dynamic_cast<const Iteration *>(par);
				const Mapping* mpp = dynamic_cast<const Mapping *>(par);
				while (par != NULL && !exists ) {
					if (ite != NULL && ite->active()) {
						exists = ite->fieldexists(input_name,errstring);
					}
					if (mpp != NULL && mpp->active()) {
						exists = mpp->field(input_name,discarded_result); 
					}
					if (par != NULL && !exists) {
						par = par->p;
						ite = dynamic_cast<const Iteration *>(par);
						mpp = dynamic_cast<const Mapping *>(par);
					}														
				}
				if (!errstring.empty()) {
					string erv; XML::Manager::transcode(input_name,erv);		
					log(Log::error,"Error. Field " + erv + " : " + errstring);
				}
			}
		} break;
		case xmlnamespace: { 
			string iname; XML::Manager::transcode(input_name,iname);		
			exists = ItemStore::nsexists(iname,release);
		} break;
		case xmlgrammar: {
			string iname; XML::Manager::transcode(input_name,iname);		
			exists = ItemStore::grammarexists(iname,release);
		} break;
		case store: {
            if (!is_context && !xpath.empty()) {
				if (owner->doc_store != NULL) {
					exists = owner->doc_store->exists(input_name,xpath,false,errstring);
				}
				if (!exists) {
					exists = owner->storeexists(input_name,xpath,release,errstring);
				}
            } else {
				if (owner->doc_store != NULL) {
					exists = owner->doc_store->exists(input_name,false,errstring);
				}
				if (!exists) {
					exists = owner->storeexists(input_name,release,errstring);
				}
            }
            if (!errstring.empty()) {
                log(Log::error,"Error. Store error: " + errstring);
            } 
		} break;
		case fnparm: {
			exists = owner->parmexists(input_name);
		} break;					
		case file: {
			string iname; XML::Manager::transcode(input_name,iname);		
			string file_path; setfilepath(iname,file_path);
			string orig_wd(FileUtils::Path::wd());
			FileUtils::Path destination; 
			destination.cd(file_path);
			std::string dest_out = destination.output(true);
			Manager::transcode(dest_out,name_v);			//Keep the resulting path for use elsewhere.
			FileUtils::File file(dest_out);
			exists = file.exists();				//used by comparison to test...
			destination.cd(orig_wd);
		} break;
		case url: {
			if (httpready()) {
				string redirect_str,timeout_str;
				unsigned long long max_redirects=ULLONG_MAX,timeout_secs=ULLONG_MAX;
				owner->metastore("REDIRECT_BREAK_COUNT",max_redirects);
				owner->metastore("URL_TIMEOUT_SECS",timeout_secs);
				string req_url; XML::Manager::transcode(input_name,req_url);		
				string req_errors,body,response_head,response_body;
				HTTPFetch my_req(req_url,"GET","HTTP/1.0",body,(int)max_redirects,(int)timeout_secs,req_errors);
				exists  = my_req.doRequest(response_head,response_body,(int)max_redirects,(int)timeout_secs, req_errors);
			}
		} break;
		case cookie: {
			string iname; XML::Manager::transcode(input_name,iname);		
			exists = env->cookieexists(iname);
		} break;
		case sysparm: {
			string iname; XML::Manager::transcode(input_name,iname);		
			exists = env->parmexists(iname);
		} break;
		case sysenv: {
			if (legalsysenv(input_name)) {
				if ( input_name.find(UCS2(L"CURRENT_"),0,8) == !string::npos) {
					DataItem* dummy = NULL;
					exists = currentenv(input_name.substr(8,string::npos),ut_existence,this,dummy);
				} else { 
					string iname; XML::Manager::transcode(input_name,iname);		
					exists = env->envexists(iname);
				}
			}
		} break;
		case IKO::error: {
			doerrspace(input_name);
			exists = true;
		} break;
	}	
	return exists;
}
bool IKO::valuefromspace(u_str& input_name,const inp_space the_space,const bool is_context,const bool release,const kind_type ikind, DataItem*& container) {
	Environment* env = Environment::service();
	exists = false;
	string ikoname,fresult;	//used to hold result for most spaces.
/* Adding xpath facility to all spaces */	
	//now this is odd re. xpaths. If there is no xpath, then we throw a non-existence error..
	//however, if there is an xpath which doen't exist, we return nothing and don't throw an error.
	pair<u_str,u_str> namepath;
	bool node_expected = false;
	if (!is_context && !xpath.empty()) {
		namepath.first=input_name;
		namepath.second=xpath;
	} else {
		if(the_space != store ) { //only stores use # for xpath. everything else uses the xpath value.
			namepath.first=input_name;
		} else {
			XMLObject::npsplit(input_name,namepath,node_expected);
		}
	}
	if (namepath.second.empty()) {
		node_expected = true;
	}
/* Now we have name,path,expected */
	
	switch ( the_space ) { //now do all the named input_spaces!
		case none: break; //exists = false by default.
		case immediate: { 
			exists = true; //value handled by caller!!!
		} break;
		case field: {
			std::string errstring = "It does not exist or is not available."; 
			if ( namepath.first.empty() ) {
				log(Log::error,"Error. Field name missing.");
			} else {
				const ObyxElement* cur = this;
				const ObyxElement* par = p;
				const Iteration* ite = dynamic_cast<const Iteration *>(par);
				const Mapping* mpp = dynamic_cast<const Mapping *>(par);
				while (par != NULL && !exists ) {
					if (ite != NULL && ite->active() && (cur->wotzit==body)) {
						exists = ite->field(namepath.first,container,ikind,errstring);
					}
					if (mpp != NULL && mpp->active() && (cur->wotzit==match)) {
						exists = mpp->field(namepath.first,fresult); 
					}
					if (par != NULL && !exists) {
						cur = par;
						par = par->p;
						ite = dynamic_cast<const Iteration *>(par);
						mpp = dynamic_cast<const Mapping *>(par);
					}														
				}
				if (!exists) {
					string erv; XML::Manager::transcode(namepath.first,erv);		
					log(Log::error,"Error. Field " + erv + " : " + errstring);
				}
			}
		} break;
		case xmlnamespace: { 
			XML::Manager::transcode(namepath.first,ikoname);		
			exists = ItemStore::getns(ikoname,container,release);
			if (!exists)  {
				log(Log::error,"Error. Namespace " + ikoname + " does not exist");
			}
		} break;
		case xmlgrammar: {
			XML::Manager::transcode(namepath.first,ikoname);		
			exists = ItemStore::getgrammar(ikoname,container,ikind,release);
			if (!exists)  {
				log(Log::error,"Error. Grammar " + ikoname + " does not exist");
			}
		} break;
		case store: {
			string errstring;
			exists = owner->getstore(namepath.first,namepath.second,container,node_expected,release,errstring);
			if ((node_expected && !exists) || !errstring.empty()) {
				string erv; XML::Manager::transcode(namepath.first,erv);		
				if (errstring.empty()) { errstring = "does not exist.";}
				log(Log::error,"Error. Store error: " + erv + " " + errstring);
			} 
		} break;
		case fnparm: {
			const DataItem* ires = NULL; // we need to copy the parm from owner, not adopt it.
			exists = owner->getparm(namepath.first,ires);
			if (exists) {
				if (ires != NULL) { //it can be empty and exist.
					ires->copy(container);
				}
			} else {
				string erv; XML::Manager::transcode(namepath.first,erv);		
				log(Log::error,"Error. Parm " + erv + " does not exist here.");
			} 
		} break;					
		case file: {
			XML::Manager::transcode(namepath.first,ikoname);		
			string file_path; setfilepath(ikoname,file_path);
			string orig_wd(FileUtils::Path::wd());
			FileUtils::Path destination; 
			destination.cd(file_path);
			std::string dest_out = destination.output(true);
			Manager::transcode(dest_out,name_v);			//Keep the resulting path for use elsewhere.
			FileUtils::File file(dest_out);
			exists = file.exists();				//used by comparison to test...
			if (exists) {
				file.readFile(fresult);
			} else {
				string root(env->getpathforroot());
				string wd(FileUtils::Path::wd());	//we are grabbing the derived working directory.
				if ( wd.find(root) == 0 ) {
					wd.erase(0,root.length());
					if ( wd.empty() ) wd = "/";
				}
				log(Log::error,"Error. File '" + ikoname + "' glossed to '" + dest_out + "' does not exist. wd: " + wd );
			}
			destination.cd(orig_wd);
		} break;
		case url: {
			if (httpready()) {
				XML::Manager::transcode(namepath.first,ikoname);		
				string redirect_val,timeout_val,errstr;				
				unsigned long long max_redirects = ULLONG_MAX,timeout_secs = ULLONG_MAX;
				owner->metastore("REDIRECT_BREAK_COUNT",max_redirects);
				owner->metastore("URL_TIMEOUT_SECS",timeout_secs);
				string req_errors,body,response_head;
				HTTPFetch my_req(ikoname,"GET","HTTP/1.0",body,(int)max_redirects,(int)timeout_secs,req_errors);
				exists  = my_req.doRequest(response_head,fresult,(int)max_redirects,(int)timeout_secs,req_errors);
				if (!req_errors.empty()) {
					log(Log::error,"Error. url '" + ikoname + "' had error '" + req_errors + "'");
				}
			}
		} break;
		case cookie: {
			XML::Manager::transcode(namepath.first,ikoname);		
			exists = env->getcookie_req(ikoname,fresult);
			if( !exists ) {
				log(Log::error,"Error. Cookie " + ikoname + " does not exist.");
			}
		} break;
		case sysparm: {
			XML::Manager::transcode(namepath.first,ikoname);		
			exists = env->getparm(ikoname,fresult);
			if (!exists) {
				log(Log::error,"Error. Sysparm " + ikoname + " does not exist.");
			}
		} break;
		case sysenv: {
			if (legalsysenv(namepath.first)) {
				string errmsg = " does not exist.";
				if ( namepath.first.find(UCS2(L"CURRENT_"),0,8) == !string::npos) {
					exists = currentenv(namepath.first.substr(8,string::npos),ut_value,this,container);
				} else { //it's not a CURRENT_
					XML::Manager::transcode(namepath.first,ikoname);		
					exists = env->getenv(ikoname,fresult);
				}
				if (!exists) { 
					XML::Manager::transcode(namepath.first,ikoname);		
					log(Log::error,"Error. Sysenv " + ikoname + errmsg);
				}				
			}
		} break;
		case IKO::error: {
			doerrspace(namepath.first);
			exists = true;
		} break;
	}	
	if ( exists && container == NULL ) {
		if (!fresult.empty()) {
			kind_type fkind = ikind;
			switch ( encoder ) {
				case e_sha1:
				case e_sha224:
				case e_sha384:
				case e_sha256:
				case e_sha512: 
				case e_dss1:
				case e_mdc2:
				case e_ripemd160:
				case e_secret:
				case e_md5: 
				case e_name: 
				case e_digits:
				case e_hex:
				case e_base64:
				case e_xml:
				case e_url: {
					if (process == encode) {fkind = di_text;} 
				}  break;
				default: break;
			}
			container = DataItem::factory(fresult,fkind); //test for xml if needs be. This is not being deleted.
			if (container->empty()) {
				log(Log::error,"IKO value composition error with: " + ikoname);
			}
			//if there was an error, we have ikoname here.
		}
	}
	
	if (!namepath.second.empty() && container != NULL) {
		DataItem* tmp = container; container=NULL;
		if (tmp->kind() == di_object) {
			string xp_errors;
			XMLObject* xobj = *tmp;
			xobj->xp(namepath.second, container, false, xp_errors);
			if (!xp_errors.empty()) {
				log(Log::error,"Error. url '" + ikoname + "' had xpath error '" + xp_errors + "'");
			}
		} else {
			log(Log::error,"IKO xpath will not work on non-object: " + ikoname);
		}
		delete tmp;
	}
	
	return exists;
}
bool IKO::sigfromspace(const u_str& input_name,const inp_space the_space,const bool release,const kind_type ikind,DataItem*& container) {
	/*
	 Sig from space needs the container, as there are further tests that may be required for significance, such as
	 eval, and encoders.
	 */
	Environment* env = Environment::service();
	exists = false;
	string errstring,fresult;	//used to hold result for most spaces.
	u_str uresult; 
	switch ( the_space ) { //now do all the named input_spaces!
		case none: break; //exists = false by default.
		case immediate: { 
			exists = true; //value handled by caller!!!
		} break;
		case field: {
			if ( input_name.empty() ) {
				log(Log::error,"Error. Field name missing.");
			} else {
				const ObyxElement* cur = this;
				const ObyxElement* par = p;
				const Iteration* ite = dynamic_cast<const Iteration *>(par);
				const Mapping* mpp = dynamic_cast<const Mapping *>(par);
				while (par != NULL && !exists ) {
					if (ite != NULL && ite->active() && cur->wotzit==body ) {
						exists = ite->field(input_name,container,ikind,errstring);
					}
					if (mpp != NULL && mpp->active() && cur->wotzit==match) {
						exists = mpp->field(input_name,fresult); 
					}
					if (par != NULL && !exists) {
						cur = par;
						par = par->p;
						ite = dynamic_cast<const Iteration *>(par);
						mpp = dynamic_cast<const Mapping *>(par);
					}														
				}
				if (!errstring.empty()) {
					string erv; XML::Manager::transcode(input_name,erv);		
					log(Log::error,"Error. Field " + erv + " : " + errstring);
				}
			}
		} break;
		case xmlnamespace: { 
			string skey; XML::Manager::transcode(input_name,skey);		
			exists = ItemStore::nsexists(skey,release);
			if (exists) {uresult = input_name;}
		} break;
		case xmlgrammar: {
			string skey; XML::Manager::transcode(input_name,skey);		
			exists = ItemStore::grammarexists(skey,release);
			if (exists) {uresult = input_name;}
		} break;
		case store: {
			bool node_expected = false;
			pair<u_str,u_str> np;
			XMLObject::npsplit(input_name,np,node_expected);
			exists = owner->getstore(np.first,np.second,container,node_expected,release,errstring);
			if (!errstring.empty()) {
				log(Log::error,"Error. Store error: " + errstring);
			} 
		} break;
		case fnparm: {
			const DataItem* ires = NULL; // we need to copy the parm from owner, not adopt it.
			exists = owner->getparm(input_name,ires);
			if (exists && ires != NULL) {
				ires->copy(container);
			} 
		} break;		
		case file: {
			string skey; XML::Manager::transcode(input_name,skey);		
			string file_path; setfilepath(skey,file_path);
			string orig_wd(FileUtils::Path::wd());
			FileUtils::Path destination; 
			destination.cd(file_path);
			std::string dest_out = destination.output(true);
			Manager::transcode(dest_out,name_v);			//Keep the resulting path for use elsewhere.
			FileUtils::File file(dest_out);
			exists = file.exists();				//used by comparison to test...
			if (exists) {
				if (file.getSize() == 0) {
					fresult.clear();
				} else {
					fresult = dest_out;
				}
			}
			destination.cd(orig_wd);
		} break;
		case url: {
			if (httpready()) {
				string req_url; XML::Manager::transcode(input_name,req_url);		
				string redirect_val,timeout_val,errstr;
				unsigned long long max_redirects = ULLONG_MAX,timeout_secs = ULLONG_MAX;
				owner->metastore("REDIRECT_BREAK_COUNT",max_redirects);
				owner->metastore("URL_TIMEOUT_SECS",timeout_secs);
				string req_errors,body,response_head;
				exists = false;
				HTTPFetch my_req(req_url,"GET","HTTP/1.0",body,max_redirects,timeout_secs,req_errors);
				if (req_errors.empty()) {
					exists = my_req.doRequest(response_head,fresult,max_redirects,timeout_secs,req_errors);
					if (req_errors.empty()) {
						exists = ! fresult.empty();
					}
				}
			}
		} break;
		case cookie: {
			string skey; XML::Manager::transcode(input_name,skey);		
			exists = env->getcookie_req(skey,fresult);
		} break;
		case sysparm: {
			string skey; XML::Manager::transcode(input_name,skey);		
			exists = env->getparm(skey,fresult);
		} break;
		case sysenv: {
			if (legalsysenv(input_name)) {
				if ( input_name.find(UCS2(L"CURRENT_"),0,8) == !string::npos) {
					exists = currentenv(input_name.substr(8,string::npos),ut_significant,this,container);
				} else { //it's a CURRENT_
					string skey; XML::Manager::transcode(input_name,skey);		
					exists = env->getenv(skey,fresult);
				}
			}
		} break;
		case IKO::error: {
			doerrspace(input_name);
			exists = true;
		} break;
	}	
	if ( exists && container == NULL ) {
		if (!fresult.empty()) {
			container = DataItem::factory(fresult,di_text); //test for xml if needs be.
		} else {
			if (!uresult.empty()) {
				container = DataItem::factory(uresult,di_text); //test for xml if needs be.
			}
		}
	} 
	return exists;
}
void IKO::keysinspace(const u_str& input_name,const inp_space the_space,set<string>& keylist) {
	Environment* env = Environment::service();
	string errstring;			
	switch ( the_space ) { //now do all the named input_spaces!
		case immediate: {  
			string skey; XML::Manager::transcode(input_name,skey);		
			keylist.insert(skey);		
		} break;
		case field: {
			const ObyxElement* cur = this;
			const ObyxElement* par = p;
			const Iteration* ite = dynamic_cast<const Iteration *>(par);
			while (par != NULL ) {
				if (ite != NULL && ite->active() && cur->wotzit==body ) {
					ite->fieldkeys(input_name,keylist);
				}
				if (par != NULL) {
					cur = par;
					par = par->p;
					ite = dynamic_cast<const Iteration *>(par);
				}														
			}
		} break;
		case xmlnamespace: { log(Log::error,"Error. finding key over namespace space not yet supported."); } break;
		case xmlgrammar: { log(Log::error,"Error. finding key over grammar space not yet supported."); } break;
		case store: {
			owner->storekeys(input_name,keylist,errstring);
			if (!errstring.empty()) { log(Log::error,"Error. Store error: " + errstring); } 
		} break;
		case fnparm: { 
			owner->parmkeys(input_name,keylist);		
		} break;					
		case file: { 
			vector<FileUtils::File> list;
			string skey; XML::Manager::transcode(input_name,skey);		
			string root(env->getpathforroot());
			string file_path; setfilepath("",file_path);
			FileUtils::Path basis(file_path); 
			basis.listFiles(list,true,skey);
			for (size_t i=0; i < list.size(); i++) {
				string kyresult(list[i].output(false));
				if ( kyresult.find(root) == 0 ) {
					kyresult.erase(0,root.length());
					if (kyresult.empty() ) kyresult = "/";
				}
				keylist.insert(kyresult);
			}
		} break;
		case url: { log(Log::error,"Error. finding keys over url space not supported."); } break;
		case cookie: { 
			string skey; XML::Manager::transcode(input_name,skey);		
			env->cookiekeys(skey,keylist); 
		} break;
		case sysparm: { 
			string skey; XML::Manager::transcode(input_name,skey);		
			env->parmkeys(skey,keylist); 
		} break;
		case sysenv: { 
			if (legalsysenv(input_name)) { 
				string skey; XML::Manager::transcode(input_name,skey);		
				env->envkeys(skey,keylist); 
			} 
		} break;
		case none:
		case IKO::error: break; // error was already done.
	}	
}
void IKO::init() {
}
void IKO::finalise() {
}
void IKO::startup() {
	InputType::startup();
	Output::startup();
	current_types.insert(current_type_map::value_type(UCS2(L"OBJECT"),c_object));
	current_types.insert(current_type_map::value_type(UCS2(L"NAME"),c_name));
	current_types.insert(current_type_map::value_type(UCS2(L"REQUEST"),c_request));
	current_types.insert(current_type_map::value_type(UCS2(L"OSI_RESPONSE"),c_osi_response));
	current_types.insert(current_type_map::value_type(UCS2(L"TIMING"),c_timing));
	current_types.insert(current_type_map::value_type(UCS2(L"TIME"),c_time));
	current_types.insert(current_type_map::value_type(UCS2(L"TS"),c_ts));
	current_types.insert(current_type_map::value_type(UCS2(L"VERSION"),c_version));
	current_types.insert(current_type_map::value_type(UCS2(L"VERSION_NUMBER"),c_vnumber));
	current_types.insert(current_type_map::value_type(UCS2(L"RESPONSE"),c_response));
	current_types.insert(current_type_map::value_type(UCS2(L"POINT"),c_point));
	current_types.insert(current_type_map::value_type(UCS2(L"COOKIES"),c_cookies));
	
	kind_types.insert(kind_type_map::value_type(UCS2(L"auto"), di_auto));
	kind_types.insert(kind_type_map::value_type(UCS2(L"raw"), di_raw));
	kind_types.insert(kind_type_map::value_type(UCS2(L"text"), di_text));
	kind_types.insert(kind_type_map::value_type(UCS2(L"object"), di_object));
	
	enc_types.insert(enc_type_map::value_type(UCS2(L"none"), e_none));
	enc_types.insert(enc_type_map::value_type(UCS2(L"qp"), e_qp));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sql"), e_sql));
	enc_types.insert(enc_type_map::value_type(UCS2(L"xml"), e_xml));
	enc_types.insert(enc_type_map::value_type(UCS2(L"url"), e_url));
	enc_types.insert(enc_type_map::value_type(UCS2(L"name"), e_name));
	enc_types.insert(enc_type_map::value_type(UCS2(L"digits"), e_digits));
	enc_types.insert(enc_type_map::value_type(UCS2(L"btwoc"), e_btwoc));
	enc_types.insert(enc_type_map::value_type(UCS2(L"base16"), e_10to16));
	enc_types.insert(enc_type_map::value_type(UCS2(L"base64"), e_base64));
	enc_types.insert(enc_type_map::value_type(UCS2(L"hex"), e_hex));
	enc_types.insert(enc_type_map::value_type(UCS2(L"message"), e_message));
	enc_types.insert(enc_type_map::value_type(UCS2(L"md5"), e_md5));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sha1"), e_sha1));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sha224"), e_sha224));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sha256"), e_sha256));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sha384"), e_sha384));
	enc_types.insert(enc_type_map::value_type(UCS2(L"sha512"), e_sha512));
	enc_types.insert(enc_type_map::value_type(UCS2(L"dss1"), e_dss1));
	enc_types.insert(enc_type_map::value_type(UCS2(L"mdc2"), e_mdc2));
	enc_types.insert(enc_type_map::value_type(UCS2(L"ripemd160"), e_ripemd160));
	enc_types.insert(enc_type_map::value_type(UCS2(L"secret"), e_secret));
	enc_types.insert(enc_type_map::value_type(UCS2(L"deflate"), e_deflate));
	enc_types.insert(enc_type_map::value_type(UCS2(L"json"), e_json));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"none"), immediate));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"field"), field ));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"url"), url ));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"file"), file ));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"parm"), fnparm));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"sysparm"), sysparm));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"sysenv"), sysenv));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"cookie"), cookie)); 
	ctx_types.insert(inp_space_map::value_type(UCS2(L"store"), store));
	ctx_types.insert(inp_space_map::value_type(UCS2(L"namespace"), xmlnamespace));
}
void IKO::shutdown() {
	InputType::shutdown();
	Output::shutdown();
	current_types.clear();
	kind_types.clear();
	enc_types.clear();
	ctx_types.clear();
}
