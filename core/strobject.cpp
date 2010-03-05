/* 
 * dataitem.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * dataitem.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "commons/logger/logger.h"
#include "commons/string/strings.h"
#include "strobject.h"
#include "xmlobject.h"
#include "dataitem.h"

StrObject::StrObject(const std::string& s) : DataItem(),o_str(s) {}
StrObject::StrObject(u_str s) : DataItem(),o_str("") { 
	XML::transcode(s.c_str(),o_str);
}
StrObject::StrObject(std::string& s) : DataItem(),o_str(s) {}

StrObject::StrObject(const char* s) : DataItem(),o_str(s) {}

StrObject::StrObject(const DataItem& s) : DataItem(),o_str(s) {}

StrObject::operator XMLObject*() {
	return new XMLObject(o_str);
}

StrObject::operator xercesc::DOMDocument*() const {
	return  XML::Manager::parser()->loadDoc(o_str);
}

StrObject::operator xercesc::DOMNode*() const {
	return XML::Manager::parser()->loadDoc(o_str);
}

StrObject::operator u_str() const {
	u_str ustr; 
	XML::transcode(o_str,ustr);
	return ustr;
}

StrObject::operator std::string() const { 
	return o_str; 
}	

void StrObject::copy(DataItem*& container) const {
	container = DataItem::factory(o_str,di_text); 	
}

bool StrObject::empty() const {
	return o_str.empty();
}

void StrObject::append(DataItem*& s) {
	std::string ns = *s;
	o_str.append(ns);
	delete s;
}

bool StrObject::find(const DataItem* o,std::string&) const {
	if ( o != NULL) {
		return o_str.find(*o) != string::npos;
	} else {
		return false;
	}
}

bool StrObject::find(const char* o,std::string&) const {
	return o_str.find(o) != string::npos;
}

bool StrObject::same(const DataItem* o) const {
	if (o != NULL) {
		return o_str.compare(*o) == 0;
	} else {
		return false;
	}
}

void StrObject::clear() {
	o_str.clear();
}

void StrObject::trim() {
	if (!o_str.empty()) {
		String::trim(o_str);
	}
}

long long StrObject::size() const {
	unsigned long long result = 0;
	String::length(o_str,result);
	return result;
}

StrObject::~StrObject() {
	o_str.clear();
	o_str.reserve(0);
}