/* 
 * iteration.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * iteration.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_H
#define OBYX_CONTEXT_H

#include <string>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include "commons/vdb/vdb.h"

#include "function.h"
#include "obyxelement.h"

class InputType;
class DefInpType;
using namespace qxml;

class Iteration : public Function {
private:
	bool ctlevaluated;								//have we evaluated ctl
	bool evaluated;									//have we done this one already?
	bool may_eval_outputs() {return (numreps > 0) && lastrow && results.final();} // if lastrow is false, then there was a null result in control
	bool evaluate_this();							//private evaluation
	bool operation_sql();							//private evaluation
	bool operation_repeat();						//private evaluation
	bool operation_while(bool);						//private evaluation
	
	Vdb::Query *query;		//this is NOT the query -but a reference to it.
	
private:
	friend class Function;
	static void init();
	static void finalise();
	static void startup(); 
	static void shutdown();	
	
protected:
	friend class IKO;
	friend class ItemStore;
	
	static it_type_map it_types;
	
	it_type	operation;					//sql, xpath, while, repeat 
	bool		 lastrow;				//if this is true, it's the 'last row'
	bool		 expanded;				//if this is true, it's the 'last row'
	unsigned long long currentrow;		//if this is 0, then it's once only..
	unsigned long long numreps;
	
public:
	bool active() const { return ctlevaluated && !evaluated; }
	bool field(const string&,string&,string&) const;
	bool fieldexists(const string&,string&) const;
	const unsigned long long row() const {return currentrow;}
	const it_type op() const {return operation;}
	Iteration(ObyxElement*,const Iteration*); //copy.
	Iteration(xercesc::DOMNode* const&,ObyxElement*);
	virtual ~Iteration();
	virtual void addInputType(InputType*);
	virtual void addDefInpType(DefInpType*);	
	static void list(const ObyxElement*);
};

#endif
