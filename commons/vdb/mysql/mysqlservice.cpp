/* 
 * mysqlservice.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlservice.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <dlfcn.h>

#include "mysql.h"

#include "commons/logger/logger.h"
#include "commons/environment/environment.h"

#include "commons/vdb/servicefactory.h"
#include "commons/vdb/service.h"
#include "commons/vdb/connection.h"
#include "mysqlservice.h"
#include "mysqlconnection.h"

namespace Vdb {
	
	/*
	 MySQLService is the database CLIENT service for mysql. used to load up what is necessary to connect to a mysql SERVER.
	 A service is identified by a 'name' signature -- eg this mysql client service could be 'mysql'.
	 All Services are accessed from ServiceFactory.
	 Responsibilities of Service are to supply Connection instances for a given service, and to load/unload libraries for that service.
	 */
//	otool -Rv  /usr/local/mysql/lib/libmysqlclient_r.dylib | grep -v undefined | grep -v private | sort
	
	MySQLService::MySQLService(bool fatal_necessity) : 
		Service(),zip_lib_handle(NULL),mysql_lib_handle(NULL),serviceHandle(NULL),
		library_init(NULL),library_end(NULL),select_db(NULL),close(NULL),data_seek(NULL),my_errno(NULL),error(NULL),
		fetch_field_direct(NULL),
		fetch_lengths(NULL),fetch_row(NULL),free_result(NULL),init(NULL),insert_id(NULL),list_fields(NULL),
		list_tables(NULL),num_fields(NULL),num_rows(NULL),options(NULL),real_connect(NULL),real_escape_string(NULL),
		real_query(NULL),row_seek(NULL),row_tell(NULL),character_set_name(NULL),set_character_set(NULL),store_result(NULL)
	{ 
		string ziplib;
		if (!Environment::getenv("OBYX_LIBZIPSO",ziplib)) ziplib = "libz.so";
		string mysqllib;
		if (!Environment::getenv("OBYX_LIBMYSQLCRSO",mysqllib)) mysqllib = "libmysqlclient_r.so";
		zip_lib_handle = dlopen(ziplib.c_str(),RTLD_LAZY | RTLD_GLOBAL);  //dlopen MUST have one of RTLD_LAZY,RTLD_NOW
		if (zip_lib_handle != NULL ) {
			mysql_lib_handle = dlopen(mysqllib.c_str(),RTLD_LAZY);
			if (mysql_lib_handle != NULL ) {
				library_init = (int (*)(int, char **,char **)) dlsym(mysql_lib_handle,"mysql_server_init");
				library_end = (void (*)()) dlsym(mysql_lib_handle,"mysql_server_end");
				select_db = (int (*)(MYSQL*, const char*)) dlsym(mysql_lib_handle,"mysql_select_db");
				close = (void (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_close");
				data_seek = (void (*)(MYSQL_RES*,my_ulonglong)) dlsym(mysql_lib_handle,"mysql_data_seek");
				my_errno = (unsigned int (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_errno");
				error = (const char* (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_error");
				fetch_field_direct = (MYSQL_FIELD* (*)(MYSQL_RES*,unsigned int)) dlsym(mysql_lib_handle,"mysql_fetch_field_direct");
				fetch_lengths = (unsigned long* (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_fetch_lengths");
				fetch_row = (MYSQL_ROW (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_fetch_row");
				free_result = (void (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_free_result");
				init = (MYSQL* (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_init");
				insert_id = (my_ulonglong (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_insert_id");
				list_fields = (MYSQL_RES* (*)(MYSQL*, const char*,const char*)) dlsym(mysql_lib_handle,"mysql_list_fields");
				list_tables = (MYSQL_RES* (*)(MYSQL*,const char*)) dlsym(mysql_lib_handle,"mysql_list_tables");
				num_fields = (unsigned int (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_num_fields");
				num_rows = (my_ulonglong (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_num_rows");
				options = (int	(*)(MYSQL *,enum mysql_option,const char*)) dlsym(mysql_lib_handle,"mysql_options");
				real_connect = (MYSQL* (*)(MYSQL*, const char*,const char*,const char*,const char*, unsigned int,const char*, unsigned long)) dlsym(mysql_lib_handle,"mysql_real_connect");
				real_escape_string = (unsigned long (*)(MYSQL*,char*,const char*, unsigned long)) dlsym(mysql_lib_handle,"mysql_real_escape_string");
				real_query = (int (*)(MYSQL*, const char*, unsigned long)) dlsym(mysql_lib_handle,"mysql_real_query");
				row_seek = (MYSQL_ROW_OFFSET (*)(MYSQL_RES*, MYSQL_ROW_OFFSET)) dlsym(mysql_lib_handle,"mysql_row_seek");
				row_tell = (MYSQL_ROW_OFFSET (*)(MYSQL_RES*)) dlsym(mysql_lib_handle,"mysql_row_tell");
				character_set_name = (const char* (*)(MYSQL *)) dlsym(mysql_lib_handle,"mysql_character_set_name"); 
				set_character_set = (int (*)(MYSQL*, const char*)) dlsym(mysql_lib_handle,"mysql_set_character_set"); 
				store_result = (MYSQL_RES* (*)(MYSQL*)) dlsym(mysql_lib_handle,"mysql_store_result");
				service=true;
				
			} else {
				service=false;
				if (fatal_necessity) {
					string msg;	char* err = dlerror(); if ( err != NULL) msg=err;
					*Logger::log <<  Log::fatal << Log::LI << "MySQLService error:: Failed to load the " << mysqllib << " dynamic library. " << msg << Log::LO << Log::blockend; 
				}
			}
		} else {
			service=false;
			if (fatal_necessity) {
				string msg;	char* err = dlerror(); if ( err != NULL) msg=err;
				*Logger::log <<  Log::fatal << Log::LI << "MySQLService error:: Failed to load the " << ziplib << " dynamic library. " << msg << Log::LO << Log::blockend; 
			}
		}
		if (service) {
			serviceHandle = init(NULL);
			if (serviceHandle == NULL) {
				service=false;
				if (fatal_necessity) {
					*Logger::log <<  Log::fatal << Log::LI << "MySQLService error:: Failed to initialise a MySQL client service handle." << Log::LO << Log::blockend; 
				}				
				if ( mysql_lib_handle != NULL ) { 
					if (fatal_necessity) {
						*Logger::log << " mysql_lib_handle was found."; 
					}				
					dlclose(mysql_lib_handle);
					mysql_lib_handle = NULL;
				} else {
					if (fatal_necessity) {
						*Logger::log << " mysql_lib_handle was NOT found."; 
					}				
				}
				if ( zip_lib_handle != NULL ) { 
					if (fatal_necessity) {
						*Logger::log << " zip_lib_handle was found."; 
					}				
					dlclose(zip_lib_handle);
					zip_lib_handle = NULL;
				} else {
					if (fatal_necessity) {
						*Logger::log << " zip_lib_handle was NOT found."; 
					}				
				}
				if (fatal_necessity) {
					*Logger::log << Log::LO << Log::blockend; 
				}				
			} else {
				options(serviceHandle,MYSQL_READ_DEFAULT_GROUP,"cgi_sql_services");
				options(serviceHandle,MYSQL_SET_CHARSET_NAME,"utf8");
			}
		}
	}

	MySQLService::~MySQLService() {
		if (serviceHandle != NULL) {
			close(serviceHandle);
		}
		if ( mysql_lib_handle != NULL ) { 
			dlclose(mysql_lib_handle);
		}
		if ( zip_lib_handle != NULL ) { 
			dlclose(zip_lib_handle);
		}
	}

	Connection* MySQLService::instance() {
		return new MySQLConnection(this);
	}
	
}

#endif // ALLOW_MYSQL

