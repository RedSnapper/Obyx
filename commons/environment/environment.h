/*
 * environment.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * environment.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef environment_h
#define environment_h

#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <ext/hash_map>
#include <map>

#include "commons/string/strings.h"

using namespace std;
using namespace __gnu_cxx; //hashmap namespace.

namespace Vdb {
	class Query;
	class Connection;
	class Service;
}

class Environment {
public:
	typedef enum {Root,Public,Console,Scripts,Logs,Web} buildarea_type;

private:
	static int gArgc;
	static char** gArgv;

//what used to be 'admin.h' values.
	static bool gDevelop;
	static int gSQLport;
	static buildarea_type the_area;
	static string gDatabase;
	static string gRootDir;
	static string gScriptsDir;
	static string gScratchDir;
	static string gScratchName;
	static string gSQLhost;
	static string gSQLuser;
	static string gSQLuserPW;

	static bool dosetdebugger;	//this is set in init, to allow for logger to load first.
	static double basetime;	//used for timing.
	static std::string empty;
	static std::string parmprefix;
	typedef hash_map<const string,string, hash<const string&> > var_map_type;
	static var_map_type env_map;								//System environment (ro)
//-- The following are RESPONSE cookies
	static var_map_type ck_map;									//store the cookie values  (rw)
	static var_map_type ck_expires_map;							//store the cookie expires  (rw)
	static var_map_type ck_path_map;							//store the cookie values  (rw)
	static var_map_type ck_domain_map;							//store the cookie values  (rw)
//-- The following are REQUEST cookies
	static var_map_type cke_map;								//store the cookie values  (r)
	static var_map_type cke_expires_map;						//store the cookie expires (r)
	static var_map_type cke_path_map;							//store the cookie values  (r)
	static var_map_type cke_domain_map;							//store the cookie values  (r)
//--
	static var_map_type parm_map;								//Application parms  (ro)
	static var_map_type httphead_map;							//Request HTTP headers  (ro)

	static bool sortvps(pair<string,string>,pair<string,string>);
	static void setbasetime();
	
	static void getenvvars();
	static void dodocument();
	static void doparms(int argc, char *argv[]);
	static void dopostparms();
	static void setnamedparm(string,unsigned long long);

	static buildarea_type do_area();
	static void do_request_cookies();

	static void setcookie_req(string,string);
	static void setcookie_req_path(string,string);
	static void setcookie_req_domain(string,string);
	static void setcookie_req_expires(string,string);
	
	static bool getcookie_req_domain(string const,string&);			//domain,path,expires not available from GET/POST
	static bool getcookie_req_path(string const,string&);
	static bool getcookie_req_expires(string const,string&);

	static bool getcookie_res(string const,string&);				//both pre-existing and new cookies
	static bool getcookie_res_domain(string const,string&);			//domain,path,expires not available from GET/POST
	static bool getcookie_res_path(string const,string&);
	static bool getcookie_res_expires(string const,string&);
	
	static bool getcookie(string const,string&);				//both pre-existing and new cookies
	static bool getcookie_domain(string const,string&);			//domain,path,expires not available from GET/POST
	static bool getcookie_path(string const,string&);
	static bool getcookie_expires(string const,string&);
	
public:
	static void gettiming(string&);
	static void do_response_cookies(string&);					//

	static void getresponsehttp(string&); //returns an osi::http response
	static void getrequesthttp(string&,string&);  //returns this http request

	static buildarea_type get_area() { return the_area; }
	static unsigned long cookiecount() { return ck_map.size() + ck_expires_map.size(); }
	static string response_cookies(bool = false);					//if bool = true, then output xml instead

	static void do_query_string(string& qstr);
	static void do_config_file(string& file);
	
	static void delcookie_res(string);
	static void setcookie_res(string,string);
	static void setcookie_res_path(string,string);
	static void setcookie_res_domain(string,string);
	static void setcookie_res_expires(string,string);
			
	static void setparm(string,string);
	static void setenv(string,string);
	static void init(int argc, char *argv[]);	//include parameters (option) bool true=process POST
	static void initwlogger(int argc, char *argv[]);
	static void initwlogger();						//Does any init that needs to wait on logger.
	
	static void getlanguage(Vdb::Connection*);		//This was nicked from rs404
	static void getfilename(Vdb::Connection*,string);
	static void gettechnology(Vdb::Connection*);

	static string getpathforroot();
	static bool getparm(string const,string&);
	static bool getcookie_req(string const,string&);	//only pre-existing cookies
	static bool envexists(string const);
	static bool getenv(string const,string&);
	static bool getenvd(const string,string& , const string);
	static unsigned long long pid() { return getpid(); }
	static int SQLport() { return gSQLport; }
	static string Database() { return gDatabase; }
	static string Path() { return gRootDir; }
	static string ScriptsDir() { return gScriptsDir; }
	static string ScratchDir() { return gScratchDir; }
	static string ScratchName() { return gScratchName; }
	
	static string SQLhost() { return gSQLhost; }
	static string SQLuser() { return gSQLuser; }
	static string SQLuserPW() { return gSQLuserPW; }
	
	static void list();						//for debugging
	static void listEnv();					//for debugging
	static void listParms();
	static void listReqCookies();
	static void listResCookies();
	
	static void list(string&);

};

#endif
