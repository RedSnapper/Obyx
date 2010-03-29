/*
 * environment.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * environment.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <time.h>
#include <sys/times.h>
#include <termios.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <iostream>
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

typedef hash_map<const string,string, hash<const string&> > var_map_type;
var_map_type Environment::env_map;

var_map_type Environment::ck_map;
var_map_type Environment::ck_expires_map;
var_map_type Environment::ck_path_map;
var_map_type Environment::ck_domain_map;

var_map_type Environment::cke_map;
var_map_type Environment::cke_expires_map;
var_map_type Environment::cke_path_map;
var_map_type Environment::cke_domain_map;

var_map_type Environment::parm_map;
var_map_type Environment::httphead_map;

bool Environment::gDevelop = false;
int Environment::gSQLport = 0;
string Environment::gDatabase="";
string Environment::gRootDir="";
string Environment::gScriptsDir="";
string Environment::gScratchDir="/tmp/";
string Environment::gScratchName="";
string Environment::gSQLhost="";
string Environment::gSQLuser="";
string Environment::gSQLuserPW="";

Environment::buildarea_type Environment::the_area = Console;

bool Environment::dosetdebugger = false;
double Environment::basetime = 0;	//used for timing.
std::string Environment::empty = "";	//used for environment.
std::string Environment::parmprefix= ""; //used to prefix parameter numbers

int Environment::gArgc=0;
char **Environment::gArgv=NULL;

//public methods
//---------------------------------------------------------------------------

void Environment::init(int argc, char *argv[]) {
	gArgc=argc;
	gArgv=argv;
	setbasetime();
	ck_map.clear();
	ck_expires_map.clear();
	ck_path_map.clear();
	ck_domain_map.clear();
	cke_map.clear();
	cke_expires_map.clear();
	cke_path_map.clear();
	cke_domain_map.clear();
	parm_map.clear();
	getenvvars();
	if ( gDevelop && envexists("LOG_DEBUG")) {
		dosetdebugger = true;
	}
}

//called by Logger. - Can use Logger messaging here.
void Environment::initwlogger() {
	setparm("_count","0");	//Just in case there are none.
	doparms(gArgc,gArgv);
	dodocument();

//-- new inserts done	
	if (dosetdebugger) {
		Httphead::doheader();
		switch (the_area) {
			case Root:		*Logger::log << Log::debug << Log::LI << "Site area: Root " << Log::LO << Log::blockend; break;
			case Public:	*Logger::log << Log::debug << Log::LI << "Site area: Public " << Log::LO << Log::blockend; break;
			case Console:	*Logger::log << Log::debug << Log::LI << "Site area: Console "  << Log::LO << Log::blockend; break;
			default: break;
		}
	}
	Environment::do_request_cookies();
	dopostparms();	
	if (dosetdebugger) { list(); }
}


//-------------------------------- COOKIES ---------------------------------
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
				bool urlenc = true;
				std::string ck_val = this_cookie_value;
				String::urlencode(ck_val);
				if (this_cookie_value.compare(ck_val) == 0) urlenc = false;
				if (explaining) {
					XMLChar::encode(this_cookie_name);
					cookiestream << "<m:header name=\"Set-Cookie\" cookie=\"" << this_cookie_name << "\">";
					cookiestream << "<m:subhead name=\"" << this_cookie_name << "\" value=\"" << ck_val << "\"";
					if (urlenc) { cookiestream << " urlencoded=\"true\""; }
					cookiestream << "/>";
				} else {
					cookiestream << Httphead::cookiesig << ": " << this_cookie_name << "=" << ck_val;
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
	if ( Logger::debugging() ) {
		*Logger::log << Log::debug << Log::LI;
		*Logger::log << Log::subhead << Log::LI << "Cookie Processing Initiated" << Log::LO << Log::blockend;
		*Logger::log << Log::LO;
	}
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
	} else {
		if ( Logger::debugging() ) { 
			*Logger::log << info << Log::LI << "Cookie Processing skipped: no HTTP_COOKIE environmental" << Log::LO << blockend;
		}
	}
	if ( Logger::debugging() ) {
		*Logger::log << Log::LI << "Cookie Processing finished" << Log::LO << Log::blockend;
	}
}

//Cookies done
//----------------------------------------------------------------------------------
//
bool Environment::getparm(string const name,string& container) {
	bool retval = false;
	container.clear();
	var_map_type::iterator it = parm_map.find(name);
	if (it != parm_map.end()) {
		container = ((*it).second);
		retval = true;
	}
	return retval;
}

bool Environment::envexists(string const name) {
	bool retval = false;
	if ( env_map.empty() ) {
		string dummy;
		retval = Environment::getenv(name,dummy);
	}
	var_map_type::iterator it = env_map.find(name);
	if (it != env_map.end()) {
		retval = true;
	} 
	return retval;
}

void Environment::dodocument() { //for POST values
	string input,test_filename;
	if (getenv("TEST_FILE",test_filename)) {
		File test_file(test_filename);
		test_file.readFile(input);
		setparm("THIS_REQ_BODY",input);	
		setenv("INF","testfile");	
	} else {
		if (envexists("GATEWAY_INTERFACE")) { //this is a cgi.
			ostringstream sb;
			while ( std::cin >> sb.rdbuf() );			
			input = sb.str();
			setparm("THIS_REQ_BODY",input);	
			setenv("INF","cgi");	
		} else {
			struct termios tio;
			if( tcgetattr(0,&tio) < 0) { //basically, if we cannot get the struct for cin, we can read this from the buffer. weird.
				ostringstream sb;
				while ( std::cin >> sb.rdbuf() );			
				input = sb.str();
				setparm("THIS_REQ_BODY",input);	
				setenv("INF","cin");	
			}
		}
	}
}

//Called before LOGGER installed.
bool Environment::getenv(string const name,string& container) {
	bool retval = false;
	if ( env_map.empty() ) {
		unsigned int eit = 0;
		while ( environ[eit] != NULL ) {
			string parmstring = environ[eit++];
			size_t split =  parmstring.find('=');
			if (split != string::npos && split > 0 ) {
				string n = parmstring.substr(0,split);
				string v = parmstring.substr(split+1,string::npos);
				if ( n.find("HTTP_",0,5) != string::npos) {
					string hn = n.substr(5,string::npos);
					String::tolower(hn);
					String::fandr(hn,'_','-');
					pair<var_map_type::iterator, bool> ins = httphead_map.insert(var_map_type::value_type(hn,v));
				} else {
					if ( n.find("CONTENT_",0,8) != string::npos) {
						if ( n.compare("CONTENT_LENGTH") != 0) {
							string hn = n;
							String::tolower(hn);
							String::fandr(hn,'_','-');
							pair<var_map_type::iterator, bool> ins = httphead_map.insert(var_map_type::value_type(hn,v));
						}
					}
				}
				setenv(n,v);
			}
		}
	}
	container.clear();  //should we clear this?
	var_map_type::iterator it = env_map.find(name);
	if (it != env_map.end()) {
		container = ((*it).second);
		retval = true;
	} 
	return retval;
}

bool Environment::getenvd(const string name,string& container,const string defaultval) {
	bool found = getenv(name,container);
	if ( ! found ) container = defaultval;
	return found;
}

void Environment::listParms() {
	vector<pair<string,string> >vmp;
	for(var_map_type::iterator imt = parm_map.begin(); imt != parm_map.end(); imt++) {
		vmp.push_back(pair<string,string>(imt->first,imt->second));
	}
	if (!vmp.empty()) {
		*Logger::log << Log::subhead << Log::LI << "List of sysparms" << Log::LO;
		*Logger::log << Log::LI << Log::even ;
		std::sort(vmp.begin(), vmp.end(), sortvps); 
		for(vector<pair<string,string> >::iterator vmpi = vmp.begin(); vmpi != vmp.end(); vmpi++) {
			*Logger::log << Log::LI << Log::II << vmpi->first << Log::IO << Log::II << vmpi->second << Log::IO << Log::LO;
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
/*		
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
 */
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
	for(var_map_type::iterator imt = env_map.begin(); imt != env_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	if (!vme.empty()) {
		*Logger::log << Log::subhead << Log::LI << "List of sysenv" << Log::LO;
		*Logger::log << Log::LI << Log::even;
		std::sort(vme.begin(), vme.end(), sortvps); 
		for(vector<pair<string,string> >::iterator vmei = vme.begin(); vmei != vme.end(); vmei++) {
			if ( vmei->first.find("OBYX_") == string::npos) {
				*Logger::log << Log::LI << Log::II << vmei->first << Log::IO << Log::II << vmei->second << Log::IO << Log::LO;
			}
		}
		*Logger::log << Log::blockend << Log::LO << Log::blockend ; //even .. subhead.
	}
}

void  Environment::list(string& result) {
	ostringstream buffer;

	vector<pair<string,string> >vme;
	for(var_map_type::iterator imt = env_map.begin(); imt != env_map.end(); imt++) {
		vme.push_back(pair<string,string>(imt->first,imt->second));
	}
	std::sort(vme.begin(), vme.end(), sortvps); 
	for(vector<pair<string,string> >::iterator vmei = vme.begin(); vmei != vme.end(); vmei++) {
		if ( gDevelop || vmei->first.find("OBYX_",0,5) == string::npos) {
			buffer << Log::debug << Log::LI << "Environment " << Log::II << vmei->first << Log::IO << Log::II << vmei->second << Log::IO << LO << blockend;
		}
	}
	vector<pair<string,string> >vmp;
	for(var_map_type::iterator imt = parm_map.begin(); imt != parm_map.end(); imt++) {
		vmp.push_back(pair<string,string>(imt->first,imt->second));
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

//private methods
//---------------------------------------------------------------------------
bool Environment::sortvps(pair<string,string> n1,pair<string,string> n2) {
	return (n1.first.compare(n2.first) < 0) ? true : false;
}

void Environment::dopostparms() {
	long long numparms = 0;
	string req_method, test_filename;
	string input;
	if (getenv("REQUEST_METHOD",req_method)) { //if REQ_METHOD
		if ( req_method.compare("POST") == 0 ) {
			if ( Logger::debugging() ) {
				*Logger::log << subhead << LI << "POST Processing Initiated" << LO << debug;
			}
			getparm("THIS_REQ_BODY",input);
			string contenttype;
			if ( getenv("CONTENT_TYPE",contenttype) ) { //if it exists..
				size_t post_mime = contenttype.find("multipart/form-data");
				if (post_mime == string::npos ) {
					size_t xmltest = contenttype.find("text/xml");
					size_t xmltestB = contenttype.find("application/xml");
					size_t xmltest1_2 = contenttype.find("application/soap+xml");
					if ( (xmltest != string::npos) || (xmltestB != string::npos) || (xmltest1_2 != string::npos) ) {
						setparm(parmprefix+"_n[1]","XML_DOCUMENT"); 
						setparm(parmprefix+"_v[1]",input); 
						setparm("XML_DOCUMENT",input);
						return;
					} else {
						size_t post_wwform = contenttype.find("application/x-www-form-urlencoded"); //just the same as if it were a GET/QUERYSTRING
						if (post_wwform != string::npos) {
							doparms(-1,NULL);
							return;
						} else { // unknown or empty
							setparm(parmprefix+"_n[1]","POST_MIME"); 
							setparm(parmprefix+"_v[1]",contenttype); 
							setparm("POST_MIME",contenttype);
							setparm(parmprefix+"_n[2]","THIS_REQ_BODY"); 
							setparm(parmprefix+"_v[2]",input); 
							setparm("_count","2");
							return;
						}
					}
				}
				//multipart/form-data mime follows
				// text/xml; charset="utf-8"
//				CONTENT_TYPE]=[multipart/form-data; boundary=Message_Boundary_000002]
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
				setenv("POST_BOUNDARY",boundary);
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
//						*Logger::log << Log::debug << Log::LI << "Input block" << Log::LO;
						//A typical block is below.
						/*
						 --#312999087#multipart#boundary#1049282996#
						 Content-Disposition: form-data; name="id:2"
						 
						 106
						 */
						
						//A typical file block is below.
						/*
						 --#312999087#multipart#boundary#1049282996#
						 Content-Disposition: form-data; name="img:1"; filename="p.gif"
						 Content-Type: image/gif
						 
						 GIF89a\001\000\001\000\304\000\000\u02c7\u02c7\u02c7\000\000\000!\u02d8\004\001\000\000\000\000,\000\000\000\000\001\000\001\000\000\002\002D\001\000;
						 */
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
								namestart += namep;
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
								lineend = block.find("\x00d\x00a", linestart);
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
//									*Logger::log << LI << "Posting parm:[" << name << "]=[" << value << "]" << LO; //should be in debug here..
										setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",name); 
										setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",value); 
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
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fname); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",filename); 
											setparm(fname,filename);
											numparms++;
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fbase); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",base); 
											setparm(fbase,base);
											numparms++;
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fext); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",ext); 
											setparm(fext,ext);
											numparms++;
										}
										if (! mimetype.empty() ) {
											string lenval = String::tostring(static_cast<long long>(value.length()));
//											*Logger::log << Log::LI <<  name <<" Length of file:[" << lenval << "]" << Log::LO;
											string flength(name+"_length");
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",flength); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",lenval); 
											setparm(flength,lenval);
											numparms++;
											
											string fmime(name+"_mime");
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fmime); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",mimetype); 
											setparm(fmime,mimetype);
											numparms++;
											//This requires the Info structure								
											//Now get any media information.. 
											int width=0;
											int height=0;
											istringstream ist(value);
											try {
												Info info( ist );
												if ( info.check() ) {
													string fwidth(name+"_width");
													string fheight(name+"_height");
													width = info.getWidth();
													height = info.getHeight();
													string swidth;
													String::tostring(swidth,width);
													string sheight;
													String::tostring(sheight,height);
//													*Logger::log << Log::LI << "Posting parm width: " << fwidth << "=" << swidth <<  Log::LO;
													setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fwidth); 
													setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",swidth); 
													setparm(fwidth,swidth);
													numparms++;
													
//													*Logger::log <<  Log::LI << "Posting parm height: " << fheight << "=" << sheight <<  Log::LO;
													setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",fheight); 
													setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",sheight); 
													setparm(fheight,sheight);
													numparms++;
												} else {
//													*Logger::log << Log::LI << "File failed media info.check " << Log::LO;													
												}
											} catch ( exception e ) {
//												*Logger::log << Log::LI << "Media Error exception : " << e.what() << Log::LO;
												// Need to be silent here generally. Except for debugging..
											}
											//----- media info finishes.									
										} else {
//											*Logger::log <<"Content-Type is empty, so no file details were discovered." << Log::LO;											
										}
									} else { // still add empty, if it isn't a file.
										if ( mimetype.empty() && filename.empty() ) {
//											*Logger::log << LI << "Empty Field:[" << name << "]=[" << value << "]" << LO;
											setparm(parmprefix+"_n["+String::tostring(numparms+1)+"]",name); 
											setparm(parmprefix+"_v["+String::tostring(numparms+1)+"]",value); 
											setparm(name,value);
											numparms++;
										}
									}
								}
							} //end of find name test
						} else { 
							setenv("POST_NON_FORM_DATA",line);
						}
//						*Logger::log << Log::LO << Log::blockend;;
					} // end of postfinished test
				} // end of while
				setparm(parmprefix+"_count",String::tostring(numparms)); 
				if ( Logger::debugging() ) {
					*Logger::log << Log::blockend << Log::LI << "POST Processing Completed" << Log::LO << Log::blockend; //
				}
			} //if CONTENT_TYPE
		} 
	} 
}

void Environment::setnamedparm(string parmstring,unsigned long long pnum) {
	ostringstream numparm;
	numparm << pnum+1 << flush;
	size_t split =  parmstring.find('=');
	if (split != string::npos) {
		ostringstream nameparm;
		if (split > 0) {
			string splitparm = parmstring.substr(0,split);
			if (gDevelop && splitparm.compare("LOG_DEBUG") == 0) {
				dosetdebugger = true;
			} else {
				nameparm << splitparm << flush;
				ostringstream parmnstr;
				parmnstr << parmprefix << "_n[" << numparm.str() << "]";
				setparm(nameparm.str(),parmstring.substr(split+1,string::npos));
				setparm(parmnstr.str(),nameparm.str()); 
			}
		} 
		setparm(parmprefix+"_v["+numparm.str()+"]",parmstring.substr(split+1,string::npos));
	} else {
		if (gDevelop && parmstring.compare("LOG_DEBUG") == 0) {
			dosetdebugger = true;
		} else {
			setparm(parmstring,"");
			setparm(parmprefix+"_n["+numparm.str()+"]",parmstring); 
			setparm(parmprefix+"_v["+numparm.str()+"]",""); 
		}
	}
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
				if ( (the_area==Console) && (!envexists("PATH_TRANSLATED")) && (getparm("_n[1]",console_file)) ) {
					setenv("PATH_TRANSLATED",console_file);
					setenv("OBYX_DEVELOPMENT","true");
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
		setnamedparm(raw,count); //lets see if it is a named parameter
		count++;
		start = find + 1;
	}
	if (start != qstr.length()) {
		raw = qstr.substr(start, qstr.length() - start);
		String::urldecode(raw);
		setnamedparm(raw,count);
		count++;
	}
	setparm(parmprefix+"_count",String::tostring(count)); //This is correct for query_String
}

void Environment::setbasetime() {
#ifdef __MACH__
	struct tms tb;
	times(&tb);
	unsigned long long clocktime = tb.tms_utime + tb.tms_stime + tb.tms_cutime + tb.tms_cstime;;
	basetime = static_cast<double>(clocktime) / sysconf(_SC_CLK_TCK);	
#else
	struct timespec tb = {0,0};
	clock_gettime(CLOCK_REALTIME,&tb);
	unsigned long long clocktime = tb.tv_sec * 1000000000 + tb.tv_nsec;
	basetime = static_cast<double>(clocktime) / 1000000000; // nanoseconds
#endif
}

void Environment::gettiming(string& result) {
#ifdef __MACH__
	struct tms tb;
	times(&tb);
	unsigned long long clocktime = tb.tms_utime + tb.tms_stime + tb.tms_cutime + tb.tms_cstime;
	double timing = static_cast<double>(clocktime) / sysconf(_SC_CLK_TCK);
	timing = timing - basetime;
	result = String::tostring(timing,12L);
	
#else
	struct timespec tb;
	int err = clock_gettime(CLOCK_REALTIME,&tb);
	if ( err != 0 ) *Logger::log << Log::error << Log::LI << "Error. Environment::setbasetime error:" << err << Log::LO << Log::blockend;
	unsigned long long clocktime = tb.tv_sec * 1000000000 + tb.tv_nsec;
	double timing = static_cast<double>(clocktime) / 1000000000; // nanoseconds
	timing = timing - basetime;
	result = String::tostring(timing,12L);
#endif
}

void Environment::getresponsehttp(string& result) {
	Httphead::explain(result);
}

//This is recomposited from environment. 
void Environment::getrequesthttp(string& head,string& body) {
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
	head = rh.str();
	getparm("THIS_REQ_BODY",body);
}

//Called before LOGGER is initialised.
void Environment::getenvvars() { 
	pair<unsigned long long,bool> tmp;
	string envtmp;
	if (gArgc > 0) {
		bool this_one = false;
		for(int i = 1; i < gArgc; i++) {
			string av = gArgv[i];
			if (av.compare("-c") == 0) {
				this_one = true;
			} else {
				if (this_one) {
					do_config_file(av);
					this_one=false;
				}
			}
		}
	}
	if (getenv("OBYX_CONFIG_FILE",envtmp)) {
		do_config_file(envtmp);
	} 
	if (getenv("OBYX_ROOT_DIR",gRootDir)) {
		if ( gRootDir[gRootDir.size()-1] != '/') gRootDir+='/';
	} else {
		if (getenv("OBYX_PATH",gRootDir)) {	//legacy name
			if ( gRootDir[gRootDir.size()-1] != '/') gRootDir+='/';
		}
	}
	getenv("LOG_DEBUG",envtmp);
	if (getenv("OBYX_SQLPORT",envtmp)) {
		tmp = String::znatural(envtmp);
		if (tmp.second) gSQLport = (unsigned int)tmp.first;
	}
	gDevelop = getenv("OBYX_DEVELOPMENT",envtmp);
	getenv("OBYX_SQLHOST",gSQLhost); //gSQLhost="localhost";
	getenv("OBYX_SQLUSER",gSQLuser); //gSQLuser="obyx"; 
	if (!getenv("OBYX_SQLDATABASE",gDatabase)) {
		getenv("OBYX_DATABASE",gDatabase);
	}
	if (getenv("OBYX_SCRIPTS_DIR",gScriptsDir)) { //defaults to nothing.
		if ( gScriptsDir[gScriptsDir.size()-1] != '/') gScriptsDir+='/';
	}
	if (getenvd("OBYX_SCRATCH_DIR",gScratchDir,"/tmp/")) { //defaults to /tmp/.
		if ( gScratchDir[gScratchDir.size()-1] != '/') gScratchDir+='/';
	}
	pid_t pid = getpid();
	if (pid < 0) {
//Need to add some sort of unique thing for this process here..		
	} else {
		string pidnum;
		String::tostring(pidnum,(unsigned long long)pid); 
		gScratchName = 'p' + pidnum + '_';
	}
	getenv("OBYX_SQLUSERPW",gSQLuserPW);
	getenv("OBYX_PARM_PREFIX",parmprefix); //Used to prevent ambiguity - someone may need parmxxx
	the_area = do_area();
}

void Environment::do_config_file(string& filepathstring) {
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
						setenv(envname,envval);
					}
				}
			}
		}
	}
}

//return the path for root ie if a local url is prefixed / return the path to that.
string Environment::getpathforroot() {
	string the_result = gRootDir;
	if (the_result.empty()) { //return current working directory.
		the_result = Path::wd();
	}
	return the_result;
}

//Called before LOGGER is initialised. Default area is public
Environment::buildarea_type Environment::do_area() {
	Environment::buildarea_type retval = Console;
	string req_method;
	if ( getenv("REQUEST_METHOD",req_method) ) {
		if ( req_method.compare("CONSOLE") == 0) {
			retval = Console;
		} else {
			string srvroot;
			if (! getenv("DOCUMENT_ROOT",srvroot)) {
				string fpath;
				if ( getenv("PATH_TRANSLATED",fpath)) {
					string fpathd(fpath,0,fpath.find_last_of('/') ); 
					if ( fpathd.find(gRootDir) != string::npos ) {
						retval = Public;
					} else {
						retval = Root;
					}
				}
			} else {
				string rootdir = srvroot.substr(1+srvroot.find_last_of('/')); 
				if (rootdir.compare(gRootDir) == 0 ) {
					retval = Public;
				} else {
					if (rootdir.compare(gRootDir) == 0 ) {
						retval = Web;
					} else {
						retval = Root;
					}
				}
			}
		}
	}
	return retval;
}

//Called before LOGGER is initialised.
void Environment::setenv(string name,string value) {
	pair<var_map_type::iterator, bool> ins = env_map.insert(var_map_type::value_type(name, value));
	if (!ins.second)	{ // Cannot insert (something already there with same ref
		env_map.erase(ins.first);
		env_map.insert(var_map_type::value_type(name, value));
	}
}

//setparm is NOT very good. We need to be able to handle name arrays.
void Environment::setparm(string name,string value) {
	pair<var_map_type::iterator, bool> ins = parm_map.insert(var_map_type::value_type(name, value));
	if (!ins.second)	{ // Cannot insert (something already there with same ref
		parm_map.erase(ins.first);
		parm_map.insert(var_map_type::value_type(name, value));
	}
}
