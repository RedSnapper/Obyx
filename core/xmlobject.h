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

//typedef pair<u_str,XMLObject*> uobj;
//bool uobj::operator<(const uobj&) const;

class XMLObject : public DataItem {
private:
	typedef hash_map<const u_str,xercesc::DOMXPathExpression*, hash<const u_str&> > xpe_map_type;
	
	XMLObject() : DataItem() {}
	bool xp_result(const u_str&,DOMXPathResult*&,std::string&);
	void set_pnsr(); // Set the pnsr with the latest list of namespaces.
	void del_pnsr(); // Delete the pnsr and release the xpe cache.
	xercesc::DOMXPathExpression* xpe(const u_str& );

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

	//utility
	static bool npsplit(const u_str&,pair<u_str,u_str>&,bool&);
	static pair<unsigned long long,bool> hex(const u_str&);
	static pair<unsigned long long,bool> znatural(const u_str&);
	static double real(const u_str&);
	static double real(u_str::const_iterator&);
	
	static void trim(u_str&);
	static void rtrim(u_str&);
	
	XMLObject(const std::string);
	XMLObject(const u_str);
	XMLObject(const XMLObject&);	
	XMLObject(const DataItem&);	
	bool xp(const u_str&,DataItem*&,bool,std::string&); //get a result from xpath into a dataitem
	bool xp(const DataItem*,const u_str&,DOMLSParser::ActionType,bool,std::string&); //set a value by xpath
	bool sort(const u_str&,const u_str&,bool,bool,std::string&); //Sorts in place!
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
	virtual bool find(const DataItem*,std::string&);
	virtual bool find(const char*,std::string&);
	virtual bool find(const XMLCh*,std::string&);
	virtual bool same(const DataItem*) const;
	virtual void clear();
	virtual void trim();
	virtual ~XMLObject();
	static void startup();
	
private:
	unsigned int x;								//Used during debugging to see how a doc was created.
	xercesc::DOMDocument* 			x_doc;		//Actual document itself.
	xercesc::DOMXPathNSResolver* 	xpnsr;		//namespace resolver.
	unsigned long 					xpnsr_v;	//used to indicate if the namespace has changed.
	xpe_map_type					xpe_map;

	static u_str_map_type 			object_ns_map;	//Store set of active namespaces across objects
	static unsigned long 			ns_map_version;	//used to indicate if the namespace has changed.
	
};

#endif

