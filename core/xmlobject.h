/*
 * xmlobject.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlobject.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_XML_OBJECT_H
#define OBYX_XML_OBJECT_H

#include <ext/hash_map>
#include <string>

#include "dataitem.h"
#include "pairqueue.h"
#include "commons/xml/xml.h"

namespace {
	using xercesc::DOMDocument;
	using xercesc::DOMNode;
	using xercesc::DOMLSParser;	
}	

class XMLObject : public DataItem {
private:
	XMLObject() {}
	void setxp(const std::string&,const std::string&,xercesc::DOMLSParser::ActionType);
	bool xp_result(const string&,DOMXPathResult*&,std::string&) const;
	static inline bool a_compare(pair<string,XMLObject*>,pair<string,XMLObject*>);
	static inline bool d_compare(pair<string,XMLObject*>,pair<string,XMLObject*>);
	
protected:
	friend class DataItem;
	friend class StrObject;
	XMLObject(const XMLObject*);
	XMLObject(const xercesc::DOMDocument*);
	XMLObject(xercesc::DOMDocument*&);
	XMLObject(const xercesc::DOMNode*);
	
public:
	//	u_str
	typedef hash_map<const u_str,u_str, hash<const u_str&> > u_str_map_type;
	static const u_str_map_type* get_ns_map() {return &object_ns_map;}
	
	static bool setns(const u_str&, const u_str&);
	static bool getns(const u_str&, u_str&,bool);
	
	XMLObject(const std::string);
	XMLObject(const u_str);
	XMLObject(const XMLObject&);	
	XMLObject(const DataItem&);	
	bool xp(const std::string&,DataItem*&,bool,std::string&) const; //get a result from xpath into a dataitem
	bool xp(const DataItem*,const std::string&,DOMLSParser::ActionType,bool,std::string&); //set a value by xpath
	bool sort(const std::string&,const std::string&,bool,bool,std::string&); //Sorts in place!
	operator xercesc::DOMNode*&();
	
	void copy(XMLObject*&) const;
	void copy(DOMDocument*&) const;
	void take(DOMDocument*&);
	void take(DOMNode*&);
	
	//dataitem API.
	virtual operator XMLObject*();
	virtual operator u_str() const;	
	virtual operator std::string() const;	
	virtual operator xercesc::DOMDocument*() const;	
	virtual operator xercesc::DOMNode*() const;	
	virtual void copy(DataItem*&) const;
	virtual kind_type kind() const { return di_object; }
	virtual long long size() const;
	virtual bool empty() const;
	virtual bool find(const DataItem*,std::string&) const;
	virtual bool find(const char*,std::string&) const;
	virtual bool same(const DataItem*) const;
	virtual void clear();
	virtual void trim();
	virtual ~XMLObject();
	
private:
	unsigned int x;									//Used during debugging to see how a doc was created.
	xercesc::DOMDocument* x_doc;
	static u_str_map_type object_ns_map;			//Store set of active namespaces across objects
	
};

#endif

