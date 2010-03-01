/* 
 * mysqlquery.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlquery.h is a part of Obyx - see http://www.obyx.org .
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
#ifdef ALLOW_MYSQL

#ifndef OBYX_VDB_MYSQLQUERY_H
#define OBYX_VDB_MYSQLQUERY_H

#include <mysql.h>

#include "commons/vdb/query.h"
#include "commons/vdb/connection.h"

#include <string>

namespace Vdb {
	class MySQLService;    
	class MySQLConnection; 	

	class MySQLQuery : public Query {
	private:
		MySQLService* s;    //grandparent
		MySQLConnection* c; //parent		
		MYSQL* queryHandle;	//copy of ConnectionHandle
		
		MYSQL_RES* result;
		MYSQL_ROW row;
		MYSQL_ROW_OFFSET start;
		MYSQL_ROW_OFFSET end;
		unsigned long* mysqlRowLengths; 
		void prepareResult();

	private:
		friend class Query;
		friend class MySQLConnection;
		bool readfield(long long, long long, std::string&, std::string&);
		void readFieldName(long long i, std::string&, std::string& );
		
		MySQLQuery(MySQLService* const, MySQLConnection* const, MYSQL*&, std::string&);
			

		virtual bool execute();
		void reset();

		virtual ~MySQLQuery();
	};
} // namespace Vdb

#endif // OBYX_VDB_MYSQLQUERY_H

#endif //ALLOW
