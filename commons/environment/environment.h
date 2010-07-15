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
//	typedef enum {Root,Public,Console,Scripts,Logs,Web} buildarea_type;
	
private:
	typedef hash_map<const string,string, hash<const string&> > var_map_type;
	
	Environment();
	~Environment();
	static Environment* instance;
	
	static var_map_type cgi_rfc_map;			//CGI_NAME rfc Header map (ro)
	static var_map_type benv_map;				//Base     System environment (ro)
	var_map_type ienv_map;				//Instance System environment (ro)
	
	int gArgc;
	char** gArgv;
	bool gDevelop;
	int gSQLport;
//	buildarea_type the_area;
	string gDatabase;
	string gRootDir;
	string gScriptsDir;
	string gScratchDir;
	string gScratchName;
	string gSQLhost;
	string gSQLuser;
	string gSQLuserPW;
	double basetime;	//used for timing.
	std::string empty;
	std::string parmprefix;
	//-- The following are RESPONSE cookies
	var_map_type ck_map;								//store the cookie values  (rw)
	var_map_type ck_expires_map;						//store the cookie expires  (rw)
	var_map_type ck_path_map;							//store the cookie values  (rw)
	var_map_type ck_domain_map;							//store the cookie values  (rw)
	//-- The following are REQUEST cookies
	var_map_type cke_map;								//store the cookie values  (r)
	var_map_type cke_expires_map;						//store the cookie expires (r)
	var_map_type cke_path_map;							//store the cookie values  (r)
	var_map_type cke_domain_map;						//store the cookie values  (r)
	//--
	var_map_type parm_map;								//Application parms  (ro)
	var_map_type httphead_map;							//Request HTTP headers  (ro)
	
	void setbasetime();
	
	void getenvvars_base();
	void getenvvars();
	void dodocument();
	void doparms(int argc, char *argv[]);
	void dopostparms();
	void setnamedparm(string,unsigned long long);
	
//	buildarea_type do_area();
	void do_request_cookies();
	
	void setcookie_req(string,string);
	void setcookie_req_path(string,string);
	void setcookie_req_domain(string,string);
	void setcookie_req_expires(string,string);
	
	bool getcookie_req_domain(string const,string&);			//domain,path,expires not available from GET/POST
	bool getcookie_req_path(string const,string&);
	bool getcookie_req_expires(string const,string&);
	
	bool getcookie_res(string const,string&);				//both pre-existing and new cookies
	bool getcookie_res_domain(string const,string&);			//domain,path,expires not available from GET/POST
	bool getcookie_res_path(string const,string&);
	bool getcookie_res_expires(string const,string&);
	
	bool getcookie(string const,string&);					//both pre-existing and new cookies
	bool getcookie_domain(string const,string&);			//domain,path,expires not available from GET/POST
	bool getcookie_path(string const,string&);
	bool getcookie_expires(string const,string&);
	
	
	//private statics
	static void setbenv(string,string); //should be private..
	static void setbenvmap();
	static void init_cgi_rfc_map();
	static bool sortvps(pair<string,string>,pair<string,string>);
	static void do_config_file(string&);
public:
	//statics
	static void startup(string&,string&);		//everything that doesn't change across multiple runs
	static void shutdown();
	static void init(int,char **,char **);

	static void finalise();
	static Environment* service(); 
	static bool getbenv(string const,string&);
	
	void setienvmap(char ** environment);
	void gettiming(string&);
	void do_response_cookies(string&);					//
	
	void getresponsehttp(string&); //returns an osi::http response
	void getrequesthttp(string&,string&);  //returns this http request
	
	unsigned long cookiecount() { return ck_map.size() + ck_expires_map.size(); }
	string response_cookies(bool = false);					//if bool = true, then output xml instead
	
	void do_query_string(string& qstr);
	
	void delcookie_res(string);
	void setcookie_res(string,string);
	void setcookie_res_path(string,string);
	void setcookie_res_domain(string,string);
	void setcookie_res_expires(string,string);
	
	void setparm(string,string);
	void setienv(string,string);
	void initwlogger(int argc, char *argv[]);
	void initwlogger();						//Does any init that needs to wait on logger.
	void init_httphead();
	
	void getlanguage(Vdb::Connection*);		//This was nicked from rs404
	void getfilename(Vdb::Connection*,string);
	void gettechnology(Vdb::Connection*);
	
	string getpathforroot();
	bool getparm(string const,string&);
	bool getcookie_req(string const,string&);	//only pre-existing cookies
	bool envexists(string const);
	bool getenv(string const,string&);
	bool envfind(string const pattern);
	bool parmfind(string const pattern);
	bool cookiefind(string const pattern);

	bool getenvd(const string,string& , const string);
	unsigned long long pid() { return getpid(); }
	int SQLport() { return gSQLport; }
	string Database() { return gDatabase; }
	string Path() { return gRootDir; }
	string ScriptsDir() { return gScriptsDir; }
	string ScratchDir() { return gScratchDir; }
	string ScratchName() { return gScratchName; }
	
	string SQLhost() { return gSQLhost; }
	string SQLuser() { return gSQLuser; }
	string SQLuserPW() { return gSQLuserPW; }
	
	void list();						//for debugging
	void listEnv();					//for debugging
	void listParms();
	void listReqCookies();
	void listResCookies();
	void list(string&);
};

#endif
