/* 
 * osiapp.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * osiapp.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_OSI_APP_H
#define OBYX_OSI_APP_H

#include <string>
#include <xercesc/dom/DOMNode.hpp>

#include "commons/xml/xml.h"
#include "obyxelement.h"
#include "dataitem.h"

using namespace obyx;

class OsiAPP {	
private:
	struct msg {
		string head;
		string body;
		string nl;
		size_t nlsize;
		string mech;
		string type;
		string subtype;
		string boundary;
	};
	
	const static std::string crlf;			// "\r\n";
	const static std::string crlfcrlf;		// "\r\n\r\n";
	const static std::string boundary;	    // "Message_Boundary_";
	static std::string last_response;		//
	
	static unsigned int counter;			// 0...;
	static void identify_nl(string&,string&,size_t&);
	static void split_msg(string&,msg&);
	static void do_headers(msg&);
	static void unfold_headers(msg&);
	
	void compile_http_response(string&, string&, string&); //private - testing. use Environment()

public:
	static const std::string last_osi_response() { return last_response; }	//	
	void decompile_message(const xercesc::DOMNode*,vector<string>&,string&, bool = true, bool = false);
	void compile_http_request(string&, string&, string&); 
	bool request(const xercesc::DOMNode*,DataItem*&);
};

#endif
