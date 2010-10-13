/* 
 * strings.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * strings.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_STRING_H
#define OBYX_STRING_H

#include "xercesc/util/Xerces_autoconf_config.hpp"
#include "comparison.h"
#include "convert.h"
#include "fandr.h"
#include "translate.h"
#include "regex.h"
#include "crypto.h"
#include "chars.h"

using namespace std;
using namespace __gnu_cxx; //hashmap namespace.

#define UCS2(x) (const XMLCh*)(x)
namespace {
	typedef std::basic_string<XMLCh> u_str;
}	

namespace __gnu_cxx {	//used in various places..
	
	template<> struct hash<const string& > {
		size_t operator()(const string& s) const { unsigned long h = 0	; for (string::const_iterator a = s.begin() ; a != s.end() ; h = 5*h + *a++); return size_t(h); }
	};

	template<> struct hash<const u_str& > {
		size_t operator()(const u_str& s) const { unsigned long h = 0	; for (u_str::const_iterator a = s.begin() ; a != s.end() ; h = 5*h + *a++); return size_t(h); }
	};
	
} //namespace __gnu_cxx

#include "infix.h"

#endif
