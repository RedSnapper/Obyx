/* 
 * comparison.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * comparison.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_COMPARISON_H
#define OBYX_CONTEXT_COMPARISON_H

#include "function.h"

using namespace obyx;

class Comparison : public Function {
private:
	
	static cmp_type_map  cmp_types;
	
	cmp_type  operation;			   // instruction operation
	bool invert;					   // are we using the complement operation?
	bool cbreak;					   // should we break at the first logical opportunity?
	logic_t logic;					   // any/all
	
	bool eval_found;					//did we find a comparator to evaluate?
	bool cmp_evaluated;					//have we evaluated the comparators
	bool def_evaluated;					//have we evaluated the deferred inputs (ontrue/onfalse)
	char operation_result;				//'T' 'F' or 'X' <-- unevaluated
	
	bool may_eval_outputs();			//{return operation_result=='X';} 
	bool evaluate_this();				//private evaluation
	
private:
	friend class Function;
	static void init();
	static void finalise();
	static void startup(); 
	static void shutdown();	
	
public:
	Comparison(ObyxElement*,const Comparison*); // : Function(par,orig) { copy(orig); }
	Comparison(xercesc::DOMNode* const&,ObyxElement*); 
	virtual void addInputType(InputType*);
	virtual void addDefInpType(DefInpType*);	
	const cmp_type op() const {return operation;}
	virtual ~Comparison();
	
};

#endif
