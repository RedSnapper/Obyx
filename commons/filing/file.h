/* 
 * file.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * file.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_FILE_FILE_H
#define OBYX_FILE_FILE_H
//-------------------------------------------------------------------
// System includes
//-------------------------------------------------------------------
#include <sys/stat.h>
#include <string>

//-------------------------------------------------------------------
// Local includes
//-------------------------------------------------------------------
#include "path.h"


	namespace FileUtils
	{
		class File : public Path
		{
		protected:
			string base;
			char extension_separator;
			string extension;

		protected:
			void init();

		public:
			File();
			File(const File &newfile);
			File(const Path newpath);
			File(const Path newpath, const string newfilename);
			File(const string newfilename);
			File(const string newpath, const string newfilename);
			File(const string newdevice, const string newpath, const string newfilename);
			File(const string newdevice, const string newpath, const string newbase, const string newextension);
			virtual ~File();

			void clear();
			File &operator=(const File newfile);
			void setExtensionSeparator(const char separator);
			const char getExtensionSeparator() const;
			void setBase(const string newbase);
			const string getBase() const;
			void setExtension(const string newextension);
			const string getExtension() const;
			void setFileName(const string newfilename);
			const string getFileName() const;
			virtual const string output(bool abs = false) const;
			bool exists(mode_t = S_IFREG) const; //New bool is used for special files such as char_special, etc.
			bool moveTo(const File file) const;
			bool moveTo(const Path path) const;
			bool removeFile() const;
			off_t getSize() const;
			long getCreateDate() const;
			long getModDate() const;
			bool copyTo(const File, string&, bool) const;
			bool copyTo(const Path newpath, bool overwrite = false) const;
			void readFile(string&) const;
			void readFile(string&,size_t,mode_t) const;
			bool writeFile(const string contents) const;

		};

	}	// namespace FileUtils

#endif

