/* 
 * path.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * path.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_FILE_PATH_H
#define OBYX_FILE_PATH_H
//-------------------------------------------------------------------
// System includes
//-------------------------------------------------------------------    
#include <string>
#include <vector>
#include <stack>
//-------------------------------------------------------------------
// Local includes
//-------------------------------------------------------------------
#include "device.h"
                 
	namespace FileUtils {
		using std::vector;
		using std::stack;

		class File;

		class Path : public Device {
		protected:
			static std::stack<string> wdstack;
			vector<string *>   path;
			string	directory_separator;

		protected:
			void listFilesA(vector<File *> *list, const string regexp) const;
			off_t getSizeA() const;

		public:
			Path();
			Path(const Path &newpath);
			Path(const string newpath);
			Path(const string newdevice, const string newpath);
			virtual ~Path();

			void clear();
			Path &operator=(const Path &newpath);

			void setDirectorySeparator(const string separator);
			const string getDirectorySeparator() const;
			void setPath(const string newpath);
			void addPath(const string newpath);
			const string getPath() const;
			const string getPath(size_t index) const;
			const string getEndPath() const;
			const size_t getPathCount() const;
			void cd(const string newpath);
			virtual const string output(bool abs = false) const;
			bool exists() const;
			bool makeDir(bool recursive = false) const;
			bool removeDir(bool recursive = false, bool stop_on_error = false) const;
			bool moveTo(const Path &newpath) const ;
			off_t getSize(bool recursive = false) const;
			bool copyTo(const Path &newpath, bool overwrite = false, bool stop_on_error = false) const;
			bool mergeTo(const Path &newpath, bool overwrite = true, bool stop_on_error = false) const;

			void listDirs(vector<Path *> *list, bool recursive = false) const;
			void listFiles(vector<File *> *list, bool recursive = false, const string regexp = ".*") const;

			bool makeRelativeTo(const Path &newpath);
			bool makeAbsoluteFrom(const Path &newpath);

			bool makeRelative(vector<Path *> *list);
			bool makeAbsolute(vector<Path *> *list);
			bool makeTempDir(const string name = "TEMP");
			
			static std::string wd();
			static void cwd(std::string);
			static void push_wd(std::string);
			static void pop_wd();
		};

	}	// namespace FileUtils

#endif 


