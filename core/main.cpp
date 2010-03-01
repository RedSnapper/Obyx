/* 
 * main.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * main.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <string>
#include <ctime>

#include "commons/httphead/httphead.h"
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "commons/filing/filing.h"
#include "commons/vdb/vdb.h"

#include "filer.h"
#include "document.h"
#include "strobject.h"
#include "osimessage.h"

using namespace std;
using namespace Vdb;
using namespace Log;
using namespace qxml;

int main(int argc, char *argv[]) {
	string version = "Obyx v1.10.03.01 Supported (Xerces 3.0/XQilla 2.2)";
    if (argc == 2 && argv[1][0]=='-' && argv[1][1]=='V' ) {
		string compiledate(__DATE__);
		string compiletime(__TIME__);
		std::cout << version << ", Build:" << compiledate << " " << compiletime;
	} else {
		Logger::title = version;
		XMLChar::init();
		Environment::init(argc,argv);					//yes, process post please!
		Environment::setenv("OBYX_VERSION",version);	//Let coders know what version we are in!
		ostream* os = Logger::startup();		//Logger
		Httphead::init(os);
		Document::init();
		OsiMessage::init();
		Vdb::ServiceFactory* dbsf = Vdb::ServiceFactory::startup(Environment::envexists("OBYX_SQLSERVICE_REQ"));	//Create a service, initialising it.
		string sourcefilepath="";
		if ( Environment::getenv("PATH_TRANSLATED",sourcefilepath)) {
			size_t wdp = sourcefilepath.rfind('/');
			if ( wdp != string::npos) {
				FileUtils::Path::push_wd(sourcefilepath.substr(0,wdp));
			} 
			string errorstr="",sourcefile="";
			kind_type kind = di_null;
			if (Filer::getfile(sourcefilepath,sourcefile,errorstr)) {
				ObyxElement::init(dbsf);
				DataItem* sfile = DataItem::factory(sourcefile,di_text); //will parse as object inside logging.
				string out_str;				
				if (true) { // used to delete document before finalising stuff
					DataItem* result = NULL;
					Document* obyxdoc = new Document(sfile,Document::Main,sourcefilepath);	//Main = called from here!
					obyxdoc->results.takeresult(result);
					if (result != NULL) { 
						out_str = *result; 
						kind = result->kind();
						delete result;
					}
					delete obyxdoc;
				}
				Filer::output(out_str,kind);
				delete sfile;
				ObyxElement::finalise();	
			} else {
				Httphead::setcode(404);
				string wd = FileUtils::Path::wd();
				*Logger::log << Log::fatal << Log::LI << "Error loading main file. ";
				*Logger::log << errorstr;
				*Logger::log << "Path: [" << sourcefilepath << "], Directory: [" << wd << "]. Obyx needs a source file to parse." << Log::LO << blockend;	
				Httphead::doheader();
			}
			if ( wdp != string::npos) {
				FileUtils::Path::pop_wd();
			}
		} else {
			Httphead::setcode(404);		
			*Logger::log << Log::fatal << Log::LI << "Error. PATH_TRANSLATED was not set. Obyx needs a source file to parse." << Log::LO << blockend;	
			Httphead::doheader();
		}
		Vdb::ServiceFactory::shutdown();	//Remove the database service, disposing of the dbs at the same time.
		Document::shutdown();		
		Logger::shutdown();			
	}
	return 0;
}
