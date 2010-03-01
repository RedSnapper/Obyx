/* 
 * date.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * date.cpp is a part of Obyx - see http://www.obyx.org .
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

//-------------------------------------------------------------------
// System includes
//-------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
//-------------------------------------------------------------------
// Local includes
//-------------------------------------------------------------------
#include "date.h"

	namespace DateUtils {
		//-------------------------------------------------------------------------
		// Constructor inits Date to now
		//-------------------------------------------------------------------------
		Date::Date() : tt(0),local(NULL),utc(NULL) {
			tt = time(&tt);
			local = localtime(&tt);
			utc = gmtime(&tt);
		}
		
		//-------------------------------------------------------------------------
		// Constructor inits Date to specified UTC in seconds
		//-------------------------------------------------------------------------
		Date::Date(time_t ttime) : tt(ttime), local(NULL),utc(NULL) {
			utc = gmtime(&tt);
			local = localtime(&tt);
		}
		
		Date::~Date() {
//			if (utc != NULL) free(utc);	 //"not stack'd or malloc'd"
//			if (local != NULL) free(local);  //??
		}
		
		//static utility
		void Date::getUTCTimeOfDay(string& result) {
			size_t buffsz = 256;
			char* buff = (char *)malloc(buffsz+1);
			time_t the_time;
			struct tm utc_time;
			the_time = time(NULL);
			gmtime_r(&the_time,&utc_time);
			strftime(buff, buffsz, "%Y-%m-%d %H:%M:%S",&utc_time);
			result = buff;
			free(buff); // clear it out now  
		}
		
		//-------------------------------------------------------------------------
		// Fetch the formatted date
		//-------------------------------------------------------------------------
		void Date::getDateStr(struct tm* ttime, const string& format, string& cont)  {
			size_t buffsz = 32 * format.length() + 80;
			char* buff = (char *)malloc(buffsz+2);
			if (format.empty())
				strftime(buff, buffsz, "%Y-%m-%d %H:%M:%S", ttime);
			else
				strftime(buff, buffsz, format.c_str(), ttime);
			cont.append(buff);
			free(buff); // clear it out now  
		}
		
		//-------------------------------------------------------------------------
		// Fetch the formatted date
		//-------------------------------------------------------------------------
		void Date::getDateStr(struct tm* ttime, string &cont)  {
			size_t buffsz = 256;
			char* buff = (char *)malloc (buffsz+2);
			strftime(buff, buffsz, "%Y-%m-%d %H:%M:%S", ttime);
			cont.append(buff);
			free(buff); // clear it out now  
		}
		//-------------------------------------------------------------------------
		// Fetch the formatted UTC date
		//-------------------------------------------------------------------------
		void Date::getUTCDateStr(const string &format, string &cont) {
			getDateStr(utc, format, cont);
		}
		
		//-------------------------------------------------------------------------
		// Fetch the formatted Local date
		//-------------------------------------------------------------------------
		void Date::getLocalDateStr(const string &format, string &cont) {
			getDateStr(local, format, cont);
		}
		
		//-------------------------------------------------------------------------
		// Fetch some time in the past for cookies..
		//-------------------------------------------------------------------------
		void Date::getCookieExpiredDateStr(string &cont) {
			size_t buffsz = 256;
			time_t 	thetime = time(&thetime);
			struct tm* utctime = gmtime(&thetime);
			utctime->tm_year--;				//one year ago.
			thetime = timegm(utctime);		//normalise.
			char* buff = (char *)malloc(buffsz+1);
			strftime(buff, buffsz, "%a, %d-%b-%Y %T GMT",utctime);
			cont.append(buff);
			free(buff); // clear it out now  
		}

		//-------------------------------------------------------------------------
		// Fetch one year in the future for cookies..
		//-------------------------------------------------------------------------
		void Date::getCookieDateStr(string &cont) {
			size_t buffsz = 256;
			time_t 	thetime = time(&thetime);
			struct tm* utctime = gmtime(&thetime);
			utctime->tm_year++;				//one year away.
			thetime = timegm(utctime);		//normalise.
			char* buff = (char *)malloc(buffsz+1);
			strftime(buff, buffsz, "%a, %d-%b-%Y %T GMT", utctime);
			cont.append(buff);
			free(buff); // clear it out now  
		}
		
		//-------------------------------------------------------------------------
		// get UTC in seconds
		//-------------------------------------------------------------------------
		const time_t Date::getUTC() const {
			return tt;
		}
		void Date::getNow(string& cont) {
			tt = time(&tt);
			local = localtime(&tt);
			getDateStr(local,cont);
		} 
		void Date::getNowDateStr(const string &format, string &cont) {
			tt = time(&tt);
			local = localtime(&tt);
			getDateStr(local, format, cont);
		}
	}       // namespace DateUtils
