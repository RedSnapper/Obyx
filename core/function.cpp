/* 
 * function.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * function.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <ctype.h>
#include <math.h>
#include <xercesc/dom/DOMNode.hpp>

#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"

#include "pairqueue.h"
#include "iteration.h"
#include "obyxelement.h"
#include "function.h"
#include "inputtype.h"
#include "output.h"
#include "document.h"

#include "instruction.h"
#include "comparison.h"
#include "mapping.h"
//#include "objecttype.h"
#include "itemstore.h"

//using namespace Log;
using namespace XML;
using namespace obyx;

/*
 c++ Note  
 'break' may only be used inside a loop, such as a for or while loop, or a switch statement. it breaks out of that loop. 
 'continue' terminates the current iteration of a loop and proceeds directly to the next. 
 In the case of a for loop it jumps to its increment-expression.
 */
Function::Function(xercesc::DOMNode* const& n,elemtype el,ObyxElement* par) : 
ObyxElement(par,el,flowfunction,n),deferred(false),finalised(false),
catcher(NULL),fnnote(),outputs(),inputs(),definputs() {
	ObyxElement* ft = par; 
	while (ft != NULL && ft->wotzit != xmldocument ) {
		if (ft->wotspace == defparm || ft->wotzit == output) {
			deferred=true; 
			break;
		}
		Function* fx = dynamic_cast<Function*>(ft);
		if (fx!= NULL) {
			deferred = fx->deferred;
			break;
		} else {
			ft = ft->p;
		}
	}
	if (el != endqueue) {
		Manager::attribute(n,UCS2(L"note"),fnnote);
	}
}
Endqueue::Endqueue(ObyxElement* par,const Endqueue* orig) : Function(par,orig) {
	
}
Endqueue::~Endqueue()  {
	if ( outputs.size() != 0) {
		outputs.clear();
	}
	results.clear();
}
bool Function::pre_evaluate(string& errs) {
	bool retval = false; //returns true if it has been evaluated (and may be deleted)
	if ( !deferred || p->wotzit == xmldocument ) {
		Function* fn = NULL;
		if (p->wotzit == key || (p->wotzit == xmldocument && p->p != NULL) ) {
			fn = dynamic_cast<Function*>(p->p->p);
		} else {
			fn = dynamic_cast<Function*>(p->p);
		}
		if ( (fn != NULL && ! fn->deferred) || (p->wotzit == xmldocument && p->p == NULL)) {
			evaluate();
			DataItem* di = NULL;
			results.takeresult(di);
			p->results.append(di);
			retval = true; 
		} else {
			p->results.append(this,errs);
		}
	} else {
		p->results.append(this,errs);
		deferred = true;		//wait until they are evaluated before running!!
	}
	return retval;
}
Function* Function::FnFactory(ObyxElement* par,const Function* orig) { 
	Function* retval = NULL;
	switch (orig->wotzit) {
		case iteration: {
			retval = new Iteration(par,dynamic_cast<const Iteration *>(orig)); 
		} break;
		case instruction: { 
			retval = new Instruction(par,dynamic_cast<const Instruction *>(orig));
		} break;
		case mapping: { 
			retval = new Mapping(par,dynamic_cast<const Mapping *>(orig));
		} break;
		case comparison: { 
			retval = new Comparison(par,dynamic_cast<const Comparison *>(orig)); 
		} break;
		case endqueue: { 
			retval = const_cast<Endqueue *>(dynamic_cast<const Endqueue *>(orig)); 
		} break;
		default: {
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. " << orig->name() << " being passed off as a flow-function!" << Log::LO << Log::blockend;
		} break;
	}
	return retval;
}
Function::Function(ObyxElement* par,const Function* orig) : 
ObyxElement(par,orig),deferred(orig->deferred),finalised(orig->finalised),catcher(NULL),
fnnote(orig->fnnote),outputs(),inputs(),definputs() { 
	if (wotzit != endqueue) {
		for ( unsigned int i = 0; i < orig->inputs.size(); i++ )
			inputs.push_back(new InputType(this,orig->inputs[i]));
		for ( unsigned int i = 0; i < orig->definputs.size(); i++ )
			definputs.push_back(new DefInpType(this,orig->definputs[i]));
		for ( unsigned int i = 0; i < orig->outputs.size(); i++ ) {
			Output *ot = new Output(this,orig->outputs[i]);
			if (ot->type == out_error) {
				catcher=ot;
			}
			outputs.push_back(ot);
		}
	}
}
void Function::do_catch(Output* out_error) { 			// set catcher to this.
	if (catcher != NULL) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. Only one output with space='error' allowed per function." << Log::LO << Log::blockend;
	} else {
		catcher = out_error;							//if this has an output/space.error, it will be set here.
	}
}
void Function::evaluate(size_t,size_t) {
	finalised = false;
	if (!deferred) {
		prep_breakpoint();
		if (wotzit != endqueue) {		
			if ( ! results.final() ) {
				finalised = evaluate_this();
				if (finalised) results.normalise();		//need to keep inputs on partial evaluation.
			}
			if (results.final() && !outputs.empty()) {
				if ( may_eval_outputs() ) {	
					DataItem* imm_result = NULL; //need to keep immediate results.
					if (catcher != NULL) {
						catcher->evaluate();
					}
					if (catcher == NULL || !(catcher->caughterr())) {
						size_t os = outputs.size();
						for (size_t s = 0; s < os; s++) { //if nothing was caught, then do non-error outputs.
							Output* theoutput = outputs[s];
							if (theoutput->gettype() != out_error) { //evaluate what we can.
								theoutput->evaluate(s+1,os);
								if ( theoutput->gettype() == out_immediate ) {
									theoutput->results.takeresult(imm_result);
								}
							}
						}
					}
					catcher = NULL;
					results.setresult(imm_result); //now encoded and what-have-you - or may be null.					
				} else {
					finalised=false;
					results.clearresult();
				}
			} else {
				//results are taken by pairqueue of parent.
			}
		} 
	} else {
		finalised=true; 
	}
	do_breakpoint();
} 
bool Function::final() {
	return finalised;
}
Function::~Function() {
	//results is deleted by ~ObyxElement
	while ( inputs.size() > 0) {
		delete inputs.front();
		inputs.pop_front(); 
	}
	while ( definputs.size() > 0) {
		delete definputs.front();
		definputs.pop_front(); 
	}
	while ( outputs.size() > 0) {
		delete outputs.front();
		outputs.pop_front(); 
	}
}
void Function::init() {
	//init once per main document..
	Instruction::init();
	Comparison::init();
	Iteration::init();
	Mapping::init();	
}
void Function::finalise() {
	//finalise once per main document..
	Instruction::finalise();
	Comparison::finalise();
	Iteration::finalise();
	Mapping::finalise();
}
void Function::startup() {
	//startup once per process..
	Instruction::startup();
	Comparison::startup();
	Iteration::startup();
	Mapping::startup();	
	PairQueue::startup();
}
void Function::shutdown() {
	//shutdown once per process..
	PairQueue::shutdown();
	Instruction::shutdown();
	Comparison::shutdown();
	Iteration::shutdown();
	Mapping::shutdown();	
}
void Endqueue::addInputType(InputType*) {
	*Logger::log << Log::error << Log::LI << "Internal Error. Endqueue cannot accept InputTypes." << Log::LO; 
	trace();
	*Logger::log << Log::blockend;
}
void Endqueue::addDefInpType(DefInpType*) {
	*Logger::log << Log::error << Log::LI << "Internal Error. Endqueue cannot accept DefInpTypes." << Log::LO; 
	trace();
	*Logger::log << Log::blockend;	
}


