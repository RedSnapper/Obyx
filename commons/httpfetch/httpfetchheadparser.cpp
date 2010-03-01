/* 
 * httpfetchheadparser.cpp is authored and maintained by Sam Lindley, Ben Griffin of Red Snapper Ltd 
 * httpfetchheadparser.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <sstream>
#include <iostream>
#include <cstddef>

#include "httpfetchheadparser.h"
#include "httpfetchheader.h"

namespace {
	struct HTTPFetchHeadParserException {};
}

namespace Fetch {
	using std::operator>>;
	
	HTTPFetchHeadParser::HTTPFetchHeadParser( const std::string& p_headerString, std::string& p_cookies, HTTPFetchHeader& p_header)
	: headerString(p_headerString), cookiesString(p_cookies), header(p_header)
	{}
	
	void HTTPFetchHeadParser::operator()() {
		size_t pos;
		try {
			pos = readFirstLine();
			pos = readAttributes(pos);
		}
		catch(const HTTPFetchHeadParserException&) {
//			std::cerr << "Parser error in HTTP header:\n" << headerString << std::endl;
		}
	}
	
	size_t HTTPFetchHeadParser::readFirstLine() {
		size_t endLinePos = headerString.find_first_of("\r\n");
		if(endLinePos == std::string::npos)
			throw HTTPFetchHeadParserException();
		
		size_t pos = 0;
		pos = readVersion(pos);
		pos = readStatusCode(pos);
		pos = readReasonPhrase(pos, endLinePos);
		
		return pos;
	}
	
	size_t HTTPFetchHeadParser::readVersion(size_t pos) {
		size_t nextPos = headerString.find(' ', pos);
		if(nextPos == std::string::npos)
			throw HTTPFetchHeadParserException();
		header.version = headerString.substr(pos, nextPos-pos);
		return ++nextPos;
	}
	
	size_t HTTPFetchHeadParser::readStatusCode(size_t pos) {
		size_t nextPos = headerString.find(' ', pos);
		if(nextPos == std::string::npos)
			throw HTTPFetchHeadParserException();
		std::istringstream ist(headerString.substr(pos, nextPos-pos));
		ist >> header.statusCode;
		return ++nextPos;
	}
	
	size_t HTTPFetchHeadParser::readReasonPhrase(size_t pos, size_t endLinePos) {
		header.reasonPhrase = headerString.substr(pos, endLinePos-pos);
		return nextLine(endLinePos);
	}
	
	// move onto next line
	size_t HTTPFetchHeadParser::nextLine(size_t pos) {
		// according to RFC 2616 lines should end in 
		// "\r\n", but medusa, for instance just returns '\n'
		// so we just keep going until there are no more '\r' or '\n' characters
		return headerString.find_first_not_of("\r\n", pos);
	}
	
	
	size_t HTTPFetchHeadParser::readAttribute(size_t startLinePos) {
		size_t endLinePos = headerString.find_first_of("\r\n", startLinePos);
		if(endLinePos == std::string::npos)
			return std::string::npos;
		size_t pos = headerString.find(':', startLinePos);
		if(pos > endLinePos) // ignore bad attributes
			return nextLine(endLinePos);
		std::string name(headerString, startLinePos, pos-startLinePos);
		// skip leading white space
		if (name.compare("Set-Cookie") == 0) {
			std::string ckline(headerString, startLinePos, endLinePos);
			cookiesString.append(ckline); 
		}
        pos = headerString.find_first_not_of("\t ", pos+1);
		std::string value(headerString, pos, endLinePos-pos);
		(header.fields)[name] = value;
		return nextLine(endLinePos);
	}
	
	size_t HTTPFetchHeadParser::readAttributes(size_t startLinePos) {
		size_t pos = startLinePos;
		while((pos = readAttribute(pos)) != std::string::npos);
		return pos;
	}
}//namespace RFile

