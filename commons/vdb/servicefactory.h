/* 
 * servicefactory.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * servicefactory.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_VDB_SERVICE_FACTORY_H
#define OBYX_VDB_SERVICE_FACTORY_H

#include <string>
#include <map>

/*
 ServiceFactory is a Singleton Factory pattern. It initialises a set of Services, and then handles
 requests for Service instances. 

 A Service here is a database CLIENT service - used to load up what is necessary to connect to a SERVER.
 A service is identified by a 'name' signature -- eg a mysql client service could be 'mysql'
 
 Service models are added to the factory during initialisation..
 */
using namespace std;

namespace Vdb {
	class Service;
	
	class ServiceFactory {
private:
		static unsigned int instances;				//used to deal with multiple startup/shutdowns..
		static ServiceFactory* singleton;
		typedef std::map<std::string, Service*> ServiceMap;
		ServiceMap serviceMap;
		ServiceFactory(bool);
		~ServiceFactory();
				
public: 
		static ServiceFactory*& startup(bool = true);
		static void shutdown();
		Service* getService(const std::string&);
		
	};
	
}

#endif
