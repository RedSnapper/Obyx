/* 
 * osimessage.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * osimessage.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <string>
#include <sstream>
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include "commons/xml/xml.h"
#include "osimessage.h"

using namespace std;

const std::string OsiMessage::crlf		= "\x0D\x0A";
const std::string OsiMessage::crlft		= "\x0D\x0A\x09";
const std::string OsiMessage::boundary	= "Message_Boundary_";
OsiMessage::header_type_map OsiMessage::header_types;
unsigned int OsiMessage::counter = 1;
void OsiMessage::identify_nl(string& msg) {
	/*
	 Given a message, identify the newline(chars) and the newline size
	 Correct newline is crlf = "\r\n" = "\x0D\x0A";
	 LS:    Line Separator, U+2028 = E280A8 
	 */
	string t_crlf(crlf);
	size_t base = msg.find_first_of(t_crlf);	//either cr or lf
	if (base != string::npos) {
		//CR:    Carriage Return, U+000D
		//CR+LF: CR (U+000D) followed by LF (U+000A) (and badly formed LFCR)
		//LF:    Line Feed, U+000A
		if ((msg[base] == '\x0D') && (msg[base+1] == '\x0A')) {
			nlsize = 2;
			nl = t_crlf;
		} else {
			if ((msg[base] == '\x0A') && (msg[base+1] == '\x0D')) {	//topsy turvy - BAD code!
				nlsize = 2;
				nl = "\x0A\x0D";
			} else {
				nlsize = 1;
				nl = msg[base];
			}		
		}		
	} else {
		if (msg.find("\xE2\x80\xA8") != string::npos ) {
			nlsize = 3;
			nl = "\xE2\x80\xA8";
		} else {
			nlsize = 2;
			nl = t_crlf;
		}
	}
}
void OsiMessage::split_msg(string& msg_str) {
	if(!msg_str.empty()) {
		string nlnl = nl + nl;
		string::size_type pos = msg_str.find(nlnl); 
		if (pos == string::npos) {
			head = msg_str;
			body = "";
		} else {
			head = msg_str.substr(0, pos);
			body = msg_str.substr(pos + nl.size() + nl.size(), string::npos);
		}
	}
}
void OsiMessage::split_header_value(string& v,string& s,string delim) { //splits value at the first ;
	string::size_type pos = v.find_first_of(delim); 
	if (pos != string::npos) {
		s = v.substr(pos + 1, string::npos);
		v.erase(pos,string::npos);
		String::trim(s);
		String::trim(v);
	}
}
void OsiMessage::unfold_headers() {
	/*
	 The process of moving from this folded multiple-line representation of a header field 
	 to its single line representation is called "unfolding". Unfolding is accomplished by 
	 simply removing any CRLF that is immediately followed by WSP. Each header field should
	 be treated in its unfolded form for further syntactic and semantic evaluation.
	 */
	string nlsp = nl+' ';
	string nltab = nl+'\x09';
	String::fandr(head,nlsp," ");
	String::fandr(head,nltab,"\x09");
}
bool OsiMessage::do_angled(string& v) {
	bool retval = false;
	size_t e = v.size()-1;
	if ( v[0] == '<' && v[e] == '>' ) {
		v.resize(e);
		v.erase(0,1);
		retval = true;
	}
	return retval;
}
bool OsiMessage::do_encoding(string& v) {
	bool retval = false;
	if ( v.find_first_of("\"><&\x00") != string::npos ) {
		String::urlencode(v);
		retval = true;
	}
	return retval;
}
void OsiMessage::do_comments(string& str,vector< comment >& comments) {
	size_t bp = str.find_first_of("(;");	//as soon as we see a ; we have gone into subheads.
	while (bp != string::npos) {
		if (str[bp] != ';') {				//hit subheads - so stop
			size_t bx = bp+1;
			unsigned int blev = 1;
			while (blev != 0) {
				bx = str.find_first_of("()",bx);
				if (str[bx] == '(') {
					blev++; 
				} else {
					blev--;
				}
				bx++;
			}
			comment c;
			c.v = str.substr(bp+1,(bx-bp)-2); //-2 cos we want to skip the brackets 
			c.a = do_angled(c.v);
			c.u = do_encoding(c.v);
			ostringstream hx;
			if (!c.v.empty()) {
				hx << " value=\"" << c.v << "\"";
				if (c.a) {
					hx << " angled=\"true\"";
				}
				if (c.u) {
					hx << " urlencoded=\"true\"";
				}
			}
			c.x = hx.str();
			comments.push_back(c);
			str.erase(bp,bx-bp);
			bp = str.find_first_of("(;");
		} else {
			break;
		}
	}
}
void OsiMessage::do_mailbox(string& str,vector< address >& addresses) {
	// mailbox= ([string] <addr-spec>) / (addr-spec) /  "," (and again)
	while (!str.empty()) {
		size_t bp = str.find_first_of("<,");	//as soon as we see a , we have gone into another one.
		if (bp != string::npos && str[bp] == ',') {
			str.erase(0,bp +1);
		} else {
			address c;
			if (bp == string::npos) {
				c.v = str;
				str.clear();
			} else {
				size_t bx = str.find('>',bp+1);
				c.n = str.substr(0,bp);     //note. 
				c.v = str.substr(bp+1,(bx-bp)-1); //-1 cos we want to skip the brackets 
				if (bx != string::npos) {
					str.erase(0,bx+1); 
					String::trim(str);
				} else {
					str.clear();
				}
			} 
			String::trim(c.v);
			c.a = do_angled(c.v);
			c.u = do_encoding(c.v);
			ostringstream hx;
			if (!c.n.empty()) {
				String::trim(c.n);
				String::xmlencode(c.n);		//don't want it going wild on us.				
				hx << " note=\"" << c.n << "\"";
			}			
			if (!c.v.empty()) {
				hx << " value=\"" << c.v << "\"";
				if (c.a) {
					hx << " angled=\"true\"";
				}
				if (c.u) {
					hx << " urlencoded=\"true\"";
				}
			}
			c.x = hx.str();
			addresses.push_back(c);
		}
	}
}
void OsiMessage::do_trace_subheads(header& h) {
	//This goes name ws value ws name ws value 
	while (!h.s.empty()) {
		subhead s;
		do_comments(h.s,s.comments);
		String::trim(h.s);
		size_t k = h.s.find_first_of(" \t");
		if (k != string::npos) {
			s.n = h.s.substr(0,k);
			if (k != string::npos) {
				h.s.erase(0,k+1);
			} else {
				h.s.clear();
			}
			k = h.s.find_first_of(" \t");
		}
		s.v = h.s.substr(0,k);
		if (k != string::npos) {
			h.s.erase(0,k+1); //we don't want the ';'
		} else {
			h.s.clear();
		}
		s.a = do_angled(s.v);
		s.u = do_encoding(s.v);
		String::trim(s.n);
		String::trim(s.v);
		
		ostringstream hx;
		if (!s.n.empty()) {
			hx << " name=\"" << s.n << "\"";
		}
		if (!s.v.empty()) {
			hx << " value=\"" << s.v << "\"";
			if (s.a) {
				hx << " angled=\"true\"";
			}
			if (s.u) {
				hx << " urlencoded=\"true\"";
			} 
		}
		s.x = hx.str();
		h.subheads.push_back(s);
	}
}
void OsiMessage::do_list_subheads(header& h) {
	while (!h.v.empty()) {
		size_t rm = h.v.find_first_not_of("\t ,");
		h.v.erase(0,rm-1);
		size_t bp = h.v.find_first_of("<");	//as soon as we see a ; we have gone into subheads.
		while (bp != string::npos) {
			size_t bx = bp+1;
			unsigned int blev = 1;
			while (blev != 0) {
				bx = h.v.find_first_of("<>",bx);
				if (h.v[bx] == '<') {
					blev++; 
				} else {
					blev--;
				}
				bx++;
			}
			subhead s;
			s.v = h.v.substr(bp+1,(bx-bp)-2); //-2 cos we want to skip the brackets 
			s.a = true; //list items are angled.
			s.u = do_encoding(s.v);
			ostringstream hx;
			if (!s.v.empty()) {
				hx << " value=\"" << s.v << "\"";
				if (s.a) {
					hx << " angled=\"true\"";
				}
				if (s.u) {
					hx << " urlencoded=\"true\"";
				}
			}
			s.x = hx.str();
			h.subheads.push_back(s);
			h.v.erase(bp,bx-bp);
			size_t rm = h.v.find_first_not_of("\t ,");
			h.v.erase(0,rm);
			bp = h.v.find_first_of("<");
		}
	}
}
void OsiMessage::do_address_subheads(header& h) {
	while (!h.s.empty()) {
		subhead s;
		do_comments(h.s,s.comments);
		size_t k = h.s.find_first_of(":;");
		if (h.s[k] == ':') {
			s.n = h.s.substr(0,k);
			if (k != string::npos) {
				h.s.erase(0,k+1);
			} else {
				h.s.clear();
			}
			k = h.s.find(';');
		}
		s.v = h.s.substr(0,k);
		if (k != string::npos) {
			h.s.erase(0,k+1); //we don't want the ';'
		} else {
			h.s.clear();
		}
		String::trim(s.n);
		do_mailbox(s.v,s.addresses);
		ostringstream hx;
		if (!s.n.empty()) {
			hx << " name=\"" << s.n << "\"";
		}
		if (!s.v.empty()) {
			hx << " value=\"" << s.v << "\"";
			if (s.a) {
				hx << " angled=\"true\"";
			}
			if (s.u) {
				hx << " urlencoded=\"true\"";
			} 
		}
		s.x = hx.str();
		h.subheads.push_back(s);
	}
}
void OsiMessage::do_header_subheads(header& h,char delim) {
	while (!h.s.empty()) {
		subhead s;
		string delims("=");
		delims.push_back(delim);
		do_comments(h.s,s.comments);
		size_t k = h.s.find_first_of(delims);
		if (h.s[k] == '=') {
			s.n = h.s.substr(0,k);
			if (k != string::npos) {
				h.s.erase(0,k+1);
			} else {
				h.s.clear();
			}
			k = h.s.find(delim);
		}
		s.v = h.s.substr(0,k);
		if (k != string::npos) {
			h.s.erase(0,k+1); //we don't want the ';'
		} else {
			h.s.clear();
		}
		String::trim(s.v);
		if (s.v.size() > 2 && s.v[0] == '"' && s.v[s.v.size()-1] == '"') {
			s.v.resize(s.v.size()-1);
			s.v.erase(0,1);
		}
		s.a = do_angled(s.v);
		s.u = do_encoding(s.v);
		String::trim(s.n);
		
		ostringstream hx;
		if (!s.n.empty()) {
			hx << " name=\"" << s.n << "\"";
			if (!s.v.empty()) {
				hx << " value=\"" << s.v << "\"";
				if (s.a) {
					hx << " angled=\"true\"";
				}
				if (s.u) {
					hx << " urlencoded=\"true\"";
				} 
			}
		} else {
			//angled/urlencoded will fail here.
			if (!s.v.empty()) {
				hx << " name=\"" << s.v << "\"";
				s.n = s.v; s.v.clear();
			}
		}
		s.x = hx.str();
		h.subheads.push_back(s);
	}
}
void OsiMessage::analyse_cookie(header& h,char fn) {	//'q/s' (req/response)
	//  Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001; SHIPPING=FEDEX
	// http://www.ietf.org/rfc/rfc2965.txt
	// http://wp.netscape.com/newsref/std/cookie_spec.html
	string ckname,ckattr,ckvar;        
	size_t start=0;
	size_t find = string::npos;
	string cook = h.v;
	h.v = "";
	bool finished = false;
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
			switch (fn) {
				case 'q':
				case 's': 
				case 'x': { 
					ostringstream x;
					x << "<m:header name=\"Set-Cookie\" cookie=\"" << ckattr << "\">";
					x << "<m:subhead name=\"" << ckattr << "\" value=\"" << ckvar << "\"";
					h.x = x.str();
				} break;
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
void OsiMessage::construct_header_value(header& h) {
	switch (h.t) {
		case contenttype: {
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			split_header_value(h.v,h.s); //subheads now in h.s
			do_header_subheads(h); //subheads are recognised by ; only
			String::trim(h.v);
			size_t sh_slsh = h.v.find('/');
			if (sh_slsh != string::npos) { //
				body_type = h.v.substr(0,sh_slsh);
				body_subtype = h.v.substr(sh_slsh+1,string::npos);
			}
			size_t afx = 0;
			if (h.subheads.size() > 0) {
				for (size_t i = 0; i < h.subheads.size(); i++) {
					if (h.subheads[i].n.compare("boundary") == 0 ) {
						body_boundary=h.subheads[i].v;
						if (h.subheads[i].u) { String::urldecode(body_boundary); }
						h.subheads[i].comments.clear();
						h.subheads[i].x.clear();
						afx++;
					}
					if (h.subheads[i].n.compare("format") == 0 ) {
						body_format=h.subheads[i].v;
						if (h.subheads[i].u) { String::urldecode(body_format); }
						h.subheads[i].comments.clear();
						h.subheads[i].x.clear();
						afx++;
					}
					if (h.subheads[i].n.compare("charset") == 0 ) {
						body_charset=h.subheads[i].v;
						if (h.subheads[i].u) { String::urldecode(body_charset); }
						h.subheads[i].comments.clear();
						h.subheads[i].x.clear();
						afx++;
					}
				}
			}
			if (afx == h.subheads.size()) {
				h.subheads.clear();
				h.comments.clear();
			}
		} break;
		case cte: {
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			split_header_value(h.v,h.s); //subheads now in h.s
			body_mechanism=h.v;
			if (h.u) { String::urldecode(body_mechanism); }
			String::trim(body_mechanism);
			h.subheads.clear();
			h.comments.clear();
		} break;
		case mailbox: {
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			string::size_type pos = h.v.find_first_of(":;<,"); 
			if (pos != string::npos && h.v[pos] == ':') {
				h.s = h.v;
				h.v.clear();
				String::trim(h.s);
			}			
			do_address_subheads(h); //subheads are recognised by :; only
			String::trim(h.v);
			do_mailbox(h.v,h.addresses);
			if (h.subheads.size() > 0) { //groups
				for (size_t i = 0; i < h.subheads.size(); i++) {
					do_mailbox(h.subheads[i].v,h.subheads[i].addresses);
				}
			}
			ostringstream hx;
			if (!h.n.empty()) {
				hx << " name=\"" << h.n << "\"";
			}
			h.x = hx.str();
		} break;
		case trace: {
			split_header_value(h.v,h.s); //subheads now in h.s
			string v = h.s;
			h.s = h.v;
			h.v = v;
			do_trace_subheads(h);
			ostringstream hx;
			if (!h.n.empty()) {
				hx << " name=\"" << h.n << "\"";
			}
			if (!h.v.empty()) {
				h.a = do_angled(h.v);
				h.u = do_encoding(h.v);
				hx << " value=\"" << h.v << "\"";
				if (h.a) {
					hx << " angled=\"true\"";
				}
				if (h.u) {
					hx << " urlencoded=\"true\"";
				} 
			}
			h.x = hx.str();
		} break;
		case list: { //comma delimited angled addresses.
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			do_list_subheads(h);
			ostringstream hx;
			if (!h.n.empty()) {
				hx << " name=\"" << h.n << "\"";
			}
			if (!h.v.empty()) {
				h.a = do_angled(h.v);
				h.u=do_encoding(h.v);
				hx << " value=\"" << h.v << "\"";
				if (h.a) {
					hx << " angled=\"true\"";
				}
				if (h.u) {
					hx << " urlencoded=\"true\"";
				} 
			}
			h.x = hx.str();
		} break;
		case rescookie: { //special cookie parse.
			h.n = "Set-Cookie";
		} // no break - using reqcookie code also.
		case reqcookie: { //special cookie parse.
			h.x.append(" name=\"" + h.n + "\"");
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			h.s = h.v; h.v.clear();
			do_header_subheads(h);
		} break;
		case cdisp:
			h.n = "Content-Disposition"; //otherwise, unstructured.
		case qvalue: 
		case date_time: 
		case msg_id: 
		case version: 
		case unstructured: {
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			split_header_value(h.v,h.s); //subheads now in h.s
			do_header_subheads(h); //subheads are recognised by ; only
			String::trim(h.v);
			ostringstream hx;
			if (!h.n.empty()) {
				hx << " name=\"" << h.n << "\"";
			}
			if (!h.v.empty()) {
				h.a = do_angled(h.v);
				h.u=do_encoding(h.v);
				hx << " value=\"" << h.v << "\"";
				if (h.a) {
					hx << " angled=\"true\"";
				}
				if (h.u) {
					hx << " urlencoded=\"true\"";
				} 
			}
			h.x = hx.str();
		} break;
		case commadelim: {
			do_comments(h.v,h.comments);  //will leave the value (if there is one) followed by a ;
			String::ltrim(h.v);
			split_header_value(h.v,h.s,"\t\r\n "); //subheads now in h.s
			do_header_subheads(h,',');  //subheads are recognised by , only
			String::trim(h.v);
			ostringstream hx;
			if (!h.n.empty()) {
				hx << " name=\"" << h.n << "\"";
			}
			if (!h.v.empty()) {
				h.a = do_angled(h.v);
				h.u=do_encoding(h.v);
				hx << " value=\"" << h.v << "\"";
				if (h.a) {
					hx << " angled=\"true\"";
				}
				if (h.u) {
					hx << " urlencoded=\"true\"";
				} 
			}
			h.x = hx.str();
		} break;
	}
}
void OsiMessage::do_header_array() {
	while( !head.empty() ) { //do header newline by newline here..
		size_t elp = head.find(nl);
		std::string header_str(head,0,elp); //This should be 1 header.
		if (elp != string::npos) {
			head.erase(0,elp+nlsize);
		} else {
			head.clear();
		}
		size_t nvpos = header_str.find(':');
		if (nvpos != string::npos) { //else discard it.
			header h;
			h.n = string(header_str,0,nvpos);
			h.v = string(header_str,nvpos+1,string::npos);
			header_type_map::const_iterator i = header_types.find(h.n);
			if( i != header_types.end() ) {
				h.t = i->second; 
			} else {
				h.t = unstructured;
			}
			construct_header_value(h);
			if (!(h.x.empty() && h.addresses.empty() && h.subheads.empty() && h.comments.empty())) {
				headers.push_back(h);
			}
		}
	}
}
void OsiMessage::do_headers() {
	unfold_headers();
	do_header_array();	
}
string OsiMessage::xml_val(header& h) {
	ostringstream x;
	if (!(h.x.empty() && h.addresses.empty() && h.comments.empty() && h.subheads.empty())) {
		x << "<m:header" << h.x;
		if (h.comments.empty() && h.addresses.empty() && h.subheads.empty()) {
			x << "/>";
		} else {
			x << ">";
			for (size_t i = 0; i < h.comments.size(); i++) {
				x << "<m:comment" << h.comments[i].x << "/>";
			}
			for (size_t i = 0; i < h.addresses.size(); i++) {
				x << "<m:address" << h.addresses[i].x << "/>";
			}
			for (size_t i = 0; i < h.subheads.size(); i++) {
				x << "<m:subhead" << h.subheads[i].x;
				if (h.subheads[i].comments.empty() && h.subheads[i].addresses.empty()) {
					x << "/>" ;
				} else {
					x << ">";
					for (size_t j = 0; j < h.subheads[i].comments.size(); j++) {
						x << "<m:comment" << h.subheads[i].comments[j].x << "/>";
					}
					for (size_t j = 0; j < h.subheads[i].addresses.size(); j++) {
						x << "<m:address" << h.subheads[i].addresses[j].x << "/>";
					}
					x << "</m:subhead>";
				}				
			}
			x << "</m:header>";
		}
	}
	return x.str();
}
void OsiMessage::compile(string& msg_str, ostringstream& res, bool do_namespace) {
	//compile turns an RFC message into an xml osi.
	identify_nl(msg_str);
	split_msg(msg_str);
	do_headers();
	if (do_namespace) {
		res << "<m:message xmlns:m=\"http://www.obyx.org/message\">";
	} else {
		res << "<m:message>";
	}
	for (size_t i = 0; i < headers.size(); i++) {
		res << xml_val(headers[i]);
	}
	if (!body.empty()) {
		bool isnt_multipart = body_type.compare("multipart") != 0;
		res << "<m:body";
		if (!body_type.empty()) res << " type=\"" << body_type << "\"";
		if (!body_subtype.empty()) res << " subtype=\"" << body_subtype << "\"";
		if (!body_mechanism.empty()) {
			if (body_mechanism.compare("quoted-printable") == 0) {
				String::qpdecode(body,nl);
			} else {
				if (body_mechanism.compare("base64") == 0) {
					String::base64decode(body);
				} else {
					res << " mechanism=\"" << body_mechanism << "\"";
				}
			}
		}
		if (!body_format.empty()) res << " format=\"" << body_format << "\"";
		if (!body_charset.empty()) {
			if ((body_charset.compare("UTF-8") !=0) && (body_charset.compare("utf-8") !=0) ) {
				u_str x_body;
				XML::transcode(body,x_body,body_charset);
				XML::transcode(x_body.c_str(),body);
			}
			res << " charset=\"UTF-8\"";
		}
		if ( isnt_multipart ) {
			if ( (!body_type.empty() && (body_type.compare("text")!=0)) || (body.find_first_of("<&\x00") != string::npos)) {
				String::urlencode(body);
				res << " urlencoded=\"true\"";
			}
		}
		res << ">";
		if ( isnt_multipart ) {
			res << body;	
		} else {
			string startboundary = "--" + body_boundary; 
			string endboundary = startboundary + "--";
			size_t blockstart = 0;
			size_t blockend = 0;
			size_t boundlen = startboundary.length();
			bool bodydone = false;
			while(! bodydone) {
				blockstart = body.find(startboundary, blockend);
				size_t first_cut = blockstart + boundlen + nlsize;
				if (blockstart == string::npos) bodydone = true;
				blockend = body.find(startboundary,first_cut);
				if (blockend == string::npos) blockend = body.find(endboundary,first_cut);
				if (blockend == string::npos) bodydone = true;			
				if (! bodydone ) {
					string msg_block = body.substr(first_cut, (blockend - first_cut) - nlsize);
					OsiMessage msg;
					msg.compile(msg_block,res,false);
				}
			}
		}
		res << "</m:body>";
	}
	res << "</m:message>";
}

void OsiMessage::until_xml(const xercesc::DOMNode*& n) {
	while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE && n->getNodeType() != DOMNode::TEXT_NODE )  { //Skip whitespace.
		n = n->getNextSibling();
	}
}
void OsiMessage::next_xml(const xercesc::DOMNode*& n) {
	n = n->getNextSibling();
	until_xml(n);
}
void OsiMessage::until_el(const xercesc::DOMNode*& n) {
	while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  { //Skip whitespace.
		n = n->getNextSibling();
	}
}
void OsiMessage::next_el(const xercesc::DOMNode*& n) {
	n = n->getNextSibling();
	until_el(n);
}
void OsiMessage::next_ch(const xercesc::DOMNode*& n) {
	n=n->getFirstChild();	//first element of message == header OR body OR NULL OR Whitespace.
	until_el(n);
}
void OsiMessage::encode_nodes(const xercesc::DOMNode*& n,std::string& result) {
	const DOMNode* cn=n;
	for (until_xml(cn); cn!=NULL;next_xml(cn)) {
		string sh; XML::Manager::parser()->writenode(cn,sh);
		string st(sh); String::trim(st);
		if (!st.empty()) {
			result.append(sh);
		}
	}
}
void OsiMessage::encode_comment(const xercesc::DOMNode*& n,std::string& result) {
	std::string comment,sh,encoded_s,angled_s;
	if(! XML::Manager::attribute(n,"value",comment)) { //comment value
		const DOMNode* cn=n;
		for (next_ch(cn); cn!=NULL;next_el(cn)) {
			XML::Manager::parser()->writenode(cn,sh);
			comment.append(sh);
		}
	}
	if( XML::Manager::attribute(n,"urlencoded",encoded_s)) { //subhead value
		if ( encoded_s.compare("true") == 0 ) String::urldecode(comment);
	}
	if( XML::Manager::attribute(n,"angled",angled_s)) { //subhead value
		if ( angled_s.compare("true") == 0 ) comment = '<' + comment + '>';
	}
	result.append(comment);
}
void OsiMessage::decompile(const xercesc::DOMNode* n,ostream& result,bool addlength,bool inlatin) {
	//Decompile turns an xml osi into an RFC message.
	//wrapper for a full message decompile.
	vector<string> heads; string body;
	decompile(n,heads,body,addlength,inlatin);
	for (unsigned int i=0; i < heads.size(); i++) {
		String::fandr(heads[i],crlf,crlft);		//crlf in heads need a tab after them to indicate that they are not heads.
		result << heads[i] << crlf;
	}
	result << crlf << body; 
}
void OsiMessage::decompile(const xercesc::DOMNode* n,vector<std::string>& heads, string& body,bool addlength,bool inlatin) {
	//Take an xml osi message and turn it into an RFC standard message.
	//n must point to root message element.
	//inlatin is to indicate whether or not we need to be concerned with http://tools.ietf.org/html/rfc2047
	std::string head,encoded_s,angled_s;
	until_el(n);
	if ( n != NULL) {
		std::string elname;
		XML::transcode(n->getLocalName(),elname);
		if (elname.compare("message") == 0) {
			for (next_ch(n); n!=NULL;next_el(n)) { //for each child of n...
				XML::transcode(n->getLocalName(),elname);
				if (elname.compare("header") == 0) {	//basically - headers
					//Handle the (multiple) header elements.
					//Header is a single line: name: value; subvalue="foo"; subvue="bar";
					header_type htype(unstructured);
					std::string name,headv,comment,header_value;
					XML::Manager::attribute(n,"name",name); //mandatory.
					head.append(name); head.append(": ");	//okay so now we have the start of a new header
					header_type_map::const_iterator i = header_types.find(name);
					if( i != header_types.end() ) {	htype = i->second; } else { htype = unstructured; }
					if ( XML::Manager::attribute(n,"value",header_value)) { //optional
						if( XML::Manager::attribute(n,"urlencoded",encoded_s)) { //subhead value
							if ( encoded_s.compare("true") == 0 ) String::urldecode(header_value);
						}
						if( XML::Manager::attribute(n,"angled",angled_s)) { //subhead value
							if ( angled_s.compare("true") == 0 ) header_value = '<' + header_value + '>';
						}
					}
					const DOMNode* ch=n;	//Now do all the subheads..
					for (next_ch(ch); ch!=NULL; next_el(ch)) {
						string subhead(""),shcomment("");
						XML::transcode(ch->getLocalName(),subhead);
						if (subhead.compare("subhead") == 0 || subhead.compare("address") == 0) {
							std::string shvalue,shnote,shname;
							bool url_encoded=false;
							bool usequotes;
							if (htype==commadelim) {
								usequotes=true;
							} else {
								usequotes=false;
							}							
							if (XML::Manager::attribute(ch,"name",shname)) { 
								headv.append(shname);
								if (shname.compare("name") == 0) {
									usequotes = true;
								}
							}
							if( XML::Manager::attribute(ch,"urlencoded",encoded_s)) { //subhead value
								if ( encoded_s.compare("true") == 0 ) url_encoded=true;
							}
							if(! XML::Manager::attribute(ch,"value",shvalue)) { //subhead value
								const DOMNode* shvo=ch;
								for (next_ch(shvo); shvo!=NULL;next_el(shvo)) {
									encode_nodes(shvo,shvalue);
								}
							}
							const DOMNode* shvo=ch; string shcom("");
							for (next_ch(shvo); shvo!=NULL;next_el(shvo)) {
								XML::transcode(shvo->getLocalName(),shcom);
								if (shcom.compare("comment") == 0) {
									shcomment.push_back('(');
									encode_comment(shvo,shcomment);
									shcomment.push_back(')');
								}
							}
							if (! shvalue.empty() || ! shnote.empty()) {
								if (url_encoded) String::urldecode(shvalue);
								if( XML::Manager::attribute(ch,"angled",angled_s)) { //subhead value
									if ( angled_s.compare("true") == 0 ) shvalue = '<' + shvalue + '>';
								}
								string shstring;
								if (usequotes) {
									shstring.push_back('"');
									shstring.append(shvalue);
									shstring.push_back('"');
								} else {
									shstring = shvalue;
								}
								switch(htype) {
									case mailbox: {
										XML::Manager::attribute(ch,"note",shnote);
										if (!shnote.empty()) {
											if (!inlatin) {
												headv.append(shnote);
											} else {
												if (String::islatin(shnote)) {
													headv.append(shnote);
												} else {
													String::base64encode(shnote,false);
													headv.append("=?utf-8?B?");
													headv.append(shnote);
													headv.append("?=");
												}
											}											
											headv.push_back(' ');
											headv.push_back('<');
											headv.append(shvalue);
											headv.push_back('>');
										} else {
											headv.append(shvalue);
										}
									} break;
									case cdisp: {
										name="Content-Disposition";
										headv.append("=\"");
										headv.append(shvalue);
										headv.push_back('"');
									} break;
									case list: {
										headv.append(shstring);
									} break;
									case trace: {
										headv.push_back(' '); 
										headv.append(shstring);	
									} break;
									case commadelim:
									default: {
										headv.push_back('=');
										headv.append(shstring);
									} break;
								}
							} 
							headv.append(shcomment);
							const DOMNode* nx=ch; next_el(nx);
							if (nx != NULL) {
								switch(htype) {
									case list: {
										headv.append(",");
									} break;
									case trace: { //Received is done further up.
										headv.append(crlf); //tab will be dealt with higher up.
									} break;
									case commadelim: {
										headv.append(", ");
									} break;
									default: {
										headv.append("; ");
									} break;
								}
							}
						} else {
							if (subhead.compare("comment") == 0) {
								comment.push_back('(');
								encode_comment(ch,comment);
								comment.push_back(')');
							} else {
								encode_nodes(ch,comment);
							}
						}
					}
					if (!header_value.empty()) {
						switch (htype) {
							case trace: {
								if (!headv.empty()) {
									headv.append("; ");
								}
								headv.append(header_value); //now finished with the main part of the line.
							} break;
							case reqcookie: {
								headv = header_value + headv;
							} break;
							case commadelim: {
								if (!headv.empty()) {
									headv = header_value + " " + headv;
								} else {
									headv = header_value;
								}
							} break;
							default: {
								if (!headv.empty()) {
									headv = header_value + "; " + headv;
								} else {
									headv = header_value;
								}
							} break;
						}
					}
					headv.append(comment);
					if (!inlatin) {
						head.append(headv);
					} else {
						if (String::islatin(headv)) {
							head.append(headv);
						} else {
							String::base64encode(headv,false);
							head.append("=?utf-8?B?");
							head.append(headv);
							head.append("?=");
						}
					}
					heads.push_back(head); 
					head.clear();
					headv.clear();
				} else { //must be body, if we are validating.
					std::string mechanism,type,subtype,encoded,charset,bformat;
					XML::Manager::attribute(n,"mechanism",mechanism); //optional
					XML::Manager::attribute(n,"type",type);           //optional
					XML::Manager::attribute(n,"subtype",subtype);     //optional
					XML::Manager::attribute(n,"urlencoded",encoded);  //optional, NOT with multiparts..
					XML::Manager::attribute(n,"charset",charset);  //optional, NOT with multiparts..
					XML::Manager::attribute(n,"format",bformat);  //optional, NOT with multiparts..
					if (!mechanism.empty()) {
						head.append("Content-Transfer-Encoding: ");
						head.append(mechanism); 
						heads.push_back(head); head.clear();
					}
					// Content-Type: type=multipart/mixed; boundary=boundary-002
					if ( ! type.empty() ) {
						head.append("Content-Type: ");
						head.append(type); head.push_back('/'); head.append(subtype); 
						if (!charset.empty()) {
							head.append("; charset=");
							head.append(charset);
						}
						if (!bformat.empty()) {
							head.append("; format=");
							head.append(bformat);
						}
						if (type.compare("multipart") == 0) { // need to compose a set of messages..
							vector<std::string> messages;
							string loc_boundary(boundary);
							loc_boundary.append(String::tofixedstring(6,counter++));
							const DOMNode* ch=n;	//Now do all the subheads..
							for (next_ch(ch); ch!=NULL;next_el(ch)) {
								vector<string> tmphds;
								string tmpmsg,tmpbody;
								decompile(ch,tmphds,tmpbody,false);
								for (unsigned int i=0; i < tmphds.size(); i++) {
									tmpmsg.append(tmphds[i]);
									tmpmsg.append(crlf);
								} 
								tmpmsg.append(crlf); 
								tmpmsg.append(tmpbody);
								bool boundary_clash = (tmpmsg.find(loc_boundary) != string::npos);
								while (boundary_clash) {
									loc_boundary=boundary;
									loc_boundary.append(String::tofixedstring(6,counter++));
									for (size_t x=0; x < messages.size(); x++) {
										boundary_clash = messages[x].find(loc_boundary) != string::npos;
										if (boundary_clash) break;
									}
									if (!boundary_clash) { 
										boundary_clash = (tmpmsg.find(loc_boundary) != string::npos);
									}
								}
								messages.push_back(tmpmsg);
							}
							//we now have a vector of messages and we have a non-clashing boundary...
							//quotes on the boundary subhead appear to be very poorly supported.
							//						head.append("; boundary=\"");
							//						head.append(loc_boundary);
							//						head.push_back('"');
							head.append("; boundary=");
							head.append(loc_boundary);
							heads.push_back(head); head.clear();
							for (unsigned int x=0; x < messages.size(); x++) {
								body.append("--");
								body.append(loc_boundary);
								body.append(crlf);
								body.append(messages[x]);
								body.append(crlf);
							}
							body.append("--");
							body.append(loc_boundary);
							body.append("--");
						} else {
							heads.push_back(head); head.clear();
							const DOMNode* ch=n->getFirstChild();	//.
							encode_nodes(ch,body);
							if (encoded.compare("true") == 0 ) String::urldecode(body);
							if (addlength) {
								head.append("Content-Length: ");
								head.append(String::tostring((long long unsigned int)body.size())); 
								heads.push_back(head); head.clear();
							}
						}
					} else {
						const DOMNode* ch=n->getFirstChild();	//.
						encode_nodes(ch,body);
						if (encoded.compare("true") == 0 ) String::urldecode(body);
						if (addlength) {
							head.append("Content-Length: ");
							head.append(String::tostring((long long unsigned int)body.size())); 
							heads.push_back(head); head.clear();
						}
					}
				}
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. OSI application, message was expected, not '" << elname << "'." << Log::LO << Log::blockend;
		}
	}
}

void OsiMessage::init() {
}
void OsiMessage::finalise() {
}
void OsiMessage::startup() { //(default is  unstructured)
	//RFC 2822 Internet Message Format 
	header_types.insert(header_type_map::value_type("Date", date_time));			//"Date:" date-time CRLF 3.6.1.
	header_types.insert(header_type_map::value_type("From", mailbox));				//"From:" mailbox-list CRLF
	header_types.insert(header_type_map::value_type("Sender", mailbox));			//"Sender:" mailbox CRLF
	header_types.insert(header_type_map::value_type("Reply-To",mailbox));			//"Reply-To:" address-list CRLF	
	header_types.insert(header_type_map::value_type("To",mailbox));					//"To:" address-list CRLF
	header_types.insert(header_type_map::value_type("Cc",mailbox));					//"Cc:" address-list CRLF
	header_types.insert(header_type_map::value_type("Bcc",mailbox));				//"Bcc:" (address-list / [CFWS]) CRLF
	header_types.insert(header_type_map::value_type("Message-ID",msg_id));			//"Message-ID:" msg-id CRLF
	header_types.insert(header_type_map::value_type("In-Reply-To",msg_id));			//"In-Reply-To:" 1*msg-id CRLF
	header_types.insert(header_type_map::value_type("References",msg_id));			//"References:" 1*msg-id CRLF
	header_types.insert(header_type_map::value_type("Resent-Date", date_time));		//"Date:" date-time CRLF 3.6.1.
	header_types.insert(header_type_map::value_type("Resent-From", mailbox));		//"From:" mailbox-list CRLF
	header_types.insert(header_type_map::value_type("Resent-Sender", mailbox));		//"Sender:" mailbox CRLF
	header_types.insert(header_type_map::value_type("Resent-To",mailbox));			//"To:" address-list CRLF
	header_types.insert(header_type_map::value_type("Resent-Cc",mailbox));			//"Cc:" address-list CRLF
	header_types.insert(header_type_map::value_type("Resent-Bcc",mailbox));			//"Bcc:" (address-list / [CFWS]) CRLF
	header_types.insert(header_type_map::value_type("Resent-Message-ID",msg_id));	//"Message-ID:" msg-id CRLF
	header_types.insert(header_type_map::value_type("Return-Path",mailbox));		//
	header_types.insert(header_type_map::value_type("Received",trace));				//
	//RFC 2045 Multipurpose Internet Mail Extensions
	header_types.insert(header_type_map::value_type("MIME-Version",version));				//RFC 2045 Section 5
	header_types.insert(header_type_map::value_type("Content-Type",contenttype));			//RFC 2045 Section 6
	header_types.insert(header_type_map::value_type("Content-Transfer-Encoding",cte));		//RFC 2045 Section 7
	header_types.insert(header_type_map::value_type("Content-ID",msg_id));					//RFC 2045 Section 8
	//RFC 2616 Hypertext Transfer Protocol -- HTTP/1.1
	header_types.insert(header_type_map::value_type("Accept",unstructured));			//; Section 14.1  Accept:		  text/*;q=0.3, text/html;q=0.7, text/html;level=1, text/html;level=2;q=0.4, */*;q=0.5
	header_types.insert(header_type_map::value_type("Accept-Charset",unstructured));	//; Section 14.2  Accept-Charset:  iso-8859-5, unicode-1-1;q=0.8
	header_types.insert(header_type_map::value_type("Accept-Encoding",unstructured));	//; Section 14.3  Accept-Encoding: gzip;q=1.0, identity; q=0.5, *;q=0
	header_types.insert(header_type_map::value_type("Accept-Language",unstructured));	//; Section 14.4  Accept-Language: da, en-gb;q=0.8, en;q=0.7
	header_types.insert(header_type_map::value_type("Cache-Control",unstructured));		//; Section 14.9
	header_types.insert(header_type_map::value_type("Connection",unstructured));		//; Section 14.10
	header_types.insert(header_type_map::value_type("Date",unstructured));				//; Section 14.18
	header_types.insert(header_type_map::value_type("Pragma",unstructured));			//; Section 14.32
	header_types.insert(header_type_map::value_type("Trailer",unstructured));			//; Section 14.40
	header_types.insert(header_type_map::value_type("Transfer-Encoding",unstructured));	//; Section 14.41
	header_types.insert(header_type_map::value_type("Upgrade",unstructured));			//; Section 14.42
	header_types.insert(header_type_map::value_type("Via",unstructured));				//; Section 14.45
	header_types.insert(header_type_map::value_type("Warning",unstructured));			//; Section 14.46
	header_types.insert(header_type_map::value_type("Expect",unstructured));			//; Section 14.20
	header_types.insert(header_type_map::value_type("From",unstructured));				//; Section 14.22
	header_types.insert(header_type_map::value_type("Host",unstructured));				//; Section 14.23
	header_types.insert(header_type_map::value_type("If-Match",unstructured));			//; Section 14.24
	header_types.insert(header_type_map::value_type("If-Modified-Since",unstructured));	//; Section 14.25
	header_types.insert(header_type_map::value_type("If-None-Match",unstructured));		//; Section 14.26
	header_types.insert(header_type_map::value_type("If-Range",unstructured));			//; Section 14.27
	header_types.insert(header_type_map::value_type("If-Unmodified-Since",unstructured));	//; Section 14.28
	header_types.insert(header_type_map::value_type("Max-Forwards",unstructured));		//; Section 14.31
	header_types.insert(header_type_map::value_type("Proxy-Authorization",unstructured));	//; Section 14.34
	header_types.insert(header_type_map::value_type("Range",unstructured));				//; Section 14.35
	header_types.insert(header_type_map::value_type("Referer",unstructured));			//; Section 14.36
	header_types.insert(header_type_map::value_type("TE",unstructured));				//; Section 14.39
	header_types.insert(header_type_map::value_type("User-Agent",unstructured));		//; Section 14.43
	//Identifying specific requirements for Auth header RFC 2617 (comma delim)
	header_types.insert(header_type_map::value_type("Authorization",commadelim));		//; Section 14.8
	
	header_types.insert(header_type_map::value_type("Accept-Ranges",unstructured));		//; Section 14.5
	header_types.insert(header_type_map::value_type("Age",unstructured));				//; Section 14.6
	header_types.insert(header_type_map::value_type("ETag",unstructured));				//; Section 14.19
	header_types.insert(header_type_map::value_type("Location",unstructured));			//; Section 14.30
	header_types.insert(header_type_map::value_type("Proxy-Authenticate",unstructured));	//; Section 14.33
	header_types.insert(header_type_map::value_type("Retry-After",unstructured));		//; Section 14.37
	header_types.insert(header_type_map::value_type("Server",unstructured));			//; Section 14.38
	header_types.insert(header_type_map::value_type("Vary",unstructured));				//; Section 14.44
	header_types.insert(header_type_map::value_type("WWW-Authenticate",unstructured));	//; Section 14.47
	
	header_types.insert(header_type_map::value_type("Allow",unstructured));				//; Section 14.7
	header_types.insert(header_type_map::value_type("Content-Encoding",unstructured));	//; Section 14.11
	header_types.insert(header_type_map::value_type("Content-Language",unstructured));	//; Section 14.12
	header_types.insert(header_type_map::value_type("Content-Length",unstructured));	//; Section 14.13
	header_types.insert(header_type_map::value_type("Content-Location",unstructured));	//; Section 14.14
	header_types.insert(header_type_map::value_type("Content-MD5",unstructured));		//; Section 14.15
	header_types.insert(header_type_map::value_type("Content-Range",unstructured));		//; Section 14.16
	//	header_types.insert(header_type_map::value_type("Content-Type",contenttype));		//; Section 14.17 (but above also)
	header_types.insert(header_type_map::value_type("Expires",unstructured));			//; Section 14.21
	header_types.insert(header_type_map::value_type("Last-Modified",unstructured));		//; Section 14.29
	//Added List headers RFC 2369
	header_types.insert(header_type_map::value_type("List-Archive",list));				//Section 3.6
	header_types.insert(header_type_map::value_type("List-Owner",list));				//Section 3.5
	header_types.insert(header_type_map::value_type("List-Post",list));					//Section 3.4
	header_types.insert(header_type_map::value_type("List-Subscribe",list));			//Section 3.3
	header_types.insert(header_type_map::value_type("List-Unsubscribe",list));			//Section 3.2
	header_types.insert(header_type_map::value_type("List-Help",list));					//Section 3.1
	
	//Not sure what RFC.. (common broken headers)
	header_types.insert(header_type_map::value_type("Delivered-To",mailbox));		//
	header_types.insert(header_type_map::value_type("Reply-to",mailbox));			//
	header_types.insert(header_type_map::value_type("Set-Cookie",rescookie));		//Special: response cookie 
	header_types.insert(header_type_map::value_type("Set-cookie",rescookie));		//Special: response cookie 
	header_types.insert(header_type_map::value_type("content-type",contenttype));	//Special: May be broken.
	header_types.insert(header_type_map::value_type("Cookie",reqcookie));			//Special: request cookie 
	header_types.insert(header_type_map::value_type("content-disposition",cdisp));	//homogenise c-d.
	header_types.insert(header_type_map::value_type("Content-Disposition",cdisp));	//
	header_types.insert(header_type_map::value_type("content-Disposition",cdisp));	//
	header_types.insert(header_type_map::value_type("Content-disposition",cdisp));	//
	
}
void OsiMessage::shutdown() {
	header_types.clear();
}

