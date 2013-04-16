/* 
 * filer.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * filer.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <string>

#include "commons/filing/filing.h"
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"
#include "commons/httphead/httphead.h"
#include "commons/xml/xml.h"

#include "filer.h"
#include "document.h"

using namespace std;
using namespace Log;
using namespace xercesc;
using namespace XML;

namespace Filer {
	bool getfile(string filepathstring,string& filecontainer,string& error_str) {
		Environment* env = Environment::service();
		bool success = false;
		if (filepathstring[0] != '/') { //we don't want to use file root, but site root.
			filepathstring = env->getpathforroot() + '/' +filepathstring;
		}
		FileUtils::Path destination; 
		destination.cd(filepathstring);
		string final_file_path = destination.output(true);
		FileUtils::File file(final_file_path);
		if (file.exists()) {
			off_t flen = file.getSize();
			if ( flen != 0 ) {
				file.readFile(filecontainer);
				success = true;
			}
			if (filecontainer.empty()) {
				error_str = "File appears empty. Check file-read permission. ";
				success = false;
			}
		} else {
			error_str = "File does not exist. ";
		}
		return success;
	}
	
	void output(string& finalfile,kind_type kind) {
		Environment* env = Environment::service();
		Httphead* http = Httphead::service();	
		if ( ! Logger::wasfatal() ) {	  //This means there were some bugs...
			if (! http->mime_changed() && (kind != di_object) && (!http->contentset())) {
				http->setmime("text/plain");
			}
			string temp_var;
			if (env->getenv("REQUEST_METHOD",temp_var) && temp_var.compare("HEAD") == 0) {
				http->doheader();
			} else {
				if (! (http->contentset() && kind == di_null)) {
					http->setcontent(finalfile);
				}
				http->doheader();
			}
		}
	}
	
	void outputRedirect() {
		//get the values that we found in the store, if there are any.
		Httphead* http = Httphead::service();	
		http->setcode(302);	
		http->doheader(); 
	}
	void defaultHTTPHeader(bool complete) {
		Httphead* http = Httphead::service();	
		http->setcode(200);	
		if (complete) { 
			http->doheader();
		}
	}
}	// namespace Filer
