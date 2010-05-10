/* 
 * inputtype.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * inputtype.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_INPUTTYPE_H
#define OBYX_CONTEXT_INPUTTYPE_H

#include <string>

#include <xercesc/dom/DOMNode.hpp>

#include "obyxelement.h"
#include "pairqueue.h"
#include "output.h"

class Iteration;
class Function;
using namespace obyx;

class InputType : public IKO {
protected:
	friend class Instruction;
	friend class Output;
	friend class DefInpType; 
	static inp_space_map  inp_spaces;
	static kind_type_map  kind_types;
	
	bool	  eval;				//referring to eval attribute.
	bool	  release;		    //release attribute - do we release the store/object ?
	inp_space  type;				//the TYPE of input - ie store, immediate, etc. derived from "type" attribute
	u_str    parm_name;			//name as used in the called function parm value
	
public:
	inp_space gettype()	const {return type;}
	virtual bool evaluate(size_t=0,size_t=0); 
	InputType(ObyxElement*,const InputType*);
	InputType(xercesc::DOMNode* const&,ObyxElement* = NULL, elemtype = input);
	virtual ~InputType() {}
	static void startup(); 
	static void shutdown();	
};

class DefInpType : public InputType {
protected:
	friend class InputType;
	friend class Comparison;
	friend class Mapping;
	bool			k_break;			    //key sets this.   - break here. true by default.
	bool			k_scope;			    //scope sets this. - true (all) by default.
	unsigned char 	k_format;		        //key_format key sets this. It is either literal or it is regex. 'l' or 'r' 'l' by default.	
	InputType *key;						//used by match only.
	bool evaluate_key();
	
public:
	DefInpType(ObyxElement*,const DefInpType* );
	DefInpType(xercesc::DOMNode* const&,ObyxElement* = NULL, elemtype = input);
	virtual bool evaluate(size_t=0,size_t=0); 
	virtual ~DefInpType();
};

#endif
