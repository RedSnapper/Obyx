/* 
 * pairqueue.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * pairqueue.h is a part of Obyx - see http://www.obyx.org
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

#ifndef OBYX_PairQueue_H
#define OBYX_PairQueue_H

#include <iostream>
#include <string>
#include <deque>

#include "dataitem.h"

using namespace std;

class Function;
class ObyxElement;

class PairQueue {
public:	
	typedef pair< DataItem* , Function* > pqpair;
	typedef deque< pqpair > pqueue;

private:
	pqueue queue;
	DataItem* theresult;
	bool finalised;

private:
	friend class ObyxElement;
	~PairQueue();
	bool trim(DataItem*&,bool);
	
protected:
	friend class DefInpType;
	bool undefer(ObyxElement* = NULL);

protected:
	friend class Function;
	friend class InputType;
	friend class Output;
	static Function* pqendthing;	// must be declared with return flow-function pqend()=true;

public:	
	static void startup();
	static void shutdown();
	PairQueue(bool = true);
	void copy(ObyxElement*,const PairQueue&);	 //copy the pairqueue into this.
	void normalise(bool = false);	//remove finalised instructions - used by evaluate..
	bool empty() { return (queue.size() - 1 > 0) ? false: true; } //should always be false
	pqueue::iterator end() { return queue.end(); }
	pqueue::iterator begin() { return queue.begin(); }
	pqueue::const_iterator end() const { return queue.end(); }
	pqueue::const_iterator begin() const { return queue.begin(); }
	pqpair& back() { return queue.back(); }
	pqpair& front() { return queue.front(); }
	size_t size() { return queue.size();  }
	void evaluate( bool = false);
	bool final() { return finalised; }
	
	void takeresult(DataItem*&); 
	const DataItem* result() const; 

	void clear(bool reset=true); //set up an endthing.
	void setresult(DataItem*&, bool = false); 
	void setresult(PairQueue&); 
	void clearresult();// { if (theresult != NULL) theresult->clear(); finalised=true; }	
	void append(PairQueue&,ObyxElement *par);	
	void append(pqpair&,ObyxElement *);
	void append(DataItem*&);
	void append(std::string,kind_type);
	void append(u_str,kind_type);
	void append(Function*,std::string&);
	void explain();
};

#endif

