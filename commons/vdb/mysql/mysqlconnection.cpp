/* 
 * mysqlconnection.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlconnection.cpp is a part of Obyx - see http://www.obyx.org .
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
#ifdef ALLOW_MYSQL
#include <iostream>			

#include "mysql.h"
#include "errmsg.h"

#include "commons/logger/logger.h"

#include "commons/vdb/service.h"
#include "commons/vdb/query.h"
#include "mysqlservice.h"
#include "mysqlconnection.h"
#include "mysqlquery.h"

namespace Vdb {

	MySQLConnection::MySQLConnection(MySQLService* service) : s(service),connectionHandle(nullptr),conn_open(false),db_open(false) {
		s->library_init(0,nullptr,nullptr);
		connectionHandle = s->init(nullptr);
		if (connectionHandle == nullptr && Logger::log != nullptr) {
			*Logger::log <<  Log::fatal << Log::LI << "MySQLConnection error:: Failed to initialise a MySQL client connection handle" << Log::LO << Log::blockend; 
		}
	};

	MySQLConnection::~MySQLConnection() {
		if (connectionHandle != nullptr) { 
			s->close(connectionHandle);
		}
		s->library_end();
	}

	bool MySQLConnection::open(const std::string& host, const std::string& user, const unsigned int port,const std::string& password) {
		if ( ! isopen() ) {	
			const char* tsocket = nullptr;
			thost = host;
			tuser = user;
			tpasswd = password;
			tport = port;
			if (s->real_connect(connectionHandle, thost.c_str(), tuser.c_str(), tpasswd.c_str(), nullptr, tport, tsocket, 0) == nullptr) {
				string errorMessage = s->error(connectionHandle);
				if (Logger::debugging() && Logger::log != nullptr) {
					*Logger::log << Log::info << Log::LI << "MySQLConnection error:: Connection failed with '" << errorMessage << "'" << Log::LO << Log::blockend; 
				}
				conn_open = false;
			} else {
				conn_open = true;
				const char* charset_c= s->character_set_name(connectionHandle);
				string charset="unknown";
				if (charset_c != nullptr) {
					charset = charset_c;
					if (Logger::debugging() && Logger::log != nullptr) {
						*Logger::log << Log::info << Log::LI << "MySQLConnection:: Connection default charset:'" << charset_c << "'" << Log::LO << Log::blockend; 
					}
				}
				if ( charset.compare("utf8") != 0 && (void *)(s->set_character_set) != nullptr ) { // set_character_set only available from mysql 5.07
					int scs_result = s->set_character_set(connectionHandle,"utf8");
					if (scs_result != 0 )  {
						string errorMessage = s->error(connectionHandle);
						if (Logger::debugging() && Logger::log != nullptr) {
							*Logger::log << Log::info << Log::LI << "MySQLConnection:: Connection failed to set utf-8 charset. Error:" << scs_result << " Msg:" << errorMessage << Log::LO << Log::blockend;  
						}
					} else {
						if (Logger::debugging() && Logger::log != nullptr) {
							*Logger::log << Log::info << Log::LI << "MySQLConnection:: Connection set to charset utf-8." << Log::LO << Log::blockend;  
						}
					}
				}
			}
		} else {
			if (Logger::log != nullptr) {
				*Logger::log << Log::fatal << Log::LI << "MySQLConnection error: Each connection can only be opened once. Instantiate a new connection." << Log::LO << Log::blockend; 
			}
		}
		return isopen();
	}
	
	bool MySQLConnection::database(const std::string& db) {
		if ( isopen() ) {
			int success = 0;
			if (db.compare("-") != 0) {
				tdbase = db;
				success = s->select_db(connectionHandle, tdbase.c_str());
			}
			if ( success != 0) {
				int error = s->my_errno(connectionHandle);
				if (error == CR_SERVER_GONE_ERROR || error == CR_SERVER_LOST) {
					close();
				}
				if (Logger::log != nullptr) {
					string errstr = s->error(connectionHandle);
					*Logger::log << Log::fatal << Log::LI << "MySQLConnection error: While attempting to open database: " << errstr <<  Log::LO << Log::blockend; 
				}
				return false;
			} else {
				db_open = true;
				return true;
			}
		} else {
			if (Logger::log != nullptr) {
				*Logger::log << Log::fatal << Log::LI << "MySQLConnection error: Open a connection before selecting a database!" << Log::LO << Log::blockend; 
			}
			return false;
		}
	}
	
	bool MySQLConnection::query(Query*& q,std::string query_str) {
		bool retval = false;
		if ( isopen() ) { 
			if ( db_open ) {
				q = new MySQLQuery(s, this, connectionHandle, query_str);
				if (q != nullptr) { 
					retval = true;
				}
			} else {
				q = nullptr;
			}
		} else {
			q = nullptr;
		}
		return retval;
	}

	Query* MySQLConnection::query(std::string query_str) {
		if ( isopen() ) { 
			if ( db_open ) {
				return new MySQLQuery(s, this, connectionHandle, query_str);
			} else {
				return nullptr;
			}
		} else {
			return nullptr;
		}
	}
	
//	"Note that MYSQL* must be a valid, open connection. This is needed because the escaping depends on the character set in use by the server."
	void MySQLConnection::escape(std::string& text) {
		if ( ! text.empty() && 	conn_open && connectionHandle != nullptr) { 
			char* tbuf = new char[text.length()*2+1];
			size_t size = s->real_escape_string(connectionHandle, tbuf, text.c_str(), text.length());
			std::string escapedString(tbuf, size);
			delete [] tbuf;
			text = escapedString;
		}
	}
	void MySQLConnection::list() {
		if (Logger::log != nullptr) {
			*Logger::log << Log::info << Log::LI << "mysql;" << thost << ";" << tuser << ";" << tport << ";" << tdbase << ";" << Log::LO << Log::blockend; 
		}
	}
	void MySQLConnection::close() {
		if (conn_open && connectionHandle != nullptr) { 
			s->close(connectionHandle);
			connectionHandle = nullptr;
			conn_open = false;
			db_open = false;
		}
	}

} // namespace Vdb

#endif //Allow

