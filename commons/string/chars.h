/* 
 * chars.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * chars.h is a part of Obyx - see http://www.obyx.org
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

#ifndef OBYX_Chars_H
#define OBYX_Chars_H

#include <map>
#include <string>

using namespace std;

//These classes are used to test if a given utf-8 byteset (of 1-3 characters, held in an unsigned long)
//belongs to that group.  see http://www.w3.org/TR/xml/#CharClasses for an idea of each list.
class XMLChar {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
	static void convert(std::string &);
	static void encode(std::string &);
};

class Space {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};

class Digit {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};

class Extender {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};

class CombiningChar {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};


class BaseChar {
private:
	static map<unsigned long, char> Lo;
	static map<unsigned long, char> Hi;
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};


class Ideographic {
public:	
	static void startup();
	static void shutdown();
	static bool is(unsigned long);
};

class Letter {
public:	
	static bool is(unsigned long k) { return Ideographic::is(k) || BaseChar::is(k); } 
};

#endif

