/*
 * dataitem.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * dataitem.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_DATA_ITEM_H
#define OBYX_DATA_ITEM_H

#include <string>
#include "commons/xml/xml.h"

namespace {
	using xercesc::DOMDocument;
	using xercesc::DOMNode;
	using xercesc::DOMLSParser;	
}	

namespace obyx {
	typedef enum { di_auto,di_raw,di_text,di_utext,di_object,di_fragment,di_null } kind_type;	//what kind of dataItem
	typedef std::map<u_str, kind_type > kind_type_map; 
}

class XMLObject;
class StrObject;
class UStrItem;
class RawItem;
using namespace obyx;	

//base class for xml / text that appear in PairQueue.
class DataItem {
private:
	static DataItem* autoItem(const std::string&);
	static DataItem* autoItem(const u_str&);
	
	//	typedef std::map<unsigned long, std::string > long_map; 
	//	static long_map ce_map;
	//	void do_alloc(const std::string);
	//	void do_dealloc();
	
protected:
	DataItem();
//	DataItem(const DataItem&);	
	
public:	
	static void init();
	static void finalise();
	static void startup();
	static void shutdown();
	
	static DataItem* factory(const std::string&,kind_type = di_auto);
	static DataItem* factory(std::string&,kind_type = di_auto);
	static DataItem* factory(const char*,kind_type = di_auto);
	static DataItem* factory(u_str,kind_type = di_auto);
	static DataItem* factory(const XMLCh*,kind_type = di_auto);
	static DataItem* factory(const xercesc::DOMDocument*&,kind_type = di_object);
	static DataItem* factory(xercesc::DOMDocument*&,kind_type = di_object);
	static DataItem* factory(xercesc::DOMNode* const&,kind_type = di_auto); //actually must be const.
	static DataItem* factory(DataItem*,kind_type = di_auto);
	
	static void append(DataItem*&,DataItem*&);
	
	DataItem* operator=(const DataItem*&) { return this; } 
	
	//Base Class API	
	virtual operator XMLObject*() =0;	
	virtual operator u_str() const =0;	
	virtual operator std::string() const =0;	
	virtual operator xercesc::DOMDocument*() const=0;	
	virtual operator xercesc::DOMNode*() const =0;	
	virtual void copy(DataItem*&) const=0;
	virtual kind_type kind() const =0;
	virtual long long size() const =0;
	virtual bool find(const DataItem*,std::string&) =0;
	virtual bool find(const char*,std::string&) =0;
	virtual bool find(const XMLCh*,std::string&) = 0;
	virtual bool empty() const =0;
	virtual bool same(const DataItem*) const =0;
	virtual void append(DataItem*&);	//handle xmlobjects here.
	virtual void clear() =0;
	virtual void trim() =0;
	virtual ~DataItem();
};

#endif

