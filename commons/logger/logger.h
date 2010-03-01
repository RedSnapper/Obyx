/* 
 * logger.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * logger.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef logger_h
#define logger_h

#include <string>
#include <stack>
#include <iostream>
#include <sstream>

using namespace std;

namespace Log {
	typedef enum {logger,headline,subhead,debug,info,notify,fatal,error,syntax,warn,timing,even,redirect,blockend} msgtype;	// Alternative list of msgtypes
	typedef enum {rule,urli,urlt,urlo,br} extratype;	// Alternative list of msgtypes
	typedef enum {LI,LO,RI,RO,II,IO,LIXP,LIFP,LIPP,LOXP,LOFP,LOPP,} bracketing;	// Brackets/Lines
}

class Logger {
private:
	bool		  top_line;		//first bracket contents of a block.
	bool		  inraw;
	Log::msgtype  curr_type;
	std::string	  path; 
	std::ostringstream syslogbuffer; 
	std::ostringstream storage;
	static std::ostringstream* lstore;
	
protected:
	std::stack<std::ostream*> estrm_stack; //error stream
	std::stack<Log::msgtype> type_stack;    //current log type was static Log::msgtype itype;
	bool storageused;
	bool debugflag;
	bool logging_on;
	bool isopened;
	bool hadfatal;
	bool evenodd;
	ostream*	fo;							  //final output stream
	ostream*	o;							  //output stream
	Logger(int i);
	virtual void wrap(bool) =0;					//wrapping log/warning/error etc.  
	virtual void extra(Log::extratype) =0;		//wrapping raw,urli,urlt,urlo,br,end etc.  
	virtual void open() =0;						//Open logger
	virtual void close() =0;				 //And close the logger 
	virtual void strip(string&) =0;				//strip container..
	virtual void bracket(Log::bracketing) =0;
	virtual void ltop(string&) =0;		//top log document
	virtual void ltail(string&) =0;		//tail log document
	virtual void dofatal() =0; //handle fatal.
	
public:
	int bdepth;									//bracketing depth
	static string title;
	static Logger* log;
	static ostream*& startup();
	static void shutdown();
	static void set_stream(ostream*&); 
	static void set_stream(ostringstream*&); 
	static void get_stream(ostream*&); 
	static size_t depth() { return log->estrm_stack.size(); } 

	static void unset_stream(); //  { log->unset_estream(); }

	static void top(string&);		//top log document
	static void tail(string&);		//tail log document
	
	static void stripcontainer(string& contents) { log->strip(contents); }
	static bool opened() { return log->isopened;}
	static bool wasfatal() { return log->hadfatal;}
	static bool debugging() {return log->debugflag;}
	static bool logging_available() {return log->logging_on;}
//	static void setdebug(bool val);
	Logger& operator<< (const bool);
	Logger& operator<< (const double);
	Logger& operator<< (const int);
	Logger& operator<< (const unsigned int);
	Logger& operator<< (const char*);
	Logger& operator<< (const string);
	Logger& operator<< (const Log::msgtype);
	Logger& operator<< (const Log::extratype);
	Logger& operator<< (const Log::bracketing);
	virtual ~Logger() {};						//Destructor.
};

#endif
