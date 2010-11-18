/* 
 * utils.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * utils.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/types.h> 
#include <time.h>

#include "commons/date/date.h"
#include "commons/string/strings.h"
#include "commons/logger/logger.h"

#include "utils.h"
#include "file.h"

namespace FileUtils {
	using namespace std;
	using DateUtils::Date;
	
		bool match(const string &text, const string &pattern) {
			bool retval = false;
			if ( String::Regex::available() ) {
				retval = String::Regex::match(pattern,text);
			} 
			return retval;
		}

		void display(vector<Path *> *list) {
			*Logger::log << Log::info;
			for(vector<Path *>::iterator it = list->begin(); it != list->end(); it++) {
				string bit = ((Path *)*it)->output();
				*Logger::log << Log::LI << bit << Log::LO;
			}
			*Logger::log << Log::blockend;
		}

		void display(vector<File *> *list) {
			*Logger::log << Log::info;
			for(vector<File *>::iterator it = list->begin(); it != list->end(); it++) {
				string bit = ((File *)*it)->output();
				*Logger::log << Log::LI << bit << Log::LO;
			}
			*Logger::log << Log::blockend;
		}

		bool makeDirs(vector<Path *> *list, bool stop_on_err)
		{
			for(vector<Path *>::iterator it = list->begin(); it != list->end(); it++) {
				if (!((Path *)*it)->makeDir() && stop_on_err)
					return false;
			}
			return true;
		}

		bool removeDirs(vector<Path *> *list, bool stop_on_err) {
			for(vector<Path *>::reverse_iterator it = list->rbegin(); it != list->rend(); it++) {
				if (!((Path *)*it)->removeDir() && stop_on_err)
					return false;
			}
			return true;
		}

		// Create a list of files from a list of directories
		bool listFiles(vector<Path>& dirlist, vector<File>& filelist) {
			for(vector<Path>::iterator it = dirlist.begin(); it != dirlist.end(); it++) {
				it->listFiles(filelist, ".*");
			}
			return true;
		}

}	// namespace FileUtils
