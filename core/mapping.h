/* 
 * mapping.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mapping.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#ifndef OBYX_CONTEXT_MAPPING_H
#define OBYX_CONTEXT_MAPPING_H
#include "function.h"

#include <utility>
#include <string>
#include <vector>


using namespace obyx;

class Mapping : public Function {
private:
	typedef std::vector< std::pair<std::string,std::string> > strstrvec;
	friend class Function;
	
	static map_type_map  map_types;
	map_type operation;				//mapping operation
	
	bool repeated;					//whether or not to repeat the loop.
	bool keys_evaluated;			//have we evaluated the keys yet?
	bool dom_evaluated;				//and did the domain evaluate yet?
	bool mat_evaluated;				//and did the matches evaluate yet?
	bool matched;					//and was there a match?
	std::string sdom;
	std::string skey;
	
	bool may_eval_outputs() {return !(mat_evaluated && !matched && (operation==m_switch)); } //{return sub_evaluated && def_evaluated;} //
	bool evaluate_this();				//private evaluation
	
public:
	bool active() const { return keys_evaluated && dom_evaluated && !mat_evaluated; }
	const map_type op() const {return operation;}
	bool field(const u_str&,string&) const;
	Mapping(ObyxElement*,const Mapping*); 
	Mapping(xercesc::DOMNode* const&,ObyxElement*); //done by factory
	virtual void addInputType(InputType*);
	virtual void addDefInpType(DefInpType*);	
	virtual ~Mapping();
//	static void list(const ObyxElement*);
	
private:
	static void init();
	static void finalise();
	static void startup(); 
	static void shutdown();	
	
};

#endif

