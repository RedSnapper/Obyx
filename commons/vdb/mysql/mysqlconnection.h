/* 
 * mysqlconnection.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlconnection.h is a part of Obyx - see http://www.obyx.org .
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
#ifndef OBYX_VDB_MYSQLCONNECTION_H
#define OBYX_VDB_MYSQLCONNECTION_H

#include <set>
#include <string>

#include <mysql.h>
#include "commons/vdb/connection.h"

class MySQLService;

namespace Vdb {

	class MySQLConnection : public Connection {
private:
		MySQLService* s;
		MYSQL* connectionHandle;
		bool conn_open;
		bool db_open;
		MySQLConnection();
		
		std::string thost;
		std::string tuser;
		std::string tpasswd;
		unsigned int tport;
		std::string tdbase;
		

public:
		virtual bool dbselected() { return db_open && conn_open && connectionHandle != nullptr; }
		virtual bool isopen() { return conn_open && connectionHandle != nullptr; }
		virtual bool open(const std::string&, const std::string&,const unsigned int, const std::string&); 
		virtual bool database(const std::string&);
		virtual Query* query(std::string = "");
		virtual bool query(Query*&,std::string = "");
		virtual void escape(std::string&);
		virtual void close();                            
		virtual void list();                            
		MySQLConnection(MySQLService*);
		virtual ~MySQLConnection();
	};

} // namespace Vdb

#endif // OBYX_VDB_MYSQLCONNECTION_H
#endif // ALLOW
