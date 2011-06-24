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

//our space repository hashmaps are keyed with strings because 
//we want to use Regex searches over their keys - (afaik) and regex doesn't support 
//ustr.
//Here string == std::string

namespace obyx {
	typedef xercesc::DOMLSParser::ActionType insertion_type;
	typedef hash_map<const string,DataItem*, hash<const string&> > item_map_type;
}	

using namespace obyx;
using namespace XML;

namespace obyx {
	
	class ItemStore {
	private:
		string						owner;
		item_map_type				the_item_map;
		
	public:
		static void init();
		static void finalise();
		static void startup();
		static void shutdown();
		ItemStore();
		~ItemStore();
		ItemStore(const ItemStore*);
		
		void list();									//list all current items to debugger.	
//internal api, as keys are actually held as strings..
		bool exists(const u_str&,bool,string&);	//name#path
		bool exists(const u_str&,const u_str&,bool,string&); //name,path.
		bool meta(const string&,unsigned long long&); //used for meta settings.
		bool find(const u_str&,const u_str&,bool,std::string&); //name,path.
		bool find(const u_str&,bool,string&);
		bool find(const string&,bool,string&);
		void keys(const u_str&,set<string>&,string&);
		bool release(const u_str&);
		bool sset(const u_str&,const u_str&,bool,DataItem*&, kind_type, string& );	//name,xpath,expected,document, intended kind...
		bool sget(const u_str&,const u_str&,bool,DataItem*&, bool, string&);		//name,xpath,expected,container, release?, errstr
		void setowner(const string);
		//NS functions	
		static bool nsfind(const DataItem*,bool);
		static bool setns(const DataItem*, DataItem*&);				//namespace name, namespace identity. eg setns("o","http://www.obyx.org");
		static bool getns(const string&, DataItem*&,bool);			//namespace name; returns namespace identity. eg getns("o") => "http://www.obyx.org";
		static bool nsexists(const string&,bool);					//namespace existence
		//Grammar functions	
		static bool setgrammar(const DataItem*, DataItem*&);		//set a grammar for a url
		static bool getgrammar(const string&, DataItem*&, kind_type,bool);	//get a grammar for a url
		static bool grammarexists(const string&,bool);			//grammar existence
		static bool grammarfind(const DataItem*,bool);	
		
	};
	
}

#endif


