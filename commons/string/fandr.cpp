/* 
 * fandr.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * fandr.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "fandr.h"
#include "convert.h"

using namespace std;
namespace String {

	//	fandr(s,"&amp;","&"); size_t -- returns number of replaces.
	//• --------------------------------------------------------------------------
	int fandr(string& source, const string basis, const string replace, bool all) {
		int rep_count = 0;
        size_t bsize = basis.length();
        if ( ( source.length() >= bsize ) && ( bsize > 0 ) ) {
			size_t rsize = replace.length();
			size_t spos = source.find(basis);
			if (all) {
				while ( spos != string::npos ) {
					source.replace(spos,bsize,replace); 
					rep_count++;
					spos = source.find(basis,spos+rsize);
				}
			} else {
				if (spos != string::npos) {
					source.replace(spos,bsize,replace); 
					rep_count++;
				}
			}
        }
		return rep_count;
	}
	//• --------------------------------------------------------------------------
	int fandr(string& source, const char basis, const char replace) {
		int rep_count = 0;
		string::iterator s1=source.begin();
		string::iterator s2=source.end();
		while ( s1 < s2 ) {
			if (*s1 == basis) {
				*s1=replace;
			}
			s1++;
		}
		return rep_count;
	}
	//• --------------------------------------------------------------------------
	int fandr(string &t, const string s, unsigned long long r) {
		string intval;
		tostring(intval,r);
		return fandr(t,s,intval,true);
	}
	
	//• --------------------------------------------------------------------------
	int fandr(string &source,const string basis) {
//		return fandr(source,basis,"",true);
		int rep_count = 0;
        size_t bsize = basis.length();
        if ( ( source.length() >= bsize ) && ( bsize > 0 ) ) {
			size_t spos = source.find(basis);
			while ( spos != string::npos ) {
				source.erase(spos,bsize); 
				rep_count++;
				spos = source.find(basis,spos);
			}
        }
		return rep_count;
	}
	//• --------------------------------------------------------------------------
	bool position(string const searchtext,string const context,unsigned long long& result) { 
		bool retval = false;
		result=0;
		size_t i = context.find(searchtext);
		if ( i < string::npos ) {
			retval = true;
			string::const_iterator l1=context.begin()+i;
			string::const_iterator f1=context.begin();
			while(f1 < l1) {
				if ((*f1 & 0x80) == 0x00) {
					f1++; result++;
				} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
					f1 += 2; result++;
				}
				else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
					f1 += 3; result++;
				} else {
					return false; // illegal utf-8
				}
			}
		}
		return retval;
	}
	//• --------------------------------------------------------------------------
	bool char_at(string const string_to_measure,unsigned long long pos,string& result) { 
		bool retval = true;
		result="";
		string::const_iterator f1=string_to_measure.begin();
		string::const_iterator l1=string_to_measure.end();
		while (f1 != l1 && pos > 0) {
			if ((*f1 & 0x80) == 0x00) {
				pos--;
				if (pos == 0) {
					result += *f1++;
				} else {
				  f1++;
				}
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				pos--;
				if (pos == 0) {
					result += *f1++;
					result += *f1++;
				} else {
				  f1 += 2;
				}
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				pos--;
				if (pos == 0) {
					result += *f1++;
					result += *f1++;
					result += *f1++;
				} else {
					f1 += 3;
				}
			} else {
				return false; // illegal utf-8
			}
		}
		return retval;
	}
	//• --------------------------------------------------------------------------
	bool length(string const string_to_measure,unsigned long long& result) { 
		bool retval = true;
		result = 0;
		string::const_iterator f1=string_to_measure.begin();
		string::const_iterator l1=string_to_measure.end();
		while (f1 != l1) {
			if ((*f1 & 0x80) == 0x00) {
				f1++;
				result++;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2;
				result++;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3;
				result++;
			} else {
				return false; // illegal utf-8
			}
		}
		return retval;
	}
	//• --------------------------------------------------------------------------
	bool reverse(string& base) { 
		string result;
		string::const_iterator i=base.begin();
		const string::const_iterator n = base.end();
		while ( i != n) {
			if ((*i & 0x80) == 0x00) {
				result.insert(result.begin(),*i);
				i++; 
			}
			else if ((*i & 0xe0) == 0xc0 && i + 1 != n && (i[1] & 0xc0) == 0x80) {
				result.insert(result.begin(),i,i+2);
				i += 2;
			}
			else if ((*i & 0xf0) == 0xe0 && i + 1 != n && i + 2 != n && (i[1] & 0xc0) == 0x80 && (i[2] & 0xc0) == 0x80) {
				result.insert(result.begin(),i,i+3);
				i += 3;
			}
			else {
				return false;
			}
		}
		base=result;
		return true;
	}
	//• --------------------------------------------------------------------------
	bool left(string const string_to_cut,long long numchars,string& result) { 
		if (numchars < 0) { 			//We want the length of the string, minus p2 for our leftstring.
			int charcount = 0;
			string::const_iterator f1=string_to_cut.begin();
			string::const_iterator l1=string_to_cut.end();
			for (; f1 != l1; ) {
				if ((*f1 & 0x80) == 0x00) {
					f1++; charcount++;
				}
				else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
					f1 += 2; charcount += 1;
				}
				else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
					f1 += 3; charcount += 1;
				}
				else {
					return false;
				}
			}
			numchars = charcount + numchars;		//remember, numchars is a negative value here!
		}
		string::const_iterator f1=string_to_cut.begin();
		string::const_iterator l1=string_to_cut.end();
		while((f1 != l1) && numchars > 0) {
			if ((*f1 & 0x80) == 0x00) {
				f1++;
				numchars--;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2; numchars--;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3; numchars--;
			} else {
				return false;
			}
		}
		result = std::string(string_to_cut.begin(),f1);
		return true;
	}
	//• --------------------------------------------------------------------------
	bool right(string const string_to_cut,long long numchars,string& result) {
		if (numchars < 0) { 			//We want the length of the string, minus p2 for our leftstring.
			int charcount = 0;
			string::const_iterator f1=string_to_cut.begin();
			string::const_iterator l1=string_to_cut.end();
			for (; f1 != l1; ) {
				if ((*f1 & 0x80) == 0x00) {
					f1++; charcount++;
				}
				else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
					f1 += 2; charcount += 1;
				}
				else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
					f1 += 3; charcount += 1;
				}
				else {
					return false;
				}
			}
			numchars = charcount + numchars;		//remember, numchars is a negative value here!
		}
		long long charcount = 0;
		long long lcount = 0;
		string::const_iterator f1=string_to_cut.begin();
		string::const_iterator l1=string_to_cut.end();
		while ( f1 != l1 ) {
			if ((*f1 & 0x80) == 0x00) {
				f1++; charcount++;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2; charcount++;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3; charcount++;
			} else {
				return false;
			}
		}
		lcount = charcount-numchars;
		if (lcount < 0) lcount = 0;
		f1=string_to_cut.begin();
		while((f1 != l1) && lcount > 0) {
			if ((*f1 & 0x80) == 0x00) {
				f1++; lcount--;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2; lcount--;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3; lcount--;
			} else {
				return false;
			}
		}
		result = std::string(f1,l1);
		return true;
	}
	//• --------------------------------------------------------------------------
	bool substring(string const string_to_cut,unsigned long long place_to_start,unsigned long long num_chars_to_keep,string& result) {
		string::const_iterator f1=string_to_cut.begin();
		string::const_iterator l1=string_to_cut.end();
		while((f1 != l1) && place_to_start > 0) {
			if ((*f1 & 0x80) == 0x00) {
				f1++; place_to_start--;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2; place_to_start--;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3; place_to_start--;
			} else {
				return false;
			}
		}
		string::const_iterator s1(f1);
		while((f1 != l1) && num_chars_to_keep > 0) {
			if ((*f1 & 0x80) == 0x00) {
				f1++; num_chars_to_keep--;
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				f1 += 2; num_chars_to_keep--;
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				f1 += 3; num_chars_to_keep--;
			} else {
				return false;
			}
		}
		result = string(s1,f1);
		return true;
	}
	//• --------------------------------------------------------------------------
}	//  Namespace String

