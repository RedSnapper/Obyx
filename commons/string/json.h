/*
 * json.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * json.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2009-2010 Red Snapper Ltd. http://www.redsnapper.net
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

//WIB

#ifndef obyx_json_h
#define obyx_json_h

#include <string>
#include <vector>
#include "strings.h"
#include "core/dataitem.h"


class Json  {
public:
	bool encode(DataItem**,const kind_type,std::string&) const;
	bool decode(DataItem**,const kind_type,std::string&) const;
private:
	typedef enum {obj,arr,val,none} state_t;
	typedef enum { t_string,t_number,t_bool,t_null } type_t;	//what type
	static const u_str k_name;		// = u"name";
	static const u_str k_object;	// = u"object";
	static const u_str k_array;		// = u"array";
	static const u_str k_value;		// = u"value";
	static const u_str k_type;		// = u"type";
	static const u_str kt_string;	// = u"string";
	static const u_str kt_number;	// = u"number";
	static const u_str kt_null;		// = u"null";
	static const u_str kt_bool;		// = u"bool";
	void compose(const xercesc::DOMNode*,std::ostringstream &,std::ostringstream &) const;
	void nextel(const xercesc::DOMNode*&) const;
	//now decoding fns.
	bool do_qv(string::const_iterator&,const string::const_iterator&,string&,ostringstream&) const;
	bool do_nullbool(string::const_iterator&,const string::const_iterator&,ostringstream&,const string&,ostringstream&) const;
	bool do_number(string::const_iterator&,const string::const_iterator&,ostringstream&,const string&,ostringstream&) const;
	bool do_string(string::const_iterator&,const string::const_iterator&,ostringstream&,const string&,ostringstream&) const;
	bool do_name(string::const_iterator&,const string::const_iterator&,ostringstream&,ostringstream&) const;
	bool do_vlist(string::const_iterator&,const string::const_iterator&,ostringstream&,bool,ostringstream&) const;
	bool do_object(string::const_iterator&,const string::const_iterator&,ostringstream&,const string&, ostringstream&) const;
	bool do_array(string::const_iterator&,const string::const_iterator&,ostringstream&,const string&, ostringstream&) const;
	bool do_value(string::const_iterator&,const string::const_iterator&,ostringstream&,bool,ostringstream&) const;
	
};

#endif
