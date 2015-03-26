/*
 * environment.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd
 * environment.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __MACH__
#import <mach/mach_time.h>
#endif

#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>

#include "environment.h"
#include "IInfGlobal.h"

#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/httphead/httphead.h"
#include "commons/date/date.h"
#include "commons/vdb/vdb.h"
#include "commons/filing/filing.h"

#include "core/obyxelement.h"

extern char **environ;

using namespace std;
using namespace Log;
using namespace FileUtils;
using namespace ImageInfo;

typedef unordered_map<const string,string, hash<const string&> > var_map_type;

var_map_type Environment::benv_map;
var_map_type Environment::cgi_rfc_map;
Environment* Environment::instance;
//bool Environment::config_file_done;
stack<double> Environment::runtime_version;
size_t Environment::vsize = 0;						 		//size of version stack

#ifdef __MACH__
struct mach_timebase_info Environment::time_info;
long double Environment::nano;
#endif

Environment::Environment()  : gDevelop(false),do_auto_utf8check(true),gSQLport(0),gRootDir(""),gScriptsDir(""),gScratchDir("/tmp/"),basetime(0)  {
	pid = getpid();
	gArgc=0;
	gArgv=nullptr;
}
Environment::~Environment() {
	ienv_map.clear();
	ck_map.clear();								//store the cookie values  (rw)
	ck_expires_map.clear();						//store the cookie expires  (rw)
	ck_path_map.clear();						//store the cookie values  (rw)
	ck_domain_map.clear();						//store the cookie values  (rw)
	cke_map.clear();							//store the cookie values  (r)
	cke_expires_map.clear();					//store the cookie expires (r)
	cke_path_map.clear();						//store the cookie values  (r)
	cke_domain_map.clear();						//store the cookie values  (r)
	parm_map.clear();							//Application parms  (ro)
	httphead_map.clear();						//Request HTTP headers  (ro)
}
#pragma mark STATIC CONTROL
void Environment::startup(string& v,string& vn,int argc, char **argv) {					//everything that doesn't change across multiple runs
	init_cgi_rfc_map();
	do_conf_from_args(argc,argv);
	setbenvmap();
	runtime_version.push(String::real(vn));
	setbenv("OBYX_VERSION",v);			//Let coders know what version we are in!
	setbenv("OBYX_VERSION_NUMBER",vn);	//Let coders know what version number we are in!
	string tmp("");						//The following settings only have an effect at startup!
	getbenv("OBYX_NO_EXTERNAL_GRAMMARS",tmp);
	vsize = runtime_version.size();
}
void Environment::shutdown() {
	cgi_rfc_map.clear();
	benv_map.clear();
}
//only called if not fastcgi.
void Environment::init_httphead() {
	for(var_map_type::iterator bi = benv_map.begin(); bi != benv_map.end(); bi++) {
		var_map_type::iterator it = cgi_rfc_map.find(bi->first);
		if (it != cgi_rfc_map.end()) {
			httphead_map.insert(var_map_type::value_type(it->second,bi->second));
		} else {
			if (bi->first.find("HTTP_") == 0) {
				string special=bi->first.substr(5,string::npos);
				String::cgi_to_http(special);
				httphead_map.insert(var_map_type::value_type(special,bi->second));
			}
		}
	}
}
void Environment::do_conf_from_args(int argc, char **argv) {
	if (argc > 0) {
		bool this_one = false;
		for(int i = 1; i < argc; i++) {
			string av = argv[i];
			if (av.compare("-c") == 0) { 	// none of this currently works
				this_one = true;			// because vdb/zip etc. dlopen runs in startup and
			} else {						// cli is processed at init
				if (this_one) {
					do_config_file(av);
					this_one=false;
				}
			}
		}
	}
}
//so this now sends out the header AFTER the xml.
void Environment::init(int argc, char **argv, char** env) {
	if (instance == nullptr) {
		instance = new Environment();	// instantiate singleton
		instance->gArgc=argc;
		instance->gArgv=argv;
		if (env != nullptr) {
			instance->setienvmap(env);
		} else {
			instance->init_httphead();
		}
		instance->getenvvars();			//args
		instance->getenvvars_base();	//environment
		instance->setbasetime();
	}
	vsize = runtime_version.size();
}
void Environment::finalise() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
	while (runtime_version.size() > vsize) {
		runtime_version.pop();
	}
}
Environment* Environment::service() {
	return instance;
}
double Environment::version() {
	return runtime_version.top();
}
void Environment::version_push(double v) {
	runtime_version.push(v);
}
void Environment::version_pop() {
	if (!runtime_version.empty()) {
		runtime_version.pop();
	}
}
#pragma mark COOKIE MANAGEMENT FUNCTIONS
//Request cookies: GET
bool Environment::getcookie_req(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = cke_map.find(name); //check request cookies first.
	if (it != cke_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_req_domain(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = cke_domain_map.find(name); //check request cookies first.
	if (it != cke_domain_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_req_path(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = cke_path_map.find(name); //check request cookies first.
	if (it != cke_path_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_req_expires(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = cke_expires_map.find(name); //check request cookies first.
	if (it != cke_expires_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
//Request cookies: SET
void Environment::setcookie_req(string name,string value) {
	pair<var_map_type::iterator, bool> ins = cke_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		cke_map.erase(ins.first);
		cke_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setcookie_req_path(string name,string value) {
	pair<var_map_type::iterator, bool> ins = cke_path_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		cke_path_map.erase(ins.first);
		cke_path_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setcookie_req_domain(string name,string value) {
	pair<var_map_type::iterator, bool> ins = cke_domain_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		cke_domain_map.erase(ins.first);
		cke_domain_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setcookie_req_expires(string name,string value) {
	pair<var_map_type::iterator, bool> ins = cke_expires_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		cke_expires_map.erase(ins.first);
		cke_expires_map.insert(var_map_type::value_type(name, value));
	}
}
//--------------------------Response cookie GET functions
void Environment::delcookie_res(string name) {
	var_map_type::iterator it;
	it = ck_map.find(name); if (it != ck_map.end()) ck_map.erase(it);
	it = ck_domain_map.find(name); if (it != ck_domain_map.end()) ck_domain_map.erase(it);
	it = ck_path_map.find(name); if (it != ck_path_map.end()) ck_path_map.erase(it);
	it = ck_expires_map.find(name); if (it != ck_expires_map.end()) ck_expires_map.erase(it);
}
bool Environment::getcookie_res(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = ck_map.find(name); //check request cookies first.
	if (it != ck_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_res_domain(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = ck_domain_map.find(name); //check request cookies first.
	if (it != ck_domain_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_res_path(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = ck_path_map.find(name); //check request cookies first.
	if (it != ck_path_map.end()) { container = ((*it).second); retval = true; }
	return retval;
}
bool Environment::getcookie_res_expires(string const name,string& container) {	//only pre-existing cookies
	bool retval = false; container.clear();
	var_map_type::iterator it = ck_expires_map.find(name); //check request cookies first.
	if (it != ck_expires_map.end()) {
		container = ((*it).second); retval = true;
	}
	return retval;
}
//Response cookies: SET
void Environment::setcookie_res(string name,string value) {
	if ( value.empty() ) {
		delcookie_res(name);
	} else {
		pair<var_map_type::iterator, bool> ins = ck_map.insert(var_map_type::value_type(name, value));
		if (!ins.second) {
			ck_map.erase(ins.first);
			ck_map.insert(var_map_type::value_type(name, value));
		}
	}
}
void Environment::setcookie_res_path(string name,string value) {
	pair<var_map_type::iterator, bool> ins = ck_path_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		ck_path_map.erase(ins.first);
		ck_path_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setcookie_res_domain(string name,string value) {
	pair<var_map_type::iterator, bool> ins = ck_domain_map.insert(var_map_type::value_type(name, value));
	if (!ins.second) {
		ck_domain_map.erase(ins.first);
		ck_domain_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setcookie_res_expires(string name,string value) {
	string curck;
	bool do_insert = false;
	if (value.compare("discard") == 0) {
		delcookie_res(name);
		if (Environment::getcookie_req(name,curck) ) {	//only pre-existing cookies
			do_insert = true;
		}
	} else {
		if  (Environment::getcookie_res(name,curck)) {	//existing cookies
			do_insert = true;
		} else {
			if  (Environment::getcookie_req(name,curck)) {	//pre-existing cookies
				Environment::setcookie_res(name,curck);     //Add to response cookies (should also add domain & path)
				do_insert = true;
			}
		}
	}
	if (do_insert) {
		pair<var_map_type::iterator, bool> ins = ck_expires_map.insert(var_map_type::value_type(name, value));
		if (!ins.second) {
			ck_expires_map.erase(ins.first);
			ck_expires_map.insert(var_map_type::value_type(name, value));
		}
	}
}
//--------------------------------------------------
//Gets either request cookies or response cookies.
bool Environment::getcookie(string const name,string& container) {
	bool retval = getcookie_req(name,container);
	if (!retval) {
		retval = getcookie_res(name,container);
	}
	return retval;
}
bool Environment::getcookie_domain(string const name,string& container) {
	bool retval = getcookie_req_domain(name,container);
	if (!retval) {
		retval = getcookie_res_domain(name,container);
	}
	return retval;
}
bool Environment::getcookie_path(string const name,string& container) {
	bool retval = getcookie_req_path(name,container);
	if (!retval) {
		retval = getcookie_res_path(name,container);
	}
	return retval;
}
bool Environment::getcookie_expires(string const name,string& container) {
	bool retval = getcookie_req_expires(name,container);
	if (!retval) {
		retval = getcookie_res_expires(name,container);
	}
	return retval;
}
//--- GENERAL Cookie functions
void Environment::do_response_cookies(string& result) {
	result = "<m:message xmlns:m=\"http://www.obyx.org/message\">";
	result.append(response_cookies(true));
	result.append("</m:message>");
}
string Environment::response_cookies(bool explaining) {
	var_map_type::iterator it;
	ostringstream cookiestream;
	string yearahead,yearago;
	string this_cookie_name,this_cookie_value,this_cookie_expires;
	DateUtils::Date::getCookieDateStr(yearahead);	    //for expires='persist'
	DateUtils::Date::getCookieExpiredDateStr(yearago);  //for expires='discard'
	for(var_map_type::iterator imt = ck_map.begin(); imt != ck_map.end(); imt++) {
		this_cookie_name = imt->first;
		this_cookie_value = imt->second;
		if ( ! this_cookie_name.empty() ) {
			if ( ! this_cookie_value.empty() ) {		//EMPTY cookie values break browsers like safari.
				it = ck_expires_map.find(imt->first);   //cookies may have been set elsewhere so pass them over.
				if (it != ck_expires_map.end()) {
					this_cookie_expires = it->second;
				} else {
					this_cookie_expires.clear();
				}
				if (explaining) {
					bool urlenc = true;
					std::string ck_val = this_cookie_value;
					String::urlencode(ck_val);
					if (this_cookie_value.compare(ck_val) == 0) urlenc = false;
					XMLChar::encode(this_cookie_name);
					cookiestream << "<m:header name=\"Set-Cookie\" cookie=\"" << this_cookie_name << "\">";
					cookiestream << "<m:subhead name=\"" << this_cookie_name << "\" value=\"" << ck_val << "\"";
					if (urlenc) { cookiestream << " urlencoded=\"true\""; }
					cookiestream << "/>";
				} else {
					cookiestream << Httphead::cookiesig << ": " << this_cookie_name << "=" << this_cookie_value;
				}
				if ( ( !this_cookie_expires.empty() ) && ( this_cookie_expires.compare("session") != 0) ) {
					if (this_cookie_expires.compare("persist") == 0) {
						this_cookie_expires = yearahead;
					}
					if (explaining) {
						cookiestream << "<m:subhead name=\"expires\" value=\"" << this_cookie_expires << "\"/>";
					} else {
						cookiestream << "; expires=" << this_cookie_expires;
					}
				}
				it = ck_domain_map.find(imt->first);
				if (it != ck_domain_map.end()) {
					if (explaining) {
						cookiestream << "<m:subhead name=\"domain\" value=\"" << (*it).second << "\"/>";
					} else {
						cookiestream << "; domain=" << (*it).second;
					}
				}
				//do the path component - defaults to /
				it = ck_path_map.find(imt->first);
				if (explaining) {
					if (it != ck_path_map.end()) {
						cookiestream << "<m:subhead name=\"path\" value=\"" << (*it).second << "\"/>";
					}
				} else {
					if (it != ck_path_map.end()) {
						cookiestream << "; path=" << (*it).second;
					} else {
						cookiestream << "; path=/";
					}
				}
				if (explaining) {
					cookiestream << "</m:header>";
				} else {
					cookiestream << Httphead::crlf;
				}
			}
		}
	}
	//now do request cookies where their expiry date has changed..
	for(var_map_type::iterator imd = ck_expires_map.begin(); imd != ck_expires_map.end(); imd++) {
		this_cookie_name = imd->first;
		this_cookie_expires = imd->second;
		if (!getcookie_res(this_cookie_name,this_cookie_value)) { //We don't want to rewrite cookies that have been composited as responses already.
			if (getcookie_req(this_cookie_name,this_cookie_value)) {
				if ( ! this_cookie_name.empty() && ! this_cookie_value.empty() ) {
					bool urlenc = true;
					std::string ck_val = this_cookie_value;
					String::urlencode(ck_val);
					if (this_cookie_value.compare(ck_val) == 0) {
						urlenc = false;
					} else {
						this_cookie_value = ck_val;
					}
					if (explaining) {
						XMLChar::encode(this_cookie_name);
						cookiestream << "<m:header name=\"Set-Cookie\" cookie=\"" << this_cookie_name << "\">";
						cookiestream << "<m:subhead name=\"" << this_cookie_name << "\" value=\"" << this_cookie_value << "\"";
						if (urlenc) {
							cookiestream << " urlencoded=\"true\"";
						}
						cookiestream << "/>";
					} else {
						cookiestream << Httphead::cookiesig << ": " << this_cookie_name << "=" << this_cookie_value;
					}
					if ( ( !this_cookie_expires.empty() ) && ( this_cookie_expires.compare("session") != 0) ) {
						if (this_cookie_expires.compare("persist") == 0) {
							this_cookie_expires = yearahead;
						} else {
							if (this_cookie_expires.compare("discard") == 0) {
								this_cookie_expires = yearago;
							}
						}
						if (explaining) {
							cookiestream << "<m:subhead name=\"expires\" value=\"" << this_cookie_expires << "\"/>";
						} else {
							cookiestream << "; expires=" << this_cookie_expires;
						}
					}
					if (explaining) {
						cookiestream << "</m:header>";
					} else {
						cookiestream << Httphead::crlf;
					}
				}
				
			}
		}
	}
	return cookiestream.str();
}
//  Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001; SHIPPING=FEDEX
// http://www.ietf.org/rfc/rfc2965.txt
// http://wp.netscape.com/newsref/std/cookie_spec.html
void Environment::do_request_cookies() {
	string cook;
	string ckname,ckattr,ckvar;
	size_t start=0;
	size_t find = string::npos;
	bool finished = false;
	if (getenv("HTTP_COOKIE",cook))  {
		while( !finished ) {
			ckname.clear();
			ckattr.clear();
			ckvar.clear();
			find = cook.find_first_of(";\"",start);
			if (find != string::npos ) {
				if (cook[find] == '\"' ) {
					size_t qfind = cook.find('\"', find+1);
					find = cook.find(";",qfind);
				}
			}
			string cknamvar(cook.substr(start, find - start));
			if (! cknamvar.empty() ) {
				size_t cknaml = cknamvar.find_first_not_of("\t\r\n ");
				size_t cknamr = cknamvar.find_last_not_of("\t\r\n ");
				cknamvar=cknamvar.substr(cknaml, (1+cknamr) - cknaml );
				size_t nvstart=0;
				size_t nvfind = string::npos;
				nvfind = cknamvar.find_first_of("=\"",nvstart); //a semicolon will happen on a non-value name
				if (nvfind != string::npos ) {
					if (cknamvar[nvfind] == '\"' ) {
						size_t qfind = cknamvar.find('\"', nvfind+1);
						nvfind = cknamvar.find("=",qfind);
					}
				}
				if (nvfind != string::npos ) { //retest because of quotes above.
					//nvfind should now be '='
					ckvar=cknamvar.substr(nvfind + 1, string::npos); //This should be e.g. yyy
					if (! ckvar.empty() ) {
						size_t stripl=ckvar.find_first_not_of("\t\r\n\" ");
						size_t stripr=ckvar.find_last_not_of("\t\r\n\" ");
						ckvar=ckvar.substr(stripl,(1 + stripr) - stripl );
					}
				}
				if (cknamvar[nvstart] == '$') {
					ckattr=ckname;
					ckattr.append("_");
					cknamvar=cknamvar.substr(nvstart+1, (1+nvfind) - nvstart);
					cknamvar=cknamvar.substr(cknamvar.find_first_not_of("\t\r\n\" "), 1+cknamvar.find_last_not_of("\t\r\n\" ") );
					ckattr.append(cknamvar); //This should be e.g. yyy
				} else {
					ckname.clear();
					string cnvar=cknamvar.substr(nvstart, (1+nvfind) - nvstart);
					if (!cnvar.empty()) {
						size_t cnvl=cnvar.find_first_not_of("\t\r\n\" ");
						size_t cnvr=cnvar.find_last_not_of("=\t\r\n\" ");
						cnvar=cnvar.substr( cnvl, (1+cnvr) - cnvl  );
					} else {
						cnvar=cknamvar;
					}
					ckname.append(cnvar);
					ckattr=ckname;
				}
				String::urldecode(ckattr);
				String::urldecode(ckvar);
				setcookie_req(ckattr, ckvar);
			}
			if (find == string::npos ) {
				finished = true;
			} else {
				start = find + 1;
				ckvar.clear();
			}
		}
	}
}
#pragma mark PARAMETER (UTILITY)
void Environment::dopostparms() {
	long long numparms = 0;
	string req_method, test_filename;
	string input;
	if (getenv("REQUEST_METHOD",req_method)) { //if REQ_METHOD
		if ( req_method.compare("POST") == 0 ) {
			getparm("THIS_REQ_BODY",input);
			string contenttype;
			if ( getenv("CONTENT_TYPE",contenttype) ) { //if it exists..
				size_t post_mime = contenttype.find("multipart/form-data");
				if (post_mime == string::npos ) {
					size_t xmltest = contenttype.find("text/xml");
					size_t xmltestB = contenttype.find("application/xml");
					size_t xmltest1_2 = contenttype.find("application/soap+xml");
					if ( (xmltest != string::npos) || (xmltestB != string::npos) || (xmltest1_2 != string::npos) ) {
						setparm("","XML_DOCUMENT");
						setparm("XML_DOCUMENT",input);
						return;
					} else {
						size_t post_wwform = contenttype.find("application/x-www-form-urlencoded"); //just the same as if it were a GET/QUERYSTRING
						if (post_wwform != string::npos) {
							doparms(-1,nullptr);
							return;
						} else { // unknown or empty
							setparm("POST_MIME",contenttype);
							setparm("THIS_REQ_BODY",input);
							setparm("","POST_MIME");
							setparm("","THIS_REQ_BODY");
							return;
						}
					}
				}
				//  multipart/form-data mime follows
				//  text/xml; charset="utf-8"
				//	CONTENT_TYPE]=[multipart/form-data; boundary=Message_Boundary_000002]
				string boundary;
				string startboundary;
				string endboundary;
				size_t b = contenttype.rfind("boundary=");
				if (b == string::npos ) {
					*Logger::log << Log::error << Log::LI << "Boundary field missing in multipart form data. POST must be enctype multipart/form-data" << Log::LO << Log::LI;
					list();
					*Logger::log << Log::LO << Log::LI << subhead  << Log::LI << "Post Details Follow" << Log::LO;
					*Logger::log << LI << Log::RI << input << Log::RO << Log::LO; //raw finished
					*Logger::log << blockend << Log::LO << blockend; //subhead/error finished
					return;
				}
				else {
					boundary = contenttype.substr(b + 9, contenttype.npos);
					if (boundary[0] == '"' || boundary[0] == '\'') {
						boundary = boundary.substr(1, boundary.length() - 2);
					}
					startboundary = "--" + boundary;
					endboundary = startboundary + "--";
				}
				size_t blockstart = 0;
				size_t blockend = 0;
				setienv("POST_BOUNDARY",boundary);
				// BLOCK LOOP
				bool postfinished = false;
				while(! postfinished) {
					string name(""), number(""), value(""), filename(""), mimetype("");
					blockstart = input.find(startboundary, blockend);  // find the boundary at blockend (intis at 0)
					if (blockstart == string::npos) {
						//						*Logger::log << Log::LI << "Finished Post" << Log::LO << Log::blockend;
						postfinished = true;
					}
					blockend = input.find(startboundary, blockstart + 1);
					if (blockend == string::npos)
						blockend = input.find(endboundary, blockstart + 1);
					if (blockend == string::npos) {
						//						*Logger::log << Log::LI << "Finished Post" << Log::LO << Log::blockend;
						postfinished = true;
					}
					if (! postfinished ) {
						string block = input.substr(blockstart + startboundary.length() + 2, blockend - blockstart - startboundary.length() - 4);
						//						*Logger::log << Log::info << Log::LI << "Input block" << Log::LO;
						//A typical block is below.
						//
						//						 --#312999087#multipart#boundary#1049282996#
						//						Content-Disposition: form-data; name="id:2"
						//
						//						 106
						//
						
						//A typical file block is below.
						//						 --#312999087#multipart#boundary#1049282996#
						//						 Content-Disposition: form-data; name="img:1"; filename="p.gif"
						//						 Content-Type: image/gif
						//
						//						 GIF89a\001\000\001\000\304\000\000\u02c7\u02c7\u02c7\000\000\000!\u02d8\004\001\000\000\000\000,\000\000\000\000\001\000\001\000\000\002\002D\001\000;
						//
						// FIRST LINE -- The Content-Disposition line
						size_t lineend = block.find("\x0d\x0a");
						if (lineend == string::npos) {
							*Logger::log << Log::error << Log::LI << "Newlines should be crlf in a POST" << Log::LO << Log::blockend;
							return;
						}
						string line = block.substr(0, lineend);
						if (line.compare(0, 31, "Content-Disposition: form-data;") == 0) {  //All will be skipped!
							size_t namestart = line.find("name=\"");
							if (namestart == string::npos) {
								*Logger::log << Log::error << Log::LI << "Error. Surprising end of post on name line" << Log::LO << Log::blockend;
								return;
							} else {
								namestart += 6;
								size_t namep = line.find_first_of("\"", namestart) - namestart;
								name.append(line.substr(namestart,namep));
								// namestart += namep; notused.
								// Now see if we have a filename.
								// find filename name-value
								size_t fnstart = line.find("filename=\"");
								if (fnstart != string::npos) {
									fnstart += 10;
									size_t fnamep = line.find_first_of("\"", fnstart) - fnstart;
									filename = line.substr(fnstart,fnamep);
								}
								// SECOND LINE The mime-type line. (May be empty)
								size_t linestart = lineend + 2;
								lineend = block.find("\x0d\x0a", linestart);
								if (lineend == string::npos) {
									*Logger::log << Log::error << Log::LI << "Error. Surprising end of post inside block" << Log::LO << Log::blockend;
									return;
								} else {
									line = block.substr(linestart, lineend - linestart);
									if (!line.empty()) {  // This is a mime-type line.
										size_t typestart = line.find("Content-Type: ");
										if (typestart == string::npos)
											continue;
										typestart += 14;    //Length of "Content-Type: "
										size_t typep = line.find_first_of("\" \f\r\n\t\v\0", typestart) - typestart;
										mimetype = line.substr(typestart,typep);   //We have now grabbed a content-type.
										//										*Logger::log << name <<" Content-Type:[" << mimetype << "] (must be set for media to be identified)." << Log::LO;
										lineend += 2;
									}
									// THIRD LINE. The value line.
									value = block.substr(lineend + 2, block.npos );  //The rest of the block is the value.
									//									*Logger::log <<  name <<" value:[" << value << "]" << Log::LO;
									if ( value.length() > 0 ) {
										//								*Logger::log << LI << "Posting parm:[" << name << "]=[" << value << "]" << LO; //should be in debug here..
										setparm("",name);
										setparm(name,value);
										numparms++;
										if (! filename.empty()) {
											size_t lastslash = filename.rfind('\\');
											string base;
											string ext;
											if (lastslash == filename.npos)
												base = filename;
											else
												base = filename.substr(lastslash + 1, filename.length());
											size_t dot = base.rfind('.');
											if (dot != base.npos) {
												ext = base.substr(dot + 1, base.npos);
												base = base.substr(0, dot);
											} else {
												ext = "";
											}
											string fbase(name+"_base");
											string fext(name+"_ext");
											string fname(name+"_name");
											//										*Logger::log << Log::LI << "Posting parm:[" << fname << "]=[" << fbase << "." << fext << "] file:[" << filename << "]=[" << base << "].[" << ext << "]" << Log::LO;
											setparm("",fname);
											setparm("",fbase);
											setparm("",fext);
											setparm(fname,filename);
											setparm(fbase,base);
											setparm(fext,ext);
											numparms+=3;
										}
										if (! mimetype.empty() ) {
											string lenval = String::tostring(static_cast<long long>(value.length()));
											string flength(name+"_length");
											setparm("",flength);
											setparm(flength,lenval);
											numparms++;
											string fmime(name+"_mime");
											setparm("",fmime);
											setparm(fmime,mimetype);
											numparms++;
											//This requires the Info structure
											//Now get any media information..
											int width=0;
											int height=0;
											string sheight,swidth;
											string fwidth(name+"_width");
											string fheight(name+"_height");
											istringstream ist(value);
											try {
												Info info( ist );
												if ( info.check() ) {
													width = info.getWidth();
													height = info.getHeight();
												} else {
													//*Logger::log << Log::LI << "File failed media info.check " << Log::LO;
												}
											} catch ( exception e ) {
												// Need to be silent here generally. Except for debugging..
											}
											String::tostring(swidth,width);
											String::tostring(sheight,height);
											setparm(fwidth,swidth); numparms++;
											setparm(fheight,sheight); numparms++;
											//----- media info finishes.
										} else {
											//*Logger::log <<"Content-Type is empty, so no file details were discovered." << Log::LO;
										}
									} else { // still add empty, if it isn't a file.
										if ( mimetype.empty() && filename.empty() ) {
											setparm("",name);
											setparm(name,value);
											numparms++;
										}
									}
								}
							} //end of find name test
						} else {
							setienv("POST_NON_FORM_DATA",line);
						}
						//						*Logger::log << Log::LO << Log::blockend;;
					} // end of postfinished test
				} // end of while
				if ( Logger::debugging() ) {
					*Logger::log << Log::info << Log::LI << "POST Processing Completed" << Log::LO << Log::blockend; //
				}
			} //if CONTENT_TYPE
		}
	}
}
//ok so here it can either be a 'name=value' or just 'value'
void Environment::setqsparm(string parmstring,unsigned long long) {
	size_t split =  parmstring.find('=');
	ostringstream parmnv;
	string parmn,parmv;
	if (split != string::npos && split > 0) {
		parmn = parmstring.substr(0,split);
		parmv = parmstring.substr(split+1,string::npos);
	} else {
		parmn = parmstring;
		parmv = "";
	}
	setparm(parmn,parmv); 			//foo=bar
	setparm("࿅",parmn);	// #n[1]=foo
	setparm("࿄",parmv);	// #v[1]=bar
}
void Environment::doparms(int argc, char *argv[]) {
	string qstr;
	if (argc < 0) { //called from process post..
		getparm("THIS_REQ_BODY",qstr);
		do_query_string(qstr);
	} else {
		getenv("QUERY_STRING",qstr); //Only set if using an Action. discard existence.
		if( qstr.empty() )	 { 	// Console executed
			if (argc > 1) {
				bool this_one = false;
				for(int i = 1; i < argc; i++) {
					string av = argv[i];
					if (av.compare("-c") == 0) {
						this_one = true;
					} else {
						if (this_one) {
							this_one=false;
						} else {
							string av = argv[i];
							String::urlencode(av);
							if (qstr.empty()) {
								qstr = av;
							} else {
								qstr = qstr + "&" + av;
							}
						}
					}
				}
				do_query_string(qstr);
				string console_file;
				string parm_name;
				if (version() < 1.110503 ) {
					parm_name = "_n[1]";
				} else {
					parm_name = "#n[1]";
				}
				if ( !envexists("REQUEST_METHOD") && (getparm(parm_name,console_file)) ) {
					setienv("PATH_TRANSLATED",console_file);
					setienv("OBYX_DEVELOPMENT","true");
					gDevelop = true;
				}
			}
		} else {
			do_query_string(qstr);
		}
	}
}
void Environment::do_query_string(string& qstr) {
	size_t start = 0;
	size_t find = 0;
	unsigned long long count = 0;
	string raw;
	while((find = qstr.find('&',start)) != string::npos) {
		raw = qstr.substr(start, find - start);
		String::urldecode(raw);
		setqsparm(raw,count); //lets see if it is a named parameter
		count++;
		start = find + 1;
	}
	if (start != qstr.length()) {
		raw = qstr.substr(start, qstr.length() - start);
		String::urldecode(raw);
		setqsparm(raw,count);
		count++;
	}
}
void Environment::setparm(string name,string value) {
	vector<string> tiny;
	vec_map_type::iterator it = parm_map.find(name);
	if (it != parm_map.end()) {
		tiny = it->second;
		parm_map.erase(it);
	}
	tiny.push_back(value);
	pair<vec_map_type::iterator, bool> ins = parm_map.insert(vec_map_type::value_type(name, tiny));
	if (!ins.second)	{ // Should never happen
		parm_map.erase(ins.first);
		parm_map.insert(vec_map_type::value_type(name,tiny));
	}
}
void Environment::dodocument() { //for POST values
	string input,inputlen,test_filename;
	File* test_file = nullptr;
	std::istream *iss = nullptr;
	if (getenv("TEST_FILE",test_filename)) { //This shouldn't be an osi!
		test_file = new File(test_filename);
		test_file->open(iss);
	} else {
		iss = &cin;
	}
	if (getenv("CONTENT_LENGTH",inputlen)) {
		pair<unsigned long long,bool> lenpr = String::znatural(inputlen);
		if (lenpr.second && iss != nullptr && iss->good()) { //very bad if it weren't a number!!
			streamsize cfound=0,clen = lenpr.first;
			try {
				char* content = new char[clen];
				iss->read(content, clen);
				cfound = iss->gcount();
				input = string(content,cfound);
				setparm("THIS_REQ_BODY",input);
				delete[] content;
			} catch (...) { }
		}
	} else {
		if (! envexists("GATEWAY_INTERFACE")) { //not cgi.
			const ssize_t buffsize=1024;
			ostringstream sb;
			int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
			fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
			char buffer[buffsize];
			ssize_t bytes_read = buffsize;
			while (buffsize == bytes_read) {
				bytes_read = read(STDIN_FILENO, buffer, buffsize);
				if (bytes_read > 0) {
					sb << string(buffer,bytes_read);
				}
			}
			setparm("THIS_REQ_BODY",sb.str());
		}
	}
	if (test_file != nullptr) { delete test_file;}
}
#pragma mark ENVIRONMENT (UTILITY)
void Environment::init_cgi_rfc_map() {
	cgi_rfc_map.insert(var_map_type::value_type("CONTENT_TYPE","Content-Type"));				//precise
	cgi_rfc_map.insert(var_map_type::value_type("CONTENT_LENGTH","Content-Length"));			//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_HOST","Host"));							//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_ACCEPT","Accept"));						//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_ACCEPT_ENCODING","Accept-Encoding"));		//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_ACCEPT_LANGUAGE","Accept-Language"));		//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_CONNECTION","Connection"));				//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_COOKIE","Cookie"));						//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_REFERER","Referer"));						//precise
	cgi_rfc_map.insert(var_map_type::value_type("HTTP_USER_AGENT","User-Agent"));				//precise
}
void Environment::initwlogger() {
	doparms(gArgc,gArgv);
	string fn;
	if (getenv("PATH_TRANSLATED",fn)) {
		Logger::set_path(fn);
		Logger::set_rpath(getpathforroot());
	}
	if (getenv("OBYX_AUTO_UTF8_CHECK",fn)) {
		do_auto_utf8check = (fn.compare("false") != 0); //ie, if it's not false then it's true.
	}
	dodocument();
	do_request_cookies();
	dopostparms();
}
void Environment::do_config_file(string& filepathstring) {
	//	if (!config_file_done) {
	//		config_file_done=true;
	string filecontainer;
	if (filepathstring[0] != '/') {
		filepathstring = FileUtils::Path::wd() + '/' +filepathstring;
	}
	FileUtils::Path destination;
	destination.cd(filepathstring);
	string final_file_path = destination.output(true);
	FileUtils::File file(final_file_path);
	if (file.exists()) {
		off_t flen = file.getSize();
		if ( flen != 0 ) {
			file.readFile(filecontainer);
			istringstream iss(filecontainer);
			string env_setting;
			//Format is ENV_VAR(wspace)(ENV_SETTING)(newline)+
			while( getline(iss,env_setting) ) {
				string envname,envval;
				String::trim(env_setting);
				string::size_type pos = env_setting.find_first_of("\t \r\n");
				if (pos != string::npos) {
					envname = env_setting.substr(0, pos);
					if (!envname.empty() && envname[0] != '#') {
						envval = env_setting.substr(env_setting.find_first_not_of("\t \r\n",pos+1), string::npos);
						String::strip(envval);
						setbenv(envname,envval);
					}
				}
			}
		}
	}
	
	//	}
}
bool Environment::sortvps(pair<string,string> n1,pair<string,string> n2) {
	return (n1.first.compare(n2.first) < 0) ? true : false;
}
bool Environment::sortvvps(pair<string,vector<string>* > n1,pair<string,vector<string>* > n2) {
	return (n1.first.compare(n2.first) < 0) ? true : false;
}
void Environment::setbasetime() {
#ifdef __MACH__
	basetime = mach_absolute_time();
    mach_timebase_info(&time_info);
	nano = 1e-9 * ( (long double) time_info.numer) / ((long double) time_info.denom);
#else
	clockid_t clock_id; clock_getcpuclockid(pid,&clock_id);
	timespec tb; clock_gettime(clock_id,&tb);
	basetime = ((unsigned)tb.tv_sec + (0.000000001 * (unsigned)tb.tv_nsec));
#endif
}
void Environment::setienv(string name,string value) {
	//Called before LOGGER is initialised (but still goes into ienv), once per request
	pair<var_map_type::iterator, bool> ins = ienv_map.insert(var_map_type::value_type(name, value));
	if (!ins.second)	{ // Cannot insert (something already there with same ref
		ienv_map.erase(ins.first);
		ienv_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::setbenvmap() {//per box/process environment
	unsigned int eit = 0;
	while ( environ[eit] != nullptr ) {
		string parmstring = environ[eit++];
		size_t split =  parmstring.find('=');
		if (split != string::npos && split > 0 ) {
			string n = parmstring.substr(0,split);
			string v = parmstring.substr(split+1,string::npos);
			//#ifdef FAST
			//Apache doesn't use 'name=value'. it uses 'name value'
			//			while(v.size() > 0 && v[v.size()-1] == '=') { //weird fastcgi shite.
			//				v.resize(v.size()-1);
			//			}
			//#endif
			setbenv(n,v);
			if (n.compare("OBYX_CONFIG_FILE") == 0) { //for virtual hosts shouldn't be set here.
				do_config_file(v);
			}
		}
	}
}
string Environment::SQLconfig_file() {
	string result("");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLCONFIG_FILE",result)) {
			instance->getenv("OBYX_SQLCONFIG_FILE",result);
		}
	} else {
		getbenv("OBYX_SQLCONFIG_FILE",result);
	}
	return result;
}
string Environment::Database() {
	string result("OBYX_SQLDATABASE");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLDATABASE",result)) {
			instance->getenv("OBYX_SQLDATABASE",result);
		}
	} else {
		getbenv("OBYX_SQLDATABASE",result);
	}
	return result;
}
string Environment::SQLhost() {
	string result("");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLHOST",result)) {
			instance->getenv("OBYX_SQLHOST",result);
		}
	} else {
		getbenv("OBYX_SQLHOST",result);
	}
	return result;
}
string Environment::SQLuser() {
	string result("");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLUSER",result)) {
			instance->getenv("OBYX_SQLUSER",result);
		}
	} else {
		getbenv("OBYX_SQLUSER",result);
	}
	return result;
}
string Environment::SQLuserPW() {
	string result("");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLUSERPW",result)) {
			instance->getenv("OBYX_SQLUSERPW",result);
		}
	} else {
		getbenv("OBYX_SQLUSERPW",result);
	}
	return result;
}
unsigned int Environment::SQLport() {
	string result("");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SQLPORT",result)) {
			instance->getenv("OBYX_SQLPORT",result);
		}
	} else {
		getbenv("OBYX_SQLPORT",result);
	}
	pair<long long int,bool>port = String::integer(result);
	return (int)port.first;
}
string Environment::Salt() {
	string result("[s4lt]");
	if (instance != nullptr) {
		if (!instance->getenv("OBYX_SALT",result)) {
			instance->getenv("OBYX_SALT",result);
		}
	} else {
		if (!getbenv("OBYX_SALT",result)) {
			result.append(SQLuserPW());
		}
	}
	return result;
}
void Environment::setbenv(string name,string value) {
	//Called before LOGGER is initialised, once per process.
	pair<var_map_type::iterator, bool> ins = benv_map.insert(var_map_type::value_type(name, value));
	if (!ins.second)	{ // Cannot insert (something already there with same ref - so skip it...
		benv_map.erase(ins.first);
		benv_map.insert(var_map_type::value_type(name, value));
	}
}
void Environment::resetenv(const string name) { //? used to reset per-request values.
	var_map_type::iterator i = ienv_map.find(name);
	if (i != ienv_map.end()) {
		ienv_map.erase(i);
	}
	var_map_type::iterator j = benv_map.find(name);
	if (j != benv_map.end()) {
		benv_map.erase(j);
	}
}
void Environment::setienvmap(char ** environment) {
	unsigned int eit = 0;
	while ( environment[eit] != nullptr ) {
		string parmstring = environment[eit++];
		size_t split =  parmstring.find('=');
		if (split != string::npos && split > 0 ) {
			string n = parmstring.substr(0,split);
			string v = parmstring.substr(split+1,string::npos);
			//#ifdef FAST
			//apache doesn't use 'name=value'. it uses 'name value'
			//			if(v.size() > 0 && v[v.size()-1] == '=') { //weird fastcgi shite.
			//				v.resize(v.size()-1);
			//			}
			//#endif
			setienv(n,v);
			var_map_type::iterator it = cgi_rfc_map.find(n);
			if (it != cgi_rfc_map.end()) {
				httphead_map.insert(var_map_type::value_type(it->second,v));
			} else {
				if (n.find("HTTP_") == 0) {
					string special=n.substr(5,string::npos);
					String::cgi_to_http(special);
					httphead_map.insert(var_map_type::value_type(special,v));
				}
			}
		}
	}
}
void Environment::getenvvars_base() {
	//Called once for every instance!
	string envtmp;
	pair<unsigned long long,bool> tmp;
	if (getenv("OBYX_CONFIG_FILE",envtmp)) {
		do_config_file(envtmp);
	}
	if (getenv("OBYX_ROOT_DIR",gRootDir)) {
		if ( gRootDir[gRootDir.size()-1] != '/') gRootDir+='/';
	} else {
		if (getenv("OBYX_DEFAULT_ROOT_DIR",gRootDir)) {	//fallback
			if ( gRootDir[gRootDir.size()-1] != '/') gRootDir+='/';
		}
	}
	if (getenv("OBYX_SQLPORT",envtmp)) {
		tmp = String::znatural(envtmp);
		if (tmp.second) gSQLport = (unsigned int)tmp.first;
	}
	
	gDevelop = getenvtf("OBYX_DEVELOPMENT");
	if (getenv("OBYX_SCRIPTS_DIR",gScriptsDir)) { //defaults to nothing.
		if ( gScriptsDir[gScriptsDir.size()-1] != '/') gScriptsDir+='/';
	}
	if (getenvd("OBYX_SCRATCH_DIR",gScratchDir,"/tmp/")) { //defaults to /tmp/.
		if ( gScratchDir[gScratchDir.size()-1] != '/') gScratchDir+='/';
	}
	
	string version_str;
	if (getenv("OBYX_DEFAULT_VERSION",version_str)) {
		double e_version=String::real(version_str);
		if (! std::isnan(e_version)) {
			runtime_version.push(e_version);
		}
	}
}
void Environment::getenvvars() {
	//Called before LOGGER is initialised.
	string ps;
	if (!getenv("PATH_TRANSLATED",ps)) { //FAST is more likely to be called via script.
		if (getenv("SCRIPT_FILENAME",ps)) {
			size_t pslen = ps.size();
			if (ps.substr(pslen-3,3).compare("cgi") != 0) {
				setienv("PATH_TRANSLATED",ps);
			}
		}
	}
	if (gArgc > 0) {
		bool this_one = false;
		for(int i = 1; i < gArgc; i++) {
			string av = gArgv[i];
			if (av.compare("-c") == 0) { 	// none of this currently works
				this_one = true;			// because vdb/zip etc. dlopen runs in startup and
			} else {						// cli is processed at init
				if (this_one) {
					do_config_file(av);
					this_one=false;
				}
			}
		}
	}
}
bool Environment::getenvd(const string name,string& container,const string defaultval) {
	bool found = getenv(name,container);
	if ( ! found ) container = defaultval;
	return found;
}
bool Environment::getbenv(string const name,string& container) {	//used for base configuration settings.
	bool retval = false;
	container.clear();  //should we clear this?
	var_map_type::iterator it = benv_map.find(name);
	if (it != benv_map.end()) {
		container = ((*it).second);
		retval = true;
	}
	return retval;
}
bool Environment::getbenvtf(string const name,const bool undef) {	//used for base configuration settings.
	bool retval = undef;
	var_map_type::iterator it = benv_map.find(name);
	if (it != benv_map.end()) {
		retval= (*it).second.compare("true") == 0;
	}
	return retval;
}
bool Environment::benvexists(const string&  name) {	//used for base configuration settings.
	var_map_type::iterator it = benv_map.find(name);
	return (it != benv_map.end());
}
#pragma mark ENVIRONMENT (PUBLIC)
string Environment::ScratchName() {
	string unique;
	uniq(unique);
	string scratchname = 'p' + unique + '_';
	return scratchname;
}
void Environment::uniq(string& basis) {
	string errs;
	if ( String::Digest::available(errs) ) {
		String::Digest::random(basis,16);
	} else {
		File f("/dev/random");
		f.readFile(basis,16,S_IFCHR);
	}
	String::tohex(basis);
}
bool Environment::envfind(const string& pattern) { //regex..
	bool retval = false;
	if ( String::Regex::available() ) {
		for(var_map_type::iterator imt = ienv_map.begin(); !retval && imt != ienv_map.end(); imt++) {
			retval= String::Regex::match(pattern,imt->first);
		}
		for(var_map_type::iterator imt = benv_map.begin(); !retval && imt != benv_map.end(); imt++) {
			retval= String::Regex::match(pattern,imt->first);
		}
	} else {
		retval = envexists(pattern);
	}
	return retval;
}
bool Environment::parmfind(const string& pattern) { //regex..
	bool retval = false;
	if ( String::Regex::available() ) {
		for(vec_map_type::iterator imt = parm_map.begin(); !retval && imt != parm_map.end(); imt++) {
			retval= String::Regex::match(pattern,imt->first);
		}
	} else {
		retval = parmexists(pattern);
	}
	return retval;
}
bool Environment::cookiefind(const string& pattern) { //request cookies...
	bool retval = false;
	if ( String::Regex::available() ) {
		for(var_map_type::iterator imt = cke_map.begin(); !retval && imt != cke_map.end(); imt++) {
			retval= String::Regex::match(pattern,imt->first);
		}
	} else {
		retval = cookieexists(pattern);
	}
	return retval;
}
void Environment::envkeys(const string& pattern,set<string>& keylist) {
	if ( String::Regex::available() ) {
		for(var_map_type::iterator imt = ienv_map.begin(); imt != ienv_map.end(); imt++) {
			if (String::Regex::match(pattern,imt->first)) {
				keylist.insert(imt->first);
			}
		}
		for(var_map_type::iterator imt = benv_map.begin(); imt != benv_map.end(); imt++) {
			if (String::Regex::match(pattern,imt->first)) {
				keylist.insert(imt->first);
			}
		}
	} else {
		if (envexists(pattern)) {
			keylist.insert(pattern);
		}
	}
}
void Environment::parmkeys(const string& pattern,set<string>& keylist) {
	if ( String::Regex::available() ) {
		for(vec_map_type::iterator imt = parm_map.begin(); imt != parm_map.end(); imt++) {
			string pname=imt->first;
			if (!(pname.compare("࿅") == 0 || pname.compare("࿄") == 0) && String::Regex::match(pattern,pname)) {
				keylist.insert(pname);
			}
		}
	} else {
		if (parmexists(pattern)) {
			keylist.insert(pattern);
		}
	}
}
void Environment::cookiekeys(const string& pattern,set<string>& keylist) {
	if ( String::Regex::available() ) {
		for(var_map_type::iterator imt = cke_map.begin(); imt != cke_map.end(); imt++) {
			if (String::Regex::match(pattern,imt->first)) {
				keylist.insert(imt->first);
			}
		}
	} else {
		if (cookieexists(pattern)) {
			keylist.insert(pattern);
		}
	}
}
bool Environment::parmexists(const string& name_i) { //regex..
	bool retval=false;
	string name(name_i);
	if (version() < 1.110503 && !name.empty() && name[0]=='_') {
		name[0] = '#';
	}
	size_t veclen = 0;
	pair<string,string> result;
	if ( String::rsplit('#',name,result) && (!result.second.empty()) ) {
		if (!result.first.empty()) {
			vec_map_type::iterator it = parm_map.find(result.first);
			if (it != parm_map.end()) {
				veclen = it->second.size();
			}
			string::const_iterator numit = result.second.begin()+2;
			size_t index = String::natural(numit);
			if (index <= veclen) {
				retval = true;
			}
		} else {
			vec_map_type::iterator it = parm_map.find("࿅");
			if (it != parm_map.end()) {
				veclen = it->second.size();
			}
			string::const_iterator numit = result.second.begin()+2;
			size_t index = String::natural(numit);
			if (index <= veclen) {
				retval = true;
			}
		}
	} else {
		retval = (parm_map.find(name) != parm_map.end()) ;
	}
	return retval;
}
bool Environment::cookieexists(const string& name) { //request cookies...
	var_map_type::iterator it = cke_map.find(name);
	return (it != cke_map.end());
}
bool Environment::getenvtf(const string& name,const bool undef) {	//used for base configuration settings.
	bool retval = undef;
	var_map_type::iterator it = ienv_map.find(name);
	if (it != ienv_map.end()) {
		retval = (*it).second.compare("true") == 0;
	} else {
		retval = getbenvtf(name,undef);
	}
	return retval;
}
bool Environment::envexists(const string& name) {
	bool retval = false;
	var_map_type::iterator it = ienv_map.find(name);
	if (it != ienv_map.end()) {
		retval = true;
	} else {
		var_map_type::iterator it = benv_map.find(name);
		retval = (it != benv_map.end());
	}
	return retval;
}
bool Environment::getenv(const string name,string& container) {
	bool retval = false;
	container.clear();  //should we clear this?
	var_map_type::iterator bt = ienv_map.find(name);
	if (bt != ienv_map.end()) {
		container = ((*bt).second);
		retval = true;
	} else {
		var_map_type::iterator it = benv_map.find(name);
		if (it != benv_map.end()) {
			container = ((*it).second);
			retval = true;
		}
	}
	return retval;
}
bool Environment::getparm(string const name_i,string& container) {
	bool retval = false;
	string name(name_i);
	if (version() < 1.110503 && !name.empty() && name[0]=='_') {
		name[0] = '#';
	}
	pair<string,string> result;
	container.clear();
	vec_map_type::iterator it = parm_map.find(name);
	if (it != parm_map.end()) {
		vector<string> parmvals = it->second;
		container = parmvals.back();
		retval = true;
	} else {
		if (String::rsplit('#',name,result) && (!result.second.empty())) {
			size_t index = 0;
			if (!result.first.empty()) { // ie, a named parameter.
				vec_map_type::iterator ip = parm_map.find(result.first);
				if (ip != parm_map.end()) {
					if (result.second.compare("count") == 0) {
						index = ip->second.size();
						String::tostring(container,(unsigned long long)index);
						retval = true;
					} else {
						//a value may be accessed via eg myparm#2  myparm#v[2]
						if (result.second.size() >= 4 && (result.second[1] == '[') &&(result.second[0] == 'n' || result.second[0] == 'v')) {
							if (result.second[0] == 'n') {
								container = result.first;
								retval = true;
							} else { //value.
								string::const_iterator numit = result.second.begin()+2;
								index = String::natural(numit);
								if (index - 1 < ip->second.size()) {
									container = ip->second[index-1];
									retval = true;
								} //else false.
							}
						} else { // a direct number?
							pair<unsigned long long,bool> idx = String::znatural(result.second);
							if (idx.second && (size_t)(idx.first - 1) < ip->second.size()) {
								container = ip->second[(size_t)idx.first - 1];
								retval = true;
							}
						} //else false.
					}
				}
			} else { //This starts with a #
				if (!result.second.empty()) {
					if (result.second.compare("count") == 0) {
						vec_map_type::iterator it = parm_map.find("࿅"); //get the names list.
						if (it != parm_map.end()) {
							index = it->second.size();
						}
						String::tostring(container,(unsigned long long)index);
						retval = true;
					} else { //not count. either with the
						if (result.second.size() >= 4 && (result.second[1] == '[') &&(result.second[0] == 'n' || result.second[0] == 'v')) {
							if (result.second[0] == 'n') {
								vec_map_type::iterator it = parm_map.find("࿅"); //get the names list.
								if (it != parm_map.end()) {
									string::const_iterator numit = result.second.begin()+2;
									index = String::natural(numit);
									if (index - 1 < it->second.size()) {
										retval = true;
										container = it->second[index-1];
									}
								} //else false.
							} else { //value.
								vec_map_type::iterator it = parm_map.find("࿄"); //get the values list.
								if (it != parm_map.end()) {
									string::const_iterator numit = result.second.begin()+2;
									index = String::natural(numit);
									if (index - 1 < it->second.size()) {
										retval = true;
										container = it->second[index-1];
									} //else false.
								}
							}
						} else { // a direct number?
							vec_map_type::iterator it = parm_map.find("࿅"); //get the names list.
							if (it != parm_map.end()) {
								pair<unsigned long long,bool> idx = String::znatural(result.second);
								if (idx.second && (size_t)(idx.first - 1) < it->second.size()) {
									retval = true;
									container = it->second[(size_t)idx.first - 1];
								}
							}
						}
					}
				}
			}
		}
	}
	return retval;
}
string Environment::getpathforroot() {
	//return the path for root ie if a local url is prefixed / return the path to that.
	string the_result = gRootDir;
	if (the_result.empty()) { //return current working directory.
		the_result = Path::wd();
	}
	return the_result;
}
void Environment::gettiming(string& result) {
	ostringstream ost;
	long double timing = 0;
#ifdef __MACH__
	uint64_t delta = mach_absolute_time() - basetime;
	timing = ((double) delta) * nano;
#else
	clockid_t clock_id; clock_getcpuclockid(pid,&clock_id);
	timespec tb; clock_gettime(clock_id,&tb);
	timing = ((unsigned)tb.tv_sec + (0.000000001 * (unsigned)tb.tv_nsec)) - basetime;
#endif
	ost << fixed << setprecision(16L) << timing;
	result = ost.str();
}
void Environment::getresponsehttp(string& result) {
	Httphead* http = Httphead::service();
	http->explain(result);
}
void Environment::getrequesthttp(string& head,string& body) {
	//This is recomposited from environment.
	ostringstream rh;
	string method,version,url,tmp;
	if (!getenv("REQUEST_METHOD",method)) {
		method = "CONSOLE";
	}
	if (getenv("SCRIPT_URI",url)) {
		if (getenv("QUERY_STRING",tmp) && ! tmp.empty() ) {
			url.append("?");
			url.append(tmp);
		}
	} else {
		getenv("PATH_TRANSLATED",url);
	}
	if (!getenv("SERVER_PROTOCOL",version)) {
		version = "SYSTEM";
	}
	rh << method << " " << url << " " << version << Httphead::crlf;
	for(var_map_type::iterator imt = httphead_map.begin(); imt != httphead_map.end(); imt++) {
		rh << imt->first << ":" << imt->second << Httphead::crlf;
	}
	//	content-type
	head = rh.str();
	getparm("THIS_REQ_BODY",body);
}
#pragma mark Debugging/Disclosure
void Environment::listParms() {
	vector<pair<string,vector<string>* > >vmp;
	for(vec_map_type::iterator imt = parm_map.begin(); imt != parm_map.end(); imt++) {
		vmp.push_back(pair<string,vector<string>* >(imt->first,&(imt->second)));
	}
	if (!vmp.empty()) {
		*Logger::log << Log::subhead << Log::LI << "List of sysparms" << Log::LO;
		*Logger::log << Log::LI << Log::even ;
		std::sort(vmp.begin(),vmp.end(),sortvvps);
		for(vector<pair<string,vector<string>* > >::iterator vmpi = vmp.begin(); vmpi != vmp.end(); vmpi++) {
			string name=vmpi->first;
			if (name.compare("࿅") == 0) { name = "#n[]";
			} else {if (name.compare("࿄") == 0) { name = "#v[]"; }	}
			*Logger::log << Log::LI << Log::II << name << Log::IO << Log::II;
			vector<string>* vals = vmpi->second;
			unsigned int i=0;
			if (vals->size() == 1) {
				*Logger::log << (*vals)[0];
			} else {
				for(vector<string>::iterator vmpv = vals->begin(); vmpv != vals->end(); vmpv++) {
					*Logger::log << 1+i++ << ":[" << *vmpv << "] ";
				}
			}
			*Logger::log << Log::IO << Log::LO;
		}
		
		*Logger::log << Log::blockend << Log::LO << Log::blockend ; //even .. subhead.
	}
}
void Environment::listReqCookies() {
	vector<pair<string,string> >vmc;
	for(var_map_type::iterator imt = cke_map.begin(); imt != cke_map.end(); imt++) {
		vmc.push_back(pair<string,string>(imt->first,imt->second));
	}
	if (!vmc.empty()) {
		*Logger::log << Log::subhead  << Log::LI << "List of request cookies" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		std::sort(vmc.begin(), vmc.end(), sortvps);
		for(vector<pair<string,string> >::iterator vmci = vmc.begin(); vmci != vmc.end(); vmci++) {
			*Logger::log << Log::LI << Log::II << vmci->first << Log::IO << Log::II << vmci->second << Log::IO << Log::LO;
			ostringstream vck; var_map_type::iterator itd;
			vck << "Request Cookie name=\"" << vmci->first <<  "\" value=\"" << vmci->second << "\"";
			itd = cke_domain_map.find(vmci->first);
			if (itd != cke_domain_map.end()) vck << " domain=\"" << ((*itd).second) << "\"";
			itd = cke_path_map.find(vmci->first);
			if (itd != cke_path_map.end()) vck << " path=\"" << ((*itd).second) << "\"";
			itd = cke_expires_map.find(vmci->first);
			if (itd != cke_expires_map.end()) {
				vck << " expires=\"" << ((*itd).second) << "\"";
			}
			*Logger::log << Log::LI << vck.str() << Log::LO;
		}
		*Logger::log << Log::blockend << Log::LO << Log::blockend ; //even .. subhead.
	}
}
void Environment::listResCookies() {
	vector<pair<string,string> >vmc;
	for(var_map_type::iterator imt = ck_map.begin(); imt != ck_map.end(); imt++) {
		vmc.push_back(pair<string,string>(imt->first,imt->second));
	}
	if (!vmc.empty()) {
		std::sort(vmc.begin(), vmc.end(), sortvps);
		*Logger::log << Log::subhead << Log::LI << "List of response cookies" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		for(vector<pair<string,string> >::iterator vmci = vmc.begin(); vmci != vmc.end(); vmci++) {
			ostringstream vck; var_map_type::iterator itd;
			vck << "Response Cookie name=\"" << vmci->first <<  "\" value=\"" << vmci->second << "\"";
			itd = ck_domain_map.find(vmci->first);
			if (itd != ck_domain_map.end()) vck << " domain=\"" << ((*itd).second) << "\"";
			itd = ck_path_map.find(vmci->first);
			if (itd != ck_path_map.end()) vck << " path=\"" << ((*itd).second) << "\"";
			itd = ck_expires_map.find(vmci->first);
			if (itd != ck_expires_map.end()) {
				vck << " expires=\"" << ((*itd).second) << "\"";
			}
			*Logger::log << Log::LI << vck.str() << Log::LO;
		}
		*Logger::log << Log::blockend << Log::LO << Log::blockend ; //even .. subhead.
	}
}
void Environment::list() {
	*Logger::log << Log::subhead << Log::LI << "Environment" << Log::LO << Log::LI ;
	listEnv();
	listParms();
	listReqCookies();
	listResCookies();
	*Logger::log << Log::LO << Log::blockend;
}
void Environment::listEnv() {
	vector<pair<string,string> >vme;
	for(var_map_type::iterator imt = ienv_map.begin(); imt != ienv_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	for(var_map_type::iterator imt = benv_map.begin(); imt != benv_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	if (!vme.empty()) {
		*Logger::log << Log::subhead << Log::LI << "List of sysenv" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		std::sort(vme.begin(), vme.end(), sortvps);
		for(vector<pair<string,string> >::iterator vmei = vme.begin(); vmei != vme.end(); vmei++) {
			if (( (vmei->first.find("OBYX_SQL",0,8) != 0) && (vmei->first.compare("OBYX_SALT") != 0)) || (vmei->first.compare("OBYX_SQLPER_REQUEST") == 0)  || (vmei->first.compare("OBYX_SQLCONFIG_FILE") == 0)) {
				//			if ( vmei->first.find("OBYX_",0,5) != 0) {
				*Logger::log << Log::LI << Log::II << vmei->first << Log::IO << Log::II << vmei->second << Log::IO << Log::LO;
			}
		}
		*Logger::log << Log::blockend << Log::LO << Log::blockend ; //even .. subhead.
	}
}
void  Environment::list(string& result) {
	ostringstream buffer;
	
	vector<pair<string,string> >vme;
	for(var_map_type::iterator imt = ienv_map.begin(); imt != ienv_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	for(var_map_type::iterator imt = benv_map.begin(); imt != benv_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	std::sort(vme.begin(), vme.end(), sortvps);
	for(vector<pair<string,string> >::iterator vmei = vme.begin(); vmei != vme.end(); vmei++) {
		if (( vmei->first.find("OBYX_SQL",0,8) != 0) && (vmei->first.compare("OBYX_SALT") != 0)) {
			buffer << Log::info << Log::LI << "Environment " << Log::II << vmei->first << Log::IO << Log::II << vmei->second << Log::IO << LO << blockend;
		}
	}
	vector<pair<string,string> >vmp;
	for(vec_map_type::iterator imt = parm_map.begin(); imt != parm_map.end(); imt++) {
		vector<string> vals=imt->second;
		for(vector<string>::iterator ivt = vals.begin(); ivt != vals.end(); ivt++) {
			vmp.push_back(pair<string,string>(imt->first,*ivt));
		}
	}
	std::sort(vmp.begin(), vmp.end(), sortvps);
	for(vector<pair<string,string> >::iterator vmpi = vmp.begin(); vmpi != vmp.end(); vmpi++) {
		buffer << Log::LI << "SystemParm " << Log::II << vmpi->first << Log::IO << Log::II << vmpi->second << Log::IO << Log::LO;
	}
	vector<pair<string,string> >vmc;
	for(var_map_type::iterator imt = ck_map.begin(); imt != ck_map.end(); imt++) {
		vmc.push_back(pair<string,string>(imt->first,imt->second));
	}
	std::sort(vmc.begin(), vmc.end(), sortvps);
	for(vector<pair<string,string> >::iterator vmci = vmc.begin(); vmci != vmc.end(); vmci++) {
		ostringstream vck; var_map_type::iterator itd;
		vck << "Response Cookie name=\"" << vmci->first <<  "\" value=\"" << vmci->second << "\"";
		itd = ck_domain_map.find(vmci->first);
		if (itd != ck_domain_map.end()) vck << " domain=\"" << ((*itd).second) << "\"";
		itd = ck_path_map.find(vmci->first);
		if (itd != ck_path_map.end()) vck << " path=\"" << ((*itd).second) << "\"";
		itd = ck_expires_map.find(vmci->first);
		if (itd != ck_expires_map.end()) vck << " expires=\"" << ((*itd).second) << "\"";
		buffer << vck.str() << "\r";
	}
	vmc.clear();
	for(var_map_type::iterator imt = cke_map.begin(); imt != cke_map.end(); imt++) {
		vmc.push_back(pair<string,string>(imt->first,imt->second));
	}
	std::sort(vmc.begin(), vmc.end(), sortvps);
	for(vector<pair<string,string> >::iterator vmci = vmc.begin(); vmci != vmc.end(); vmci++) {
		ostringstream vck; var_map_type::iterator itd;
		vck << "Request Cookie name=\"" << vmci->first <<  "\" value=\"" << vmci->second << "\"";
		itd = cke_domain_map.find(vmci->first);
		if (itd != cke_domain_map.end()) vck << " domain=\"" << ((*itd).second) << "\"";
		itd = cke_path_map.find(vmci->first);
		if (itd != cke_path_map.end()) vck << " path=\"" << ((*itd).second) << "\"";
		itd = cke_expires_map.find(vmci->first);
		if (itd != cke_expires_map.end()) vck << " expires=\"" << ((*itd).second) << "\"";
		buffer << vck.str() << "\r";
	}
	result = buffer.str();
}
//### FILE ENDS ###

