/*
 * fragmentobject.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * fragmentobject.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_FRAGMENT_OBJECT_H
#define OBYX_FRAGMENT_OBJECT_H

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

class FragmentObject : public DataItem {
private:
	FragmentObject() {}
	
protected:
	friend class DataItem;
	friend class StrObject;
	friend class XMLObject;
	
public:
	
	static void init();
	static void finalise();
	static void startup();
	static void shutdown();
	
	FragmentObject(const std::string);
	FragmentObject(u_str);
	FragmentObject(const FragmentObject&);	
	FragmentObject(const DataItem&);	
	FragmentObject(const xercesc::DOMNode*);
	operator xercesc::DOMNode*&();
	void take(DOMNode*&);
	
	//dataitem API.
	virtual operator FragmentObject*();
	virtual operator XMLObject*();
	virtual operator u_str() const;	
	virtual operator std::string() const;	
	virtual operator xercesc::DOMDocument*() const;	
	virtual operator xercesc::DOMNode*() const;	
	virtual void copy(DataItem*&) const;
	virtual kind_type kind() const { return di_fragment; }
	virtual long long size() const;
	virtual bool empty() const;
	virtual bool find(const DataItem*,std::string&) const;
	virtual bool find(const char*,std::string&) const;
	virtual bool find(const XMLCh*,std::string&) const;
	virtual void append(DataItem*&);
	virtual bool same(const DataItem*) const;
	virtual void clear();
	virtual void trim();
	virtual ~FragmentObject();
	
private:
	static xercesc::DOMDocument* frag_doc;
	xercesc::DOMDocumentFragment* fragment;
};

#endif

