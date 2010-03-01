/* 
 * service.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * service.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_VDB_SERVICE_H
#define OBYX_VDB_SERVICE_H

#include <string>


namespace Vdb {
	class ServiceFactory;
	class Connection;

/*
	 A Service is a database CLIENT service - used to load up what is necessary to connect to a SERVER.
	 A service is identified by a 'name' signature -- eg a mysql client service could be 'mysql'.
	 Service models are added to the ServiceFactory during factory initialisation.
	 All Services are accessed from a ServiceFactory.
	 Responsibilities of Service are to ((maintain a connection factory)) for a given service..
	 Responsibilities of Service are to supply Connection instances for a given service..
	 Service->Connection* instance(); and then c->open(host, etc...); to connect to a server.
*/
	class Service {
	protected:
			friend class ServiceFactory;
			bool service;	//used to show if the service is up or down.
			Service() : service(false) {}
			virtual ~Service() {}

	public:
			//Connection instance. This doesn't connect (use open for that), just sets up ready to connect (eg creates a handle)...
			virtual Connection* instance() =0;
	};
}

#endif
