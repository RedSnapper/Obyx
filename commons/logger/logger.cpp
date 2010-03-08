/* 
 * logger.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * logger.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <ios>
#include <cstdlib>
#include <string>
#include <sstream>

#include "commons/httphead/httphead.h"
#include "commons/environment/environment.h"
#include "commons/string/strings.h"

#include "logger.h"
#include "httplogger.h"
#include "clilogger.h"

#include <syslog.h>

using namespace Log;

Logger*				Logger::log = NULL;
string				Logger::title="Obyx Log";
std::ostringstream* Logger::lstore = NULL;

//startup identifies if we are outputting as a cgi script or as a commandline utility.
//console is set to false if we are running in cgi mode, and true if we are in commandline mode.


//should be the only way to set log->o
void Logger::set_stream(std::ostringstream*& errstr) { 
	if (errstr != NULL ) { 
		log->storageused=true; //not always true.. if cout is set here...
		log->estrm_stack.push(errstr);
		log->o = log->estrm_stack.top();
		log->o->sync_with_stdio();
	}
}	

void Logger::set_stream(std::ostream*& errstr) {
	if (errstr != NULL ) { 
		log->storageused=true; //not always true.. if cout is set here...
		log->estrm_stack.push(errstr);
		log->o = log->estrm_stack.top();
		log->o->sync_with_stdio();
	}
}	

void Logger::get_stream(ostream*& container) {
	container = log->estrm_stack.top();
}


//should always be paired with a set_stream..
void Logger::unset_stream() {
	if (log->storageused) {
		if (!log->estrm_stack.empty()) {
			log->estrm_stack.pop();
		} else {
			log->storageused=false;
			log->o = log->fo;
	//		*log << error << LI << "Internal: unset_stream nesting failure." << LO << blockend;  
		}
		if (log->estrm_stack.empty() ) { 
			log->storageused=false;
			log->o = log->fo;
		} else {
			log->o = log->estrm_stack.top();
		}
		log->o->sync_with_stdio();
	}
}	

Logger::Logger(int i) : syslogging(true),bdepth(i) {
	fo = NULL;		//final output stream
	o = NULL;		//current output stream
	debugflag=false;
	storageused=false;
	isopened=false;
	hadfatal=false;
	top_line=false;		//first bracket contents of a block.
	inraw = false;
	evenodd = true;
	curr_type = info;
	type_stack.push(logger);    //current log type was static Log::msgtype itype;
}

ostream*& Logger::startup() {
	string tmp_env;
	setlocale(LC_CTYPE,"UTF-8");
	Environment::buildarea_type area = Environment::get_area();
	if ( (area != Environment::Console) ) {
		log = new HTTPLogger();	// create new http reporter
	} else {
		log = new CLILogger();	// create new command line reporter
	}
	/*
	 OBYX_DEVELOPMENT LOG_DEBUG OBYX_LOGGING_OFF OBYX_SYSLOG_OFF RESULT
	 on				on			off				on			syslog(debug) (syslog opened)
	 on				off			off				on			syslog(warn)  (syslog opened)
	 on				on			on				on			syslog(debug) (syslog opened)
	 on				off			on				on			[no syslog]   (syslog not opened)
	 off			on			on				on			[no syslog]   (syslog not opened)		
	 off			off			on				on			[no syslog]   (syslog not opened)		
	 off			on			off				on			syslog(warn)  (syslog opened)		
	 off			off			off				on			syslog(warn)  (syslog opened)	
	 on				on			off				off			[no syslog]   (syslog not opened)
	 on				off			off				off			[no syslog]   (syslog not opened)
	 on				on			on				off			[no syslog]   (syslog not opened)
	 on				off			on				off			[no syslog]   (syslog not opened)
	 off			on			on				off			[no syslog]   (syslog not opened)		
	 off			off			on				off			[no syslog]   (syslog not opened)		
	 off			on			off				off			[no syslog]   (syslog not opened)		
	 off			off			off				off			[no syslog]   (syslog not opened)	
	 */
	log->syslogging = ! Environment::getenv("OBYX_SYSLOG_OFF",tmp_env);
	log->logging_on =   Environment::getenv("OBYX_DEVELOPMENT",tmp_env);
	if (log->syslogging) {
		if (log->logging_on) {
			log->debugflag = Environment::getenv("LOG_DEBUG", tmp_env);
			if (log->debugflag) {
				setlogmask(LOG_UPTO (LOG_DEBUG ));
				openlog("Obyx",0,LOG_USER);
			} else {
				if (!Environment::getenv("OBYX_LOGGING_OFF",tmp_env)) {
					setlogmask(LOG_UPTO (LOG_WARNING));
					openlog("Obyx",0,LOG_USER);
				}
			}
		} else {
			if (! Environment::getenv("OBYX_LOGGING_OFF",tmp_env)) {
				setlogmask(LOG_UPTO (LOG_WARNING));
				openlog("Obyx",0,LOG_USER);
			}
		}
	}
	log->fo = &std::cout;					//set final output.
	std::ios_base::sync_with_stdio(true);
	lstore = &(log->storage);
	set_stream(lstore);				//set up storage.
	log->open();
	Environment::initwlogger(); //Continue environment load.
	Environment::getenv("PATH_TRANSLATED",log->path);
	if (area == Environment::Web) {
		*log << error << LI << "Obyx requires that you set up a public root directory to work." << LO << blockend;  
	}
	return log->fo;
}

void Logger::top(string& container) {
	log->ltop(container);
}

void Logger::tail(string& container) {
	log->ltail(container);
}

void Logger::shutdown() {
	log->close();
	if (log->logging_on) {
		closelog();	 //system log!
	}
	
	if (! log->hadfatal) {
		unset_stream();			//remove.
	}
	delete log;					// tidying up
}

Logger& Logger::operator<< (const double val ) { 
	if (log->top_line && log->syslogging ) { log->syslogbuffer << val;}
	if ( logging_on && ( type_stack.top() != debug || debugging()) )  {
		*o << val; 
	}
	return *this;
}

Logger& Logger::operator<< (const bool val ) { 
	if (log->top_line && log->syslogging ) { log->syslogbuffer << val;}
	if ( logging_on && ( type_stack.top() != debug || debugging()) )  {
		if (val) {
			*o << "true"; 
		} else {
			*o << "false"; 
		}
	}
	return *this;
}

Logger& Logger::operator<< (const int val ) { 
	if (log->top_line && log->syslogging ) { log->syslogbuffer << val;}
	if ( logging_on && ( type_stack.top() != debug || debugging()) )  {
		*o << static_cast<int>(val); 
	}
	return *this;
}	

Logger& Logger::operator<< (const unsigned int val ) { 
	if (log->top_line && log->syslogging ) { log->syslogbuffer << val;}
	if ( logging_on && ( type_stack.top() != debug || debugging()) )  {
		*o << static_cast<unsigned int>(val); 
	}
	return *this;
}	
// estrm_stack.empty()
Logger& Logger::operator << (const msgtype mtype) {
	if ( !hadfatal && ((mtype == fatal || mtype == warn || mtype == syntax) || (mtype == error && estrm_stack.size() == 1)) ) {
		hadfatal = true;
		dofatal();
		string messages = lstore->str();
		*(log->fo) << messages;
		unset_stream();
	}
	if (mtype == blockend) {
		if ( logging_on && ( type_stack.top() != debug || debugging()) )  {  wrap(false); }
		type_stack.pop();
		top_line = false;
	} else {
		top_line = true;
		if (log->syslogging) {
			syslogbuffer.str("");
		}
		type_stack.push(mtype); // starting
		if ( logging_on && ( type_stack.top() != debug || debugging()) )  {  wrap(true); }
	}
	return *this;
}

Logger& Logger::operator << (const extratype extrabit) {
	if ( logging_on && ( type_stack.top() != debug || debugging()) )  {
		extra(extrabit); 
	}
	return *this;
}	

Logger& Logger::operator<< (const char* msg) { 
	string mesg(msg);
	if (log->top_line && log->syslogging) { log->syslogbuffer << mesg;}
	XMLChar::encode(mesg);
	if ( logging_on && (type_stack.top() != debug || debugging()) )  {
		*o << mesg;  
	}
	return *this;
}

Logger& Logger::operator<< (const std::string msg ) {  
	if (log->top_line && log->syslogging) { log->syslogbuffer << msg;}
	string mesg(msg);
	if ( logging_on && (type_stack.top() != debug || debugging()) )  {
//		if ( type_stack.top() != raw ) {
			if (!inraw) XMLChar::encode(mesg);
//		} 
		*o << mesg;  
	}
	return *this;
}

Logger& Logger::operator<< (const bracketing bkt) { 
	if (bkt == RI || bkt == RO) { 
		inraw = bkt == RI ? true : false;
	} else {
		if (bkt == LO && log->top_line && !log->syslogbuffer.str().empty() && estrm_stack.size() < 3 ) {
			if (log->syslogging) {
				switch ( type_stack.top() ) {
					case debug : { 
						syslog(LOG_DEBUG,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					case warn : { 
						syslog(LOG_WARNING,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					case fatal : { 
						syslog(LOG_CRIT,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					case syntax : { 
						syslog(LOG_ERR,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					case error : { 
						syslog(LOG_ERR,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					case timing : 
					case even : 
					case info : 
					case headline : 
					case notify:
					case subhead : { 
						syslog(LOG_NOTICE,"[%s]: %s (%s)",title.c_str(),path.c_str(),log->syslogbuffer.str().c_str());
					} break;
					default : break;
				}
			}
			top_line=false;
		}
		if ( logging_on ) {
			if ( debugging() ) {
				bracket(bkt);  
			} else {
				const msgtype tst = type_stack.top();
				if ( tst != debug  ) {
					bracket(bkt);  
				} 
			}
		}
		if ( bdepth < 0 ) bdepth = 0;
	}
	return *this;
}					 
