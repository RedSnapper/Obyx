/*
 * translate.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * translate.cpp is a part of Obyx - see http://www.obyx.org .
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


// see RFC 1738 for URL spec.
#include <string>
#include <sstream>

#include "translate.h"
#include "fandr.h"
#include "convert.h"

using namespace std;

namespace String {

	// http://www.ietf.org/rfc/rfc1521.txt (actually made redundant by)
	// http://tools.ietf.org/html/rfc2045
	bool qpdecode(string& quoted_printable,const string CRLF) {
		bool retval = false;
		vector<string> qplines;
		string::size_type pos = quoted_printable.find(CRLF);
		while (pos != string::npos) {
			qplines.push_back(quoted_printable.substr(0, pos));
			quoted_printable.erase(0,pos+2);
			pos = quoted_printable.find(CRLF);
		}
		if (!quoted_printable.empty()) {
			qplines.push_back(quoted_printable);
			quoted_printable.clear();
		}
		char const* lwsp = " \t";
		for (size_t i=0; i < qplines.size(); i++) {
			size_t terminating = qplines[i].find_last_not_of(lwsp);
			qplines[i].erase(terminating+1);
			
			//manage hard/soft line-breaks.
			if (!qplines[i].empty()) {
				if (*(qplines[i].end()-1) != '=') {
					qplines[i].append(CRLF);
				} else {
					qplines[i].resize(qplines[i].size() - 1);
				}
			} else {
				qplines[i]=CRLF;
			}
			size_t x = qplines[i].find('=');
			while (x != string::npos) {
				string code,err;
				code.push_back(qplines[i][x+1]);
				code.push_back(qplines[i][x+2]);
				if (fromhex(code,err)) {
					qplines[i].replace(x,3,code);
				} else {
					retval = false;
				}
				x = qplines[i].find('=',x+1);
			}
			quoted_printable.append(qplines[i]);
		} 
		qplines.clear();
		return retval;
	}
/*	
	quoted-printable := qp-line *(CRLF qp-line)
	
	qp-line := *(qp-segment transport-padding CRLF)
	qp-part transport-padding
	
	qp-part := qp-section
	; Maximum length of 76 characters
	
	qp-segment := qp-section *(SPACE / TAB) "="
	; Maximum length of 76 characters
	
	qp-section := [*(ptext / SPACE / TAB) ptext]
	
	ptext := hex-octet / safe-char
	
	safe-char := <any octet with decimal value of 33 through
	60 inclusive, and 62 through 126>
	; Characters not listed as "mail-safe" in
	; RFC 2049 are also not recommended.
	
	hex-octet := "=" 2(DIGIT / "A" / "B" / "C" / "D" / "E" / "F")
	; Octet must be used for characters > 127, =,
		; SPACEs or TABs at the ends of lines, and is
		; recommended for any character not listed in
			; RFC 2049 as "mail-safe".
	
	transport-padding := *LWSP-char
	; Composers MUST NOT generate
	; non-zero length transport
	; padding, but receivers MUST
	; be able to handle padding
	; added by message transports.
	
*/	
	
//rfc2045 -- here we include a CRLF after every 72 characters.
//"All line breaks or other characters not found in Table 1 must be ignored by decoding software"
//---------------------------------------------------------------------------
	bool base64decode(string& s) {
// values 0..63 64=40  So  /=(2F) => 3F(63)
//		[ 41---5A                ][    61---7A             ][ 30-39  ]2B2F
//		ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		const char x[257]=
//         0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F <LSB  V-MSB	
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// 0 
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// 1
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x3E\x40\x40\x40\x3F"		// 2
		"\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x40\x40\x40\x40\x40\x40"		// 3 -3D,'=' is 0 (padding) OR EOF...
		"\x40\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E"		// 4
		"\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x40\x40\x40\x40\x40"		// 5
		"\x40\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28"		// 6
		"\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x40\x40\x40\x40\x40"		// 7
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// 8
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// 9
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// A
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// B
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// C
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// D
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40"		// E
		"\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40";		// F
		unsigned char i[4];
		std::string source(s); 
		s.clear(); 
		s.reserve( 3 * (source.length() >> 2) ); 
		string::const_iterator c = source.begin();
		string::const_iterator n = source.end();
		while( c < n ) {
			unsigned int a=0;
			while ( a < 4 && c < n) {
				i[a] = x[(unsigned char)(*c++)]; 
				if ( i[a] != '\x40' ) a++;
			}
			switch (a) {
				case 2: //1 character
					s.push_back(i[0] << 2 | i[1] >> 4);
					break;
				case 3: //2 characters
					s.push_back(i[0] << 2 | i[1] >> 4);
					s.push_back(i[1] << 4 | i[2] >> 2);
					break;
				case 4: //3 characters
					s.push_back(i[0] << 2 | i[1] >> 4);
					s.push_back(i[1] << 4 | i[2] >> 2);
					s.push_back(i[2] << 6 | i[3]);
					break;
			}
		}
		return true;
	}
	
	//---------------------------------------------------------------------------
	bool base64encode(string& s,bool withlf) {
		const char x[65]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		unsigned char i[3];
		unsigned int line_time = 0;
		std::string source(s); 
		s.clear(); 
		s.reserve( source.length() + (source.length() >> 1) );
		string::const_iterator c = source.begin();
		string::const_iterator n = source.end();
		while( c < n ) {
			unsigned int chars=3;
			i[0] = *c++;
			if ( c < n ) { 
				i[1] = *c++;
				if ( c < n ) {
					i[2] = *c++;
				} else {
					i[2] = 0; chars=2; 
				}
			} else {
				i[1] = 0; i[2] = 0; chars=1;
			}
			s.push_back( x[ i[0] >> 2 ] );
			s.push_back( x[ ( (i[0] & 0x03) << 4) | ( (i[1] & 0xf0) >> 4) ] );
			s.push_back( chars > 1 ? x[ ( (i[1] & 0x0f) << 2) | ( (i[2] & 0xc0) >> 6) ] : '=' );
			s.push_back( chars > 2 ? x[ i[2] & 0x3f ] : '=' );
			if (withlf) {
				line_time += 4; 
				if ( line_time >= 72) {
					line_time = 0; 
					s.push_back('\r'); 
					s.push_back('\n'); 
				}
			}
		}
		return true;
	}
	
	//---------------------------------------------------------------------------
	// http://www.w3.org/TR/2008/REC-xml-20081126/#sec-references 
	void xmldecode(string& source) {  //XML de-escape
		if (! source.empty() ) {	//
			const size_t bsize = 4;	//minimum size. &lt;
			if ( ( source.length() >= bsize ) ) {
				size_t amp_pos = source.find('&');
				while ( amp_pos != string::npos ) {
					size_t semicolon_pos = source.find(';',amp_pos);	// find the first ';'
					if ( semicolon_pos != string::npos) {
						size_t entity_size = ++semicolon_pos - amp_pos; //eg for &amp; this will be 4.
						if (source[amp_pos+1] == '#') { //handle Character References
							string utf;
							pair<unsigned long long,bool> interch;
							if (source[amp_pos+2] == 'x') { //hexencoded.
								interch = hex(source.substr(amp_pos+3,entity_size - 4));
							} else { //decimals
								interch = znatural(source.substr(amp_pos+2,entity_size - 3));
							}
							if (interch.second && interch.first != 0) {
								utf = UCS4toUTF8(interch.first);
								source.replace(amp_pos,entity_size,utf);
							}
						} else { //handle Entity References
							string entity=source.substr(amp_pos,entity_size);
							switch (entity_size) {
								case 4: { //&lt; &gt;
									if (! entity.compare("&lt;")) {
										source.replace(amp_pos,entity_size,"<");
									} else {
										if (! entity.compare("&gt;")) {
											source.replace(amp_pos,entity_size,">");
										} 
									}
								} break;
								case 5: {
									if (! entity.compare("&amp;")) {
										source.replace(amp_pos,entity_size,"&");
									} 
								} break;
								case 6: {
									if (! entity.compare("&apos;")) {
										source.replace(amp_pos,entity_size,"'");
									} else {
										if (! entity.compare("&quot;")) {
											source.replace(amp_pos,entity_size,"\"");
										} 
									}
								} break;
								default: {
									//report unknown xml entity here? (this function shouldnt handle non xml - eg html entities!)
								} break;
							}
						}
						amp_pos = source.find('&',amp_pos+1);
					} else {
						break;
					}
				}
			} 
		}
	}

	void xmlencode(string& s) {  //XML de-escape
		if (! s.empty() ) {
			fandr(s,"&","&amp;");	//otherwise the amp will do a long double decode.
			fandr(s,"<","&lt;");
			fandr(s,">","&gt;"); 
			fandr(s,"\"","&quot;"); 
		}
	}
	//• --------------------------------------------------------------------------
	bool normalise(string& container) { 
		bool retval = true;
		ostringstream result;	
		string::const_iterator f1=container.begin();
		string::const_iterator l1=container.end();
		while (f1 != l1) {
			if (((*f1 & 0x80) == 0x00) && (*f1 != 0x00)) {
				result.put(*f1++);
			} else if ((*f1 & 0xe0) == 0xc0 && f1 + 1 != l1 && (f1[1] & 0xc0) == 0x80) {
				result.put(*f1++);
				result.put(*f1++);
			} else if ((*f1 & 0xf0) == 0xe0 && f1 + 1 != l1 && f1 + 2 != l1 && (f1[1] & 0xc0) == 0x80 && (f1[2] & 0xc0) == 0x80) {
				result.put(*f1++);
				result.put(*f1++);
				result.put(*f1++);
			} else {
				f1++;
				retval = false; // illegal utf-8 found
				break;
			}
		}
		container = result.str();
		return retval;
	}
	
//--------------------------------------------------------------------------------
//xml name encoding http://www.w3.org/TR/REC-xml/#NT-Name
//	[4]   	NameChar   ::=   	 Letter | '_' | ':' | Digit | '.' | '-' | CombiningChar | Extender
//  [5a]    NameInit   ::=       Letter | '_' | ':'
//	[5]   	Name	   ::=   	 (NameInit) (NameChar)*
// This needs to support UTF8
//---------------------------------------------------------------------------------
	//Letters = 'A-Za-z'
	bool nameencode(string& s) {  //xml name encoding returns false if it can't return a valid name.
		bool retval=true;
		std::string result;
		unsigned char c;
		string::size_type q=s.size();
		if ( q > 0) {
			string::size_type p;
			for (p=0 ; p < q; p++) { //while we don't have a good initial character, carry on
				c=s[p];
				if ( 
					(c >= 'A' && c <= 'Z') || 
					(c >= 'a' && c <= 'z') ||
					(c == '_' ) || (c == ':' ) 
				) {
					result += c;
					break;
				}
			}
			if ( p >= q) { //no legal initial character...
				retval=false; 
			} else {	
				//we now have a NameInit 
				for (p++; p < q; p++) {  //carry p on from where it left off.
					c=s[p];
					if ( 
						(c >= 'A' && c <= 'Z') || 
						(c >= 'a' && c <= 'z') || 
						(c == '_' )  || (c == ':' ) || 
						(c >= '0' && c <= '9') || 
						(c == '.' ) || 
						(c == '-' ) 
						) {
						result += c;
					}
				}
			} 
		} else {
			retval=false;
		}
		//we now have a NameInit (NameChar)* or false.
		s = result;
		return retval;
	}

	//--------------------------------------------------------------------------------
	//xml name encoding http://www.w3.org/TR/REC-xml/#NT-Name
	//	[4]   	NameChar   ::=   	 Letter | '_' | ':' | Digit | '.' | '-' | CombiningChar | Extender
	//  [5a]    NameInit   ::=       Letter | '_' | ':'
	//	[5]   	Name	   ::=   	 (NameInit) (NameChar)*
	// This needs to support UTF8
	//---------------------------------------------------------------------------------
	//Letters = 'A-Za-z'
	
	bool nametest(const string& s) {  //xml name encoding returns false if it can't return a valid name.
		bool retval=true;
		unsigned char c;
		string::size_type q=s.size();
		if ( q > 0) {
			c=s[0];
			if ( 
				(c >= 'A' && c <= 'Z') || 
				(c >= 'a' && c <= 'z') ||
				(c == '_' ) || (c == ':' ) 
				) {
					for (string::size_type p = 1; p < q; p++) {  //carry p on from where it left off.
						c=s[p];
						if (!( 
							(c >= 'A' && c <= 'Z') || 
							(c >= 'a' && c <= 'z') || 
							(c == '_' )  || (c == ':' ) || 
							(c >= '0' && c <= '9') || 
							(c == '.' ) || 
							(c == '-' ) 
							)) {
							retval=false;
							break;
						}
					}
			} else {
				retval=false;
			}
		} else {
			retval=false;
 		}
		return retval;
	}
	
	//---------------------------------------------------------------------------
	// mailencode is more restrictive than RFC2822, in that [! ` ' $ { }] are stripped from the local part of the mail.
	// it is less restrictive in that it allows sequences of '.'
	// The email address can safely be used in a shell (bash) call, as long as it is "quoted"
	bool mailencode(string& mailbasis) {
		bool retval=true;
		std::string result;
		string::size_type q=mailbasis.size();
		if ( q > 0) {
			pair<string,string> mail_pair;
			if ( split('@', mailbasis,mail_pair)) { 
				char const* msafe = "#%*/?|^~&+-=_.0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
				string::size_type wsi = mail_pair.first.find_first_not_of(msafe);
				while ( wsi != string::npos ) {
					retval=false;
					string::size_type wso = mail_pair.first.find_first_of(msafe,wsi);
					mail_pair.first.erase(wsi,wso-wsi);
					wsi = mail_pair.first.find_first_not_of(msafe);
				}
				if (mail_pair.first.size() > 64) {
					retval = false;
					left(mail_pair.first,64,result); 
				} else {
					result = mail_pair.first;
				}
				result.push_back('@');
				//now for domain safe conversion
				char const* dsafe = "-.0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
				wsi = mail_pair.second.find_first_not_of(dsafe);
				while ( wsi != string::npos ) {
					retval=false;
					string::size_type wso = mail_pair.second.find_first_of(dsafe,wsi);
					mail_pair.second.erase(wsi,wso-wsi);
					wsi = mail_pair.second.find_first_not_of(dsafe);
				}
				string dn;
				if (mail_pair.second.size() > 255) {
					retval = false;
					left(mail_pair.second,255,dn); 
				} else {
					dn = mail_pair.second;
				}
				result.append(dn);
			} else {
				retval=false;
			}
			
		}
		return retval;
	}

	//---------------------------------------------------------------------------
	//url encoding is in uppercase.
	string hexencode(const unsigned char& c) {
		std::string result("%");
		unsigned char v[2]= { c/16, c%16 };
		if (v[0] <= 9) result+= (v[0] + '0');
		else result+= (v[0] - 10 + 'A');
		if (v[1] <= 9) result+= (v[1] + '0');
		else result+= (v[1] - 10 + 'A');
		return result;
	}
	//---------------------------------------------------------------------------
	bool tohex(string& s) {
		const unsigned char Abase = 'a' - 10;
		bool success=true;
		std::string orig;
		orig.swap(s);
		string::size_type sz = orig.size();
		s.reserve(sz >> 1);
		for (string::size_type p= 0; p < sz; ++p) {
			unsigned char v[2]= { orig[p] & 0x00F0 , orig[p] & 0x000F };
			if (v[0] <= 0x0090 ) { 
				s.push_back( (v[0] >> 4) + '0');
			} else {
				s.push_back( (v[0] >> 4) + Abase );
			}
			if (v[1] <= 0x0009 ) { 
				s.push_back(v[1] + '0');
			} else {
				s.push_back(v[1] + Abase );
			}			
		}
		return success;
	}
	//---------------------------------------------------------------------------
	bool fromhex(string& s,string& errstr) {
		const unsigned char Aubase = 'A' - 10;
		const unsigned char Albase = 'a' - 10;
		bool success=true;
		std::string orig;
		orig.swap(s);
		string::size_type sz = orig.size();
		if ((sz % 2 ) != 0) {
			errstr = "The decode text had an odd number of characters";
			success = false;
		} else {
			if (sz > 1 && orig[0] == '0' && (orig[1] == 'x' || orig[1] == 'X')) {
				orig.erase(0,2);
			}
			s.reserve(sz << 1);
			for (string::size_type p= 0; p < sz && success; p += 2) {
				unsigned char hi = orig[p];
				unsigned char lo = p+1 < sz ? orig[p+1] : '0';
				if ( hi >= '0' && hi <= '9' ) {
					hi -= '0';
				} else {
					if ( hi >= 'A' && hi <= 'F' ) {
						hi -= Aubase;
					} else {
						if ( hi >= 'a' && hi <= 'f' ) {
							hi -= Albase;
						} else {
							hi = 0;
							errstr = "A high nybble was out of range in the decode text.";
							success = false;
						}
					}
				}
				if ( lo >= '0' && lo <= '9' ) {
					lo -= '0';
				} else {
					if ( lo >= 'A' && lo <= 'F' ) {
						lo -= Aubase;
					} else {
						if ( lo >= 'a' && lo <= 'f' ) {
							lo -= Albase;
						} else {
							lo = 0;
							errstr = "A low nybble was out of range in the decode text.";
							success = false;
						}
					}
				}
				unsigned char ch = hi << 4 | lo;
				s.push_back(ch);
			}
		}
		return success;
	}	
	//---------------------------------------------------------------------------
	void urldecode(std::string& s) {
		std::string result;
		unsigned char c;
		for (unsigned int p= 0; p < s.size(); ++p) {
			c= s[p];
			if (c == '+') c= ' ';  // translate '+' to ' ':
								   // translate %XX:
			if ((c == '%') && (p + 2 < s.size())) {  // check length
				unsigned char v[2];
				for (unsigned int i= p + 1; i < p + 3; ++i)
					// check and translate syntax
					if ((s[i] >= '0') && (s[i] <= '9'))
						v[i - p - 1]= s[i] - '0';
					else if ((s[i] >= 'A') && (s[i] <= 'F'))
						v[i - p - 1]= 10 + s[i] - 'A';
					else if ((s[i] >= 'a') && (s[i] <= 'f'))
						v[i - p - 1]= 10 + s[i] - 'a';
					else v[i - p - 1]= 16;  // error
				if ((v[0] < 16) && (v[1] < 16)) {  // check ok
					c= (unsigned char) 16*v[0] + v[1];
					p+= 2;
				}
			}
			result+= c;
		}
		s = result;
	}

//---------------------------------------------------------------------------
// as defined in RFCs 1738 2396
		void urlencode(std::string& s) {
			std::string result;
			unsigned char c;
			for (unsigned int p= 0; p < s.size(); ++p) {
				c= s[p];
				if ((c <= 31) || (c >= 127))  // CTL, >127
					result+= hexencode(c);
				else switch (c) {
					case ';':
					case '/':
					case '?':
					case ':':
					case '@':
					case '&':
					case '=':
					case '+':  // until here: reserved
					case '"':
					case '\'':
					case '#':
					case '%':
					case '<':
					case '>':  // until here: unsafe
					case '{':
					case '}':
					case '|':
					case '\\':
					case '^':
					case '~':
					case '[':
					case ']': //yet more unsafe.
					case '!':
					case ',':
					case '*':
					case '$':
					case '(':
					case ')': //twitts.
					case ' ': //space may be hex encoded.
						result+= hexencode(c);
						break;
						//					case ' ':  // SP
						//						result+= '+';
						//						break;
					default:  // no need to encode
						result+= c;
						break;
				}
			}
		s = result;
	}
	
}

