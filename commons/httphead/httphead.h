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

using namespace std;
using namespace __gnu_cxx; //hashmap namespace.

class Httphead {

private:
	typedef hash_map<const string, string, hash<const string&> > type_nv_map;	//hashmap of name-values
	typedef enum { httpdate, server, cookie, cookie2, expires, cache, pragma, modified, range, mime, location, contentlength, disposition, p3p, connection, custom } http_msg;	
	typedef map<string, http_msg > http_msg_map; 
	
	static ostream*	o;									//output stream
	static bool isdone;
	static bool mime_is_changed;
	static		 string	httpsig;
	static const string	datesig;
	static const string	serversig;
	static const string	expirysig;
	static const string	cachesig;
	static const string	pragmasig;
	static const string	modsig;
	static const string	rangesig;
	static const string	mimesig;
	static const string	locasig; 
	static const string	lengthsig; 
	static const string	disposig; 
	static const string	p3psig; 
	static const string connsig;
	
//httphead settings
	static unsigned int		httpcode;
	static size_t			content_length;
	static bool				nodate;
	static bool				nocaching;	
	static bool				nocodeline;	
	static bool				noheaders;	
	static bool				http_req_method;
	static string			servervalue;
	static string			rangevalue;
	static string			codemessage;
	static string			defaultdatevalue;
	static string			datevalue;
	static string			moddatevalue;
	static string			expiresline;
	static string			cacheprivacy;	//	= "private"; or "public"
	static string			cacheline;		//	= "s-maxage=0, max-age=0, must-revalidate";
	static string			pragmaline;		//	= "no-cache";
	static string			mimevalue;
	static string			dispvalue;
	static string			locavalue;
	static string			content;
	static string			connectionvalue;
	static string			p3pline;
	static http_msg_map		http_msgs;
	static type_nv_map		customlines;

	static void objectwrite(DOMNode* const&,string&);
	
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
	//Standard http head sigs
	static response_format http_fmt;
	static response_format xml_fmt;
//	static void init(); //legacy
	
public:
	static const string	cookiesig;
	static const string	cookie2sig;
	static const string crlf;
	static void init(ostream*);
	static void addcustom(string, string);
	static const bool mime_changed()			{ return mime_is_changed; } 
	static const bool done()					{ return isdone; } 
	static void setmime(string newmime)			{ mimevalue = newmime; mime_is_changed=true; }
	static void setdisposition(string newdisp)  { dispvalue = newdisp; }	
	static void setconnection(string newconn)	{connectionvalue=newconn;}
	static void setcontent(string c)			{content=c; content_length = c.length();}	//Use to drive content for 5xx and 4xx
	static void setcontentlength(size_t l)      {content_length = l;}						//Use to drive content for 5xx and 4xx
	static void setcode(unsigned int);
	static void setdate(string dl)				{ datevalue = dl; }
	static void setexpires(string dl)			{ expiresline = dl; }
	static void setlocation(string loc)			{ locavalue = loc; }
	static void setmoddate(string dl)			{ moddatevalue = dl; }
	static void setserver(string srv)			{ servervalue = srv; }
	static void setrange(string rng)			{ rangevalue = rng; }
	static void setcache(string cl)				{ cacheline = cl; }
	static void setp3p(string pl)				{ p3pline = pl; }
	static void setpragma(string pl)			{ pragmaline = pl; }
	static void setlength(unsigned int newlen)	{ content_length = newlen;}	
	static void setprivate(bool priv)			{ cacheprivacy = priv ? "private": "public"; }	//privacy=false is default.
	static void nocode(bool codeline_q) 		{ nocodeline = codeline_q;}					//nocode= false is default.
	static void nocache(bool); 			//{ nocaching = nocach;}						//nocache=true is default.
	static void noheader(bool);			//{ noheaders = codeline_q;}					//noheaders= false is default.
	static void nodates(bool dateline_q) 		{ nodate = dateline_q;}						//nodates= false is default.
	static void doheader();
	static void explain(string&);
	static void doheader(ostream*,bool,const response_format&);
	static void objectparse(xercesc::DOMNode* const&);
};

#endif

