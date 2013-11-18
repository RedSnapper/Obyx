/* 
 * rawitem.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * rawitem.cpp is a part of Obyx - see http://www.obyx.org .
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
#include "rawitem.h"
#include "dataitem.h"
#include "xmlobject.h"

RawItem::RawItem(const std::string& s) : DataItem(),o_str(s) {
	//	do_alloc("1 "+o_str);
}
RawItem::RawItem(u_str s) : DataItem(),o_str("") { 
	XML::Manager::transcode(s.c_str(),o_str);
	//	do_alloc("2 "+o_str);
}
RawItem::RawItem(std::string& s) : DataItem(),o_str(s) {
	//	do_alloc("3 "+o_str);
}

RawItem::RawItem(const char* s) : DataItem(),o_str(s) {
	//	do_alloc("4 "+o_str);
}

RawItem::RawItem(const DataItem& s) : DataItem(),o_str(s) {
	//	do_alloc("5 "+o_str);
}
RawItem::RawItem(const xercesc::DOMNode* s) : DataItem(),o_str() {
	XML::Manager::parser()->writenode(s,o_str);
}

RawItem::~RawItem() {
	//do_dealloc();
	o_str.clear();
}

RawItem::operator XMLObject*() {
	//This will fail.
	return new XMLObject(o_str);
}

RawItem::operator xercesc::DOMDocument*() const {
	//This will fail.
	return  XML::Manager::parser()->loadDoc(o_str);
}

RawItem::operator xercesc::DOMNode*() const {
	//This will fail.
	return XML::Manager::parser()->loadDoc(o_str);
}

RawItem::operator u_str() const {
	//This will fail. Transcode only deals with utf-8.
	u_str ustr;
	XML::Manager::transcode(o_str,ustr);
	return ustr;
}

RawItem::operator std::string() const { 
	return o_str; 
}	

void RawItem::copy(DataItem*& container) const {
	container = DataItem::factory(o_str,di_raw); 	
}

bool RawItem::empty() const {
	return o_str.empty();
}

void RawItem::append(DataItem*& s) {
	std::string ns = *s;
	o_str.append(ns);
	delete s;
}

bool RawItem::find(const DataItem* o,std::string&)  {
	if ( o != nullptr) {
		return o_str.find(*o) != string::npos;
	} else {
		return false;
	}
}

bool RawItem::find(const char* o,std::string&)  {
	return o_str.find(o) != string::npos;
}

bool RawItem::find(const XMLCh* s,std::string&)  {
	if (s!=nullptr) {
		u_str tmp(pcu(s));
		string srch;
		XML::Manager::transcode(tmp,srch);
		return o_str.find(srch) != string::npos;
	} else {
		return true; //null is always found?!
	}
}

bool RawItem::same(const DataItem* o) const {
	if (o != nullptr) {
		return o_str.compare(*o) == 0;
	} else {
		return false;
	}
}

void RawItem::clear() {
	o_str.clear();
}

void RawItem::trim() {
	//do nothing
}

long long RawItem::size() const {
	unsigned long long result = o_str.size();
	return result;
}

