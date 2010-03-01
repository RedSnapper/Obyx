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

#include <xercesc/dom/DOMDocument.hpp>

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
		bool success = false;
		if (filepathstring[0] != '/') { //we don't want to use file root, but site root.
			filepathstring = Environment::getpathforroot() + '/' +filepathstring;
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
		} else {
			error_str = "File does not exist. ";
		}
		if (filecontainer.empty()) {
			error_str = "File appears empty. Check file-read permission. ";
			success = false;
		}
		return success;
	}

	void output(const string finalfile,kind_type kind) {
		if ( ! Logger::wasfatal() ) {	  //This means there were some bugs...
			if (! Httphead::mime_changed() && (kind != di_object)) {
				Httphead::setmime("text/plain");
			}
			string temp_var;
			if (Environment::getenv("REQUEST_METHOD",temp_var) && temp_var.compare("HEAD") == 0) {
				Httphead::doheader();
			} else {
				Httphead::setcontent(finalfile);
				Httphead::doheader();
			}
		}
	}
	
	//get the values that we found in the store, if there are any.
	void outputRedirect() {
		Httphead::setcode(302);	
		Httphead::doheader(); 
	}
		
	void defaultHTTPHeader(bool complete) {
		Httphead::setcode(200);	
		if (complete) { 
			Httphead::doheader();
		}
	}

}	// namespace Filer
