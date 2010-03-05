/* httphead.cpp is authored and maintained by Sam Lindley, Ben Griffin of Red Snapper Ltd 
 * httphead.cpp is a part of Obyx - see http://www.obyx.org .
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

//See http://www.ietf.org/rfc/rfc2616.txt

#include "httphead.h"
#include "commons/date/date.h"
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"

using namespace std;

ostream*		Httphead::o					= NULL;	//output stream
bool			Httphead::isdone			= false;
bool			Httphead::mime_is_changed	= false;
const string	Httphead::crlf				= "\r\n";
/*
	Note about the distinction between HTTPD (nph-) and CGI.
	When outputting CGI back to the server, the HTTP/1.1 signature is 
	replaced with Status.  The 'advantage' of nph- being spawned is no
	longer present with Apache 2, as everything is handled as a child.
	This would also mean stripping the nph- prefix from our CGI.
*/
	  string	Httphead::httpsig	= "Status";	//or cgi or nph- see init()
const string	Httphead::datesig	= "Date";
const string	Httphead::serversig	= "Server";
//const string	Httphead::cookiesig	= "Set-Cookie";
const string	Httphead::cookiesig	= "Set-Cookie";
const string	Httphead::cookie2sig= "Set-Cookie2";
const string	Httphead::expirysig	= "Expires";
const string	Httphead::cachesig	= "Cache-Control";
const string	Httphead::pragmasig	= "Pragma";
const string	Httphead::modsig	= "Last-Modified";
const string	Httphead::rangesig	= "Accept-Ranges";
const string	Httphead::mimesig	= "Content-Type";
const string	Httphead::locasig   = "Location"; 
const string	Httphead::lengthsig = "Content-Length"; 
const string	Httphead::disposig	= "Content-Disposition"; 
const string	Httphead::p3psig	= "P3P";
const string	Httphead::connsig	= "Connection";

Httphead::response_format Httphead::http_fmt = {
	"",			//response_i
	"",		    //response_o
	"",			//protocol_i
	"",			//protocol_o
	"",			//protoval_i
	crlf,		//protoval_o
	"",			//version_i
	" ",		//version_o
	"",			//code_i
	" ",		//code_o
	"",			//reason_i
	"",			//reason_o
	"",			//message_i
	"",			//message_o
	"",			//headers_i
	crlf,		//headers_o
	"",			//header_i
	crlf,		//header_o
	"",			//name_i
	": ",		//name_o
	"",			//value_i
	"",			//value_o
	"",			//body_i
	""			//body_o
};

Httphead::response_format Httphead::xml_fmt = {
	"<osi:http xmlns:osi=\"http://www.obyx.org/osi-application-layer\">",	//response_i
	"</osi:http>",		//response_o
	"<osi:response",	//protocol_i
	"</osi:response>",	//protocol_o
	"",					//protoval_i
	">",				//protoval_o
	" version=\"",		//version_i
	"\"",				//version_o
	" code=\"",			//code_i
	"\"",				//code_o
	" reason=\"",		//reason_i
	"\"",				//reason_o
	"<m:message xmlns:m=\"http://www.obyx.org/message\">",	//message_i
	"</m:message>",		//message_o
	"",					//headers_i
	"",					//headers_o
	"<m:header",		//header_i
	"/>",				//header_o
	" name=\"",			//name_i
	"\"",				//name_o
	" value=\"",		//value_i
	"\"",				//value_o
	"<m:body urlencoded=\"true\">",		//body_i
	"</m:body>"		//body_o	
};

//httphead settings
unsigned int	Httphead::httpcode			= 200;
size_t			Httphead::content_length	= 0;
bool			Httphead::nocaching			= true;		//true - no cache by default.
bool			Httphead::nodate			= false;	//true - no cache by default.
bool			Httphead::nocodeline		= false;	
bool			Httphead::noheaders			= false;	
bool			Httphead::http_req_method	= false;
string			Httphead::codemessage		= "OK";
string			Httphead::cacheline			= "s-maxage=0, max-age=0, must-revalidate, no-cache";
string			Httphead::cacheprivacy		= "public";
string			Httphead::pragmaline		= "no-cache";
//string			Httphead::mimevalue			= "application/xhtml+xml; charset=utf-8";
string			Httphead::mimevalue			= "text/html; charset=utf-8";
string			Httphead::servervalue		= "Obyx/09";
string			Httphead::rangevalue		= "bytes";
string			Httphead::p3pline			= "";

string			Httphead::expiresline		= "";
string			Httphead::defaultdatevalue	= "";
string			Httphead::datevalue			= "";
string			Httphead::moddatevalue		= "";
string			Httphead::locavalue			= "";
string			Httphead::dispvalue			= "";
string			Httphead::content			= "";
string			Httphead::connectionvalue	= "close";

Httphead::type_nv_map	Httphead::customlines;
Httphead::http_msg_map	Httphead::http_msgs;

void Httphead::nocache(bool nocach ) { 
	nocaching = nocach;
}						//nocache=true is default.


void Httphead::doheader() {
	if (!isdone || ( Logger::debugging() && httpcode != 200 ) ) {
		if ( http_req_method ) {
			doheader(o,false,http_fmt);
//			Logger::localheader(true);
		} else {
			if ( o == NULL ) { 
				o = &cout; //this happens when LOG_DEBUG is on
			}
			*o << content;
//			Logger::localheader(true);
		}
		isdone=true;
	}
}

void Httphead::noheader(bool codeline_q) { 
	noheaders = codeline_q;
}

void Httphead::explain(string& container) {
	ostringstream result;
	doheader(&result,true,xml_fmt);
	container = result.str();
}

void Httphead::addcustom(string name,string value) {
	pair<type_nv_map::iterator, bool> ins = customlines.insert(type_nv_map::value_type(name, value));
	if (!ins.second)	{ // Cannot insert (something already there with same ref
		customlines.erase(ins.first);
		customlines.insert(type_nv_map::value_type(name, value));
	}
}

void Httphead::setcode(unsigned int code) {
	switch (code) { 
		case 200: 
			httpcode = code;
			codemessage = "OK";
			break;
		case 201: 
			httpcode = code;
			codemessage = "Created";
			break;
		case 204: 
			httpcode = code;
			codemessage = "No Content";
			break;
		case 205: 
			httpcode = code;
			codemessage = "Reset Content";
			break;
		case 302:
			httpcode = code;
			codemessage = "Found";
			break;
		case 500:
			httpcode = code;
			codemessage = "Internal Server Error";
			break;
		case 501:
			httpcode = code;
			codemessage = "Not Implemented";
			break;
		case 401:
			httpcode = code;
			codemessage = "Unauthorized";
			break;
		case 403:
			httpcode = code;
			codemessage = "Forbidden";
			break;
		case 404:
			httpcode = code;
			codemessage = "Not Found";
			break;
		default:	//we could stick in an error message here..
			httpcode = 500;
			codemessage = "Internal Server Error";
			break;
	}
	httpcode = code;
}


//so this now sends out the header AFTER the xml.
void Httphead::init(ostream* output) {
	if (output == NULL) {
		addcustom("x-broken","ostream broken in Httphead::init");
		Httphead::doheader();
		exit(0);
	} 
	o = output;	
	string req_method_str,cgi_type;
	if ( Environment::getenv("REQUEST_METHOD",req_method_str)) {
		if ( req_method_str.compare("CONSOLE") != 0 && req_method_str.compare("XML") != 0 ) {
			http_req_method = true;
		}
	}
	if ( http_req_method && Environment::getenv("SCRIPT_NAME",cgi_type)) {
		if ( cgi_type.find("/nph-") != string::npos ) {
			httpsig = "HTTP/1.0";
		} else {
			http_fmt.version_o = ": ";
			httpsig = "Status";
		}
	}
	
	DateUtils::Date date;
	date.getNowDateStr("%a, %d %b %Y %H:%M:%S %Z",defaultdatevalue);

//Default mime switched if MSIE.
	string dct;
	if (Environment::getenv("OBYX_CONTENT_TYPE",dct) ) {
		mimevalue = dct;
	} else {
		if (Environment::getenv("HTTP_USER_AGENT",dct) ) {
			if ( dct.find("MSIE") != string::npos ) {
				mimevalue = "text/html; charset=utf-8";
			} else {
				mimevalue = "application/xhtml+xml; charset=utf-8";
			}
		}
	}
	string p3penv;
	if (Environment::getenv("OBYX_P3P",p3penv)) {
		p3pline = "CP=\"";
		p3pline.append(p3penv); 
		p3pline.append("\", policyref=\"/w3c/p3p.xml\"");
	} 
	
	noheaders = Environment::envexists("OBYX_NO_HEADERS");

	std::ios_base::sync_with_stdio(true);
	http_msgs.insert(http_msg_map::value_type("Date", httpdate));
	http_msgs.insert(http_msg_map::value_type("Server", server));
	http_msgs.insert(http_msg_map::value_type("Set-Cookie", cookie));
	http_msgs.insert(http_msg_map::value_type("Set-Cookie2", cookie2));
	http_msgs.insert(http_msg_map::value_type("Expires", expires));
	http_msgs.insert(http_msg_map::value_type("Cache-Control", cache));
	http_msgs.insert(http_msg_map::value_type("Pragma", pragma));
	http_msgs.insert(http_msg_map::value_type("Last-Modified", modified));
	http_msgs.insert(http_msg_map::value_type("Accept-Ranges", range));
	http_msgs.insert(http_msg_map::value_type("Content-Type", mime));
	http_msgs.insert(http_msg_map::value_type("Location", location));
	http_msgs.insert(http_msg_map::value_type("Content-Length", contentlength));
	http_msgs.insert(http_msg_map::value_type("Content-Disposition", disposition));
	http_msgs.insert(http_msg_map::value_type("P3P", p3p ));
	http_msgs.insert(http_msg_map::value_type("Connection", connection));
}

void Httphead::doheader(ostream* x,bool explaining,const response_format& f) {
	if (datevalue.empty()) { 			
		datevalue=defaultdatevalue;
	}
	if ( (!noheaders) || explaining) {
		*x << f.response_i << f.protocol_i;
		if ( (!nocodeline) || explaining) {
			*x << f.protoval_i;
			if ((httpsig.compare("Status") != 0) || (&f != &Httphead::xml_fmt)) {
				*x << f.version_i << httpsig << f.version_o;
			}
			*x << f.code_i << httpcode << f.code_o;
			*x << f.reason_i << codemessage << f.reason_o ; 
			*x << f.protoval_o; 
		}
		*x << f.message_i << f.headers_i; // now do message -- headers..
		if (! mimevalue.empty() ) {
			*x << f.header_i << f.name_i << mimesig << f.name_o << f.value_i << mimevalue << f.value_o << f.header_o;				// Content-Type: text/html OR application/xhtml+xml; charset=utf-8
		}
		if (! servervalue.empty() ) {
			*x << f.header_i << f.name_i << serversig << f.name_o << f.value_i << servervalue << f.value_o << f.header_o;			// Server: RedSnapper/06.02
		}
		if (!nodate) {
			*x << f.header_i << f.name_i << datesig << f.name_o << f.value_i << datevalue << f.value_o << f.header_o;				// Date: 13th Feb 2006
		}
		if (content_length > 0) {
			*x << f.header_i << f.name_i << lengthsig << f.name_o << f.value_i << content_length << f.value_o << f.header_o;		// Content-Length: 12402
		}
		if (! connectionvalue.empty() ) {
			*x << f.header_i << f.name_i << connsig << f.name_o << f.value_i << connectionvalue << f.value_o << f.header_o;		    // Connection: close
		}
		if (! p3pline.empty() ) {
			*x << f.header_i << f.name_i << p3psig << f.name_o << f.value_i << p3pline << f.value_o << f.header_o;					//P3P: CP="ALL DSP COR CURa DEVa OUR SAMa IND PHY UNI PUR", policyref="/w3c/p3p.xml"
		} 
		if (! customlines.empty() ) {
			for ( type_nv_map::iterator i = customlines.begin(); i != customlines.end(); i++ ) {
				if ( !(*i).first.empty() ) {
					*x << f.header_i << f.name_i << (*i).first << f.name_o << f.value_i << (*i).second << f.value_o << f.header_o;
				}
			}
		}
		if (httpcode == 302 && locavalue.empty()) httpcode=200;
		switch (httpcode) { 
			case 200:
				if ( ! rangevalue.empty() ) { 
					*x << f.header_i << f.name_i << rangesig << f.name_o << f.value_i << rangevalue << f.value_o << f.header_o;		// Accept-Ranges: bytes
				}
				if ( ! dispvalue.empty() ) { 
					*x << f.header_i << f.name_i << disposig << f.name_o << f.value_i << dispvalue << f.value_o << f.header_o;			// Content-Disposition: inline; filename='xxx'
				}
				if ( nocaching ) {
					if (expiresline.empty()) { 
						expiresline=defaultdatevalue;
					}
					*x << f.header_i << f.name_i << modsig << f.name_o << f.value_i << datevalue << f.value_o << f.header_o;	// Expires: 13th Feb 2006
					*x << f.header_i << f.name_i << expirysig << f.name_o << f.value_i << expiresline << f.value_o << f.header_o;	// Expires: 13th Feb 2006
					*x << f.header_i << f.name_i << cachesig << f.name_o << f.value_i << cacheprivacy << ", " << cacheline << f.value_o << f.header_o;	// Cache-Control: [public|private], no-cache, must-revalidate
					*x << f.header_i << f.name_i << pragmasig << f.name_o << f.value_i << pragmaline << f.value_o << f.header_o;     // Pragma: no-cache
				}
				if ( Environment::cookiecount() != 0 ) {					//cookiecount will not be set if doheader is called by Logger.
					*x << Environment::response_cookies(explaining);		// Set-Cookie: xx=yy; aa=bb;
				}				
				break;
			case 302: {
				*x << f.header_i << f.name_i << locasig << f.name_o << f.value_i << locavalue << f.value_o << f.header_o;			// Forgot the ever-important extra return..
				if ( Environment::cookiecount() != 0 ) {
					*x << Environment::response_cookies(explaining);		//cookiecount will not be set if doheader is called by Logger.
				}
				string req_method;
				if (Environment::getenv("REQUEST_METHOD",req_method)) {
					if (req_method.compare("HEAD") != 0 && content.empty() ) {
						ostringstream y;
						y << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">";
						y << "<html xmlns=\"http://www.w3.org/1999/xhtml\" >";
						y << "<head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />";
						y << "<title>302 Found</title></head><body><div class=\"subhead\">";
						y << "<a class=\"redirect\" href=\"" << locavalue << "\">" << locavalue << "</a></div>";
						y << "</body></html>" ; 
						content = y.str();
					}
				}
			} break;
			case 404:
				if ( ! rangevalue.empty() ) { 
					*x << f.header_i << f.name_i << rangesig << f.name_o << f.value_i << range << f.value_o << f.header_o;			// Accept-Ranges: bytes
				}
				if ( ! dispvalue.empty() ) { 
					*x << f.header_i << f.name_i << disposig << f.name_o << f.value_i << dispvalue << f.value_o << f.header_o;		// Content-Disposition: inline; filename='xxx'
				}
				break;
			case 500:
			case 501:
				break;
			default:	//we could stick in an error message here..
				break;
		}		
		*x << f.headers_o; // now do message -- body
	}
		if (!content.empty()) {
			if ( (!noheaders) || explaining) {
				*x <<  f.body_i;
			}
			if (explaining) {
				string value(content);
				String::urlencode(value);
				*x << value;
			} else {
				*x << content;
			}
			if ( (!noheaders) || explaining) {
				*x <<  f.body_o;
			}
		}
	if ( (!noheaders) || explaining) {
		*x << f.message_o << f.protocol_o << f.response_o;
	}
}

void Httphead::objectparse(xercesc::DOMNode* const& n) {
	std::string elname;
	XML::transcode(n->getLocalName(),elname);
	if (elname.compare("header") == 0) {
		string name_attr_val;
		if ( XML::Manager::attribute(n,"name",name_attr_val)  ) {
			string value_attr_val="";
			if ( XML::Manager::attribute(n,"value",value_attr_val)  ) {
				string urlenc_attr_val;
				if ( XML::Manager::attribute(n,"url_encoded",urlenc_attr_val)  && (urlenc_attr_val.compare("true") == 0) ) {
					String::urldecode(value_attr_val); 
				}
			}
			DOMNode* ch=n->getFirstChild();
			while (ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE) ch=ch->getNextSibling();
			if ( ch != NULL ) {
				string subhead;
				XML::transcode(ch->getLocalName(),subhead);
				if (subhead.compare("subhead") == 0) {
					if (name_attr_val.compare("Set-Cookie") != 0) {
						if ( ! value_attr_val.empty() ) value_attr_val.append("; ");
							while (ch != NULL && subhead.compare("subhead") == 0) {
								std::string shvalue,shname,encoded_s;
								bool url_encoded=false;
								if (XML::Manager::attribute(ch,"name",shname)) value_attr_val.append(shname);
								if( XML::Manager::attribute(ch,"urlencoded",encoded_s)) { //subhead value
									if ( encoded_s.compare("true") == 0 ) url_encoded=true;
								}
								if(! XML::Manager::attribute(ch,"value",shvalue)) { //subhead value
									DOMNode* shvo=ch->getFirstChild();
									while ( shvo != NULL ) {
										if (shvo->getNodeType() == DOMNode::ELEMENT_NODE) {
											string sh; XML::Manager::parser()->writenode(shvo,sh);
											shvalue.append(sh);
										}
										shvo=shvo->getNextSibling();
									}
								}
								if (! shvalue.empty() ) {
									if (url_encoded) String::urldecode(shvalue);
									value_attr_val.append("='");
									value_attr_val.append(shvalue);
									value_attr_val.push_back('\'');								
								}
								ch=ch->getNextSibling();
								while (ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE) ch=ch->getNextSibling();
								if (ch != NULL) {
									value_attr_val.append("; ");
									XML::transcode(ch->getLocalName(),subhead);
								} else {
									subhead.clear();
								}
							}
						}
					else {
						bool in_cookie = false;
						string cookie_name;
						string sh_name,sh_value,sh_enc;
						while (ch != NULL && subhead.compare("subhead") == 0) {
								if (XML::Manager::attribute(ch,"name",sh_name))
								if( XML::Manager::attribute(ch,"value",sh_value) ) {
									if( XML::Manager::attribute(ch,"urlencoded",sh_enc) && (sh_enc.compare("true") == 0 )) {
										String::urldecode(sh_value);
									}
								}
								if (!in_cookie) {
									cookie_name = sh_name;
									Environment::setcookie_res(cookie_name,sh_value);
									in_cookie = true;
								} else {
									if (sh_name.compare("domain") == 0 ) {
										Environment::setcookie_res_domain(cookie_name,sh_value);
									} else {
										if (sh_name.compare("path") == 0 ) {
											Environment::setcookie_res_path(cookie_name,sh_value); 
										} else {
											if (sh_name.compare("expires") == 0 ) {
												Environment::setcookie_res_expires(cookie_name,sh_value);
											} else {
												cookie_name = sh_name;
												Environment::setcookie_res(cookie_name,sh_value);
											}
										}
									}
								}
								ch=ch->getNextSibling();
								while (ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE) ch=ch->getNextSibling();
								if (ch != NULL) {
									XML::transcode(ch->getLocalName(),subhead);
								} else {
									subhead.clear();
								}
							}
					}
				}
			}
			http_msg_map::const_iterator i = http_msgs.find(name_attr_val);
			http_msg hm;
			if(i != http_msgs.end()) { hm = i->second; } else { hm = custom; }
			switch(hm) {			
				case httpdate : setdate(value_attr_val); break;
				case server : setserver(value_attr_val); break;
				case expires : setexpires(value_attr_val); break;
				case pragma : setpragma(value_attr_val); break;
				case modified : setmoddate(value_attr_val); break;
				case range : setrange(value_attr_val); break;
				case mime : setmime(value_attr_val); break;
				case location : setlocation(value_attr_val); break;
				case contentlength : { 
					pair<unsigned long long,bool> len = String::znatural(value_attr_val);
					if (len.first) setlength(len.second);
				}	break;
				case disposition : setdisposition(value_attr_val); break;
				case p3p : setp3p(value_attr_val); break;
				case connection : setconnection(value_attr_val); break;
				case custom : addcustom(name_attr_val, value_attr_val); break;
				case cache : {
					if (value_attr_val.find("public, ") == 0) {
						setprivate(false); value_attr_val.erase(0,8);
					} else {
						setprivate(true);
						if (value_attr_val.find("private, ") == 0) value_attr_val.erase(0,9);
					}
					setcache(value_attr_val);
				} break;
				case cookie : 
				case cookie2: break;				
			}
		} else {
			*Logger::log << Log::error << Log::LI << "header element found with no name attribute!" << Log::LO << Log::blockend;
		}
	} else {
		if (elname.compare("body") == 0) {
			*Logger::log << Log::error << Log::LI << "Sorry. Body not yet supported!" << Log::LO << Log::blockend;
		} else {
			if (elname.compare("response") == 0) {
				//do the attributes version, code, reason here.
				string code_attr_val,reason_attr_val,version_attr_val;
				if ( XML::Manager::attribute(n,"code",code_attr_val)  ) {
					pair<long long,bool> code_result = String::integer(code_attr_val);
					if (code_result.second) {
						setcode((unsigned int)code_result.first);
					}
				}
				if ( XML::Manager::attribute(n,"version",version_attr_val)  ) {
					if(httpsig.compare("Status") != 0) {
						httpsig = version_attr_val;
					}
				}
				if ( XML::Manager::attribute(n,"reason",reason_attr_val)  ) {
					codemessage = reason_attr_val;
				}
				std::string ch_elname;
				for (DOMNode* child=n->getFirstChild(); child != NULL; child=child->getNextSibling()) {
					if (child->getNodeType() == DOMNode::ELEMENT_NODE) {
						XML::transcode(child->getLocalName(),ch_elname);
						if (ch_elname.compare("message") == 0) {
							for (DOMNode* mchild=child->getFirstChild(); mchild != NULL; mchild=mchild->getNextSibling()) {
								if (mchild->getNodeType() == DOMNode::ELEMENT_NODE) {
									objectparse(mchild);   
								}
							}
						}
					}
				}
			} else {
				if (elname.compare("http") == 0) {
					DOMNode* kid=n->getFirstChild();		  //pass this on to response..
					while ( kid != NULL && kid->getNodeType() != DOMNode::ELEMENT_NODE)  { //Skip whitespace.
						kid = kid->getNextSibling();
					}
					objectparse(kid);   
				} else {
					*Logger::log << Log::error << Log::LI << "Unknown element in http object:" << elname << Log::LO << Log::blockend;
				}
			}
		}
	}
}

void Httphead::objectwrite( DOMNode* const& n,string& result) {
	for ( DOMNode* child=n->getFirstChild(); child != NULL; child=child->getNextSibling()) {
		string childstr; XML::Manager::parser()->writenode(child,childstr);
		result.append(childstr);   
	}
}
