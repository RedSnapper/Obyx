/* 
 * postgresqlconnection.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * postgresqlconnection.cpp is a part of Obyx - see http://www.obyx.org .
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
#include "libpq-fe.h"
#include <iostream>			
#include "errmsg.h"
#include "commons/logger/logger.h"

#include "commons/vdb/service.h"
#include "commons/vdb/connection.h"
#include "commons/vdb/query.h"

#include "postgresqlservice.h"
#include "postgresqlconnection.h"
#include "postgresqlquery.h"

namespace Vdb {
	
	PostgreSQLConnection::PostgreSQLConnection(PostgreSQLService* service) : s(service),connectionHandle(nullptr),conn_open(false),db_open(false) {
// nothing happens here.
	}
	
	PostgreSQLConnection::~PostgreSQLConnection() {
		if (connectionHandle != nullptr) { 
			s->PQfinish(connectionHandle);
		}
	}
		
	bool PostgreSQLConnection::open(const std::string& host, const std::string& user, const unsigned int port,const std::string& password) {
		if ( ! isopen() ) {
			pghost = host;
			login  = user;
			pwd = password;
			if (port != 0) {
				ostringstream portstr;
				portstr << port;
				pgport = portstr.str();
			} else {
				pgport="";
			}
			conn_open = true; //this is really a bit of a misnomer. but pg doesn't set a connection unless a db is specified.
		}
		return isopen();
	}
	
	bool PostgreSQLConnection::database(const std::string& db) {
		if ( isopen() ) {
			dbName=db;
			connectionHandle = s->PQsetdbLogin(pghost.c_str(),pgport.c_str(),nullptr,nullptr,db.c_str(),login.c_str(),pwd.c_str());
			if (connectionHandle != nullptr) {
				ConnStatusType status = s->PQstatus(connectionHandle);
				if (status == CONNECTION_OK) {
					int result = s->PQsetClientEncoding(connectionHandle,"UTF8");
					if (result != 0) {
						if (Logger::debugging()) {
						   *Logger::log << Log::info << Log::LI << "PostgreSQL:: Connection failed to set utf-8 charset." << Log::LO << Log::blockend; 
						}
					} else {
						db_open = true;
					}
				} else {
					string errmsg = s->PQerrorMessage(connectionHandle);
					*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: Connection failed to the database." << errmsg << Log::LO << Log::blockend; 
					close();
				}
			} else {
				*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: Failed to connect to the database." << Log::LO << Log::blockend; 
			}
		} else {
			*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: Open a connection before selecting a database!" << Log::LO << Log::blockend; 
		}
		return db_open;
	}
	
	bool PostgreSQLConnection::query(Query*& q,std::string query_str) {
		bool retval = false;
		if ( isopen() ) { 
			if ( db_open ) {
				q = new PostgreSQLQuery(s, this, connectionHandle, query_str);
				if (q != nullptr) { 
					retval = true;
				}
			} else {
				*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: select a database before creating a query!" << Log::LO << Log::blockend; 
				q = nullptr;
			}
		} else {
			*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: Open a connection before creating a query!" << Log::LO << Log::blockend; 
			q = nullptr;
		}
		return retval;
	}
	
	Query* PostgreSQLConnection::query(std::string query_str) {
		if ( isopen() ) { 
			if ( db_open ) {
				return new PostgreSQLQuery(s, this, connectionHandle, query_str);
			} else {
				*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: select a database before creating a query!" << Log::LO << Log::blockend; 
				return nullptr;
			}
		} else {
			*Logger::log << Log::fatal << Log::LI << "PostgreSQLConnection error: Open a connection before creating a query!" << Log::LO << Log::blockend; 
			return nullptr;
		}
	}
	
	//	"Note that MYSQL* must be a valid, open connection. This is needed because the escaping depends on the character set in use by the server."
	void PostgreSQLConnection::escape(std::string& text) {
		if ( ! text.empty() && 	conn_open && connectionHandle != nullptr) { 
			size_t result=0;
			const unsigned char* source = (const unsigned char*)text.c_str();
			unsigned char* buffer = s->PQescapeByteaConn(connectionHandle,source,text.size(),&result);
			text = (const char*)buffer;
			free(buffer);
		}
	}
	void PostgreSQLConnection::list() {
		if (Logger::log != nullptr) {
			*Logger::log << Log::info << Log::LI << "PostgreSQL;" << pghost << ";" << login << ";" << pgport << ";" << dbName << ";" << Log::LO << Log::blockend; 
		}
	}
	void PostgreSQLConnection::close() {
		if (connectionHandle != nullptr) { 
			s->PQfinish(connectionHandle);
		}
		connectionHandle = nullptr;
		conn_open = false;
		db_open = false;
	}
	
} // namespace Vdb

#endif //allow
