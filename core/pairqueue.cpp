/* 
 * pairqueue.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * pairqueue.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <string>
#include <deque>

#include "commons/logger/logger.h"
#include "commons/string/strings.h"

#include "pairqueue.h"
#include "function.h"
#include "iteration.h"
#include "dataitem.h"
#include "xmlobject.h"

using namespace std;
using namespace Log;

Function* PairQueue::pqendthing = nullptr; 

void PairQueue::startup() {
	//startup once per process..
	PairQueue::pqendthing = new Endqueue();  
}
void PairQueue::shutdown() {
	//shutdown once per process..
	if (PairQueue::pqendthing != nullptr) {
		delete PairQueue::pqendthing;  
		PairQueue::pqendthing = nullptr;  
	}
}
PairQueue::PairQueue(bool pb) : queue(),theresult(nullptr),finalised(false) {
	if (pb) {
		queue.push_back(pqpair(nullptr,pqendthing));
	}
}
PairQueue::~PairQueue() {
	clear(false);
	if (theresult != nullptr) {
		delete theresult;
		theresult = nullptr;
	}
}
void PairQueue::clear(bool add_endthing) { 
	size_t n = queue.size();
	if (n == 1 && queue[0].second == pqendthing) {
		DataItem*& di = queue[0].first;	
		if ( di != nullptr ) {
			delete di;
			di = nullptr;
		}
		if (!add_endthing) {
			queue.clear(); 
		}
	} else {
		for ( size_t i = 0; i < n; i++) {
			DataItem* di = queue[i].first;	
			queue[i].first = nullptr;
			if ( di != nullptr ) {
				delete di;
			}
			Function* qic = queue[i].second; 
			queue[i].second = nullptr;
			if (qic != nullptr && qic != pqendthing) { //Not really sure why this is nullptr sometimes.
				delete qic; //should get here when paths aren't followed...
			}
		}
		queue.clear(); 
		if (add_endthing) {
			queue.push_back(pqpair(nullptr,pqendthing));
	//		finalised = false; //DID CAUSE TROUBLE
		}
	}
}

void PairQueue::setresult(PairQueue& other) {
	if (theresult != nullptr) {
		delete theresult;
		theresult = nullptr;
	}
	theresult=other.theresult;
	other.theresult = nullptr;
	finalised=true; 
	clear(true);
}

void PairQueue::setresult(DataItem*& res, bool wsstrip) { 
	if (theresult != nullptr) {
		delete theresult;
		theresult = nullptr;
	}
	if (res != nullptr) {
		trim(res,wsstrip);
		theresult=res;
		res = nullptr;
	}
	finalised=true; 
	clear(true);
}
const DataItem* PairQueue::result() const { 
	return theresult;
} 
void PairQueue::takeresult(DataItem*& container) { 
	container = theresult;
	theresult = nullptr;
	clear(true);
} 
void PairQueue::copy(ObyxElement* mypar,const PairQueue& orig) {	//Pointers are not shared
	//typedef pair< DataItem*, Function* > pqpair; dynamic_cast<Output *>
	//This is the new unique record. The orig is what I am copying from.
	finalised = orig.finalised;
	queue.clear();	//remove even the endthing - because we will be adding it here..
	if (theresult != nullptr) {
		delete theresult; theresult = nullptr;
	}	
	if ( orig.theresult != nullptr ) {
		theresult = DataItem::factory(orig.theresult); //copy construction
	}
	size_t n = orig.queue.size();
	for ( size_t i = 0 ; i < n ; i++ ) {
		DataItem* qi1 = nullptr;
		if (orig.queue[i].first != nullptr) {
			orig.queue[i].first->copy(qi1);
		}
		if ( orig.queue[i].second != pqendthing) {
			Function* qi2 = Function::FnFactory(mypar,orig.queue[i].second);
			queue.push_back(pqpair(qi1,qi2));
		} else {
			queue.push_back(pqpair(qi1,pqendthing));
		}
	}
}
void PairQueue::append(PairQueue& xqueue,ObyxElement *par) {
	//when we do this, are we taking, or copying? let's assume taking. (used by iteration)
	if ( xqueue.final() ) {
		DataItem* fn = nullptr;
		xqueue.takeresult(fn);
		append(fn);
	} else {
		if (finalised) {
			if (!queue.empty()) {
				*Logger::log << Log::info << Log::LI << "Finalised result has a non empty queue in append result!" << Log::LO << Log::blockend; 
			} else {
				queue.push_back(pqpair(theresult,pqendthing));
				theresult = nullptr;
			}
			finalised = false;
		}
		DataItem::append(queue.back().first,xqueue.queue[0].first);
		queue.back().second = Function::FnFactory(par,xqueue.queue[0].second);		
		size_t n = xqueue.size();
		for ( size_t i = 1; i < n; i++) { //will copy new endthing from xqueue
			DataItem*& qi1 = xqueue.queue[i].first;  //grab this!
			xqueue.queue[i].first = nullptr;
			if ( xqueue.queue[i].second != pqendthing) {
				Function* qi2 = Function::FnFactory(par,xqueue.queue[i].second);
				delete xqueue.queue[i].second; //delete old.
				queue.push_back(pqpair(qi1,qi2));
			} else {
				queue.push_back(pqpair(qi1,pqendthing));
				break;
			}
		}
	}
	xqueue.clear(false); 
}
void PairQueue::append(pqpair& new_pair,ObyxElement *par) {
	if (finalised) {
		DataItem::append(theresult,new_pair.first);
		if ( new_pair.second != pqendthing) {
			queue.back().first = theresult;
			queue.back().second = Function::FnFactory(par,new_pair.second); 		
			queue.push_back(pqpair(nullptr,pqendthing));
			if (theresult != nullptr ) {
				delete theresult;
				theresult = nullptr;
			}
			finalised = false;
		}
	} else {
		DataItem::append(queue.back().first,new_pair.first);
		if ( new_pair.second != pqendthing) {
			queue.back().second = Function::FnFactory(par,new_pair.second); 		
			queue.push_back(pqpair(nullptr,pqendthing));
		}
	}
}
void PairQueue::append(DataItem*& stuff) { //stuff is taken from here.
	if (finalised) {
		DataItem::append(theresult,stuff);
	} else {
		DataItem::append(queue.back().first,stuff);
	}	
}
void PairQueue::append(u_str stuff,kind_type kind) { //from xmlnode / xmleelement
	//used by instruction etc.
	if (finalised) {
		if (theresult == nullptr) {
			theresult = DataItem::factory(stuff,kind);
		} else {
			DataItem* tmp = DataItem::factory(stuff,kind);
			DataItem::append(theresult,tmp);
		}
	} else {
		if (queue.back().first == nullptr) {
			queue.back().first = DataItem::factory(stuff,kind);
		} else {
			DataItem* tmp = DataItem::factory(stuff,kind);
			DataItem::append(queue.back().first,tmp);
		}
	}
}
void PairQueue::append(std::string stuff,kind_type kind) { //from xmlnode / xmleelement
	//used by instruction etc.
	if (finalised) {
		if (theresult == nullptr) {
			theresult = DataItem::factory(stuff,kind);
		} else {
			DataItem* tmp = DataItem::factory(stuff,kind);
			DataItem::append(theresult,tmp);
		}
	} else {
		if (queue.back().first == nullptr) {
			queue.back().first = DataItem::factory(stuff,kind);
		} else {
			DataItem* tmp = DataItem::factory(stuff,kind);
			DataItem::append(queue.back().first,tmp);
		}
	}
}
void PairQueue::append(Function* ins,std::string& errs) { //used a lot.
	if ( ins != pqendthing ) {
		if (finalised) {
			if (!queue.empty()) {
				errs = "There is already a value set for the result."; 
			} else {
				queue.push_back(pqpair(theresult,ins));
				queue.push_back(pqpair(nullptr,pqendthing));
				theresult = nullptr;
				finalised = false;
			}
		} else {
			queue.back().second = ins; //->unique(); //This should NOT be unique. this is the join for the instructiontypes
			queue.push_back(pqpair(nullptr,pqendthing));
		}
	}
}
void PairQueue::clearresult() { 
	if (theresult != nullptr) {
		delete theresult;
		theresult = nullptr;
		finalised=true; 
	}	
}
bool PairQueue::trim(DataItem*& item,bool strip) {
	if (item != nullptr) {
		if (strip) {
			item->trim();
		}
		if (item->empty()) {
			delete item;
			item = nullptr;
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}
void PairQueue::evaluate(bool wsstrip) {
	if ( ! finalised ) {
		size_t i = 0;
		while (  i < queue.size() ) {
			bool qif = trim(queue[i].first,wsstrip);
			Function*  qi2 = queue[i].second;
			if ( qi2 != pqendthing && qi2 != nullptr) {
				if ( qi2->final() ) {
					bool qjf = trim(qi2->results.theresult,wsstrip);
					if (qjf) {
						if (qif) {
							DataItem::append(queue[i].first,qi2->results.theresult);
						} else {
							queue[i].first = qi2->results.theresult;
							qif = true;
						}
						qi2->results.theresult = nullptr;
					}
					delete qi2;	
					bool qp2  = trim(queue[i+1].first,wsstrip);
					if (qp2) {
						if (qif) {
							DataItem::append(queue[i].first,queue[i+1].first);
						} else {
							queue[i].first = queue[i+1].first;
						}
					}
					queue[i].second = queue[i+1].second;
					queue.erase(queue.begin()+i+1);  
				} else {
					qi2->evaluate();
					bool qjf = trim(qi2->results.theresult,wsstrip);
					if (qjf) {
						if (qif) {
							DataItem::append(queue[i].first,qi2->results.theresult);
						} else {
							queue[i].first = qi2->results.theresult;
							qi2->results.theresult = nullptr;
							qif = true;
						}
					}
					delete qi2;	
					bool qp2  = trim(queue[i+1].first,wsstrip);
					if (qp2) {
						if (qif) {
							DataItem::append(queue[i].first,queue[i+1].first);
						} else {
							queue[i].first = queue[i+1].first;
						}
					}
					queue[i].second = queue[i+1].second;
					queue.erase(queue.begin()+i+1);  
				}
			} else {
				i++;
			}
		}
		if ( queue.size() == 1 ) {
			theresult=queue.front().first;
			queue.front().first = nullptr;
			queue.clear(); 
			finalised=true; 
		}
	}
}
bool PairQueue::undefer(ObyxElement*) {
	bool retval = true;
	if ( ! finalised ) {
		size_t n = queue.size();
		for ( size_t i = 0; i < n; i++) {
			Function* qi2 = queue[i].second;
			if ( qi2 != pqendthing) {
				qi2->deferred = false;
				size_t i_n = qi2->inputs.size();
				for ( size_t j = 0; j < i_n; j++ ) {
					(qi2->inputs[j])->results.undefer(qi2);
				}
			}
		}
	}
	return retval;
}
void PairQueue::normalise(bool wsstrip) {
	if ( !finalised && queue.size() == 1) {
		theresult=queue.front().first;
		if (theresult != nullptr && wsstrip) {
			theresult->trim();
		}
		finalised=true; 
		queue.clear(); 
	}
}
void PairQueue::explain() {
	*Logger::log << Log::subhead  << Log::LI << "result" << Log::LO;	
	if ( finalised ) {
		string res_to_show;
		if (theresult != nullptr) {
			res_to_show = *theresult;
			*Logger::log << Log::LI << res_to_show << Log::LO; 
		} else {
			*Logger::log << Log::LI << "result is final and empty" << Log::LO; 
		}
	} else {
		size_t n = queue.size();
		if ( n > 1 ) {
			*Logger::log << Log::LI ;
			for ( size_t i = 0; i < n; i++) {
				if ( queue[i].first != nullptr && ! queue[i].first->empty() ) {
					*Logger::log << Log::II << std::string(*(queue[i].first)) << Log::IO;
				}
				Function* it = queue[i].second;
				if ( queue[i].second != pqendthing ) {
					*Logger::log << Log::II;
					it->explain();
					*Logger::log << Log::IO;
				}
			}
			*Logger::log << Log::LO; 
		} else {
			string res_to_show;
			if (theresult != nullptr) {
				res_to_show = *theresult;
				*Logger::log << Log::LI << Log::II << "Not finalised " << Log::IO << Log::II << res_to_show << Log::IO << Log::LO; 
			} else {
				*Logger::log << Log::LI << "The result queue and result is empty." << Log::LO; 
			}
		}
	}
	*Logger::log << Log::blockend;
}
