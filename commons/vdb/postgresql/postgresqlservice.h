/* 
 * postgresqlservice.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * postgresqlservice.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#ifndef OBYX_POSTGRESQL_SERVICE_H
#define OBYX_POSTGRESQL_SERVICE_H

#include <string>
#include "libpq-fe.h"

namespace Vdb {
	
	class Service;
	class ServiceFactory;
	class PostgreSQLConnection;
	
	/*
	 PostgreSQLService is the database CLIENT service for mysql. used to load up what is necessary to connect to a PostgreSQL SERVER.
	 A service is identified by a 'name' signature -- eg this PostgreSQL client service could be 'postgre'.
	 All Services are accessed from ServiceFactory.
	 Responsibilities of Service are to supply Connection instances for a given service, and to load/unload libraries for that service.
	 All Connections are created from Service.
	 */
	class PostgreSQLService : public Service {
	private:
		void * postgres_lib_handle;
		
	private:
		PostgreSQLService(const PostgreSQLService&) {}		//not implemented.
		PostgreSQLService operator=(const PostgreSQLService&) { return *this; }  //not implemented.
		
		friend class ServiceFactory;
		PostgreSQLService(bool);
		virtual ~PostgreSQLService();
		
	private:
		friend class PostgreSQLDatabase;
		friend class PostgreSQLConnection;
		friend class PostgreSQLQuery;
		
		//The PostgreSQL API that we use.
		PGconn*        (*PQconnectdb)(const char *);
		void           (*PQfinish)(PGconn *);
		PGconn*        (*PQsetdbLogin)(const char*,const char*,const char*,const char*,const char*,const char*,const char*);
		int			   (*PQsetClientEncoding)(PGconn*,const char*);
		ConnStatusType (*PQstatus)(const PGconn*);
		unsigned char* (*PQescapeByteaConn)(PGconn*,const unsigned char*,size_t,size_t*);
		char*		   (*PQerrorMessage)(const PGconn*);
		void		   (*PQclear)(PGresult *);
		PGresult*	   (*PQexec)(PGconn*, const char*);
		ExecStatusType (*PQresultStatus)(const PGresult *);
		int			   (*PQntuples)(const PGresult*);
		int			   (*PQnfields)(const PGresult*);
		char*		   (*PQfname)(const PGresult*, int);
		Oid			   (*PQftable)(const PGresult*, int);
		char*          (*PQgetvalue)(const PGresult*,int,int);
		char*		   (*PQresultErrorMessage)(const PGresult *);

	public:
		virtual Connection* instance();
	};
}

#endif

#endif //allow
