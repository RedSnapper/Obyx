/* 
 * path.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * path.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <string>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "path.h"
#include "file.h"
#include "utils.h"

#include "commons/environment/environment.h"
#include "commons/logger/logger.h"

	namespace FileUtils {
		using namespace std;
		using namespace Log;
		std::stack<string> Path::wdstack;
		
		//-------------------------------------------------------------------------
		// Working Directory Methods
		//-------------------------------------------------------------------------
			
		void Path::push_wd(std::string new_wd ) {
			char tmp[255];
			if (! new_wd.empty() ) {
				const char *wdptr = new_wd.c_str();
				if ( chdir(wdptr) != 0 ) { /* report an error here. */ } else {
					getcwd(tmp,200 );
					string ccwd(tmp);
					wdstack.push(ccwd);
				}
			} else {
				getcwd(tmp,200 );
				string ccwd(tmp);
				wdstack.push(ccwd);
			}
		}
		
		void Path::pop_wd() {
			if ( !wdstack.empty() ) {
				wdstack.pop();
				if ( wdstack.size() > 0) {
					const char *wdptr = wdstack.top().c_str();
					if ( chdir(wdptr) != 0 ) { /* report an error here. */ }
				}
			} // else { /* report an error here. */ }
		}
	
		std::string Path::wd() {
			string retval;
			if ( wdstack.size() > 0) {
				retval = wdstack.top();
			} else {
				char tmp[1023];
				getcwd(tmp,200 );
				if (tmp != NULL) {
					retval = tmp;
				}
			}
			return retval;
		}

		void Path::cwd(std::string path) {
			const char *wdptr = path.c_str();
			if ( chdir(wdptr) != 0 ) { /* report an error here. */ }
		}

		//-------------------------------------------------------------------------
		// Default constructor
		//-------------------------------------------------------------------------
		Path::Path() : Device(),path(),directory_separator("/") {
		}

		//-------------------------------------------------------------------------
		// Creates a new Path using the specified path
		//-------------------------------------------------------------------------
		Path::Path(const string newpath) : Device(),path(),directory_separator("/") {
			setPath(newpath);
		}

		//-------------------------------------------------------------------------
		// Creates a new Path from the specified device & path
		//-------------------------------------------------------------------------
		Path::Path(const string newdevice, const string newpath) : Device(newdevice),path(),directory_separator("/") {
			setPath(newpath);
		}


		//-------------------------------------------------------------------------
		// Destructor
		//-------------------------------------------------------------------------
		Path::~Path() {
			clear();
		}

		//-------------------------------------------------------------------------
		// Clears the data contained in the Path object
		//-------------------------------------------------------------------------
		void Path::clear() {
			Device::clear();
//			for(vector<string>::iterator it = path.begin(); it != path.end(); it++)
//				delete *it;
			path.clear();
		}


		//-------------------------------------------------------------------------
		// Operator to assign values from the specified Path object
		//-------------------------------------------------------------------------
		Path &Path::operator=(const Path &newpath) {
			clear();
			device = newpath.device;
			for(vector<string>::const_iterator it = newpath.path.begin(); it != newpath.path.end(); it++) {
				path.push_back(*it);
			}
			directory_separator = newpath.directory_separator;
			root_separator = newpath.root_separator;
			return *this;
		}

		//-------------------------------------------------------------------------
		// Sets the directory separator
		//-------------------------------------------------------------------------
		void Path::setDirectorySeparator(const string separator)
		{
			directory_separator = separator;
		}


		//-------------------------------------------------------------------------
		// Gets the current directory separator
		//-------------------------------------------------------------------------
		const string Path::getDirectorySeparator() const
		{
			return directory_separator;
		}


		//-------------------------------------------------------------------------
		// Sets the path to the specified path
		//-------------------------------------------------------------------------
		void Path::setPath(const string newpath) {
			// Clear old path first
//			for(vector<string *>::iterator it = path.begin(); it != path.end(); it++)
//				delete *it;
			path.clear();
			cd(newpath);
		}


		//-------------------------------------------------------------------------
		// Adds path elements to the current path
		//-------------------------------------------------------------------------
		void Path::addPath(const string newpath)
		{
			size_t start = 0;
			size_t next = newpath.find(directory_separator, start);
			while(next < newpath.length() )
			{
				// Add an element
				if (next != start) {
					string str(newpath.substr(start, next - start));
					cd(str);
				}
				// if a root separator is the first character
				else if (next == 0) {
					cd(root_separator);
				}
				start = next + 1;
				next = newpath.find(directory_separator, start);
			}
			// Add the last element to the path
			if (start != newpath.length())
			{
				string str(newpath.substr(start, newpath.length() ));
				cd(str);
			}
		}


		//-------------------------------------------------------------------------
		// Gets a string containing only the path part
		//-------------------------------------------------------------------------
//To get a full path, including root device, use output(true)
		const string Path::getPath() const {
			string path_result="";
			for(size_t i = 0; i < getPathCount(); i++) {
				path_result += getPath(i) + directory_separator;
			}
			if (!path_result.empty()) {
				path_result.resize(path_result.size() -1);
			}
			return path_result;
		}

		//-------------------------------------------------------------------------
		// Gets a specified path part returning an empty string on error
		//-------------------------------------------------------------------------
		const string Path::getPath(size_t index) const {
			if (index >= path.size()) {
				return "";
			} else {
				return path.at(index);
			}
		}


		//-------------------------------------------------------------------------
		// Gets the number of path elements
		//-------------------------------------------------------------------------
		const size_t Path::getPathCount() const {
			return path.size();
		}


		//-------------------------------------------------------------------------
		// Gets the last path element
		//-------------------------------------------------------------------------
		const string Path::getEndPath() const {
			return path.back();
		}


		//-------------------------------------------------------------------------
		// Change directory to the specified path
		//-------------------------------------------------------------------------
		void Path::cd(const string newpath) {
			if (!newpath.empty()) {
				if (newpath == root_separator) {
					clear();
					if (device.empty())
					device = root_separator;
				}
				// Recursive cd
				else if (newpath.find(directory_separator, 0)  < newpath.length() ) {
					addPath(newpath);
				}
				// Back a directory
				else if (newpath == "..") {
					if (!path.empty()) {
						path.pop_back();
					}
				}
				else if (newpath.compare(".") != 0) {
					path.push_back(newpath);
				}
			}
		}


		//-------------------------------------------------------------------------
		// Gets the current path including device name
		// abs = true, gets an absolute path
		//-------------------------------------------------------------------------
		const string Path::output(bool abs) const {
			string retval = Device::output(abs);
			retval.append(getPath());
			return retval;
		}


		//-------------------------------------------------------------------------
		// Determine whether the current directory exists
		//-------------------------------------------------------------------------
		bool Path::exists() const {
			int result;
			string temppath = output(true);
//			if (!temppath.empty())
//				temppath = temppath.erase(temppath.length() - 1, 1);
		#ifdef WIN32
			struct _stat buf;
			result = _stat(temppath.c_str(), &buf);
		#else
			struct stat buf;
			result = stat(temppath.c_str(), &buf);
		#endif
			if (result == 0 && buf.st_mode & S_IFDIR)
				return true;
			return false;
		}


		//-------------------------------------------------------------------------
		// Makes the current directory
		// recursive = true, creates all intermediate directories if they do 
		// not exists
		//-------------------------------------------------------------------------
		bool Path::makeDir(bool recursive) const {
		int result;
		string dirstr(output().substr(0, output().length() - 1));

		#ifdef WIN32
			result = _mkdir(dirstr.c_str());
			_chdir("\\");
		#else
			result = mkdir(dirstr.c_str(), (mode_t) S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );	//Find these at /usr/include/sys/stat.h
		#endif
			if (result == 0)
				return true;
			else if (errno == EEXIST || errno == EISDIR)	// If already exists or is a directory
				return true;
			// Path does not exist
			else if ((errno == ENOENT) && recursive)	// If not exists and recursive
			{
				Path path1 = *this;
				path1.cd(root_separator);
				for(size_t i = 0; i < this->getPathCount() - 1; i++)
				{
					path1.cd(this->getPath(i));
					if (!path1.makeDir(false))
						return false;
				}
				return this->makeDir(false);
			}
			else if (!recursive || errno != ENOENT)
				*Logger::log << Log::error << Log::LI << "Error. FileUtils::Path::makeDir error:" << errno << " for makeDir " << dirstr.c_str() << Log::LO << Log::blockend;
			return false;
		}


		//-------------------------------------------------------------------------
		// Removes the current directory
		// recursive = true, removes the contents recursively
		//-------------------------------------------------------------------------
		bool Path::removeDir(bool recursive, bool stop_on_error) const
		{
			int result;
			string dirstr(output().substr(0, output().length() - 1));
		#ifdef WIN32
			// kludge to make sure we are not in the directory
			// that we are going to remove
			_chdir("\\");	
			result = _rmdir(dirstr.c_str());
		#else
			// Assume the directory is a link first
			result = unlink(dirstr.c_str());
			if (result == 0)
				return true;
			// Not a link, treat as a normal directory
			result = rmdir(dirstr.c_str());
		#endif
			if (result == 0)
				return true;
		#ifdef __SUNPRO_CC
			else if (recursive && errno == EEXIST)
		#else
			else if (recursive && errno == ENOTEMPTY)
		#endif
			{
				bool result2 = true;
				vector<Path> dirlist;
				vector<File> filelist;
				// Build a list of directories to scan
				dirlist.push_back(Path(*this));		
				listDirs(dirlist, true);
				// Build a list of files
				for(vector<Path>::iterator it = dirlist.begin(); it != dirlist.end(); it++) {
					it->listFiles(filelist);
				}
				// Removes all the files
				for(vector<File>::iterator it2 = filelist.begin(); it2 != filelist.end(); it2++) {
					bool res = it2->removeFile();
					if (!res && stop_on_error) {
						return false;
					}
					result2 &= res;
				}
				// Remove all the directories in reverse order
				for(vector<Path>::reverse_iterator it3 = dirlist.rbegin(); it3 != dirlist.rend(); it3++) {
					bool res = it3->removeDir();
					if (!res && stop_on_error) {
						return false;
					}
					result2 &= res;
				}
				return result2;
			}
			return false;
		}



		//-------------------------------------------------------------------------
		// Moves the current directory to the specified path
		//-------------------------------------------------------------------------
		bool Path::moveTo(const Path &newpath) const
		{
			if (exists()) {
				Path path1 = newpath;
				// If the destination already exists
				// we move into the destination directory
				if (newpath.exists())
					path1.cd(getEndPath());
				// If this new directory already exists, we fail
				if (path1.exists())
					{
						*Logger::log << Log::error << Log::LI << "Error. FileUtils::Path::moveTo error: " << path1.output().substr(0, output().length() - 1).c_str() << " already exists." << Log::LO << Log::blockend;
						return false;
					} else {	
						int result = rename(output().substr(0, output().length() - 1).c_str(), path1.output().substr(0, path1.output().length() - 1).c_str());
						if (result == 0)
							return true;
						else {
							result = errno;  //a stdio global, of all things.
							string error_result;
							switch (result) {
								case EACCES: error_result="Permission denied"; 
									break;
								case EBUSY: error_result="Device or Resource busy"; 
									break;
								case EEXIST: error_result="File exists"; break;
								case EINVAL: error_result="Invalid argument"; break;
								case EIO: error_result="Input/output error"; break;
								case EISDIR: error_result="Is a directory"; break;
								case ELOOP: error_result="Too many levels of symbolic links"; break;
								case EMLINK: error_result="Too many links"; break;
								case ENAMETOOLONG: error_result="Name is too long"; break;
								case ENOENT: error_result="No such file or directory"; break;
								case ENOSPC: error_result="No space left on device"; break;
								case ENOTDIR: error_result="Not a directory"; break;
								case ENOTEMPTY: error_result="Directory not empty"; break;
								case EPERM: error_result="Operation not permitted"; break;
								case EROFS: error_result="Read-only file system"; break;
								case EXDEV: error_result="Cross-device link"; break;									
								default: error_result="unknown"; break;
							}
							*Logger::log << Log::error << Log::LI << "Error. FileUtils::Path::moveTo error: " << result << " [" << error_result << "] for rename " << output().substr(0, output().length() - 1).c_str() << " to " << path1.output().substr(0, path1.output().length() - 1).c_str() << ""  << Log::LO << Log::blockend;
						}
					}
			}
			else {
				*Logger::log << Log::error << Log::LI << "Error. FileUtils::Path::moveTo error: " << output().substr(0, output().length() - 1).c_str() << " Doesn't exist." << Log::LO << Log::blockend;

			}
			return false;
		}


		//-------------------------------------------------------------------------
		// Creates a list of directories from the current path.
		// recursive = true, will recursively scan the directory tree
		//-------------------------------------------------------------------------
		void Path::listDirs(vector<Path>& list, bool recursive) const {
			struct dirent *dent;
			DIR *dir = opendir(output().c_str());
			while ((dent = readdir(dir)) != NULL) {
				string tfilename = dent->d_name;
				// We ignore system directories
				if (tfilename != "." && tfilename != "..") {
					Path path1(*this);
					path1.cd(tfilename);
					if (path1.exists()) {
 						list.push_back(path1);
						if (recursive) {
							path1.listDirs(list, recursive);
						}
					} 
				}
			}
			closedir(dir);
		}


		//-------------------------------------------------------------------------
		// Creates a list of files that match the RegExp from the current directory 
		//-------------------------------------------------------------------------
		void Path::listFilesA(vector<File>& list, const string regexp) const {
			struct dirent *dent;
			DIR *dir = opendir(output().c_str());
			while ((dent = readdir(dir)) != NULL) {
				File file1(*this,dent->d_name);
				if (match(file1.output(), regexp)) {
					if (file1.exists()) {
						list.push_back(file1);
						continue;
					}
				}
			}
			closedir(dir);
		}

		//-------------------------------------------------------------------------
		// Creates a list of files that match the RegExp
		// recursive = true, will recursively scan the directory tree for files.
		//-------------------------------------------------------------------------
		void Path::listFiles(vector<File>& list, bool recursive, const string regexp) const {
			vector<Path> dirlist; // Create a list of directories to scan
			dirlist.push_back(Path(*this));	
			if (recursive) {
				listDirs(dirlist, true);
			}
			for(vector<Path>::iterator it = dirlist.begin(); it != dirlist.end(); it++) {
				it->listFilesA(list, regexp);
			}
		}


		//-------------------------------------------------------------------------
		// Gets the size of a directory
		//-------------------------------------------------------------------------
		off_t Path::getSizeA() const
		{
			string temppath = output();
			// stat does not like '/' terminated paths
			if (!temppath.empty())
				temppath = temppath.erase(temppath.length() - 1, 1);
			off_t result;
		#ifdef WIN32
			struct _stat buf;
			result = _stat(temppath.c_str(), &buf);
			if (result == 0 && buf.st_mode & _S_IFDIR)
				return buf.st_size;
		#else
			struct stat buf;
			result = stat(temppath.c_str(), &buf);
			if (result == 0 && buf.st_mode & S_IFDIR)
				return buf.st_size; //64 bit to 32 bit conversion...
		#endif
			return 0;
		}

		//-------------------------------------------------------------------------
		// Gets the size of a directory
		// recursive = true, Scan the entire directory tree
		//-------------------------------------------------------------------------
		off_t Path::getSize(bool recursive) const {
			if (!recursive) {
				return getSizeA();
			}
			off_t size = 0;
			vector<Path> dirlist;  //= new vector<Path *>;
			vector<File> filelist; // = new vector<File *>;
			// Create a list of directories to scan
			dirlist.push_back(Path(*this));	
			listDirs(dirlist, true);
			// Add the size of each directory
			for(vector<Path>::iterator it = dirlist.begin(); it != dirlist.end(); it++) {
				size += it->getSizeA();
				it->listFilesA(filelist,".*");
			}
			for(vector<File>::iterator it2 = filelist.begin(); it2 != filelist.end(); it2++) {
				size += it2->getSize();
			}
			return size;
		}
	}	// namespace FileUtils

