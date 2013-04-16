/* 
 * httphead.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httphead.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef httpdhead_h
#define httpdhead_h

//Reporter deals with all progress and error reports.

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ext/hash_map>

#include "commons/environment/environment.h"
#include "commons/xml/xml.h"

using namespace xercesc;
using namespace std;
using namespace __gnu_cxx; //hashmap namespace.

class Httphead {
	
private:
	typedef hash_map<const string, string, hash<const string&> > type_nv_map;	//hashmap of name-values
	typedef enum { httpdate, server, cookie, cookie2, expires, cache, pragma, modified, range, mime, location ,contentlocation, contentlength, disposition, p3p, connection, custom, transencoding } http_msg;	
	typedef map<string, http_msg > http_msg_map; 
	
	Httphead(ostream*);
	~Httphead();
	
	static const string	cookiesig;
	static const string	cookie2sig;
	static const string crlf;
	static const string	datesig;
	static const string	serversig;
	static const string	expirysig;
	static const string	cachesig;
	static const string	pragmasig;
	static const string	modsig;
	static const string	rangesig;
	static const string	mimesig;
	static const string	locasig; 
	static const string	clocasig; 
	static const string	lengthsig; 
	static const string	disposig; 
	static const string	p3psig; 
	static const string connsig;
	static http_msg_map	http_msgs;
	
	ostream*	o;									//output stream
	bool isdone;
	bool initialised;
	bool mime_is_changed;
	
	//httphead settings
	unsigned int	httpcode;
	size_t			content_length;
	bool			content_set;
	bool			nodate;
	bool			nocaching;	
	bool			nocodeline;	
	bool			noheaders;	
	bool			http_req_method;
	string			servervalue;
	string			rangevalue;
	string			codemessage;
	string			defaultdatevalue;
	string			datevalue;
	string			moddatevalue;
	string			expiresline;
	string			cacheprivacy;	//	= "private"; or "public"
	string			cacheline;		//	= "s-maxage=0, max-age=0, must-revalidate";
	string			pragmaline;		//	= "no-cache";
	string			mimevalue;
	string			dispvalue;
	string			locavalue;
	string			clocavalue;
	string			content;
	string			connectionvalue;
	string			p3pline;
	string			httpsig;
	
	type_nv_map		customlines;
	
	void objectwrite(DOMNode* const&,string&);
	
protected:
	friend class Environment;
	typedef struct {
		const string response_i;
		const string response_o;
		const string protocol_i;
		const string protocol_o;
		const string protoval_i;
		const string protoval_o;
		const string version_i;
		string version_o;
		const string code_i;
		const string code_o;
		const string reason_i;
		const string reason_o;
		const string message_i;
		const string message_o;
		const string headers_i;
		const string headers_o;
		const string header_i;
		const string header_o;
		const string name_i;
		const string name_o;
		const string value_i;
		const string value_o;
		const string body_i;
		const string body_o;		
	} response_format; 
	
private:
//	static unsigned int instances;				//used to deal with multiple startup/shutdowns..
	static Httphead* singleton;
	//Standard http head sigs
	static response_format http_fmt;
	static response_format xml_fmt;
	
public:
	static void startup();
	static void shutdown();
	static void init(ostream*);
	static void finalise();
	static Httphead* service() { return singleton; }
	
	void addcustom(string, string);
	const bool mime_changed();			
	const bool done();
	const bool contentset() { return content_set;}
	void initialise();
	void setmime(string newmime);
	void setdisposition(string newdisp); 
	void setconnection(string newconn);	
	void setcontent(string c);			//Use to drive content for 5xx and 4xx
	void setcontentlength(size_t l);     //Use to drive content for 5xx and 4xx
	void setcode(unsigned int);
	void setdate(string dl);			
	void setexpires(string dl);			
	void setlocation(string loc);		
	void setclocation(string loc);		
	void setmoddate(string dl);			
	void setserver(string srv);			
	void setrange(string rng);			
	void setcache(string cl);			
	void setp3p(string pl);				
	void setpragma(string pl);			
	void setlength(unsigned int newlen);
	void setprivate(bool priv);			
	void nocode(bool codeline_q); 		
	void nocache(bool);					
	void noheader(bool);				
	void nodates(bool dateline_q); 
	void doheader();
	void explain(string&);
	void doheader(ostream*,bool,const response_format&);
	void objectparse(xercesc::DOMNode* const&);
};

#endif

