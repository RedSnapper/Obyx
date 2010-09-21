/* 
 * httpfetch.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httpfetch.h is a part of Obyx - see http://www.obyx.org .
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

/*
 *  History
 *  Created 11/06/2007.
 */

#ifndef pageretriever_H
#define pageretriever_H

#include <string>
#include <vector>
#include <curl/curl.h>
#include <curl/easy.h>

#include "httpfetcher.h"
#include "httpfetchleaf.h"

namespace Fetch {
	struct HTTPFetchHeader;
	
	class HTTPFetch {
	private:
		static void* lib_handle;
		static bool loadattempted;	//used to show if the service is up or down.
		static bool loaded;			//used to show if the service is up or down.
		static CURL* (*curl_easy_init)();
		static CURLcode (*curl_easy_setopt)(CURL *, CURLoption, ...);
		static CURLcode (*curl_easy_perform)(CURL *);
		static void (*curl_easy_cleanup)(CURL *);
		static void (*curl_slist_free_all)(struct curl_slist *);
		static struct curl_slist* (*curl_slist_append)(struct curl_slist*,const char *);
		static curl_version_info_data* (*curl_version_info)(CURLversion);

		static size_t writeMemoryCallback(char *, size_t, size_t, void *);
		static size_t readMemoryCallback(void *, size_t, size_t, void *);
		static int debugCallback(CURL*,curl_infotype,char*,size_t,void*);
		
		//-- End of dll stuff		
		struct curl_slist *headers;		
		std::string cookies;
		std::string body;
		CURL* handle;
		char* errorBuf;
		bool had_error;
		
		class PageFetcher;
		class HeaderFetcher;
		
	private:
		friend class HTTPFetchPage;	
		friend class HTTPFetchHead;	
		
		
		void processErrorCode(CURLcode, std::string&);
		bool fetchHeader(std::string&, HTTPFetchHeader&, Redirects&,std::string&);
		void setBody(std::string&);
		bool retrievePage(const std::string&, HTTPFetchHeader&, std::string&, std::string&);
		bool retrieveHeader(const std::string&, HTTPFetchHeader&, std::string&); 
		static void dlerr(std::string&);
		
	public:
		HTTPFetch(string&);
		HTTPFetch(string&,string&,string&,string&,string&);
		~HTTPFetch();
		bool fetchPage(std::string, HTTPFetchHeader&, Redirects&, std::string&, std::string&);
		void addHeader(std::string&);
		bool doRequest(string&,string&,string&);
		static bool startup();
		static bool available();
		static bool shutdown();
		
	};
} // namespace Fetch


#endif//RETRIEVE_PAGE_H
