/* 
 * filer.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * filer.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_FILER_H
#define OBYX_CONTEXT_FILER_H

#include <string>
#include "dataitem.h"

using namespace std;
using namespace obyx;

namespace Filer {
	bool getfile(string,string&,string&);
	void output(const string,kind_type = di_object);
	void defaultHTTPHeader(bool);
	void outputRedirect();
}

#endif

