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

#ifdef FAST
#include "commons/fast/fast.h"
#endif
#include "commons/httphead/httphead.h"
#include "commons/httpfetch/httpfetch.h"
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "commons/filing/filing.h"
#include "commons/vdb/vdb.h"

#include "commons/string/bitwise.h"


#include "itemstore.h"
#include "filer.h"
#include "document.h"
#include "strobject.h"
#include "osimessage.h"
#include "dataitem.h"

using namespace std;
using namespace Vdb;
using namespace Log;
using namespace obyx;

void startup(std::string&,std::string&);
void init(ostream*&,int,char**,char**);
void finalise();
void shutdown();

int main(int argc, char *argv[]) {
	string v_number = "1.110130"; //Do NOT put the v here!
#ifdef FAST
	string version  = "Obyx v"+v_number+"F Supported";
#else
	string version  = "Obyx v"+v_number+" Supported";
#endif
	if (argc == 2 && argv[1][0]=='-' && argv[1][1]=='V' ) {
		string compiledate(__DATE__);
		string compiletime(__TIME__);
	    std::cout << "Status: 200 OK\r\nContent-Type: text/plain\r\n\r\n";
		std::cout << version << ", Build:" << compiledate << " " << compiletime;
	} else {
		startup(version,v_number);
		ostream* f_out = NULL;
		char** ienv = NULL;
#ifdef FAST
		while (Fast::ready(f_out,ienv))   {
#endif
			init(f_out,argc,argv,ienv);
			Httphead* http = Httphead::service();
			Environment* env = Environment::service();
			string sourcefilepath="";
			if ( env->getenv("PATH_TRANSLATED",sourcefilepath)) {
				size_t wdp = sourcefilepath.rfind('/');
				if ( wdp != string::npos) {
					FileUtils::Path::push_wd(sourcefilepath.substr(0,wdp));
				} 
				string errorstr="",sourcefile="";
				kind_type kind = di_null;
				if (Filer::getfile(sourcefilepath,sourcefile,errorstr)) {
					ObyxElement::init();
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
					http->setcode(404);
					string wd = FileUtils::Path::wd();
					*Logger::log << Log::fatal << Log::LI << "Error loading main file. ";
					*Logger::log << errorstr;
					*Logger::log << "Path: [" << sourcefilepath << "], Directory: [" << wd << "]. Obyx needs a source file to parse." << Log::LO << blockend;	
					http->doheader();
				}
				if ( wdp != string::npos) {
					FileUtils::Path::pop_wd();
				}
			} else {
				http->setcode(404);	
				*Logger::log << Log::fatal << Log::LI << "Error. PATH_TRANSLATED was not set. Obyx needs a source file to parse." << Log::LO << blockend;
				http->doheader();
			}
			finalise();
#ifdef FAST
		}	// finish fast loop
#endif
		shutdown();
	}
	return 0;
}
void startup(std::string& version,std::string& v_number) {
	string errs;
	Environment::startup(version,v_number);				//unchanging environment stuff.
	String::Deflate::startup(errs);						//need to start up for mysql etc.	
#ifdef FAST
		Fast::startup();
		Vdb::ServiceFactory::startup();
#endif
	String::Infix::Evaluate::startup();
#ifndef DISALLOW_GMP
	String::Bit::GMP::startup();
	String::Bit::Evaluate::startup();
#endif
	String::Regex::startup();
	Httphead::startup();
	Logger::startup(version);								//Logger
	OsiMessage::startup();
	Document::startup();
	ObyxElement::startup();
	DataItem::startup();
	ItemStore::startup();
	Fetch::HTTPFetch::startup();
	XMLChar::startup();
}
void init(ostream*& f_out,int argc,char** argv,char** env) {
	Environment::init(argc,argv,env);	//
#ifndef FAST
	Vdb::ServiceFactory::startup();
#endif
	Environment* e = Environment::service();
	ostream* os = Logger::init(f_out);	//Instance Logger
	Httphead::init(os);					//Instance head
	e->initwlogger();					//Continue environment load.
	OsiMessage::init();
	Document::init();					//Instance Document
	DataItem::init();
	ItemStore::init();
}
void finalise() {
	ItemStore::finalise();
	DataItem::finalise();
	OsiMessage::finalise();
	Document::finalise();		
	Logger::finalise();
	Httphead::finalise();
#ifndef FAST
	Vdb::ServiceFactory::shutdown();	//Remove the database service
#endif
	Environment::finalise();
}
void shutdown() {
	ItemStore::shutdown();
	DataItem::shutdown();
	ObyxElement::shutdown();
	OsiMessage::shutdown();
 	XMLChar::shutdown();
	Fetch::HTTPFetch::shutdown();
	Document::shutdown();		
	Httphead::shutdown();
	String::Regex::shutdown();
	String::Infix::Evaluate::shutdown();
#ifndef DISALLOW_GMP
	String::Bit::Evaluate::shutdown();
	String::Bit::GMP::shutdown();
#endif
	String::Digest::shutdown();
	String::Deflate::shutdown();
#ifdef FAST
	Vdb::ServiceFactory::shutdown();	//Remove the database service
	Fast::shutdown();
#endif
	Environment::shutdown();
}

