/* 
 * convert.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * convert.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <string>
#include <sstream>
#include <iomanip>
#include <ios>


//for NAN
#include <math.h>
#include <cfloat>

#include "convert.h"

using namespace std;
namespace String {
	//---------------------------------------------------------------------------
	void strip(string& str) { //remove all tabs, etc.
		char const* delims = "\t\r\n";
		string::size_type wsi = str.find_first_of(delims);
		while ( wsi != string::npos ) {
			string::size_type wso = str.find_first_not_of(delims,wsi);
			str.erase(wsi,wso-wsi);
			wsi = str.find_first_of(delims);
		}
		trim(str);
	}
	//---------------------------------------------------------------------------
	void trim(string& str) {
		char const* delims = " \t\r\n";
		string::size_type  notwhite = str.find_first_not_of(delims);
		str.erase(0,notwhite);
		notwhite = str.find_last_not_of(delims);
		str.erase(notwhite+1);	
	}
	//---------------------------------------------------------------------------
	void ltrim(string& str) {
		char const* delims = " \t\r\n";
		string::size_type  notwhite = str.find_first_not_of(delims);
		str.erase(0,notwhite);
	}
	//---------------------------------------------------------------------------
	void rtrim(string& str) {
		char const* delims = " \t\r\n";
		string::size_type notwhite = str.find_last_not_of(delims);
		str.erase(notwhite+1);	
	}
	//---------------------------------------------------------------------------
	//this does fixedwidth ints, but overflowing ints will break the width.
	string tofixedstring(int w,int i) { // use a fixed width integer output - prefixed with 0 if needs be.
		ostringstream ost;
		ost.flags ( ios::right | ios::dec | ios::showbase | ios::fixed );
		ost.fill('0');
		ost << setw(w) << i;
		return ost.str();
	}
	//---------------------------------------------------------------------------
	string tostring(long long i) {
		ostringstream ost;
		ost << i;
		return ost.str();
	}
	//---------------------------------------------------------------------------
	string tostring(unsigned long long i) {
		ostringstream ost;
		ost << i;
		return ost.str();
	}
	//---------------------------------------------------------------------------
	string tostring(long double i,unsigned int precision) {
		string result;
		if ( i == -DBL_MAX || i == DBL_MAX) {
			result="NaN";
		} else {
			ostringstream ost;
			ost << fixed << setprecision(precision) << i;
			result = ost.str();
			if (result.compare("nan") == 0) result="NaN";
		}
		return result;
	}
	//---------------------------------------------------------------------------
	void tobase(long long p,unsigned int base,unsigned int digits,string& result) {
		ostringstream ost;
		result="0000000000000000000000000000000000000000000000000000000000000000"; //64 long.
		switch (base) {
			case 2: {
				unsigned long long q = p;
				unsigned long long t = 0x8000000000000000ULL; //LLONG_MIN would be better but doesn't always seem to exist.
				while (t > 0) {
					if ( q >= t ) {
						ost << "1" ;
						q = q - t;
					} else {
						ost << "0" ;
					}
					t = t >> 1;
				}
				if (digits != 0) {
					digits = digits <= 64 ? digits : 64;
					result.append(ost.str());
					result.erase(0,result.size()-digits);
				} else {
					result = ost.str();
					size_t nz = result.find('1');
					result.erase(0,nz);
				}
			} break;
			case 8: {
				//				ost << std::showbase << std::oct << p;
				ost << std::oct << p;
				if (digits != 0) {
					digits = digits <= 22 ? digits : 22;
					result.append(ost.str());
					result.erase(0,result.size()-digits);
				} else {
					result = ost.str();
				}
			} break;
			case 10: {
				//				ost << std::showbase << std::dec << p;
				ost << std::dec  << p;
				if (digits != 0) {
					digits = digits <= 20 ? digits : 20;
					result.append(ost.str());
					result.erase(0,result.size()-digits);
				} else {
					result = ost.str();
				}
			} break;
			case 16: {
				//				ost << std::showbase << std::hex << p;  //this does 0xfefe
				ost << std::hex << p;					//this does fefe
				if (digits != 0) {
					digits = digits <= 16 ? digits : 16;
					result.append(ost.str());
					result.erase(0,result.size()-digits);
				} else {
					result = ost.str();
				}
			} break;
		}
		if (result.empty()) {
			if (digits != 0) {
				result = string(digits,'0');
			} else {
				result = "0";
			}
		}		
	}
	//---------------------------------------------------------------------------
	void tobasestring(long double i ,unsigned int b,unsigned int d,string&r ) { 
		if (isnan(i)) {
			r =  "NaN";
		} else {
			long long int p = (long long int)roundl(i); 
			tobase(p,b,d,r);
		}
	} 
	//---------------------------------------------------------------------------
	void tostring(string& repository,int i) {
		ostringstream tmp;
		tmp << i;
		repository = tmp.str();
	}
	//---------------------------------------------------------------------------
	void tostring(string& repository,unsigned long long i) {
		ostringstream tmp;
		tmp << i;
		repository = tmp.str();
	}
	//---------------------------------------------------------------------------
	bool split(char splitter, const string& basis, pair<string,string>& result) {			   //Given a string, split it at the first given character (destroy the character)
		string::size_type pos = basis.find_first_of(splitter);
		if (pos == string::npos) {
			result.first = basis;
			return false;
		} else {
			result.first = basis.substr(0, pos);
			result.second = basis.substr(pos+1, string::npos);
			return true;
		}
	}
	//---------------------------------------------------------------------------
	bool rsplit(char splitter, const string& basis, pair<string,string>& result) {			   //Given a string, split it at the last given character (destroy the character)
		string::size_type pos = basis.find_last_of(splitter);
		if (pos == string::npos) {
			result.first = basis;
			return false;
		} else {
			result.first = basis.substr(0, pos);
			result.second = basis.substr(pos+1, string::npos);
			return true;
		}
	}

	// ------------------------------------------------------------------- 
	unsigned int natural(const string& s) {
		string::const_iterator in = s.begin();
		string::const_iterator out = s.end();
		unsigned int val = 0;
		while(in != out && isdigit(*in)) {
			val = val * 10 + *in - '0';
			in++;
		}
		return val;
	}
	// ------------------------------------------------------------------- 
	// this affects the iterator
	unsigned int natural(string::const_iterator& in) {
		unsigned int val = 0;
		while(isdigit(*in) ) {
			val = val * 10 + *in - '0';
			in++;
		}
		return val;
	}
	//-----------------------------------------------------------------------------
	pair<unsigned long long,bool> znatural(const string& s) { //Given a string, returns a natural from any digits that it STARTS with.
		bool isnumber = false;
		unsigned long long val = 0;
		if (!s.empty()) {
			if (s.size() > 2 && s[0]=='0' && ( s[1]=='x' || s[1]=='X' )) {
				pair<unsigned long long,bool> rsp = hex(s.substr(2));
				if (rsp.second) {
					isnumber= true;
					val = rsp.first;
				}
			} else {
				string::const_iterator in = s.begin();
				string::const_iterator out = s.end();
				while(in != out && isdigit(*in)) {
					isnumber= true;
					val = val * 10 + *in - '0';
					in++;
				}
			}
		}
		return pair<unsigned long long,bool>(val,isnumber);
	}
	
	//-----------------------------------------------------------------------------
	void tolist(vector<unsigned int>& ilist,const string &s) {
		ilist.clear();
		unsigned int i;
		char comma;
		istringstream ist(s);
		while (ist >> i) {
			ilist.push_back(i);
			ist >> comma;
		}
	}
	//---------------------------------------------------------------------------
	int integer(string::const_iterator& i) {
		bool minus = false;
		if (*i == '-') {
			minus = true; i++;
		} else if (*i == '+')
			i++;
		int val = 0;
		while (true) {
			if( *i >= '0' && *i <= '9' )
				val = val * 10 + ( *i - '0' );
			else
				break;
			i++;
		}
		if (minus)
			return -val;
		return val;
	}
	// ------------------------------------------------------------------- 
	pair<long long,bool> integer(const string& s) { //Given a string, returns a natural from any digits that it STARTS with.
		bool isnumber = false;
		long long val = 0;
		if (!s.empty()) {
			if (s.size() > 2 && s[0]=='0' && ( s[1]=='x' || s[1]=='X' )) {
				pair<long long,bool> rsp = hex(s.substr(2));
				if (rsp.second) {
					isnumber = true;
					val = rsp.first;
				}
			} else {
				istringstream ist(s);
				ist >> val;
				long long sti,ssj;
				sti = ist.tellg();
				ssj = s.length();
				if ((ist.eof() || (sti == ssj)) && !s.empty()) {
					isnumber = true;
				}
			}
		}
		return pair<long long,bool>(val,isnumber);
	}
	//---------------------------------------------------------------------------
	double real(string::const_iterator& x) {
		string::const_iterator y(x); //Start position as copy constructor.
		string valids("0123456789-+.");
		while( valids.find(*x) != string::npos ) x++;
		if ( x == y ) return NAN; //sqrt(static_cast<double>(-1.0));
		double i;
		istringstream ist(string(y,x));
		ist >> i;
		return i;
	}
	//---------------------------------------------------------------------------
	double real(const string& s) {
		double retval = NAN;
		if (!s.empty()) {
			if (s.size() > 2 && s[0]=='0' && ( s[1]=='x' || s[1]=='X' )) {
				pair<unsigned long long,bool> rsp = hex(s.substr(2));
				if (rsp.second) {
					retval = rsp.first;
				}
			} else { //true / false
				if (s[0] == 't' || s[0] == 'f') {
					if (!s.compare("true")) {
						retval = 1;
					} else {
						if (!s.compare("false")) {
							retval = 0;
						}
					}
				} else {
					string::const_iterator i = s.begin();
					retval = real( i );
				}
			}
		}
		return retval;
	}
	//---------------------------------------------------------------------------
	pair<unsigned long long,bool> hex(const string& s) { //Given a string, returns a natural from any hex that it STARTS with.
		bool isnumber = false;
		string::const_iterator in = s.begin();
		string::const_iterator out = s.end();
		if ( in != out ) isnumber = true;
		unsigned long long val = 0;
		bool ended = false;
		while (!ended && in != out) {
			switch (*in++) {
				case '0': val = val << 4; break;
				case '1': val = (val << 4) + 1; break;
				case '2': val = (val << 4) + 2; break;
				case '3': val = (val << 4) + 3; break;
				case '4': val = (val << 4) + 4; break;
				case '5': val = (val << 4) + 5; break;
				case '6': val = (val << 4) + 6; break;
				case '7': val = (val << 4) + 7; break;
				case '8': val = (val << 4) + 8; break;
				case '9': val = (val << 4) + 9; break;
				case 'a': val = (val << 4) + 10; break;
				case 'b': val = (val << 4) + 11; break;
				case 'c': val = (val << 4) + 12; break;
				case 'd': val = (val << 4) + 13; break;
				case 'e': val = (val << 4) + 14; break;
				case 'f': val = (val << 4) + 15; break;
				case 'A': val = (val << 4) + 10; break;
				case 'B': val = (val << 4) + 11; break;
				case 'C': val = (val << 4) + 12; break;
				case 'D': val = (val << 4) + 13; break;
				case 'E': val = (val << 4) + 14; break;
				case 'F': val = (val << 4) + 15; break;
				default: {
					isnumber = false;
					ended=true; 
					in--;
				} break;
			}
		}
		return pair<unsigned long long,bool>(val,isnumber);
	}
	//---------------------------------------------------------------------------
	size_t hex(string::const_iterator& i,double& val) {
		string::const_iterator j(i); //Start position as copy constructor.
		val = 0;
		bool ended = false;
		while (!ended) {
			switch (*i++) {
				case '0': val = val * 16; break;
				case '1': val = val * 16 + 1; break;
				case '2': val = val * 16 + 2; break;
				case '3': val = val * 16 + 3; break;
				case '4': val = val * 16 + 4; break;
				case '5': val = val * 16 + 5; break;
				case '6': val = val * 16 + 6; break;
				case '7': val = val * 16 + 7; break;
				case '8': val = val * 16 + 8; break;
				case '9': val = val * 16 + 9; break;
				case 'a': val = val * 16 + 10; break;
				case 'b': val = val * 16 + 11; break;
				case 'c': val = val * 16 + 12; break;
				case 'd': val = val * 16 + 13; break;
				case 'e': val = val * 16 + 14; break;
				case 'f': val = val * 16 + 15; break;
				case 'A': val = val * 16 + 10; break;
				case 'B': val = val * 16 + 11; break;
				case 'C': val = val * 16 + 12; break;
				case 'D': val = val * 16 + 13; break;
				case 'E': val = val * 16 + 14; break;
				case 'F': val = val * 16 + 15; break;
				default: ended=true; i--; break;
			}
		}
		if ( i == j ) val = NAN; //sqrt(static_cast<double>(-1.0)); //Illegal - not even 1 hexdigit
		return i - j;
	}
	// ------------------------------------------------------------------- 
	void todigits(string& s) {
		if (!s.empty()) {
			for (string::iterator i = s.begin(); i < s.end(); i++) {
				while (i != s.end() && ! isdigit(*i) ) { 
					s.erase(i);
				}
			}
		}
	}	
	//---------------------------------------------------------------------------
	void tolower(string& s) {
		for (string::iterator i = s.begin() ; i != s.end(); ++i)
			*i = std::tolower(*i);
	}

	void toupper(string& s) {
		for (string::iterator i = s.begin() ; i != s.end(); ++i)
			*i = std::toupper(*i);
	}
	
	//---------------------------------------------------------------------------
	bool isint(const string& s) {
		if (!s.empty()) { if (s.find_first_of("+-0123465789") == 0) return true; } return false;		
	}

	//---------------------------------------------------------------------------
	bool isfloat(const string &s) {
		float i;
		istringstream ist(s);
		ist >> i;
		return(!ist.fail());
	}		

	//---------------------------------------------------------------------------
	bool isdouble(const string &s) {
		double i;
		istringstream ist(s);
		ist >> i;
		return(!ist.fail());
	}		
	//---------------------------------------------------------------------------
	std::string fixUriSpaces(const std::string& uriString) {
		std::string s(uriString);
		if(s.empty()) return s;
		size_t pos = 0;
		while(pos < s.size() && isspace(s[pos])) ++pos;
		s.erase(0, pos);
		if(s.empty()) return s;
		pos = s.size();
		while(isspace(s[pos-1]) && pos > 0) --pos;
		if(pos != s.size()) s.erase(pos);
		pos = 0;
		while((pos = s.find(' ', pos)) != std::string::npos) {
			s.replace(pos, 1, "%20");
			pos += 2;
		}
		return s;
	}
/*	
	//---------------------------------------------------------------------------
	void UTF8toUCS4(vector<uint32_t>& dest,string& src) {
		static const char trailingBytesForUTF8[256] = {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
		};
		static const uint32_t offsetsFromUTF8[6] = {
			0x00000000UL, 0x00003080UL, 0x000E2080UL,
			0x03C82080UL, 0xFA082080UL, 0x82082080UL
		};	size_t srcsz=src.size();
		dest.reserve(srcsz+1);
		uint32_t ch;
		string::const_iterator i = src.begin();
		const string::const_iterator se = src.end();
		char nb;
		while (i < se) {
			nb = trailingBytesForUTF8[*i];
			if (i+nb >= se) {
				break;
			} else {
				ch = 0;
				switch (nb) {
					case 3: ch += *i++; ch <<= 6;
					case 2: ch += *i++; ch <<= 6;
					case 1: ch += *i++; ch <<= 6;
					case 0: ch += *i++;
				}
				ch -= offsetsFromUTF8[nb];
				dest.push_back(ch);
			}
		}
	}
	//---------------------------------------------------------------------------
	void UCS4toUTF8(vector<uint32_t>& src,string& dest) {
		for (vector<uint32_t>::const_iterator i = src.begin(); i < src.end(); i++) {
			dest.append(UCS4toUTF8((unsigned long long)*i));
		}
	}
*/	
	//---------------------------------------------------------------------------
	string UCS4toUTF8(unsigned long long ucs4) {
		string utf;
		if (ucs4 <= 0x7fULL) {
			utf.push_back(ucs4);
		} else if (ucs4 <= 0x7ffULL) {
			utf.push_back(0xc0 | (ucs4 >> 6));
			utf.push_back(0x80 | (ucs4 & 0x3f));
		} else if (ucs4 <= 0xffffULL) {
			utf.push_back(0xe0 | (ucs4 >> 12));
			utf.push_back(0x80 | ((ucs4 >> 6) & 0x3f));
			utf.push_back(0x80 | (ucs4 & 0x3f));
		} else if (ucs4 <= 0x001fffffULL) {
			utf.push_back(0xf0 | (ucs4 >> 18));
			utf.push_back(0x80 | ((ucs4 >> 12) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 6) & 0x3f));
			utf.push_back(0x80 | (ucs4 & 0x3f));
		} else if (ucs4 <= 0x03ffffffULL) {
			utf.push_back(0xf8 | (ucs4 >> 24));
			utf.push_back(0x80 | ((ucs4 >> 18) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 12) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 6) & 0x3f));
			utf.push_back(0x80 | (ucs4 & 0x3f));
		} else if (ucs4 <= 0x7fffffffULL) {
			utf.push_back(0xf8 | (ucs4 >> 30));
			utf.push_back(0x80 | ((ucs4 >> 24) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 18) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 12) & 0x3f));
			utf.push_back(0x80 | ((ucs4 >> 6) & 0x3f));
			utf.push_back(0x80 | (ucs4 & 0x3f));
		}
		return utf;
	}
	//---------------------------------------------------------------------------
	
} //namespace

