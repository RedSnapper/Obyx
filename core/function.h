/* 
 * function.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * function.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_INSTRUCTIONTYPE_H
#define OBYX_CONTEXT_INSTRUCTIONTYPE_H

#include <deque>
#include <utility>
#include <string>
#include <set>
#include <vector>

#include <xercesc/dom/DOMNode.hpp>

#include "obyxelement.h"
#include "output.h"
#include "inputtype.h"

using namespace obyx;

class Iteration;

class Function : public ObyxElement {
private:
	friend class PairQueue;
	friend class Document;
	
	virtual bool may_eval_outputs() =0;					//is it ok to evaluate the outputs
	virtual bool evaluate_this() =0;					//private evaluation
	bool pre_evaluate(string&);
	bool deferred;										//
	bool finalised;
	Output* catcher;									//if this has an output/space.error, it will be set here.
	string fnnote;
	
protected:
	Function(elemtype el,ObyxElement* par) : ObyxElement(par,el,flowfunction, NULL) {}
	deque< InputType* > inputs;		 //set by ~InputType
	deque< DefInpType* > definputs;  //These are the inputs that need to wait until other inputs are evaluated. 
	
public:
	deque< Output* > outputs;		 //set by ~Output
	
	static Function* FnFactory(ObyxElement*,const Function*);
	Function(xercesc::DOMNode* const&,elemtype,ObyxElement* = NULL);
	Function(ObyxElement*,const Function*);// : ObyxElement(par,orig) { copy(orig); }
	
	virtual void addInputType(InputType*) = 0;
	virtual void addDefInpType(DefInpType*) = 0;
	static void init();
	static void finalise();
	static void startup(); 
	static void shutdown();	
	
	virtual ~Function();
	bool final();
	const string note() const { return fnnote; }
	
	void evaluate(size_t=0,size_t=0);		// new context one..
	void do_catch(Output*); 			// set catcher to this.
	
};

//--------------------------------------------------------- //
class Endqueue : public Function {
private:
	friend class Function;
	bool may_eval_outputs() {return true;}	
	bool evaluate_this() {return true;}			//private evaluation
	
public:
	Endqueue(ObyxElement*,const Endqueue*); //
	Endqueue() : Function(NULL,endqueue,NULL) {} 
	virtual void addInputType(InputType*);
	virtual void addDefInpType(DefInpType*);	
	virtual ~Endqueue();
};
//--------------------------------------------------------- //

#endif

