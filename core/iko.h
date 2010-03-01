/* 
 * iko.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * iko.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_IKO_H
#define OBYX_CONTEXT_IKO_H

#include <string>

#include <xercesc/dom/DOMNode.hpp>

#include "obyxelement.h"

using namespace qxml;

// public base class for Input/Key/Output

class IKO : public ObyxElement {
private:
	friend class Function;
	
protected:
	friend class Document;
	friend class ObyxElement;

	void process_encoding(DataItem*&);
	static kind_type_map		kind_types;
	static enc_type_map			enc_types;
	static inp_type_map			ctx_types; //subset of input types.
	static current_type_map		current_types;

	kind_type kind;				//derived from the kind attribute
	enc_type  encoder;			//derived from the encoder attribute
	inp_type  context;			//derived from the context attribute
	process_t process;			//derived from the process attribute
	bool	  wsstrip;			//referring to wsstrip attribute.
	bool	  exists;		    //a value exists - is inp_type or has a context != none
	u_str     name_v;			//name value - used for tracing etc.
	
	//            input    release eval name/ref  container 
	bool evaltype(inp_type, bool, bool, kind_type, DataItem*&,DataItem*&); 
	
public:

	static void init();
	static bool currentenv(const string&,const usage_tests,const IKO*,DataItem*&);
	bool getexists() const {return exists;}
	virtual bool evaluate(size_t,size_t)=0;
	IKO(ObyxElement*,const IKO*); 
	IKO(xercesc::DOMNode* const&,ObyxElement* = NULL, elemtype = endqueue);	
	virtual ~IKO() {};
};

#endif

