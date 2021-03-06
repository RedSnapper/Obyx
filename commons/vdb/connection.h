/* 
 * connection.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * connection.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_VDB_CONNECTION_H
#define OBYX_VDB_CONNECTION_H

#include <string>
#include <set>
#include <unordered_map>
#include <map>

#include "commons/string/strings.h"

//using namespace __gnu_cxx; //hashmap namespace.
using std::string;

namespace Vdb {

	class Query;
	
	class Connection {

private:
public:
		bool connect();
		virtual bool open(const std::string&, const std::string&,const unsigned int, const std::string&)=0;
		virtual bool open(const std::string&)=0;
		virtual bool database(const std::string&) = 0;
		virtual bool dbselected()=0; //if the database is selected we can instantiate queries.
		virtual bool isopen()=0;
		virtual void list()=0;
		virtual void close()=0;
		virtual Query* query(std::string = "")=0;
		virtual bool query(Query*&,std::string = "")=0;
		virtual void escape(std::string&) =0;
		Connection();
		virtual ~Connection();
	};
	
} // namespace Vdb

#endif // RS_VDB_CONNECTION_H

