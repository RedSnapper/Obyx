/* 
 * instruction.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * instruction.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_INSTRUCTION_H
#define OBYX_CONTEXT_INSTRUCTION_H

#include "function.h"
#include <ext/hash_map>

using namespace obyx;

class Instruction : public Function {
private:
	static op_type_map  op_types;
	op_type  operation;					// instruction operation
	unsigned int precision;				// for floating point output
	unsigned int bitpadding;
	bool base_convert;
	bool inputsfinal;
	
	//necessary fulfilment of abstract virtual functions..	
	bool may_eval_outputs() {return inputsfinal && results.final();} //{return inputsfinal;} //
	bool evaluate_this();				//private evaluation
	
private:
	//special operations
	void do_function();
	void do_random(long double&,long double,long double);
	void call_system(std::string&);
	void call_sql(std::string&);
	
public:
	const op_type op() const {return operation;}
	Instruction(ObyxElement*,const Instruction*);
	Instruction(xercesc::DOMNode* const&,ObyxElement*); //done by factory
	virtual void addInputType(InputType*);
	virtual void addDefInpType(DefInpType*);	
	virtual ~Instruction();
	
private:
	friend class Function;
	static void init();
	static void finalise();
	static void startup(); 
	static void shutdown();	
	
};
#endif

