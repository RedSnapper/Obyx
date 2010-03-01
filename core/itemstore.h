/* 
 * itemstore.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * itemstore.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_ITEM_STORE_H
#define OBYX_ITEM_STORE_H

#include <string>
#include <ext/hash_map>
#include <map>

#include "commons/string/strings.h"
#include "commons/vdb/vdb.h"
#include "obyxelement.h"
#include "dataitem.h"

#include "xmlobject.h"
#include "strobject.h"

class Iteration;

namespace qxml {
	//See 	DOMLSParser::ActionType
	typedef xercesc::DOMLSParser::ActionType insertion_type;
	typedef hash_map<const string,DataItem*, hash<const string&> > item_map_type;
	typedef std::stack<item_map_type* > item_map_stack_type;												//stack of hashmaps controlled by isolated attr.
	typedef hash_map<const u_str, item_map_stack_type* , hash<const u_str&> > item_map_stack_map_type;	//hashmap of stacks controlled by ns.
	typedef std::stack<Iteration*> iter_stack_type;															//stack of hashmaps controlled by isolated attr.	
}	

using namespace qxml;
using namespace XML;

class ItemStore {
private:
	static item_map_type*			the_item_map;
	static item_map_stack_type*		the_item_map_stack;
	static item_map_stack_map_type	the_item_map_stack_map;
	static iter_stack_type*			the_iteration_stack; //stack of iterations

	
public:
	static void init();
	static void shutdown();
	
	static void list();										//list all current items to debugger.	
	static bool exists(const DataItem*,bool,std::string&);	//name#path
	static void release(const DataItem*);

	static bool set(const DataItem*, DataItem*&, kind_type, std::string& );	//name#path, document, intended kind...
	static bool get(const DataItem*, DataItem*&, bool, std::string&);		//name#path, container, release?

	static bool get(const string&, string&);		//name container (used for quick internal hacks)

	static bool setns(const DataItem*, DataItem*&);			//namespace name, namespace identity. eg setns("o","http://www.obyx.org");
	static bool getns(const DataItem*, DataItem*&,bool);	//namespace name, namespace identity. eg setns("o","http://www.obyx.org");
	static bool nsexists(const DataItem*,bool);				//namespace existence

	static bool setgrammar(const DataItem*, DataItem*&);		//set a grammar for a url
	static bool getgrammar(const DataItem*, DataItem*&, kind_type,bool);	//get a grammar for a url
	static bool grammarexists(const DataItem*,bool);			//grammar existence
	
	static void prefixpushed(const u_str&);
	static void prefixpopped(const u_str&);

};

#endif


