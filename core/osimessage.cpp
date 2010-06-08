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
#include "commons/string/strings.h"
#include "commons/xml/xml.h"
#include "osimessage.h"

using namespace std;

const std::string OsiMessage::boundary	= "Message_Boundary_";
OsiMessage::header_type_map OsiMessage::header_types;

void OsiMessage::identify_nl(string& msg) {
	/*
	 Given a message, identify the newline(chars) and the newline size
	 Correct newline is crlf = "\r\n" = "\x0D\x0A";
	 LS:    Line Separator, U+2028 = E280A8 
	 */
	string crlf = "\x0D\x0A";
	size_t base = msg.find_first_of(crlf);	//either cr or lf
	if (base != string::npos) {
		//CR:    Carriage Return, U+000D
		//CR+LF: CR (U+000D) followed by LF (U+000A) (and badly formed LFCR)
		//LF:    Line Feed, U+000A
		if ((msg[base] == '\x0D') && (msg[base+1] == '\x0A')) {
			nlsize = 2;
			nl = crlf;
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
			nl = crlf;
		}
	}
}
void OsiMessage::split_msg(string& msg_str) {
	if(!msg_str.empty()) {
		string nlnl = nl + nl;
		string::size_type pos = nl.size() + msg_str.find(nlnl); 
		if (pos == string::npos) {
			head = msg_str;
		} else {
			head = msg_str.substr(0, pos - nl.size());
			body = msg_str.substr(pos + nl.size(), string::npos);
		}
	}
}
void OsiMessage::split_header_value(string& v,string& s) { //splits value at the first ;
	string::size_type pos = v.find(";"); 
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
		if (str[bp] != ',') {				//hit <addr-spec> 
			address c;
			size_t bx = str.find('>',bp+1);
			c.n = str.substr(0,bp);     //note. 
			c.v = str.substr(bp+1,(bx-bp)-1); //-1 cos we want to skip the brackets 
			if (bx != string::npos) {
				str.erase(0,bx+1); 
				String::trim(str);
			} else {
				str.clear();
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
		} else { //found next address
			str.erase(0,bp +1);
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
void OsiMessage::do_header_subheads(header& h) {
	while (!h.s.empty()) {
		subhead s;
		do_comments(h.s,s.comments);
		size_t k = h.s.find_first_of("=;");
		if (h.s[k] == '=') {
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
			if ( (!body_type.empty() && (body_type.compare("text")!=0) || body.find_first_of("<&\x00") != string::npos)) {
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
					string msg_block = body.substr(first_cut, blockend - first_cut);
					OsiMessage msg;
					msg.compile(msg_block,res,false);
				}
			}
		}
		res << "</m:body>";
	}
	res << "</m:message>";
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
	header_types.insert(header_type_map::value_type("Authorization",unstructured));		//; Section 14.8
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
	header_types.insert(header_type_map::value_type("Reply-to",mailbox));			//"
}
void OsiMessage::shutdown() {
	header_types.clear();
}

