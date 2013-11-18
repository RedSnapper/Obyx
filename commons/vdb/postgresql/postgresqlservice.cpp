/* 
 * postgresqlservice.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * postgresqlservice.cpp is a part of Obyx - see http://www.obyx.org .
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
#ifdef ALLOW_POSTGRESQL

#include <dlfcn.h>

#include "commons/dlso.h"
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"

#include "commons/vdb/servicefactory.h"
#include "commons/vdb/service.h"
#include "commons/vdb/connection.h"
#include "postgresqlservice.h"
#include "postgresqlconnection.h"
#include "libpq-fe.h"

namespace Vdb {
	
	/*
	 PostgreSQLService is the database CLIENT service for mysql. used to load up what is necessary to connect to a mysql SERVER.
	 A service is identified by a 'name' signature -- eg this mysql client service could be 'mysql'.
	 All Services are accessed from ServiceFactory.
	 Responsibilities of Service are to supply Connection instances for a given service, 
	 and to load/unload libraries for that service.
	 */
	
	PostgreSQLService::PostgreSQLService(bool fatal_necessity) : 
	Service(),postgres_lib_handle(nullptr),
	PQconnectdb(nullptr)
	{
		string libdir,libname;
		if (!Environment::getbenv("OBYX_LIBPQSO",libname)) { 	//legacy method
			if (Environment::getbenv("OBYX_LIBPQDIR",libdir)) {
				if (!libdir.empty() && *libdir.rbegin() != '/') libdir.push_back('/');
			}
			libname = SO(libdir,libpq);
		}
		postgres_lib_handle = dlopen(libname.c_str(),RTLD_LAZY);
		if (postgres_lib_handle != nullptr ) {
			PQconnectdb =             (PGconn* (*)(const char*)) dlsym(postgres_lib_handle,"PQconnectdb"); 
			PQfinish =                   (void (*)(PGconn*)) dlsym(postgres_lib_handle,"PQfinish");
			PQsetdbLogin =            (PGconn* (*)(const char*,const char*,const char*,const char*,const char*,const char*,const char*)) dlsym(postgres_lib_handle,"PQsetdbLogin");
			PQsetClientEncoding =         (int (*)(PGconn*,const char*)) dlsym(postgres_lib_handle,"PQsetClientEncoding");
			PQstatus =         (ConnStatusType (*)(const PGconn*)) dlsym(postgres_lib_handle,"PQstatus");
			PQescapeByteaConn= (unsigned char* (*)(PGconn*,const unsigned char*,size_t,size_t*)) dlsym(postgres_lib_handle,"PQescapeByteaConn");
			PQerrorMessage=             (char* (*)(const PGconn*)) dlsym(postgres_lib_handle,"PQerrorMessage");
			PQclear=			         (void (*)(PGresult*)) dlsym(postgres_lib_handle,"PQclear");
			PQexec=			        (PGresult* (*)(PGconn*, const char*)) dlsym(postgres_lib_handle,"PQexec");
			PQresultStatus=    (ExecStatusType (*)(const PGresult*)) dlsym(postgres_lib_handle,"PQresultStatus");
			PQntuples=					  (int (*)(const PGresult*)) dlsym(postgres_lib_handle,"PQntuples");
			PQnfields=					  (int (*)(const PGresult*)) dlsym(postgres_lib_handle,"PQnfields");
			PQfname=                    (char* (*)(const PGresult*, int)) dlsym(postgres_lib_handle,"PQfname");
			PQftable=                     (Oid (*)(const PGresult*, int)) dlsym(postgres_lib_handle,"PQftable");
			PQgetvalue=			        (char* (*)(const PGresult*,int,int)) dlsym(postgres_lib_handle,"PQgetvalue");
			PQresultErrorMessage=		(char* (*)(const PGresult *)) dlsym(postgres_lib_handle,"PQresultErrorMessage");
			
			service=true;
		} else {
			service=false;
			if (fatal_necessity) {
				string msg;	char* err = dlerror(); if ( err != nullptr) msg=err;
				*Logger::log <<  Log::fatal << Log::LI << "PostgreSQLService error:: Failed to load the " << postgresqllib << " dynamic library. " << msg << Log::LO << Log::blockend; 
			}
		}
	}
	
	PostgreSQLService::~PostgreSQLService() {
		if ( postgres_lib_handle != nullptr ) { 
			dlclose(postgres_lib_handle);
		}
	}
	
	Connection* PostgreSQLService::instance() {
		return new PostgreSQLConnection(this);
	}
	
}

#endif //allow
