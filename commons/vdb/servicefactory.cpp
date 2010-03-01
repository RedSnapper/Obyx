/* 
 * servicefactory.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * servicefactory.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "servicefactory.h"
#include "service.h"
#include "commons/logger/logger.h"

#ifdef ALLOW_MYSQL
#include "mysql/mysqlservice.h"
#endif
#ifdef ALLOW_POSTGRESQL
#include "postgresql/postgresqlservice.h"
#endif
namespace Vdb {

	ServiceFactory* ServiceFactory::singleton = NULL;
	unsigned int ServiceFactory::instances = 0;

	ServiceFactory::ServiceFactory(bool fatal_necessity) : serviceMap() {
#ifdef ALLOW_MYSQL
		MySQLService* mysql = new MySQLService(fatal_necessity);
		if ( mysql->service ) { 
			serviceMap.insert(ServiceMap::value_type("mysql", mysql));
		} else {
			delete mysql; //service not available.
			if (fatal_necessity) {
				*Logger::log <<  Log::fatal << Log::LI << "MySQLService failed to be installed." << Log::LO << Log::blockend; 
			}
		}
#endif
#ifdef ALLOW_POSTGRESQL
		PostgreSQLService* postgresql = new PostgreSQLService(fatal_necessity);
		if ( postgresql->service ) { 
			serviceMap.insert(ServiceMap::value_type("postgresql", postgresql));
		} else {
			delete postgresql; //service not available.
			if (fatal_necessity) {
				*Logger::log <<  Log::fatal << Log::LI << "MySQLService failed to be installed." << Log::LO << Log::blockend; 
			}
		}
#endif
		if (serviceMap.empty() && fatal_necessity) {
			*Logger::log <<  Log::fatal << Log::LI << "No SQLServices were installed.";
#ifndef ALLOW_MYSQL
			*Logger::log  << " ALLOW_MYSQL was not defined at build time.";
#endif
#ifndef ALLOW_POSTGRESQL
			*Logger::log  << " ALLOW_POSTGRESQL was not defined at build time.";
#endif
			*Logger::log<< Log::LO << Log::blockend; 
		}
		
	}

	ServiceFactory::~ServiceFactory() {
		for(ServiceMap::iterator it = serviceMap.begin(); it != serviceMap.end(); it++) {
			delete it->second;
		}
		serviceMap.clear();
	}

	ServiceFactory*& ServiceFactory::startup(bool fatal_necessity) { //fatal_necessity = true if we MUST have sql to operate.
		instances++;
		if (singleton == NULL) {
			singleton = new ServiceFactory(fatal_necessity);	// instantiate singleton
		} 
		return singleton;	
	}

	void ServiceFactory::shutdown() {
		instances--;
		if (instances == 0) {
			delete singleton;
		}
	}

	Service* ServiceFactory::getService(const std::string& service_name) {
		Service* retval = NULL;
		ServiceMap::iterator it = serviceMap.find(service_name);
		if (it != serviceMap.end()) {
			retval = it->second;
		}
		return retval;
	}

}

