/* 
 * mysqlservice.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlservice.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_MYSQL_SERVICE_H
#define OBYX_MYSQL_SERVICE_H

#include <string>
#include "mysql.h"

namespace Vdb {

	class Service;
	class ServiceFactory;
	class MySQLConnection;
	
	/*
	 MySQLService is the database CLIENT service for mysql. used to load up what is necessary to connect to a mysql SERVER.
	 A service is identified by a 'name' signature -- eg this mysql client service could be 'mysql'.
	 All Services are accessed from ServiceFactory.
	 Responsibilities of Service are to supply Connection instances for a given service, and to load/unload libraries for that service.
	 All Connections are created from Service.
	 */
	class MySQLService : public Service {
private:
		void * mysql_lib_handle;
		MYSQL* serviceHandle;		//for services only. Use a separate connectionHandle for connections.
		
private:
		MySQLService(const MySQLService&) {}		//not implemented.
		MySQLService operator=(const MySQLService&) { return *this; }  //not implemented.

		friend class ServiceFactory;
		MySQLService(bool);
		virtual ~MySQLService();

private:
		friend class MySQLDatabase;
		friend class MySQLConnection;
		friend class MySQLQuery;

//The mysql API that we use.
		int (*library_init)(int, char **,char **);
		void (*library_end)();
		int (*select_db)(MYSQL*, const char*);
		void (*close)(MYSQL*);
		int (*mysql_reset_connection)(MYSQL*);
		void (*data_seek)(MYSQL_RES*,my_ulonglong);
		unsigned int (*my_errno)(MYSQL*);  //errno will be expanded as a macro from sys under some compilers.
		const char* (*error)(MYSQL*);
		MYSQL_FIELD* (*fetch_field_direct)(MYSQL_RES*,unsigned int);
		unsigned long* (*fetch_lengths)(MYSQL_RES*);
		MYSQL_ROW (*fetch_row)(MYSQL_RES*);
		void (*free_result)(MYSQL_RES*);
		MYSQL* (*init)(MYSQL*);
		my_ulonglong (*insert_id)(MYSQL*);
		MYSQL_RES* (*list_fields)(MYSQL*, const char*,const char*);
		MYSQL_RES* (*list_tables)(MYSQL*,const char*);
		unsigned int (*num_fields)(MYSQL_RES*);
		my_ulonglong (*num_rows)(MYSQL_RES*);
		int	(*options)(MYSQL *,enum mysql_option,const char *);	
		MYSQL* (*real_connect)(MYSQL*, const char*,const char*,const char*,const char*, unsigned int,const char*, unsigned long);
		unsigned long (*real_escape_string)(MYSQL*,char*,const char*, unsigned long);
		int (*real_query)(MYSQL*, const char*, unsigned long);
		MYSQL_ROW_OFFSET (*row_seek)(MYSQL_RES*, MYSQL_ROW_OFFSET);
		MYSQL_ROW_OFFSET (*row_tell)(MYSQL_RES*);
		const char* (*character_set_name)(MYSQL *);
		int (*set_character_set)(MYSQL *, const char *);		
		MYSQL_RES* (*store_result)(MYSQL*);
		
public:
		virtual Connection* instance();
	};
}

#endif //header

#endif //ALLOW_MYSQL

