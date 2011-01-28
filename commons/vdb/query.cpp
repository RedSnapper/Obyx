/* 
 * query.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * query.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <sstream>
#include "query.h"

#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include "commons/logger/logger.h"

namespace Vdb {
	using std::operator<<;

	
	
	Query::Query(std::string q) : querystr(q),isactive(false),numRows(-1),numFields(-1),fieldnameidx(),fieldnames() {	}
	
	Query::~Query(){
		fieldnameidx.clear();
		fieldnames.clear();
	}

	void Query::reset() {
		querystr.clear();
		numRows = -1;
		numFields = -1;
		fieldnameidx.clear();
		fieldnames.clear();
		isactive = false;
	}
/*
	void Query::setquery(const std::string& newquerystr) {
		numRows = -1;
		numFields = -1;
		isactive = false;
		fieldnameidx.clear();
		fieldnames.clear();
		querystr = newquerystr;
	}

	bool Query::fieldname(unsigned long long idx,std::string& result) {
		if (isactive && idx <= fieldnames.size() ) {
			result = fieldnames[(unsigned long)(idx-1)];
			return true;
		} else {
			return false;
		}
	}
*/
	bool Query::fieldRangeCheck(long long i) { 
		bool retval=true;
		if (i < 1 || i > numFields) {
			retval=false;
			*Logger::log <<  Log::fatal << Log::LI << "Query range error:: " << "Field index '" << (double)i <<"' out of range" << Log::LO << Log::blockend; 
		}
		return retval;
	}
	
	bool Query::rowRangeCheck(long long i) {
		bool retval=true;
		if (i < 1 || i > numRows) {
			retval=false;
			*Logger::log <<  Log::fatal << Log::LI << "Query range error:: " << "Row index '" << (double)i <<"' out of range" << Log::LO << Log::blockend; 
		} 
		return retval;
	}
	
	bool Query::rowFieldRangeCheck(long long i, long long j) {
		return (rowRangeCheck(i) &&	fieldRangeCheck(j));
	}
	

	bool Query::findfield(const std::string& pattern) {
		bool retval = false;
		if (isactive) {
			if ( String::Regex::available() ) {
				for(nameIndexMap::iterator imt = fieldnameidx.begin(); !retval && imt != fieldnameidx.end(); imt++) {
					retval= String::Regex::match(pattern,imt->first);
				}
			} else {
				nameIndexMap::iterator it = fieldnameidx.find(pattern);
				if (it != fieldnameidx.end()) {
					retval = true;
				}
			}
		}
		return retval;
	}

	void Query::fieldkeys(const std::string& pattern,set<string>& keylist) {
		if (isactive) {
			if ( String::Regex::available() ) {
				for(nameIndexMap::iterator imt = fieldnameidx.begin(); imt != fieldnameidx.end(); imt++) {
					if (String::Regex::match(pattern,imt->first)) {
						keylist.insert(imt->first);
					}
				}
			} else {
				nameIndexMap::iterator it = fieldnameidx.find(pattern);
				if (it != fieldnameidx.end()) {
					keylist.insert(pattern);
				}
			}
		}
	}
	
	bool Query::hasfield(const std::string& field) {
		bool retval = false;
		if (isactive) {
			nameIndexMap::iterator it = fieldnameidx.find(field);
			if (it != fieldnameidx.end()) {
				retval = true;
			} 
		}
		return retval;
	}

	bool Query::readfield(long long i,const std::string& field, std::string& readString, std::string& errstring ) {
		bool retval = false;
		if (isactive) {
			nameIndexMap::iterator it = fieldnameidx.find(field);
			if (it != fieldnameidx.end()) {
				retval = readfield( i, it->second, readString, errstring);
			} else {
				size_t hashpos = field.find('#');
				string tmpval;
				readString = field;
				while (hashpos != string::npos) {
					if (readString.compare(hashpos,9,"#rowcount") == 0) {
						String::tostring(tmpval,(unsigned long long)numRows);
						readString.replace(hashpos,9,tmpval);
						retval = true; 
					} 
					if ( readString.compare(hashpos,4,"#row") == 0 ) {
						String::tostring(tmpval,(unsigned long long)i);
						readString.replace(hashpos,4,tmpval);
						retval = true;
					}
					if ( readString.compare(hashpos,11,"#fieldcount") == 0 ) {
						String::tostring(tmpval,(unsigned long long)numFields);
						readString.replace(hashpos,11,tmpval);
						retval = true;
					}
					hashpos = readString.find('#',hashpos);
				}
				if (!retval) {
					readString = "";
				}
			}
		}
		return retval;
	}
		
	void Query::prepareFieldCache() {
		if (isactive) {
			for(long long i = 1; i <= numFields; i++) {
				std::string fname,tname;
				std::ostringstream tfname;
				readFieldName(i, fname, tname);
				tfname <<  tname << '.' << fname;
				nameIndexMap::const_iterator ni = fieldnameidx.find(fname);
				if ( ni != fieldnameidx.end() ) {
					*Logger::log << Log::fatal << Log::LI << "SQL Error: Field " << fname << " is ambiguous in the query " << querystr << Log::LO << Log::blockend; 
				} else {
					fieldnameidx.insert(nameIndexMap::value_type(fname, i));
					fieldnameidx.insert(nameIndexMap::value_type(tfname.str(), i));
					fieldnames.push_back(fname);
				}
			}
		} else {
			*Logger::log << Log::fatal << Log::LI << "VDB::Query::prepareFieldCache FieldCaches cannot be prepared for inactive queries." << Log::LO << Log::blockend; 
		}
	}
} // namespace
