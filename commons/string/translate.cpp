/*
 * translate.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd
 * translate.cpp is a part of Obyx - see http://www.obyx.org .
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


// see RFC 1738 for URL spec.
#include <string>
#include <sstream>

#include "commons/xml/xml.h"
#include "translate.h"
#include "fandr.h"
#include "regex.h"
#include "convert.h"

using namespace std;

namespace String {
	
	// http://www.ietf.org/rfc/rfc1521.txt (actually made redundant by)
	// http://tools.ietf.org/html/rfc2045
	bool qpdecode(string& quoted_printable,const string CRLF) {
		bool retval = false;
		size_t crsiz=CRLF.size();
		vector<string> qplines;
		string::size_type pos = quoted_printable.find(CRLF);
		while (pos != string::npos) {
			qplines.push_back(quoted_printable.substr(0, pos));
			quoted_printable.erase(0,pos+crsiz);
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
					if (i < (qplines.size()-1)) {
						qplines[i].append(CRLF);
					}
				} else {
					qplines[i].resize(qplines[i].size() - 1);
					if (qplines[i].empty()) {
						qplines[i]=CRLF;
					}
				}
			} else {
				qplines[i]=CRLF;
			}
			size_t x = qplines[i].find('=');
			fandr(qplines[i],'_',' ');
			while (x != string::npos) {
				string code,err;
				code.push_back(qplines[i][x+1]);
				code.push_back(qplines[i][x+2]);
				if (fromhex(code,err)) {
					qplines[i].replace(x,3,code);
				} else {
					retval = false;
					break;
				}
				x = qplines[i].find('=',x+1);
			}
			quoted_printable.append(qplines[i]);
		}
		qplines.clear();
		return retval;
	}
	

	//Quoted-Printable encoding REQUIRES that encoded lines be no more than 76 characters long.
	//The maximum encoding size of a character is 3.
	//Use an equal sign to indicate a soft-break. B/C of osi we also encode amps,gt,lt
	void qpencode(const std::string &input,std::string &output)
	{
		char byte;
		const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		const std::string CRLF="\r\n";
		
		int linelen=0;
		for (size_t i = 0; i < input.length() ; ++i) {
			if (input[i] == '\r' && i+1 < input.length() && input[i+1]== '\n') {
				output.append(CRLF);
				linelen=0;
				i++;
			} else {
				byte = input[i];
				if ((byte == 0x09) || (byte == 0x20) || ((byte >= 0x21) && (byte <= 126) && ((byte < 0x3C) || (byte > 0x3E) ) && (byte != 0x26) )) {
					if (linelen >= 73) {
						if (linelen == 73) {
							output.push_back(byte);
							output.push_back('=');
							output.append(CRLF);
							linelen=0;
						} else {
							output.push_back('=');
							output.append(CRLF);
							output.push_back(byte);
							linelen=1;
						}
					} else {
						output.push_back(byte);
						linelen++;
					}
				} else {
					if (linelen >= 72) {
						output.push_back('=');
						output.append(CRLF);
						linelen=0;
					}
					output.push_back('=');
					output.push_back(hex[((byte >> 4) & 0x0F)]);
					output.push_back(hex[(byte & 0x0F)]);
					linelen+=3;
				}
			}
		}
	}
	
	
	/*
	 DOS-style lines that end with (CR/LF) characters (optional for the last line)
	 Any field may be quoted (with double quotes).
	 Fields containing a
	 line-break, double-quote, and/or commas should be quoted. (If they are not, the file will likely be impossible to process correctly).
	 A (double) quote character in a field must be represented by two (double) quote characters.
	 */
	void csvencode(string& basis) {
		string::size_type d = basis.find_first_of("\",\r\n\t");	//line-break, double-quote, and/or commas should be quoted.
		if ( d != string::npos ) {
			string::size_type s = basis.find_first_of('\"');
			while ( s != string::npos ) {
				basis.insert(basis.begin()+s,'\"');
				s = basis.find_first_of('\"',s+2);
			}
			basis.push_back('\"');
			basis.insert(0,"\"");
		}
	}
	
	void csvdecode(string& basis) {
		if (basis.size() > 0 && basis[0]=='\"') { //it's quoted.
			basis.resize(basis.size()-1);
			basis.erase(0,1);
			fandr(basis,"\"\"","\"");
		}
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
	bool base64decode(string& basis) {
		// A correct string has a length that is multiple of four.
		// Every 4 encoded bytes corresponds to 3 decoded bytes, (or null terminated).
		string b64ch="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		size_t len = basis.size();
		size_t pos = 0;
		int i = 0,j = 0;
		unsigned char encbuf[4], decbuf[3];
		std::string ret;
		while (len-- && ( basis[pos] != '=') && (b64ch.find(basis[pos]) != string::npos)) {
			encbuf[i++] = basis[pos]; pos++;
			if (i ==4) {
				for (i = 0; i <4; i++) {
					encbuf[i] = b64ch.find(encbuf[i]);
				}
				decbuf[0] = (encbuf[0] << 2) + ((encbuf[1] & 0x30) >> 4);
				decbuf[1] = ((encbuf[1] & 0xf) << 4) + ((encbuf[2] & 0x3c) >> 2);
				decbuf[2] = ((encbuf[2] & 0x3) << 6) + encbuf[3];
				for (i = 0; (i < 3); i++) {
					ret.push_back(decbuf[i]);
				}
				i = 0;
			}
		}
		if (i) {
			for (j = i; j <4; j++) {
				encbuf[j] = 0;
			}
			for (j = 0; j <4; j++) {
				encbuf[j] = b64ch.find(encbuf[j]);
			}
			decbuf[0] = (encbuf[0] << 2) + ((encbuf[1] & 0x30) >> 4);
			decbuf[1] = ((encbuf[1] & 0xf) << 4) + ((encbuf[2] & 0x3c) >> 2);
			decbuf[2] = ((encbuf[2] & 0x3) << 6) + encbuf[3];
			for (j = 0; (j < i - 1); j++) {
				ret += decbuf[j];
			}
		}
		basis=ret;
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
	
	void cgi_to_http(string& s) {
		bool upper=true; //we want to skip the X
		for (string::iterator i = s.begin() ; i != s.end(); ++i) {
			if (*i == '_') {
				upper=true;
				*i = '-';
			} else {
				if (upper) {
					upper=false;
				} else {
					*i = std::tolower(*i);
				}
			}
		}
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
	//RFC 1342 encodings.
	//=?charset?encoding?encoded-text?=(sp/nl)  (encoding is either B or Q)
	bool rfc1342(string &s,bool enc) {
		bool retval = false;
		string b=s;
		if (enc) {
			/*
			 An encoded-word may not be more than 75 characters long, including charset, encoding, encoded-text, and delimiters.
			 the bits = 12. 75-12 = 63. B64 has a ratio of 8:6. 6*(63/8)=47
			 This means we have to split the text into 47 character chunks.
			 */
			size_t chunk=40; //46 is probably fine, but apple chooses a bit less.
			s.clear();
			while (! b.empty()) {
				string c=b;
				boundedleft(c,chunk);		//gets the best utf-8 approximation for 46 bytes.
				b.erase(0,c.size());
				base64encode(c,false);
				s.append("=?utf-8?B?");
				s.append(c);
				s.append("?=");
				if (!b.empty()) {
					s.append("\r\n"); 	//later this should be parameterised. basically suitable only for mail-headers.
				}
			}
			retval = true;
		} else {
			size_t pos= 0;
			pos= b.find("=?",pos);
			while ( pos != string::npos ) {
				size_t coff= b.find("?",pos+2);
				if (coff == string::npos) {
					break;
				} else {
					size_t eoff= b.find("?",coff+1);
					if (eoff == string::npos) {
						break;
					} else {
						size_t soff= b.find("?=",eoff+2);
						if (soff == string::npos) {
							break;
						} else {
							string charset= b.substr(pos+2,coff-(pos+2));
							string encoder= b.substr(coff+1,eoff-(coff+1));
							string basis= b.substr(eoff+1,soff-(eoff+1));
							if (encoder.compare("B") == 0) {
								base64decode(basis);
							} else {
								if (encoder.compare("Q") == 0) {
									qpdecode(basis);
								} else {
									break;	//unknown/broken.
								}
							}
							String::tolower(charset);
							if (charset.compare("utf-8") != 0) {
								u_str x_body;
								XML::Manager::transcode(basis,x_body,charset);
								XML::Manager::transcode(x_body.c_str(),basis);
							}
							size_t padd=0;
							if (b.size() > soff+2 ) {
								if (b[soff+2] ==' ' || b[soff+2] =='\t') {
									padd=1; //space
								} else {
									padd=2; //crlf
								}
							}
							b.replace(pos,(soff+2+padd)-pos,basis);
							pos = pos+basis.size();
						}
					}
					pos = b.find("=?",pos);
				}
			}
			s = b;
		}
		return retval;
	}
	
	
	//---------------------------------------------------------------------------
	//url encoding is in uppercase.
	string hexencode(const unsigned char& c) {
		std::string result("%");
		unsigned char v[2];
		v[0] = c >> 4; v[1] = c%16;
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
			unsigned char v[2];
			v[0] = orig[p] & 0x00F0;
			v[1] = orig[p] & 0x000F;
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
	//	5.1.  Parameter Encoding
	//	All parameter names and values are escaped using the [RFC3986] percent-encoding (%xx) mechanism.
	//	Characters not in the unreserved character set ([RFC3986] section 2.3) MUST be encoded.
	//  Characters in the unreserved character set MUST NOT be encoded.
	//	Hexadecimal characters in encodings MUST be upper case.
	//	Text names and values MUST be encoded as UTF-8 octets before percent-encoding them per [RFC3629].
	//	unreserved = ALPHA, DIGIT, '-', '.', '_', '~'
	//---------------------------------------------------------------------------
	// as defined in RFCs 1738 2396
	void urlencode(std::string& s) {
		std::string result;
		unsigned char c;
		for (unsigned int p= 0; p < s.size(); ++p) {
			c= s[p];
			if (
				((c >= 'a') && (c <= 'z')) ||
				((c >= '0') && (c <= '9')) ||
				((c >= 'A') && (c <= 'Z')) ||
				( c == '-' || c == '.' || c == '_' || c == '~' )
				) {  //reserved.
				result.push_back(c);
			} else { //unreserved.
				result.append(hexencode(c));
			}
			
			/*
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
			 */
		}
		s = result;
	}
	
}

