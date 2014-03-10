/* 
 * mysqlquery.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * mysqlquery.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <sstream>
#include <mysql_version.h>
#include "commons/logger/logger.h"

#include "commons/vdb/service.h"
#include "commons/vdb/connection.h"
#include "commons/vdb/query.h"
#include "mysqlquery.h"
#include "mysqlconnection.h"
#include "mysqlservice.h"


using namespace std;

namespace Vdb {

	MySQLQuery::MySQLQuery(MySQLService* const s_in, MySQLConnection* const c_in, MYSQL*& h_in, std::string& qstr) :
		Query(qstr),s(s_in),c(c_in),queryHandle(h_in),result(nullptr),row(0),start(0),end(0),mysqlRowLengths(nullptr) {
	}
	
	MySQLQuery::~MySQLQuery() {
		reset();
	}
		
	bool MySQLQuery::readfield(long long  i, long long  j, std::string& readString, std::string& errstring) {
		bool retval = false;
		if (isactive) {
			try {
				rowFieldRangeCheck(i, j);
				MYSQL_ROW_OFFSET currowoff = s->row_tell(result);
				s->data_seek(result, i - 1);
				MYSQL_ROW currow = s->fetch_row(result);
				const char* field = currow[j - 1];
				long long length = s->fetch_lengths(result)[j - 1];
				s->row_seek(result, currowoff);
				if (length > 0)
					readString.assign(field,(unsigned long)length);
				else
					readString.assign("");
				retval = true;
			} catch (...) {
				std::ostringstream err;
				err << "Error. MySQLQuery readField error::  for row '" << (double)i << "', field '" << (double)j << "'."; 	
				errstring = err.str();
				retval = false;
			}
		} 
		return retval;
	}
	
	void MySQLQuery::reset() {
			if (result != nullptr) {
				s->free_result(result);
			}
			row = nullptr;
			start = nullptr;
			end = nullptr;
			result = nullptr;
			Query::reset();
	}

	bool MySQLQuery::execute() {
		bool retval = false;
		try {
			if (isactive) reset();
			int error = s->real_query(queryHandle, querystr.c_str(), querystr.length());
			if (error) {
				string errorstr = s->error(queryHandle);
				*Logger::log << Log::error << Log::LI << "Error. MySQLQuery execute error:" << errorstr << Log::LO; 
				*Logger::log << Log::LI << Log::info << Log::LI << "Query: " << querystr << Log::LO << Log::blockend; 
				*Logger::log << Log::LO << Log::blockend;
				row = nullptr;
				start = nullptr;
				result = nullptr;
				Query::reset();
			} else {
				result = s->store_result(queryHandle);
				prepareResult();
				retval = true;
			}
		}
		catch (...) {
			*Logger::log <<  Log::error << Log::LI << "Error. MySQLQuery execute error for query '" << querystr << "'." << Log::LO << Log::blockend; 				
		}
		return retval;
	}

	void MySQLQuery::prepareResult() {
		if (result != nullptr) {
			numRows = s->num_rows(result); //64 bit to 32 bit conversion...
			numFields = s->num_fields(result);
			if (numRows > 0) {
				start = s->row_tell(result);
				s->data_seek(result, numRows);
				end = s->row_tell(result);
				s->row_seek(result, start);
				isactive = true;
				prepareFieldCache();
			} else {
				reset();
			}
		} else {
			row = nullptr;
			start = nullptr;
			result = nullptr;
			Query::reset();
			isactive = false;
		}
	}

	void MySQLQuery::readFieldName(long long i, std::string& fname, std::string& tname) {
		if (isactive) {
			if (fieldRangeCheck(i)) {
				MYSQL_FIELD* field = s->fetch_field_direct(result,(unsigned int)(i - 1));
				if ( field != nullptr && field-> name != nullptr) {
					fname.assign(field->name);
					if (field->table != nullptr) tname.assign(field->table);
				}
			}
		} else {
			*Logger::log <<  Log::fatal << Log::LI << "MySQLQuery readFieldName:: Result set is not active to read field '" << fname << "'." << Log::LO << Log::blockend; 				
		}
	}
} // namespace Vdb

#endif //ALLOW
