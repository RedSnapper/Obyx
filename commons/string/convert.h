/* 
 * convert.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * convert.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_STRING_CONVERT_H
#define OBYX_STRING_CONVERT_H

#include <string>
#include <vector>

using namespace std;

namespace String {
	void strip(string& str);
	void trim(string& str);
	void rtrim(string& str);
	void ltrim(string& str);
	string tofixedstring(int,int);
	string tostring(unsigned long long);
	string tostring(long long);
	string tostring(long double,unsigned int=0);
	void tobase(long long,unsigned int,unsigned int,string&);
	void tobasestring(long double,unsigned int,unsigned int,string& );
//	void tobasestring(long double,unsigned int,string&);
	void tostring(string&,int);				 //Given an integer, return a string. e.g. 3 => "3"
	void tostring(string&,unsigned long long);	//Given a natural, return a string. e.g. 3 => "3"
	bool split(char, const string&, pair<string,string>&);	   //Given a string, split it at the first given character (destroy the character)
	bool rsplit(char, const string&, pair<string,string>&);	   //Given a string, split it at the last given character (destroy the character)

	pair<unsigned long long,bool> hex(const string&);		   //Given a string, returns a natural from any hex that it STARTS with.
	pair<unsigned long long,bool> znatural(const string&);     //Given a string, returns a natural from any digits that it STARTS with.

	string			UCS4toUTF8(unsigned long long ucs4);
	void			todigits(string&);							//strip all but digits from a string.
	unsigned int	natural(const string&);						//Given a const string, returns a natural  1... +BIGINT
	unsigned int	natural(string::const_iterator&);			//Given a const string, returns a natural  1... +BIGINT 
	pair<long long,bool>  integer(const string&);				//Given a string, returns an integer -BIGINT ... 0 ... +BIGINT and a flag saying if it is a number at all.
	int				integer(string::const_iterator&);			//Given a const string, returns an integer -BIGINT ... 0 ... +BIGINT
	double			real(string::const_iterator&);				//Given a string, returns a double.
	double			real(const string&);						//Given a const string, returns a double (floating point).
	size_t			hex(string::const_iterator&,double&); 		//input e.g. x32dda outputs a hex double. error needs NaN returns bytes used		
	void			tolist(vector<unsigned int>&,const string &); //given a comma delimited set of naturals, returns a vector of naturals
	void			tolower(string&);							//Given an mixed case string, return it in lower case. e.g. "ThIs" => "this"
	void			toupper(string&);							//Given an mixed case string, return it in lower case. e.g. "ThIs" => "this"
	bool			isint(const string&);
	bool			isfloat(const string &);
	bool			isdouble(const string &);
	string			fixUriSpaces(const string& uriString);

}

#endif

