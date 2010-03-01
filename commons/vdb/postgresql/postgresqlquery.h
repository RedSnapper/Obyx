/* 
 * postgresqlquery.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * postgresqlquery.h is a part of Obyx - see http://www.obyx.org .
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
#ifndef OBYX_POSTGRESQL_QUERY_H
#define OBYX_POSTGRESQL_QUERY_H

#include "libpq-fe.h"
#include "commons/vdb/query.h"
#include "commons/vdb/connection.h"

#include <string>

namespace Vdb {
	
	class PostgreSQLService;    
	class PostgreSQLConnection; 
	
	class PostgreSQLQuery : public Query {
	private:
		PostgreSQLService* s;    //grandparent
		PostgreSQLConnection* c; //parent	
		PGresult* res;
		PGconn* connectionHandle; //copy(!) of ConnectionHandle
		long long current_row;
		void prepareResult();
		
	private:
		friend class PostgreSQLConnection;
		virtual bool readfield(long long, long long, std::string&, std::string&);
		virtual void readFieldName(long long i, std::string&, std::string& );
		virtual bool execute();
		void reset();
		PostgreSQLQuery(PostgreSQLService* const, PostgreSQLConnection* const, PGconn*&, std::string&);
		virtual ~PostgreSQLQuery();
	};
	
} // namespace Vdb

#endif //header
#endif

