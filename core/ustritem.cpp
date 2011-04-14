
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
#include "ustritem.h"
#include "strobject.h"
#include "xmlobject.h"
#include "dataitem.h"

UStrItem::UStrItem(const std::string& s) : DataItem(),o_str() {
	XML::Manager::transcode(s,o_str);
}
UStrItem::UStrItem(u_str s) : DataItem(),o_str(s) {
}
//UStrItem::UStrItem(const u_str& s) : DataItem(),o_str(s) {}
UStrItem::UStrItem(std::string& s) : DataItem() {
	XML::Manager::transcode(s,o_str);
}

UStrItem::UStrItem(const char* s) : DataItem(),o_str() {
	if (s!=NULL) {
		string tmp(s);	
		XML::Manager::transcode(tmp,o_str);
	}
}

UStrItem::UStrItem(const XMLCh* s) : DataItem(),o_str() {
	if (s!=NULL) {
		o_str = s;
	}
}

UStrItem::UStrItem(const xercesc::DOMNode* s) : DataItem(),o_str() {
	XML::Manager::parser()->writenode(s,o_str);
}

UStrItem::UStrItem(const DataItem& s) : DataItem(),o_str(s) {
}

UStrItem::~UStrItem() {
	o_str.clear();
}

UStrItem::operator XMLObject*() {
	return new XMLObject(o_str);
}

UStrItem::operator xercesc::DOMDocument*() const {
	return  XML::Manager::parser()->loadDoc(o_str);
}

UStrItem::operator xercesc::DOMNode*() const {
	return XML::Manager::parser()->loadDoc(o_str);
}

UStrItem::operator u_str() const {
	return o_str;
}

UStrItem::operator std::string() const { 
	string cstr;
	XML::Manager::transcode(o_str,cstr);
	return cstr; 
}	

void UStrItem::copy(DataItem*& container) const {
	container = DataItem::factory(o_str,di_utext); 	
}

bool UStrItem::empty() const {
	return o_str.empty();
}

void UStrItem::append(DataItem*& s) {
	string s_c;
	u_str s_str = *s;
	XML::Manager::append(o_str,s_str);
}

bool UStrItem::find(const DataItem* o,std::string&) const {
	if ( o != NULL) {
		return o_str.find(*o) != string::npos;
	} else {
		return false;
	}
}

bool UStrItem::find(const char* o,std::string&) const {
	if (o!=NULL) {
		string tmp(o);
		u_str srch;
		XML::Manager::transcode(tmp,srch);
		return o_str.find(srch) != string::npos;
	} else {
		return true; //null is always found?!
	}
}

bool UStrItem::find(const XMLCh* srch,std::string&) const {
	if (srch!=NULL) {
		return o_str.find(srch) != string::npos;
	} else {
		return true; //null is always found?!
	}
}

bool UStrItem::same(const DataItem* o) const {
	if (o != NULL) {
		return o_str.compare(*o) == 0;
	} else {
		return false;
	}
}

void UStrItem::clear() {
	o_str.clear();
}

void UStrItem::trim() {
	if (!o_str.empty()) {
		XMLObject::trim(o_str);
	}
}

long long UStrItem::size() const {
	unsigned long long result = 0;
	result = (unsigned long long)o_str.size();
	return result;
}

