/* 
 * postgresqlquery.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * postgresqlquery.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <sstream>
#include "commons/logger/logger.h"

#include "commons/vdb/service.h"
#include "commons/vdb/connection.h"
#include "commons/vdb/query.h"
#include "postgresqlquery.h"
#include "postgresqlconnection.h"
#include "postgresqlservice.h"

namespace Vdb {
	
	PostgreSQLQuery::PostgreSQLQuery(PostgreSQLService* s_in, PostgreSQLConnection* c_in, PGconn*& conn_in, std::string& qstr) :
	Query(qstr),s(s_in),c(c_in),connectionHandle(conn_in),res(nullptr),current_row(0) {
	}
	
	PostgreSQLQuery::~PostgreSQLQuery() {
		reset();
		Query::reset();
	}
	
	void PostgreSQLQuery::reset() {
		numRows = 0;
		numFields = 0;
		current_row = 0;
		if (res != nullptr) {
			s->PQclear(res);
			res = nullptr;
		}
	}
	
	bool PostgreSQLQuery::execute() {
		bool retval = false;
		numRows = 0;
		numFields = 0;
		string errstring;
		if (isactive) reset();
		res = s->PQexec(connectionHandle, querystr.c_str());
		ExecStatusType status = s->PQresultStatus(res);
		switch (status) {
			case PGRES_COMMAND_OK: { /* non-result query */
				reset();
				retval = true;
			} break;
			case PGRES_TUPLES_OK: { /* result query */
				retval = true;
			} break;
			case PGRES_EMPTY_QUERY:{ /* empty query! */
				errstring = "The query was empty.";
			} break;
			case PGRES_COPY_OUT:{ /* Copy Out data transfer in progress */
				errstring = "Copy Out data transfer in progress.";
			} break;
			case PGRES_COPY_IN:{ /* Copy In data transfer in progress! */
				errstring = "Copy In data transfer in progress.";
			} break;
			case PGRES_BAD_RESPONSE:{ /* empty query! */
				errstring = s->PQerrorMessage(connectionHandle);
				if (errstring.empty()) {
					errstring = "An unexpected response was received from the backend.";
				}
			} break;
			case PGRES_NONFATAL_ERROR:{ /* empty query! */
				errstring = s->PQerrorMessage(connectionHandle);
				if (errstring.empty()) {
					errstring = "An unknown non-fatal error occured.";
				}
			} break;
			case PGRES_FATAL_ERROR:{ /* empty query! */
				errstring = s->PQerrorMessage(connectionHandle);
				if (errstring.empty()) {
					errstring = "An unknown fatal error occured.";
				}
			} break;
		}
		if (! errstring.empty() ) {
			*Logger::log << Log::error << Log::LI << "Error. PostgreSQLQuery execute error:" << errstring << Log::LO; 
			*Logger::log << Log::LI << Log::info << Log::LI << "Query: " << querystr << Log::LO << Log::blockend; 
			*Logger::log << Log::LO << Log::blockend;
			reset();
			Query::reset();
		} else {
			prepareResult();
			retval = true;
		}
		return retval;
	}
	
	void PostgreSQLQuery::prepareResult() {
		if (res != nullptr) {
			numRows = (long long)s->PQntuples(res); //64 bit to 32 bit conversion...
			numFields = (long long)s->PQnfields(res);
			isactive = true;
			prepareFieldCache();
		} else {
			res = nullptr;
			Query::reset();
			isactive = false;
		}
	}
	
	void PostgreSQLQuery::readFieldName(long long i, std::string& fname, std::string& tname) {
		unsigned int idx = (unsigned int)i - 1; //PostgreSQL is zero indexed.
		if (isactive && res != nullptr && fieldRangeCheck(i)) {
			fname = s->PQfname(res,idx); 
			unsigned int table = (unsigned int) s->PQftable(res,idx);
			ostringstream tstr;
			tstr << table;
			tname = tstr.str();	//this is an integer...
		} else {
			*Logger::log <<  Log::fatal << Log::LI << "PostgreSQLQuery readFieldName:: Result set is not active to read field '" << fname << "'." << Log::LO << Log::blockend; 				
		}
	}
	
	//j=row index, i=field index. 
	bool PostgreSQLQuery::readfield(long long j, long long i, std::string& readString, std::string& errstring) {
		unsigned int row_idx = (unsigned int)j - 1;
		unsigned int idx = (unsigned int)i - 1;  //PostgreSQL is zero indexed.
		bool retval = false;
		const char* val = s->PQgetvalue(res,row_idx,idx);
		if (val != nullptr) {
			readString = val;
			retval = true;
		} else {
			char *buff = s->PQresultErrorMessage(res);
			if (buff != nullptr) {
				errstring = buff;
			} else {
				errstring = "field not found";
			}
		}
		return retval;
	}
	
} // namespace
#endif //allow


