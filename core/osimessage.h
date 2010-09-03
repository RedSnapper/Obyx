/* 
 * osimessage.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * osimessage.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_OSI_MESSAGE_H
#define OBYX_CONTEXT_OSI_MESSAGE_H

#include <string>
#include <vector>
#include <map>

class OsiMessage {	
	
private:
	
	typedef enum {
		//RFC 2822 Internet Message Format
		trace,unstructured,date_time,mailbox,msg_id,
		//RFC 2045
		version,contenttype,cte,
		//RFC 2616
		qvalue,
		//RFC 2369
		list,
		//Specials
		rescookie,reqcookie
	} header_type;	//what kind of dataItem
	
	typedef std::map<std::string, header_type > header_type_map; 
	
	string nl;
	size_t nlsize;
	
	string head;
	string body;
	string body_mechanism;
	string body_type;
	string body_subtype;
	string body_format;
	string body_charset;
	string body_boundary;
	
	static header_type_map	header_types;
	struct comment {
		string v;	//raw value
		string x;	//raw value
		bool u;		//url_encoded
		bool a;		//angled
	};
	struct address {
		string n;	//note value
		string v;	//value value
		string x;	//xml value
		bool u;		//url_encoded
		bool a;		//angled
	};
	struct subhead {
		vector< comment > comments;
		vector< address > addresses;
		string n;	//name
		string v;	//value
		string x;	//xml value
		bool u;		//url_encoded
		bool a;		//angled
	};
	
	struct header {
		vector< comment > comments;
		vector< subhead > subheads;
		vector< address > addresses;
		string n;	//name
		string v;	//value
		string s;	//subhead component (everything after the first ';' - including more of them)
		string x;	//raw value
		header_type t;
		bool u;		//url_encoded
		bool a;		//angled
	};
	
	vector< header > headers;
	
	const static std::string boundary;	    // "Message_Boundary_";
	
	void identify_nl(string&);
	void split_msg(string&);
	void do_headers();
	void unfold_headers();
	void do_header_array();
	string xml_val(header& h);
	bool do_encoding(string&);
	bool do_angled(string&);
	void do_mailbox(string&,vector< address >&);
	void do_comments(string&,vector< comment >&);
	void split_header_value(string&,string&);
	void analyse_cookie(header&,char);
	void do_header_subheads(header&);
	void do_address_subheads(header&);
	void do_trace_subheads(header&);
	void do_list_subheads(header&);
	void construct_header_value(header&);
public:
	static void init();
	static void finalise();
	static void startup();
	static void shutdown();
	void compile(string&, ostringstream&, bool = true);
	
};

#endif
