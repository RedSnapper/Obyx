/* 
 * logger.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * logger.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <ios>
#include <cstdlib>
#include <string>
#include <sstream>

#include "commons/xml/xml.h"
#include "commons/httphead/httphead.h"
#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include "core/obyxelement.h"

#include "logger.h"
#include "httplogger.h"
#include "clilogger.h"

#include <syslog.h>

using namespace Log;

Logger*					Logger::log = nullptr;
string					Logger::title="Logger";
bool 						Logger::debugflag = false;
//size_t				Logger::logdepth = 0;

//startup identifies if we are outputting as a cgi script or as a commandline utility.
//console is set to false if we are running in cgi mode, and true if we are in commandline mode.
//should be the only way to set log->o
void Logger::set_stream(std::ostringstream*& errstr) { 
	if (log != nullptr && errstr != nullptr ) { 
		log->storageused=true; //not always true.. if cout is set here...
		log->estrm_stack.push(errstr);
		log->o = log->estrm_stack.top();
	}
}	

void Logger::set_stream(std::ostream*& errstr) {
	if (log != nullptr && errstr != nullptr ) { 
		log->storageused=true; //not always true.. if cout is set here...
		log->estrm_stack.push(errstr);
		log->o = log->estrm_stack.top();
	}
}	

void Logger::get_stream(ostream*& container) {
	container = log->estrm_stack.top();
}

//should always be paired with a set_stream..
void Logger::unset_stream() {
	if (log != nullptr) {
		if (!log->estrm_stack.empty()) {
//			cerr << "L unset_stream. Was: " << log->estrm_stack.top();
			log->estrm_stack.pop();
		} else {
//			cerr << "L unset_stream failed. estrm_stack was empty.";
			log->storageused=false;
			log->o = log->fo;
		}
		if (log->estrm_stack.empty() ) { 
			log->storageused=false;
			log->o = log->fo;
		} else {
			log->o = log->estrm_stack.top();
		}
//		cerr << " Now: " << log->o << endl;
	}
}	

Logger::Logger(int i) : syslogging(true),bdepth(i) {
	fo = nullptr;		//final output stream
	o = nullptr;		//current output stream
	logging_on=true;
	storageused=false;
	isopened=false;
	hadfatal=false;
	infatal=false;
	top_line=false;		//first bracket contents of a block.
	inraw = false;
	evenodd = true;
	curr_type = info;
	msgdepth = 0;
	fataldepth = 0;
	lstore = &storage;
	type_stack.push(logger);    //current log type was static Log::msgtype itype;
}

Logger::~Logger() {
	while (!estrm_stack.empty()) {
		estrm_stack.pop();
	}
	while (!type_stack.empty()) {
		type_stack.pop();
	}
	fo = nullptr;		//final output stream
	o = nullptr;		//current output stream
}

void Logger::startup(string& t) {
	string tmp;
	debugflag = Environment::getbenvtf("OBYX_DEBUG");
	setlocale(LC_CTYPE,"UTF-8");
	title = t;
	log = new HTTPLogger();	// create new http reporter for startup.
	log->logging_on = true;
	log->fo = &cout;	//set final output.
	set_stream(log->lstore);	//set up storage.
	log->open();
}
void Logger::shutdown() {
	log = nullptr;
}

ostream* Logger::init(ostream*& final_out) {
	Environment* env = Environment::service();
	string tmp_env;
	if (log == nullptr) { //startup creates a temporary. fastcgi will delete it on finalise
		log = new HTTPLogger();	// create new http reporter
		set_stream(log->lstore);	//set up storage.
		log->open();
	}
	/*
	 OBYX_DEVELOPMENT OBYX_DEBUG OBYX_LOGGING_OFF OBYX_SYSLOG_OFF RESULT
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
	if (log != nullptr) { //startup creates a temporary. fastcgi will delete it on finalise
		log->syslogging = ! env->getenvtf("OBYX_SYSLOG_OFF");
		log->logging_on =   env->getenvtf("OBYX_DEVELOPMENT");
		if (log->logging_on) {
			if (debugflag) {
				if (log->syslogging) {
					setlogmask(LOG_UPTO (LOG_DEBUG ));
				}
				openlog("Obyx",0,LOG_USER);
			} else {
				if (!env->getenvtf("OBYX_LOGGING_OFF")) {
					if (log->syslogging) {
						setlogmask(LOG_UPTO (LOG_WARNING));
					}
					openlog("Obyx",0,LOG_USER);
				}
			}
		} else {
			if (!env->getenvtf("OBYX_LOGGING_OFF")) {
				if (log->syslogging) {
					setlogmask(LOG_UPTO (LOG_WARNING));
				}
				openlog("Obyx",0,LOG_USER);
			}
		}
		if (final_out != nullptr) {
			log->fo = final_out;	//set final output.
		} else {
			log->fo = &cout;	    //set final output.
		}
		return log->fo;
	} else {
		return nullptr;
	}
}

void Logger::finalise() {
	log->close();
	if (log->logging_on) {
		closelog();	 //system log!
	}
	if (! log->hadfatal) {
		unset_stream();			//remove.
	}
	log->fo = nullptr;				//set final output.
	delete log;					// tidying up
	log = nullptr;
}

void Logger::top(string& container,bool do_gubbins,bool do_tiny) {
	log->ltop(container,do_gubbins,do_tiny);
}

void Logger::tail(string& container,bool do_tiny) {
	log->ltail(container,do_tiny);
}

bool Logger::should_report() { //always report to stacked errors. 
	size_t ssize = estrm_stack.size();
	return (logging_on || ssize > 1);
}

Logger& Logger::operator<< (const double val ) { 
    if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << val;}
	if ( should_report() || debugging() )  {
		*o << val; 
	}
	return *this;
}

Logger& Logger::operator<< (const bool val ) { 
    if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << val;}
	if ( should_report() || debugging() )  {
		if (val) {
			*o << "true"; 
		} else {
			*o << "false"; 
		}
	}
	return *this;
}
Logger& Logger::operator<< (const unsigned long int val) { 
    if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << val;}
	if ( should_report() || debugging() )  {
		*o << static_cast<int>(val); 
	}
	return *this;
}	

Logger& Logger::operator<< (const int val ) { 
    if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << val;}
	if ( should_report() || debugging() )  {
		*o << static_cast<int>(val); 
	}
	return *this;
}	

Logger& Logger::operator<< (const unsigned int val ) { 
    if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << val;}
	if ( should_report() || debugging() )  {
		*o << static_cast<unsigned int>(val); 
	}
	return *this;
}	

Logger& Logger::operator << (const msgtype mtype) {
	curr_type = mtype;
	size_t ssize = estrm_stack.size();
	if ( !infatal && (mtype == fatal || mtype == Log::error || mtype == syntax || mtype == thrown) && (ssize < 2) ) {
//		*o << "depth:" << ssize;  
		infatal = true;
		
		fataldepth = msgdepth;
	}
	if (mtype == blockend) {
		msgdepth--;
		if (infatal && !hadfatal && fataldepth == msgdepth) {
			hadfatal = true;
			dofatal(syslogbuffer.str());
			while (!estrm_stack.empty()) {
				ostream* msgs = nullptr;
				get_stream(msgs);
				ostringstream* txt = dynamic_cast<ostringstream*>(msgs);
				if (txt!=nullptr ) {
					string msgtext = txt->str();
					if (log != nullptr) {
						if (log->fo != nullptr) {
							*(log->fo) << msgtext;
						}
					} else {
						cout << txt->str();
					}
				}
				unset_stream();
			}
		}
		if ( should_report() || debugging() )  {  wrap(false); }
		type_stack.pop();
		top_line = false;
	} else {
		msgdepth++;
		top_line = true;
		if (log->syslogging) {
			syslogbuffer.str("");
		}
		type_stack.push(mtype); // starting
		if ( should_report() || debugging() )  {  wrap(true); }
	}
	return *this;
}

Logger& Logger::operator << (const extratype extrabit) {
	if ( should_report() || debugging() )  {
		extra(extrabit); 
	}
	return *this;
}	

Logger& Logger::operator<< (const char* msg) { 
	if (msg != nullptr) {
		string mesg = msg;
		if (! String::normalise(mesg)) {
			mesg = msg;
			String::base64encode(mesg);
			*o << " (base64 encoded for this message) ";
		}
		if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << mesg;}
		if ( should_report() || debugging() )  {
			XMLChar::encode(mesg);
			*o << mesg;			
		}
	}
	return *this;
}

Logger& Logger::operator<< (const std::string msg ) {  
	if (msgdepth-1 == fataldepth && infatal && log->top_line) { log->syslogbuffer << msg;}
	if ( should_report() || debugging() )  {
		string mesg(msg);
		if (! String::normalise(mesg)) {
			mesg = msg;
			String::base64encode(mesg);
			*o << " (base64 encoded for this message) ";
		}
		if (!inraw) { 
			XMLChar::encode(mesg);
		}
		*o << mesg;
	}
	return *this;
}

std::string Logger::errline() {
	ostringstream err;
	err << title.c_str() << "; " << path.c_str() << "; " << ObyxElement::breakpoint_str() << "; " << log->syslogbuffer.str().c_str();
	return err.str();
}

Logger& Logger::operator<< (const bracketing bkt) { 
	if (bkt == RI || bkt == RO) { 
		inraw = bkt == RI ? true : false;
	} else {
        size_t ssize = estrm_stack.size();
		if (bkt == LO && log->top_line && !log->syslogbuffer.str().empty() && ssize < 2 ) {
			if (log->syslogging) {
				unsigned int bp = (unsigned int)ObyxElement::breakpoint();
				Environment* env = Environment::service();
				string site("");
				if (env->benvexists("HTTP_HOST")) {
					env->getbenv("HTTP_HOST",site);
					site = "(" + site + ") ";
				}
				switch ( type_stack.top() ) {
					case thrown:
					case warn : { 
						syslog(LOG_WARNING,"[%s]%s: %s; %u (%s)",title.c_str(),site.c_str(),path.c_str(),bp,log->syslogbuffer.str().c_str());
					} break;
					case fatal : { 
						syslog(LOG_CRIT,"[%s]%s: %s; %u (%s)",title.c_str(),site.c_str(),path.c_str(),bp,log->syslogbuffer.str().c_str());
					} break;
					case Log::syntax : { 
						syslog(LOG_ERR,"[%s]%s: %s; %u (%s)",title.c_str(),site.c_str(),path.c_str(),bp,log->syslogbuffer.str().c_str());
					} break;
					case Log::error : { 
						syslog(LOG_ERR,"[%s]%s: %s; %u (%s)",title.c_str(),site.c_str(),path.c_str(),bp,log->syslogbuffer.str().c_str());
					} break;
					case timing : 
					case even : 
					case info : 
					case headline : 
					case notify:
					case subhead : { 
						syslog(LOG_NOTICE,"[%s]: %s ;%u (%s)",title.c_str(),path.c_str(),bp,log->syslogbuffer.str().c_str());
					} break;
					default : break;
				}
			}
			top_line=false;
		}
		if ( should_report() || debugging() )  {
			bracket(bkt);  
		}
		if ( bdepth < 0 ) bdepth = 0;
	}
	return *this;
}					 
