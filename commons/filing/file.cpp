/* 
 * file.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * file.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "file.h"
#include <errno.h>

	namespace FileUtils
	{
		//-------------------------------------------------------------------------
		// Basic Constructor
		//-------------------------------------------------------------------------
		File::File() : Path()
		{
			init();
		}


		//-------------------------------------------------------------------------
		// Constructs a new File given a specified File
		//-------------------------------------------------------------------------
		File::File(const File &newfile) : Path(newfile)
		{
			init();
			base = newfile.base;
			extension_separator = newfile.extension_separator;
			extension = newfile.extension;
		}


		//-------------------------------------------------------------------------
		//  Constructs a new File given a Path
		//-------------------------------------------------------------------------
		File::File(const Path newpath) : Path(newpath)
		{
			init();
		}


		//-------------------------------------------------------------------------
		// Constructs a new File given a Path and a filename
		//-------------------------------------------------------------------------
		File::File(const Path newpath, const string newfilename) : Path(newpath)
		{
			init();
			setFileName(newfilename);
		}


		//-------------------------------------------------------------------------
		// Constructs a new File given a filename
		//-------------------------------------------------------------------------
		File::File(const string newfilename)
		{
			init();
			setFileName(newfilename);
		}


		//-------------------------------------------------------------------------
		// Constructs a new File given a path and filename
		//-------------------------------------------------------------------------
		File::File(const string newpath, const string newfilename) : Path(newpath)
		{
			init();
			setFileName(newfilename);
		}


		//-------------------------------------------------------------------------
		// Constructs a new File given a device, path, filename
		//-------------------------------------------------------------------------
		File::File(const string newdevice, const string newpath, const string newfilename) : Path(newdevice, newpath)
		{
			init();
			setFileName(newfilename);
		}


		//-------------------------------------------------------------------------
		// Construct a new File given a device, path, base, extension
		//-------------------------------------------------------------------------
		File::File(const string newdevice, const string newpath, const string newbase, const string newextension) : Path(newdevice, newpath)
		{
			init();
			setBase(newbase);
			setExtension(newextension);
		}


		//-------------------------------------------------------------------------
		// Destructor
		//-------------------------------------------------------------------------
		File::~File()
		{
			clear();
		}


		//-------------------------------------------------------------------------
		// Initialise the OS default 
		//-------------------------------------------------------------------------
		void File::init()
		{
			setExtensionSeparator('.');
			ifs = nullptr;
		}


		//-------------------------------------------------------------------------
		// Clears the File
		//-------------------------------------------------------------------------
		void File::clear() {
			Path::clear();
			base = "";
			extension = "";
			if (ifs != nullptr) {
				delete ifs; 
				ifs=nullptr;
			}
		}


		//-------------------------------------------------------------------------
		// Copies the contents from the specified File
		//-------------------------------------------------------------------------
		File &File::operator=(const File newfile)
		{
			Path::operator=(newfile);
			base = newfile.base;
			extension_separator = newfile.extension_separator;
			extension = newfile.extension;
			return *this;
		}


		//-------------------------------------------------------------------------
		// Sets the extension separator
		//-------------------------------------------------------------------------
		void File::setExtensionSeparator(const char separator)
		{
			extension_separator = separator;
		}


		//-------------------------------------------------------------------------
		// Gets the extension separator
		//-------------------------------------------------------------------------
		const char File::getExtensionSeparator() const
		{
			return extension_separator;
		}


		//-------------------------------------------------------------------------
		// Sets the filename Base
		//-------------------------------------------------------------------------
		void File::setBase(const string newbase)
		{
			base = newbase;
		}


		//-------------------------------------------------------------------------
		// Gets the filename Base
		//-------------------------------------------------------------------------
		const string File::getBase() const
		{
			return base;
		}


		//-------------------------------------------------------------------------
		// Gets the filename Extension
		//-------------------------------------------------------------------------
		void File::setExtension(const string newextension)
		{
			extension = newextension;
		}


		//-------------------------------------------------------------------------
		// Sets the filename Extension
		//-------------------------------------------------------------------------
		const string File::getExtension() const {
			return extension;
		}


		//-------------------------------------------------------------------------
		// Sets the filename
		//-------------------------------------------------------------------------
		void File::setFileName(const string newfilename) {
			string thefile(newfilename);
			if ( thefile.compare(0,directory_separator.length(),directory_separator) == 0 ) {
				size_t slash = thefile.rfind(directory_separator);
				setPath(thefile.substr(0, slash));
				thefile = thefile.substr(slash + directory_separator.length(), string::npos);
			}
			size_t dot = thefile.rfind(getExtensionSeparator());
			if (dot != string::npos) {
				// if the filename ends with a extension separator
				if (dot == thefile.length() - 1) {
					base = thefile.substr(0, string::npos);
					extension = "";
				} else {
					base = thefile.substr(0, dot);
					extension = thefile.substr(dot + 1, string::npos);
				}
			}
			// No extension separator
			else {
				base = thefile;
				extension = "";
			}
		}


		//-------------------------------------------------------------------------
		// Gets the formatted FileName
		//-------------------------------------------------------------------------
		const string File::getFileName() const
		{
			string result;
			// No extesnion, return just Base
			if (extension.empty())
				result = getBase();
			else
				result = getBase() + getExtensionSeparator() + getExtension();
			return result;
		}


		//-------------------------------------------------------------------------
		// Returns a formatted FileName
		// abs = true, forces an absolute FileName
		//-------------------------------------------------------------------------
		const string File::output(bool abs) const {
			return Path::output(abs) + '/' + getFileName();
		}


		//-------------------------------------------------------------------------
		// Checks whether the file exists
		//-------------------------------------------------------------------------
		bool File::exists(mode_t filter) const {
			bool existence = false;
			struct stat buf;
			string fpath = output(false);
			int result = stat(fpath.c_str(), &buf);
			if (result == 0 && (buf.st_mode & filter)) {
				existence = true;
			}
			return existence;
		}


		//-------------------------------------------------------------------------
		// Moves the current File to the specified File
		//-------------------------------------------------------------------------	
		bool File::moveTo(const File file) const
		{
			if (file.exists())
				return false;
			File newfile = file;
			// Empty filename, sets as current filename
			if (newfile.getFileName().empty())
				newfile.setFileName(getFileName());
			int result = rename(output().c_str(), newfile.output().c_str());
			if (result == 0)
				return true;
			return false;
		}


		//-------------------------------------------------------------------------
		// Moves the current File to the specified Path
		//-------------------------------------------------------------------------
		bool File::moveTo(const Path newpath) const
		{
			if (!newpath.exists())
				return false;
			File file1(newpath, getFileName());
			if (!file1.exists())
			{
				int result = rename(output().c_str(), file1.output().c_str());
				if (result == 0)
					return true;
			}
			return false;
		}


		//-------------------------------------------------------------------------
		// Removes the current File
		//-------------------------------------------------------------------------
		bool File::removeFile() const
		{
			if (unlink(output().c_str()) == 0)
				return true;
			return false;
		}

		//-------------------------------------------------------------------------
		// Gets the size of the current File
		//-------------------------------------------------------------------------
		off_t File::getSize() const {
			off_t result;
			struct stat buf;
			result = stat(output().c_str(), &buf);
			if (result == 0 && buf.st_mode & S_IFREG)
				return buf.st_size; //64 bit to 32 bit conversion...
			return 0;
		}

		//-------------------------------------------------------------------------
		// Gets the creation date of the current File
		//-------------------------------------------------------------------------
		long File::getCreateDate() const
		{
			long result;
			struct stat buf;
			result = stat(output().c_str(), &buf);
			if ((result == 0) && (buf.st_mode & S_IFREG))
				return buf.st_ctime;
			return 0;
		}

		//-------------------------------------------------------------------------
		// Gets the last modification date of the current File
		//-------------------------------------------------------------------------
		long File::getModDate() const
		{
			long result;
			struct stat buf;
			result = stat(output().c_str(), &buf);
			if (result == 0 && buf.st_mode & S_IFREG)
				return buf.st_mtime;
			return 0;
		}


		//-------------------------------------------------------------------------
		// Copies the current File to the specified File
		// overwrite = true, overwrites the dest
		//-------------------------------------------------------------------------
		bool File::copyTo(const File newfile, string& errs, bool overwrite) const {
			bool retval = false;
			string from_path = output();
			string dest_path = newfile.output();
			if ((newfile.exists() && overwrite) || !newfile.exists()) {
				FILE *in = fopen(from_path.c_str(), "rb");
				if (in != 0) {
					struct stat in_stat;
					int err = stat(from_path.c_str(),&in_stat);
					if (err != 0) { string ers = strerror(errno); errs.append(ers); errs.append("; "); retval = false; }
					FILE *out = fopen(dest_path.c_str(), "w+b");
					if (out == 0) {
						fclose(in);
					} else {
						retval = true;
						struct timeval times[2] = {{0,0},{0,0}};	/* initialise microseconds to zero */
						times[0].tv_sec = in_stat.st_atime;			/* time of last access */
						times[1].tv_sec = in_stat.st_mtime;			/* time of last modification */
						err = chmod(dest_path.c_str(),S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
						if (err != 0) { string ers = strerror(errno); errs.append(ers); errs.append("; "); retval = false; }
						char buf[8192];
						size_t i;
						while((i = fread(buf, 1, 8192, in)) != 0) {
							fwrite(buf, 1, i, out);
						}
						fclose(in);
						fclose(out);
						err = utimes(dest_path.c_str(),times);
						if (err != 0) { string ers = strerror(errno); errs.append(ers); errs.append("; "); retval = false; }
					}
				}
			}
			return retval;
		}
		
		//-------------------------------------------------------------------------
		// Copy the current File to the specified Path
		// overwrite = true, overwrites the dest
		//-------------------------------------------------------------------------
		bool File::copyTo(const Path newpath, bool overwrite) const
		{
			File file1(newpath, getFileName());
			return copyTo(file1, overwrite);
		}

		//-------------------------------------------------------------------------
		// Copies a string into a file
		//-------------------------------------------------------------------------
	
		bool File::writeFile(const string contents) const {
			if (exists()) {
				removeFile();
			}
			bool result = false;
			size_t size = contents.length();
			if (size > 0) {
				FILE *out = fopen(output().c_str(), "wb");
				if (out) {
					size_t written = fwrite(contents.c_str(),1,size,out);
					fclose(out);
					chmod(contents.c_str(),S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
					if ( written == size ) {
						result=true;
					}
				} 
			} 
			return result;
		}
		
		//-------------------------------------------------------------------------
		// Copies a file into a string
		void File::open(std::istream*& is) {
			if (exists()) {
				ifs = new std::ifstream(output().c_str(), std::ios::in | std::ios::binary | std::ios::ate);
				if (ifs->is_open() && ifs->tellg() > 0) {
					ifs->seekg(0, std::ios::beg);
					is = ifs;
				}
			}
		}

		//-------------------------------------------------------------------------
		void File::readFile(string& result) const {
			if (exists()) {
				std::ifstream::pos_type size;
				char * memblock;
				std::ifstream in(output().c_str(), std::ios::in | std::ios::binary | std::ios::ate);
				if (in.is_open()) { // reading a complete binary file
					size = in.tellg();
					if (size > 0) {
						memblock = new char[(long long)size];
						in.seekg(0, std::ios::beg);
						in.read(memblock,(long long)size);
						in.close();
						result.assign(memblock,(long long)size);
						delete[] memblock;
					}
				}
			}
		}	// namespace FileUtils
		
		void File::readFile(string& result,size_t wanted,mode_t filter) const {
			if (exists(filter)) {
				char * memblock;
				std::ifstream in(output().c_str(), std::ios::in | std::ios::binary | std::ios::ate);
				if (in.is_open()) { // reading a complete binary file
					std::ifstream::pos_type size = in.tellg();
					if ( ( (filter & S_IFCHR) != 0 )|| ((unsigned long long)size > wanted) ) {
						size = wanted;
					}
					if (size > 0) {
						memblock = new char[(unsigned long long)size];
						in.seekg(0, std::ios::beg);
						in.read(memblock,(unsigned long)size);
						in.close();
						result.assign(memblock,(unsigned long)size);
						delete[] memblock;
					}
				}
			}
		}	// namespace FileUtils
		
	}
