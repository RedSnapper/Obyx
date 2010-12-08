/* 
 * date.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * date.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2010 Red Snapper Ltd. http://www.redsnapper.net
 * The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt
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

#ifndef OBYX_DATEH
#define OBYX_DATEH

#include <ctime>
#include <string>
#include <sys/time.h>

//This is a very ancient system - limited to unixtime.
//But it sort of wraps off posix.

namespace DateUtils {
	using namespace std;
	class Date {
	protected:
		time_t tt;
		struct tm* local;
		struct tm* utc;
		
	public:
		Date();
		Date(time_t ttime);
		~Date();
		void getUTCDateStr(const string&, string&);
		void getLocalDateStr(const string&, string &);
		void getDateStr(struct tm*, const string&,string&);
		void getDateStr(struct tm*, string &cont);
		const time_t getUTC() const;
		void getNowDateStr(const string &format, string &cont);
		void getNow(string& cont);
		static void getTS(string&);
		static void getUTCTimeOfDay(string&);
		static void getCookieDateStr(string &cont);
		static void getCookieExpiredDateStr(string &cont);
	};
	
}       // namespace DateUtils

#endif
