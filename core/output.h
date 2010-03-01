/* 
 * output.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * output.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_OUTPUT_H
#define OBYX_CONTEXT_OUTPUT_H

#include <string>

#include <xercesc/dom/DOMNode.hpp>

#include "obyxelement.h"
#include "iko.h"

using namespace qxml;

class Function;

class Output : public IKO {
private:
	typedef enum { value, path, expires, domain } part_type;	// parts of a cookie.
	typedef std::map<u_str, part_type > part_type_map; 
	typedef enum { location, privacy, cache, pragma, custom, connection, server, p3p, range, content_length, code, content_type, h_expires, date, content_disposition, nocache, remove_http, remove_nocache, remove_date, http_object} http_line_type;	//
	typedef std::map<u_str, http_line_type > http_line_type_map; 

	friend class Function;
	static output_type_map output_types;
	static http_line_type_map httplinetypes;
	static part_type_map part_types;
	output_type		type;			//derived from type attribute
	part_type		part;
	bool errowner;				//so we can delete the stream just once.
	ostringstream* errs;		//error holder - used for type=error

	void sethttp(const http_line_type,const string&);

public:
	ostringstream*& geterrs()  { return errs; }
	output_type gettype() const { return type; }
	bool evaluate(size_t,size_t);
	static void init();			//set up maps
	Output(xercesc::DOMNode* const&,ObyxElement* = NULL,elemtype = output);
	Output(ObyxElement*,const Output*); //
	virtual ~Output();
};

#endif

