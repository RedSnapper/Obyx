/* 
 * chars.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * chars.cpp is a part of Obyx - see http://www.obyx.org
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

#include "chars.h"

using namespace std;

map<unsigned long, char> XMLChar::Lo;
map<unsigned long, char> XMLChar::Hi;

//Tests if a string is valid XML UTF-8
bool XMLChar::isutf8(const std::string& source) {
	bool result=true;
	unsigned long test;
	if ( ! source.empty() )  {
		string::const_iterator b=source.begin();
		string::const_iterator e=source.end();
		while(b < e) {
			unsigned char c1= *b;
			if ((c1 & 0x80) == 0x00) {
				test = c1; 
				b++;
			} else if ((c1 & 0xe0) == 0xc0 && b + 1 != e && (b[1] & 0xc0) == 0x80) {
				unsigned char c2= *(b+1);
				test = c1 << 8 | c2;
				b+= 2;
			}
			else if ((c1 & 0xf0) == 0xe0 && b + 1 != e && b + 2 != e && (b[1] & 0xc0) == 0x80 && (b[2] & 0xc0) == 0x80) {
				unsigned char c2= *(b+1),c3 = *(b+2);
				test = c1 << 16 | c2 << 8 | c3; 
				b+= 3;
			} else {
				result = false;
				break;
			}
			if (! is(test) ) {
				result = false;
				break;
			}
		}
	}
	return result;
}

//Ensures that the string is valid XML UTF-8
void XMLChar::convert(std::string& source) {
	string result;
	unsigned long test;
	if ( ! source.empty() )  {
		string::const_iterator b=source.begin();
		string::const_iterator e=source.end();
		while(b < e) {
			unsigned char c1= *b;
			if ((c1 & 0x80) == 0x00) {
				test = c1; 
				if ( is(test) ) { result.push_back(c1); }
				b++;
			} else if ((c1 & 0xe0) == 0xc0 && b + 1 != e && (b[1] & 0xc0) == 0x80) {
				unsigned char c2= *(b+1);
				test = c1 << 8 | c2;
				if ( is(test) ) {
					result.push_back(c1);
					result.push_back(c2);
				}
				b+= 2;
			}
			else if ((c1 & 0xf0) == 0xe0 && b + 1 != e && b + 2 != e && (b[1] & 0xc0) == 0x80 && (b[2] & 0xc0) == 0x80) {
				unsigned char c2= *(b+1),c3 = *(b+2);
				test = c1 << 16 | c2 << 8 | c3; 
				if ( is(test) ) {
					result.push_back(c1);
					result.push_back(c2);
					result.push_back(c3);
				}
				b+= 3;
			} else {
				b++; //illegal utf-8, but let's try to continue.
			}
		}
	}
	source = result;
}
void XMLChar::encode(std::string& source) {
	string result;
	unsigned long test = 0;
	if ( ! source.empty() )  {
		string::const_iterator b=source.begin();
		string::const_iterator e=source.end();
		while(b < e) {
			if ((*b & 0x80) == 0x00) {
				test = (unsigned char)*b;
				if ( is(test) ) {
					switch (*b) {
						case '&':  result.append("&amp;"); break;
						case '<':  result.append("&lt;"); break;
						case '>':  result.append("&gt;"); break;
						case '"':  result.append("&quot;"); break;
						case '\'': result.append("&#39;"); break;
						default: result.push_back(*b); break;
					}
				}
				b++;
			} else if ((*b & 0xe0) == 0xc0 && b + 1 != e && (b[1] & 0xc0) == 0x80) {
				test = (unsigned char)(*(b+1)); 
				test = test << 8; 
				test |= (unsigned char)(*b);
				if ( is(test) ) {
					result.push_back(*b);
					result.push_back(*(b+1));
				}
				b+= 2;
			}
			else if ((*b & 0xf0) == 0xe0 && b + 1 != e && b + 2 != e && (b[1] & 0xc0) == 0x80 && (b[2] & 0xc0) == 0x80) { //3char utf-8
				test = (unsigned char)(*(b+2)); 
				test = test << 8; 
				test |= (unsigned char)(*(b+1)); 
				test = test << 8; 
				test |= (unsigned char)(*b);
				if ( is(test) ) {
					result.push_back(*b);
					result.push_back(*(b+1));
					result.push_back(*(b+2));
				}
				b+= 3;
			} else {
				b++; //illegal utf-8, but let's try to continue.
			}
		}
	}
	source = result;
}	
bool XMLChar::is(unsigned long k) {
	return Hi.lower_bound(k)->second != Lo.upper_bound(k)->second;	
}
void XMLChar::startup() {
	Digit::startup();
	Extender::startup();
	CombiningChar::startup();
	BaseChar::startup();
	Ideographic::startup();
	//	Char	   ::=   	#x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]	
	Lo[0x000009]=0x001;		Hi[0x00000A]=0x001; // [#x9 #xA]
	Lo[0x00000D]=0x002;		Hi[0x00000D]=0x002;	//  #xD
	Lo[0x000020]=0x003;		Hi[0xED9FBF]=0x003; // [#x20-#xD7FF]
	Lo[0xEE8080]=0x004;		Hi[0xEFBFBD]=0x004; // [#xE000-#xFFFD]
}
void XMLChar::shutdown() {
	Digit::shutdown();
	Extender::shutdown();
	CombiningChar::shutdown();
	BaseChar::shutdown();
	Ideographic::shutdown();
	Lo.clear(); Hi.clear();
}
//------------ Ideographic ----------------
//Ideographic ::= [#x4E00-#x9FA5] | #x3007 | [#x3021-#x3029]	
//	[E4B880][E9BEA5] [E38087][E38087] [E380A1][E380A9]
bool Ideographic::is(unsigned long k) {
	return (
			( k >= 0x00E4B880 && k <= 0x00E9BEA5) || 
			( k >= 0x00E380A1 && k <= 0x00E380A9) || 
			( k == 0x00E38087 ) 
			);
}
void Ideographic::startup() {} //Do nothing. Yet.
void Ideographic::shutdown() {} //Do nothing. Yet.
//------------ Digit ----------------
map<unsigned long, char> Digit::Lo;
map<unsigned long, char> Digit::Hi;
bool Digit::is(unsigned long k) {
	return Hi.lower_bound(k)->second != Lo.upper_bound(k)->second;	
}
//List, in UTF16 is found at http://www.w3.org/TR/xml/#CharClasses
void Digit::startup() {
	Lo[0x30]=1;			Hi[0x39]=1;
	Lo[0xD9A0]=2;		Hi[0xD9A9]=2;
	Lo[0xDBB0]=3;		Hi[0xDBB9]=3;
	Lo[0xE0A5A6]=4;		Hi[0xE0A5AF]=4;
	Lo[0xE0A7A6]=5;		Hi[0xE0A7AF]=5;
	Lo[0xE0A9A6]=6;		Hi[0xE0A9AF]=6;
	Lo[0xE0ABA6]=7;		Hi[0xE0ABAF]=7;
	Lo[0xE0ADA6]=8;		Hi[0xE0ADAF]=8;
	Lo[0xE0AFA7]=9;		Hi[0xE0AFAF]=9;
	Lo[0xE0B1A6]=10;	Hi[0xE0B1AF]=10;
	Lo[0xE0B3A6]=11;	Hi[0xE0B3AF]=11;
	Lo[0xE0B5A6]=12;	Hi[0xE0B5AF]=12;
	Lo[0xE0B990]=13;	Hi[0xE0B999]=13;
	Lo[0xE0BB90]=14;	Hi[0xE0BB99]=14;
	Lo[0xE0BCA0]=15;	Hi[0xE0BCA9]=15;
}	
void Digit::shutdown() {
	Lo.clear();
	Hi.clear();
}
//------------ Extender ----------------
map<unsigned long, char> Extender::Lo;
map<unsigned long, char> Extender::Hi;
bool Extender::is(unsigned long k) {
	return Hi.lower_bound(k)->second != Lo.upper_bound(k)->second;	
}
//List, in UTF16 is found at http://www.w3.org/TR/xml/#CharClasses
void Extender::startup() {
	Lo[0xC2B7]=1;		Hi[0xC2B7]=1;
	Lo[0xCB90]=2;		Hi[0xCB90]=2;
	Lo[0xCB91]=3;		Hi[0xCB91]=3;
	Lo[0xCE87]=4;		Hi[0xCE87]=4;
	Lo[0xD980]=5;		Hi[0xD980]=5;
	Lo[0xE0B986]=6;		Hi[0xE0B986]=6;
	Lo[0xE0BB86]=7;		Hi[0xE0BB86]=7;
	Lo[0xE38085]=8;		Hi[0xE38085]=8;
	Lo[0xE380B1]=9;		Hi[0xE380B5]=9;
	Lo[0xE3829D]=10;	Hi[0xE3829E]=10;
	Lo[0xE383BC]=11;	Hi[0xE383BE]=11;
}	
void Extender::shutdown() {
	Lo.clear();
	Hi.clear();
}
//------------ CombiningChar ----------------
map<unsigned long, char> CombiningChar::Lo;
map<unsigned long, char> CombiningChar::Hi;
bool CombiningChar::is(unsigned long k) {
	return Hi.lower_bound(k)->second != Lo.upper_bound(k)->second;	
}
//List, in UTF16 is found at http://www.w3.org/TR/xml/#CharClasses
void CombiningChar::startup() {
	Lo[0xCC80]=1;		Hi[0xCD85]=1;
	Lo[0xCDA0]=2;		Hi[0xCDA1]=2;
	Lo[0xD283]=3;		Hi[0xD286]=3;
	Lo[0xD691]=4;		Hi[0xD6A1]=4;
	Lo[0xD6A3]=5;		Hi[0xD6B9]=5;
	Lo[0xD6BB]=6;		Hi[0xD6BD]=6;
	Lo[0xD6BF]=7;		Hi[0xD6BF]=7;
	Lo[0xD781]=8;		Hi[0xD782]=8;
	Lo[0xD784]=9;		Hi[0xD784]=9;
	Lo[0xD98B]=10;		Hi[0xD992]=10;
	Lo[0xD9B0]=11;		Hi[0xD9B0]=11;
	Lo[0xDB96]=12;		Hi[0xDB9C]=12;
	Lo[0xDB9D]=13;		Hi[0xDB9F]=13;
	Lo[0xDBA0]=14;		Hi[0xDBA4]=14;
	Lo[0xDBA7]=15;		Hi[0xDBA8]=15;
	Lo[0xDBAA]=16;		Hi[0xDBAD]=16;
	Lo[0xE0A481]=17;		Hi[0xE0A483]=17;
	Lo[0xE0A4BC]=18;		Hi[0xE0A4BC]=18;
	Lo[0xE0A4BE]=19;		Hi[0xE0A58C]=19;
	Lo[0xE0A58D]=20;		Hi[0xE0A58D]=20;
	Lo[0xE0A591]=21;		Hi[0xE0A594]=21;
	Lo[0xE0A5A2]=22;		Hi[0xE0A5A3]=22;
	Lo[0xE0A681]=23;		Hi[0xE0A683]=23;
	Lo[0xE0A6BC]=24;		Hi[0xE0A6BC]=24;
	Lo[0xE0A6BE]=25;		Hi[0xE0A6BE]=25;
	Lo[0xE0A6BF]=26;		Hi[0xE0A6BF]=26;
	Lo[0xE0A780]=27;		Hi[0xE0A784]=27;
	Lo[0xE0A787]=28;		Hi[0xE0A788]=28;
	Lo[0xE0A78B]=29;		Hi[0xE0A78D]=29;
	Lo[0xE0A797]=30;		Hi[0xE0A797]=30;
	Lo[0xE0A7A2]=31;		Hi[0xE0A7A3]=31;
	Lo[0xE0A882]=32;		Hi[0xE0A882]=32;
	Lo[0xE0A8BC]=33;		Hi[0xE0A8BC]=33;
	Lo[0xE0A8BE]=34;		Hi[0xE0A8BE]=34;
	Lo[0xE0A8BF]=35;		Hi[0xE0A8BF]=35;
	Lo[0xE0A980]=36;		Hi[0xE0A982]=36;
	Lo[0xE0A987]=37;		Hi[0xE0A988]=37;
	Lo[0xE0A98B]=38;		Hi[0xE0A98D]=38;
	Lo[0xE0A9B0]=39;		Hi[0xE0A9B1]=39;
	Lo[0xE0AA81]=40;		Hi[0xE0AA83]=40;
	Lo[0xE0AABC]=41;		Hi[0xE0AABC]=41;
	Lo[0xE0AABE]=42;		Hi[0xE0AB85]=42;
	Lo[0xE0AB87]=43;		Hi[0xE0AB89]=43;
	Lo[0xE0AB8B]=44;		Hi[0xE0AB8D]=44;
	Lo[0xE0AC81]=45;		Hi[0xE0AC83]=45;
	Lo[0xE0ACBC]=46;		Hi[0xE0ACBC]=46;
	Lo[0xE0ACBE]=47;		Hi[0xE0AD83]=47;
	Lo[0xE0AD87]=48;		Hi[0xE0AD88]=48;
	Lo[0xE0AD8B]=49;		Hi[0xE0AD8D]=49;
	Lo[0xE0AD96]=50;		Hi[0xE0AD97]=50;
	Lo[0xE0AE82]=51;		Hi[0xE0AE83]=51;
	Lo[0xE0AEBE]=52;		Hi[0xE0AF82]=52;
	Lo[0xE0AF86]=53;		Hi[0xE0AF88]=53;
	Lo[0xE0AF8A]=54;		Hi[0xE0AF8D]=54;
	Lo[0xE0AF97]=55;		Hi[0xE0AF97]=55;
	Lo[0xE0B081]=56;		Hi[0xE0B083]=56;
	Lo[0xE0B0BE]=57;		Hi[0xE0B184]=57;
	Lo[0xE0B186]=58;		Hi[0xE0B188]=58;
	Lo[0xE0B18A]=59;		Hi[0xE0B18D]=59;
	Lo[0xE0B195]=60;		Hi[0xE0B196]=60;
	Lo[0xE0B282]=61;		Hi[0xE0B283]=61;
	Lo[0xE0B2BE]=62;		Hi[0xE0B384]=62;
	Lo[0xE0B386]=63;		Hi[0xE0B388]=63;
	Lo[0xE0B38A]=64;		Hi[0xE0B38D]=64;
	Lo[0xE0B395]=65;		Hi[0xE0B396]=65;
	Lo[0xE0B482]=66;		Hi[0xE0B483]=66;
	Lo[0xE0B4BE]=67;		Hi[0xE0B583]=67;
	Lo[0xE0B586]=68;		Hi[0xE0B588]=68;
	Lo[0xE0B58A]=69;		Hi[0xE0B58D]=69;
	Lo[0xE0B597]=70;		Hi[0xE0B597]=70;
	Lo[0xE0B8B1]=71;		Hi[0xE0B8B1]=71;
	Lo[0xE0B8B4]=72;		Hi[0xE0B8BA]=72;
	Lo[0xE0B987]=73;		Hi[0xE0B98E]=73;
	Lo[0xE0BAB1]=74;		Hi[0xE0BAB1]=74;
	Lo[0xE0BAB4]=75;		Hi[0xE0BAB9]=75;
	Lo[0xE0BABB]=76;		Hi[0xE0BABC]=76;
	Lo[0xE0BB88]=77;		Hi[0xE0BB8D]=77;
	Lo[0xE0BC98]=78;		Hi[0xE0BC99]=78;
	Lo[0xE0BCB5]=79;		Hi[0xE0BCB5]=79;
	Lo[0xE0BCB7]=80;		Hi[0xE0BCB7]=80;
	Lo[0xE0BCB9]=81;		Hi[0xE0BCB9]=81;
	Lo[0xE0BCBE]=82;		Hi[0xE0BCBE]=82;
	Lo[0xE0BCBF]=83;		Hi[0xE0BCBF]=83;
	Lo[0xE0BDB1]=84;		Hi[0xE0BE84]=84;
	Lo[0xE0BE86]=85;		Hi[0xE0BE8B]=85;
	Lo[0xE0BE90]=86;		Hi[0xE0BE95]=86;
	Lo[0xE0BE97]=87;		Hi[0xE0BE97]=87;
	Lo[0xE0BE99]=88;		Hi[0xE0BEAD]=88;
	Lo[0xE0BEB1]=89;		Hi[0xE0BEB7]=89;
	Lo[0xE0BEB9]=90;		Hi[0xE0BEB9]=90;
	Lo[0xE28390]=91;		Hi[0xE2839C]=91;
	Lo[0xE283A1]=92;		Hi[0xE283A1]=92;
	Lo[0xE380AA]=93;		Hi[0xE380AF]=93;
	Lo[0xE38299]=94;		Hi[0xE38299]=94;
	Lo[0xE3829A]=95;		Hi[0xE3829A]=95;
}	
void CombiningChar::shutdown() {
	Lo.clear();
	Hi.clear();
}
//------------ BaseChar ----------------
map<unsigned long, char> BaseChar::Lo;
map<unsigned long, char> BaseChar::Hi;
bool BaseChar::is(unsigned long k) {
	return Hi.lower_bound(k)->second != Lo.upper_bound(k)->second;	
}
//List, in UTF16 is found at http://www.w3.org/TR/xml/#CharClasses
void BaseChar::startup() {
	Lo[0x41]=1;			Hi[0x5A]=1;
	Lo[0x61]=2;			Hi[0x7A]=2;
	Lo[0xC380]=3;		Hi[0xC396]=3;
	Lo[0xC398]=4;		Hi[0xC3B6]=4;
	Lo[0xC3B8]=5;		Hi[0xC3BF]=5;
	Lo[0xC480]=6;		Hi[0xC4B1]=6;
	Lo[0xC4B4]=7;		Hi[0xC4BE]=7;
	Lo[0xC581]=8;		Hi[0xC588]=8;
	Lo[0xC58A]=9;		Hi[0xC5BE]=9;
	Lo[0xC680]=10;		Hi[0xC783]=10;
	Lo[0xC78D]=11;		Hi[0xC7B0]=11;
	Lo[0xC7B4]=12;		Hi[0xC7B5]=12;
	Lo[0xC7BA]=13;		Hi[0xC897]=13;
	Lo[0xC990]=14;		Hi[0xCAA8]=14;
	Lo[0xCABB]=15;		Hi[0xCB81]=15;
	Lo[0xCE86]=16;		Hi[0xCE86]=16;
	Lo[0xCE88]=17;		Hi[0xCE8A]=17;
	Lo[0xCE8C]=18;		Hi[0xCE8C]=18;
	Lo[0xCE8E]=19;		Hi[0xCEA1]=19;
	Lo[0xCEA3]=20;		Hi[0xCF8E]=20;
	Lo[0xCF90]=21;		Hi[0xCF96]=21;
	Lo[0xCF9A]=22;		Hi[0xCF9A]=22;
	Lo[0xCF9C]=23;		Hi[0xCF9C]=23;
	Lo[0xCF9E]=24;		Hi[0xCF9E]=24;
	Lo[0xCFA0]=25;		Hi[0xCFA0]=25;
	Lo[0xCFA2]=26;		Hi[0xCFB3]=26;
	Lo[0xD081]=27;		Hi[0xD08C]=27;
	Lo[0xD08E]=28;		Hi[0xD18F]=28;
	Lo[0xD191]=29;		Hi[0xD19C]=29;
	Lo[0xD19E]=30;		Hi[0xD281]=30;
	Lo[0xD290]=31;		Hi[0xD384]=31;
	Lo[0xD387]=32;		Hi[0xD388]=32;
	Lo[0xD38B]=33;		Hi[0xD38C]=33;
	Lo[0xD390]=34;		Hi[0xD3AB]=34;
	Lo[0xD3AE]=35;		Hi[0xD3B5]=35;
	Lo[0xD3B8]=36;		Hi[0xD3B9]=36;
	Lo[0xD4B1]=37;		Hi[0xD596]=37;
	Lo[0xD599]=38;		Hi[0xD599]=38;
	Lo[0xD5A1]=39;		Hi[0xD686]=39;
	Lo[0xD790]=40;		Hi[0xD7AA]=40;
	Lo[0xD7B0]=41;		Hi[0xD7B2]=41;
	Lo[0xD8A1]=42;		Hi[0xD8BA]=42;
	Lo[0xD981]=43;		Hi[0xD98A]=43;
	Lo[0xD9B1]=44;		Hi[0xDAB7]=44;
	Lo[0xDABA]=45;		Hi[0xDABE]=45;
	Lo[0xDB80]=46;		Hi[0xDB8E]=46;
	Lo[0xDB90]=47;		Hi[0xDB93]=47;
	Lo[0xDB95]=48;		Hi[0xDB95]=48;
	Lo[0xDBA5]=49;		Hi[0xDBA6]=49;
	Lo[0xE0A485]=50;	Hi[0xE0A4B9]=50;
	Lo[0xE0A4BD]=51;	Hi[0xE0A4BD]=51;
	Lo[0xE0A598]=52;	Hi[0xE0A5A1]=52;
	Lo[0xE0A685]=53;	Hi[0xE0A68C]=53;
	Lo[0xE0A68F]=54;	Hi[0xE0A690]=54;
	Lo[0xE0A693]=55;	Hi[0xE0A6A8]=55;
	Lo[0xE0A6AA]=56;	Hi[0xE0A6B0]=56;
	Lo[0xE0A6B2]=57;	Hi[0xE0A6B2]=57;
	Lo[0xE0A6B6]=58;	Hi[0xE0A6B9]=58;
	Lo[0xE0A79C]=59;	Hi[0xE0A79D]=59;
	Lo[0xE0A79F]=60;	Hi[0xE0A7A1]=60;
	Lo[0xE0A7B0]=61;	Hi[0xE0A7B1]=61;
	Lo[0xE0A885]=62;	Hi[0xE0A88A]=62;
	Lo[0xE0A88F]=63;	Hi[0xE0A890]=63;
	Lo[0xE0A893]=64;	Hi[0xE0A8A8]=64;
	Lo[0xE0A8AA]=65;	Hi[0xE0A8B0]=65;
	Lo[0xE0A8B2]=66;	Hi[0xE0A8B3]=66;
	Lo[0xE0A8B5]=67;	Hi[0xE0A8B6]=67;
	Lo[0xE0A8B8]=68;	Hi[0xE0A8B9]=68;
	Lo[0xE0A999]=69;	Hi[0xE0A99C]=69;
	Lo[0xE0A99E]=70;	Hi[0xE0A99E]=70;
	Lo[0xE0A9B2]=71;	Hi[0xE0A9B4]=71;
	Lo[0xE0AA85]=72;	Hi[0xE0AA8B]=72;
	Lo[0xE0AA8D]=73;	Hi[0xE0AA8D]=73;
	Lo[0xE0AA8F]=74;	Hi[0xE0AA91]=74;
	Lo[0xE0AA93]=75;	Hi[0xE0AAA8]=75;
	Lo[0xE0AAAA]=76;	Hi[0xE0AAB0]=76;
	Lo[0xE0AAB2]=77;	Hi[0xE0AAB3]=77;
	Lo[0xE0AAB5]=78;	Hi[0xE0AAB9]=78;
	Lo[0xE0AABD]=79;	Hi[0xE0AABD]=79;
	Lo[0xE0ABA0]=80;	Hi[0xE0ABA0]=80;
	Lo[0xE0AC85]=81;	Hi[0xE0AC8C]=81;
	Lo[0xE0AC8F]=82;	Hi[0xE0AC90]=82;
	Lo[0xE0AC93]=83;	Hi[0xE0ACA8]=83;
	Lo[0xE0ACAA]=84;	Hi[0xE0ACB0]=84;
	Lo[0xE0ACB2]=85;	Hi[0xE0ACB3]=85;
	Lo[0xE0ACB6]=86;	Hi[0xE0ACB9]=86;
	Lo[0xE0ACBD]=87;	Hi[0xE0ACBD]=87;
	Lo[0xE0AD9C]=88;	Hi[0xE0AD9D]=88;
	Lo[0xE0AD9F]=89;	Hi[0xE0ADA1]=89;
	Lo[0xE0AE85]=90;	Hi[0xE0AE8A]=90;
	Lo[0xE0AE8E]=91;	Hi[0xE0AE90]=91;
	Lo[0xE0AE92]=92;	Hi[0xE0AE95]=92;
	Lo[0xE0AE99]=93;	Hi[0xE0AE9A]=93;
	Lo[0xE0AE9C]=94;	Hi[0xE0AE9C]=94;
	Lo[0xE0AE9E]=95;	Hi[0xE0AE9F]=95;
	Lo[0xE0AEA3]=96;	Hi[0xE0AEA4]=96;
	Lo[0xE0AEA8]=97;	Hi[0xE0AEAA]=97;
	Lo[0xE0AEAE]=98;	Hi[0xE0AEB5]=98;
	Lo[0xE0AEB7]=99;	Hi[0xE0AEB9]=99;
	Lo[0xE0B085]=100;	Hi[0xE0B08C]=100;
	Lo[0xE0B08E]=101;	Hi[0xE0B090]=101;
	Lo[0xE0B092]=102;	Hi[0xE0B0A8]=102;
	Lo[0xE0B0AA]=103;	Hi[0xE0B0B3]=103;
	Lo[0xE0B0B5]=104;	Hi[0xE0B0B9]=104;
	Lo[0xE0B1A0]=105;	Hi[0xE0B1A1]=105;
	Lo[0xE0B285]=106;	Hi[0xE0B28C]=106;
	Lo[0xE0B28E]=107;	Hi[0xE0B290]=107;
	Lo[0xE0B292]=108;	Hi[0xE0B2A8]=108;
	Lo[0xE0B2AA]=109;	Hi[0xE0B2B3]=109;
	Lo[0xE0B2B5]=110;	Hi[0xE0B2B9]=110;
	Lo[0xE0B39E]=111;	Hi[0xE0B39E]=111;
	Lo[0xE0B3A0]=112;	Hi[0xE0B3A1]=112;
	Lo[0xE0B485]=113;	Hi[0xE0B48C]=113;
	Lo[0xE0B48E]=114;	Hi[0xE0B490]=114;
	Lo[0xE0B492]=115;	Hi[0xE0B4A8]=115;
	Lo[0xE0B4AA]=116;	Hi[0xE0B4B9]=116;
	Lo[0xE0B5A0]=117;	Hi[0xE0B5A1]=117;
	Lo[0xE0B881]=118;	Hi[0xE0B8AE]=118;
	Lo[0xE0B8B0]=119;	Hi[0xE0B8B0]=119;
	Lo[0xE0B8B2]=120;	Hi[0xE0B8B3]=120;
	Lo[0xE0B980]=121;	Hi[0xE0B985]=121;
	Lo[0xE0BA81]=122;	Hi[0xE0BA82]=122;
	Lo[0xE0BA84]=123;	Hi[0xE0BA84]=123;
	Lo[0xE0BA87]=124;	Hi[0xE0BA88]=124;
	Lo[0xE0BA8A]=125;	Hi[0xE0BA8A]=125;
	Lo[0xE0BA8D]=126;	Hi[0xE0BA8D]=126;
	Lo[0xE0BA94]=127;	Hi[0xE0BA97]=127;
	Lo[0xE0BA99]=128;	Hi[0xE0BA9F]=128;
	Lo[0xE0BAA1]=129;	Hi[0xE0BAA3]=129;
	Lo[0xE0BAA5]=130;	Hi[0xE0BAA5]=130;
	Lo[0xE0BAA7]=131;	Hi[0xE0BAA7]=131;
	Lo[0xE0BAAA]=132;	Hi[0xE0BAAB]=132;
	Lo[0xE0BAAD]=133;	Hi[0xE0BAAE]=133;
	Lo[0xE0BAB0]=134;	Hi[0xE0BAB0]=134;
	Lo[0xE0BAB2]=135;	Hi[0xE0BAB3]=135;
	Lo[0xE0BABD]=136;	Hi[0xE0BABD]=136;
	Lo[0xE0BB80]=137;	Hi[0xE0BB84]=137;
	Lo[0xE0BD80]=138;	Hi[0xE0BD87]=138;
	Lo[0xE0BD89]=139;	Hi[0xE0BDA9]=139;
	Lo[0xE182A0]=140;	Hi[0xE18385]=140;
	Lo[0xE18390]=141;	Hi[0xE183B6]=141;
	Lo[0xE18480]=142;	Hi[0xE18480]=142;
	Lo[0xE18482]=143;	Hi[0xE18483]=143;
	Lo[0xE18485]=144;	Hi[0xE18487]=144;
	Lo[0xE18489]=145;	Hi[0xE18489]=145;
	Lo[0xE1848B]=146;	Hi[0xE1848C]=146;
	Lo[0xE1848E]=147;	Hi[0xE18492]=147;
	Lo[0xE184BC]=148;	Hi[0xE184BC]=148;
	Lo[0xE184BE]=149;	Hi[0xE184BE]=149;
	Lo[0xE18580]=150;	Hi[0xE18580]=150;
	Lo[0xE1858C]=151;	Hi[0xE1858C]=151;
	Lo[0xE1858E]=152;	Hi[0xE1858E]=152;
	Lo[0xE18590]=153;	Hi[0xE18590]=153;
	Lo[0xE18594]=154;	Hi[0xE18595]=154;
	Lo[0xE18599]=155;	Hi[0xE18599]=155;
	Lo[0xE1859F]=156;	Hi[0xE185A1]=156;
	Lo[0xE185A3]=157;	Hi[0xE185A3]=157;
	Lo[0xE185A5]=158;	Hi[0xE185A5]=158;
	Lo[0xE185A7]=159;	Hi[0xE185A7]=159;
	Lo[0xE185A9]=160;	Hi[0xE185A9]=160;
	Lo[0xE185AD]=161;	Hi[0xE185AE]=161;
	Lo[0xE185B2]=162;	Hi[0xE185B3]=162;
	Lo[0xE185B5]=163;	Hi[0xE185B5]=163;
	Lo[0xE1869E]=164;	Hi[0xE1869E]=164;
	Lo[0xE186A8]=165;	Hi[0xE186A8]=165;
	Lo[0xE186AB]=166;	Hi[0xE186AB]=166;
	Lo[0xE186AE]=167;	Hi[0xE186AF]=167;
	Lo[0xE186B7]=168;	Hi[0xE186B8]=168;
	Lo[0xE186BA]=169;	Hi[0xE186BA]=169;
	Lo[0xE186BC]=170;	Hi[0xE18782]=170;
	Lo[0xE187AB]=171;	Hi[0xE187AB]=171;
	Lo[0xE187B0]=172;	Hi[0xE187B0]=172;
	Lo[0xE187B9]=173;	Hi[0xE187B9]=173;
	Lo[0xE1B880]=174;	Hi[0xE1BA9B]=174;
	Lo[0xE1BAA0]=175;	Hi[0xE1BBB9]=175;
	Lo[0xE1BC80]=176;	Hi[0xE1BC95]=176;
	Lo[0xE1BC98]=177;	Hi[0xE1BC9D]=177;
	Lo[0xE1BCA0]=178;	Hi[0xE1BD85]=178;
	Lo[0xE1BD88]=179;	Hi[0xE1BD8D]=179;
	Lo[0xE1BD90]=180;	Hi[0xE1BD97]=180;
	Lo[0xE1BD99]=181;	Hi[0xE1BD99]=181;
	Lo[0xE1BD9B]=182;	Hi[0xE1BD9B]=182;
	Lo[0xE1BD9D]=183;	Hi[0xE1BD9D]=183;
	Lo[0xE1BD9F]=184;	Hi[0xE1BDBD]=184;
	Lo[0xE1BE80]=185;	Hi[0xE1BEB4]=185;
	Lo[0xE1BEB6]=186;	Hi[0xE1BEBC]=186;
	Lo[0xE1BEBE]=187;	Hi[0xE1BEBE]=187;
	Lo[0xE1BF82]=188;	Hi[0xE1BF84]=188;
	Lo[0xE1BF86]=189;	Hi[0xE1BF8C]=189;
	Lo[0xE1BF90]=190;	Hi[0xE1BF93]=190;
	Lo[0xE1BF96]=191;	Hi[0xE1BF9B]=191;
	Lo[0xE1BFA0]=192;	Hi[0xE1BFAC]=192;
	Lo[0xE1BFB2]=193;	Hi[0xE1BFB4]=193;
	Lo[0xE1BFB6]=194;	Hi[0xE1BFBC]=194;
	Lo[0xE284A6]=195;	Hi[0xE284A6]=195;
	Lo[0xE284AA]=196;	Hi[0xE284AB]=196;
	Lo[0xE284AE]=197;	Hi[0xE284AE]=197;
	Lo[0xE28680]=198;	Hi[0xE28682]=198;
	Lo[0xE38181]=199;	Hi[0xE38294]=199;
	Lo[0xE382A1]=200;	Hi[0xE383BA]=200;
	Lo[0xE38485]=201;	Hi[0xE384AC]=201;
	Lo[0xEAB080]=202;	Hi[0xED9EA3]=202;
}
void BaseChar::shutdown() {
	Lo.clear();
	Hi.clear();
}
//--------------------------------------

