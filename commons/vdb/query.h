/* 
 * query.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * query.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_VDB_QUERY_H
#define OBYX_VDB_QUERY_H

#include <vector>
#include <map>
#include <string>

#include "connection.h"

namespace Vdb {
	
	class Query {
protected:
		friend class Connection;

protected:
		std::string querystr;
		bool isactive;
		long long numRows;
		long long numFields;
		typedef std::map<std::string,unsigned long long> nameIndexMap;
		nameIndexMap fieldnameidx;								//indexes of fieldnames (for name->number lookup)
		std::vector<std::string> fieldnames;					//implicitly ordered by field number (for number->name lookup)

		bool rowRangeCheck(long long);
		bool fieldRangeCheck(long long);
		bool rowFieldRangeCheck(long long, long long);
		
		virtual void reset();									// reset the query		--used by vdb
		virtual bool readfield(long long, long long, std::string&, std::string&)=0;		//
		virtual void readFieldName(long long i, std::string&, std::string&)=0;	
		void prepareFieldCache();
		
public:
		bool readfield(long long, const std::string&,std::string&, std::string&);	
		bool hasfield(const std::string&);	
		bool findfield(const std::string&);	
		void fieldkeys(const string&,set<string>&);
		const std::string getquery() const { return querystr; }	
		long long getnumrows() const { return numRows; }	
	    const std::vector<std::string>& fieldlist() const { return fieldnames;}
		virtual bool execute()=0;
		Query(std::string = "");
		virtual ~Query();
	};

} // namespace Vdb

#endif // RS_VDB_QUERY_H

