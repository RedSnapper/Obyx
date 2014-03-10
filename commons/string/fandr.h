/* 
 * fandr.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * fandr.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_STRING_FANDR_H
#define OBYX_STRING_FANDR_H

#include <string>

using namespace std;

	namespace String {
		unsigned const int MAX_MATCH = 9;		//maximum regexp matches
		int fandr(string&, const string);				    //find and replace with emptystring
		int fandr(string&, const string, unsigned int);  //find and replace with integer
		int fandr(string&, const string, const string, bool=true);    //find and replace with substitute string
		int fandr(string&, const string, unsigned long long);
		int fandr(string&, const char, const char);
		bool length(string const,unsigned long long&);	
		bool char_at(string const,unsigned long long,string&);
		bool position(string const,string const,unsigned long long&);	//return the character position of first string in second string.
		bool left(string const,long long,string&);
		bool right(string const,long long,string&);
		bool boundedleft(string&,size_t);
		bool reverse(string&);
		bool substring(string const,unsigned long long,unsigned long long,string&);
	}

#endif

