/* 
 * httpfetcher.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httpfetcher.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "httpfetcher.h"
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "commons/string/strings.h"
#include "commons/xml/xml.h"
#include "core/itemstore.h"

using namespace Log;
using namespace XML;

namespace Fetch {
	
	HTTPFetcher::HTTPFetcher(std::string& page_i, HTTPFetchHeader& header_i, std::string& err_i, std::vector<std::string>& redirects_i): page(page_i), header(header_i),errstr(err_i),redirects(redirects_i) {}
	
	bool HTTPFetcher::operator()() {
		bool found = false;
		while(!found && retrieve(errstr) ) {
				if(header.statusCode < 300 || header.statusCode >= 400) {
					found = true; // no redirect
				} else {	// redirect
					string redirect_val;
					unsigned long maxRedirects = 10;
					if (ItemStore::get("REDIRECT_BREAK_COUNT",redirect_val)) {
						pair<long long,bool> enval = String::integer(redirect_val);
						maxRedirects = (unsigned long)enval.first;
					} 
					if( redirects.size() >= maxRedirects ) {
						*Logger::log << Log::warn << Log::LI << "Stopped redirecting after maximum number ("
						<< (double)maxRedirects << ") of redirects" << Log::LO << Log::blockend;
						redirects.push_back(page);
						return false;
					}
					HTTPFetchHeader::HeaderMap::const_iterator p = header.fields.find("Location");
					if(p == header.fields.end()) { // failed to specify the location for the redirect
						break; 
					} else {
						std::string relativeUri(String::fixUriSpaces(p->second));
						if(relativeUri.empty()) {						
							break; // empty redirect location
						} else {
							redirects.push_back(page);
							XMLCh* relUri = XML::transcode(relativeUri);
							XMLCh* xpage = XML::transcode(page);
							XMLUri base(xpage);
							XMLUri uri(&base, relUri);
							transcode(uri.getUriText(),page);
							XMLString::release(&relUri);
							XMLString::release(&xpage);
							if (Logger::debugging()) {
								*Logger::log << Log::info << Log::LI << "Redirecting to: " << page << Log::LO << Log::blockend;
							}
						}
					}
				}
			
		}
		if(found == false || header.statusCode < 200 || header.statusCode >= 300) {
			return false;
		}
		return true;
	}
	
} //namespace

