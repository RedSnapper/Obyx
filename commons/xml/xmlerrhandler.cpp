/*
 * xmlerrhandler.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlerrhandler.cpp is a part of Obyx - see http://www.obyx.org .
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
#include "commons/filing/filing.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"

using Log::warn;
using xercesc::DOMErrorHandler;
using xercesc::DOMError;

namespace XML {
	// ---------------------------------------------------------------------------
	//  XMLErrorHandler: Constructors and Destructor
	// ---------------------------------------------------------------------------

	XMLErrorHandler::XMLErrorHandler() : fSawErrors(false) {}
	XMLErrorHandler::~XMLErrorHandler() { }
	
//errors are already framed within a li - so we must start with a type and end with a blockend.
	bool XMLErrorHandler::handleError(const xercesc::DOMError& domError) {
		if (domError.getSeverity() != xercesc::DOMError::DOM_SEVERITY_WARNING) { //discard warnings?
			fSawErrors = true;
			string err_message; 
			transcode(domError.getMessage(),err_message);
			*Logger::log << Log::error << Log::LI << "DOM Error:" << err_message << Log::LO;
			DOMLocator* loc = domError.getLocation(); //
			if (loc != NULL) {
				DOMNode* errnode = loc->getRelatedNode();
				if (!fGrammar && (errnode != NULL)) { 
					string info_string;
					basic_string<XMLCh> xp = Manager::parser()->xpath(errnode);
					transcode(xp.c_str(),info_string);
					if (info_string.empty()) {
						DOMDocument* errdoc = errnode->getOwnerDocument();
						Manager::parser()->writedoc(errdoc,info_string);
						if (!info_string.empty()) {
							*Logger::log << Log::LI << "The parser managed to get" << Log::LO ;
							*Logger::log << Log::LI << Log::info << Log::LI << info_string << Log::LO << Log::blockend << Log::LO;
						}
					} else {
						*Logger::log << Log::LI << "XPath: " << info_string << Log::LO ;
					}
				}
			}
			*Logger::log << Log::blockend;
		}
		return true;
	}
	
	bool XMLErrorHandler::hadErrors() {
		return fSawErrors;
	}

	void XMLErrorHandler::resetErrors() {
		fSawErrors = false;
	}

} // namespace XML
