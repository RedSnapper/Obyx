/*
 * xmlerrhandler.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlerrhandler.h is a part of Obyx - see http://www.obyx.org .
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

// xmlerrhandler.h
#ifndef XML_ERROR_HANDLER_H
#define XML_ERROR_HANDLER_H

#include "commons/xml/xml.h"
#include "commons/string/strings.h"

#include <string>
#include <unordered_map>
#include <map>

namespace XML {

	using xercesc::DOMErrorHandler;
	using xercesc::DOMError;
	using xercesc::DOMNode;
	using namespace std;

	typedef unordered_map<const string,string, hash<const string&> > ss_map_type;
	
	class XMLErrorHandler : public DOMErrorHandler {
private:
		bool fSawErrors;
		bool fGrammar;

public:
	
		void setGrammar(bool set) { fGrammar = set;}
		bool getGrammar() { return fGrammar;}
		void resetErrors();
		bool handleError(const DOMError& domError);
		bool hadErrors();

		XMLErrorHandler();
		~XMLErrorHandler();
	};

	
} // XML

#endif // XML_ERROR_HANDLER_H
