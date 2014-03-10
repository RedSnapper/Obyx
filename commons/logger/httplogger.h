/* 
 * httplogger.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httplogger.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef httplogger_h
#define httplogger_h

#include "logger.h"

#include <iostream>
#include <string>

using namespace std;
using namespace Log;

// class for http reporting  
class HTTPLogger : public Logger {
private:
	static bool minititle;						//for show/hide
	bool topped,tailed;							//top just once.
	
protected:
	friend class Logger;
	HTTPLogger();// : Logger(1),topped(false),tailed(false) {}
	~HTTPLogger() {}
	virtual void wrap(bool);
	virtual void extra(extratype); 
	virtual void open();
	virtual void close();
	virtual void strip(string&);		//strip container..
	virtual void bracket(bracketing);
	virtual void dofatal(std::string);
	virtual void ltop(string&,bool,bool=false);	//top log document
	virtual void ltail(string&,bool=false);		//tail log document
	
};

#endif
