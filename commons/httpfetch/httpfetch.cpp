/* 
 * httpfetch.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httpfetch.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <set>
#include <string>
#include <sstream>
#include <cstddef>
#include <cassert>
#include <dlfcn.h>

#include <curl/curl.h>

#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "core/itemstore.h"

#include "httpfetch.h"
#include "httpfetchheadparser.h"

using namespace Log;
using namespace XML;

namespace Fetch {
	size_t HTTPFetch::writeMemoryCallback(char *ptr, size_t size, size_t nmemb, void *data) {
		std::string* s = static_cast<std::string*>(data);
		size_t realSize = size * nmemb;
		s->append(ptr, ptr + realSize);
		return realSize;
	}
/*
 Function pointer that should match the following prototype: int curl_debug_callback (CURL *, curl_infotype, char *, size_t, void *); 
 CURLOPT_DEBUGFUNCTION replaces the standard debug function used when CURLOPT_VERBOSE is in effect. 
 This callback receives debug information, as specified with the curl_infotype argument. 
 This function must return 0. 
 The data pointed to by the char * passed to this function WILL NOT be zero terminated, 
 but will be exactly of the size as told by the size_t argument.
  */	
	int HTTPFetch::debugCallback(CURL*,curl_infotype i,char* m,size_t len,void*) {
		switch (i) {
			case CURLINFO_HEADER_OUT:
			case CURLINFO_DATA_OUT: {
				string message(m,len);
				*Logger::log << Log::LI << "curl (o) '" << message << "'." << Log::LO;
			} break;
			case CURLINFO_HEADER_IN:
			case CURLINFO_DATA_IN: {
				string message(m,len);
				*Logger::log << Log::LI << "curl (i) '" << message << "'." << Log::LO;
			} break;
			default: {
				string message(m,len);
				*Logger::log << Log::LI << "curl (-) '" << message << "'." << Log::LO;
			} break; 
		}
		return 0;
	}
	
	/*
	 Function pointer that should match the following prototype: 
	 size_t function( void *ptr, size_t size, size_t nmemb, void *stream); 
	 This function gets called by libcurl as soon as it needs to read data in order to send it to the peer. 
	 The data area pointed at by the pointer ptr may be filled with at most size multiplied with nmemb number of bytes. 
	 Your function must return the actual number of bytes that you stored in that memory area. 
	 Returning 0 will signal end-of-file to the library and cause it to stop the current transfer.
	 
	 If you stop the current transfer by returning 0 "prematurely" 
	 (i.e before the server expected it, like when you've told you will upload N bytes and you upload less than N bytes),
	 you may experience that the server "hangs" waiting for the rest of the data that won't come.
	 
	 If you set the callback pointer to NULL, or doesn't set it at all, the default internal read 
	 function will be used. It is simply doing an fread() on the FILE * stream set with CURLOPT_READDATA.	
	 */ 
	size_t HTTPFetch::readMemoryCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
		Environment* env = Environment::service();
		env->setparm("bodyread","true");
		std::string* s = static_cast<std::string*>(stream); //we must actually change the string...
		size_t realSize = size * nmemb;
		size_t result = s->copy(static_cast<char*>(ptr), realSize);
		try { s->erase(0, result); } catch(...) { }
		return result;
	}
	
	bool HTTPFetch::loadattempted = false;
	bool HTTPFetch::loaded = false;
	void* HTTPFetch::lib_handle = NULL;
	
	void (*HTTPFetch::curl_slist_free_all)(struct curl_slist *)= NULL;
	struct curl_slist* (*HTTPFetch::curl_slist_append)(struct curl_slist*,const char *) = NULL;
	CURL* (*HTTPFetch::curl_easy_init)() = NULL;
	CURLcode (*HTTPFetch::curl_easy_setopt)(CURL*, CURLoption, ...) = NULL;
	CURLcode (*HTTPFetch::curl_easy_perform)(CURL*)= NULL;
	void (*HTTPFetch::curl_easy_cleanup)(CURL*) = NULL;
	curl_version_info_data* (*HTTPFetch::curl_version_info)(CURLversion)  = NULL;

	
	bool HTTPFetch::available() {
		if (!loadattempted) startup();
		return loaded;
	}
	void HTTPFetch::dlerr(std::string& container) {
		const char *err = dlerror();
		if (err != NULL) {
			container.append(err);
		}
	}
	
	bool HTTPFetch::startup() {	
		std::string err=""; //necessary IFF script uses pcre.
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string curllib;
			if (!Environment::getbenv("OBYX_LIBCURLSO",curllib)) curllib = "libcurl.so";
			lib_handle = dlopen(curllib.c_str(),RTLD_GLOBAL | RTLD_NOW);
			dlerr(err);
			if (err.empty() && lib_handle != NULL ) {
				curl_easy_init = (CURL* (*)()) dlsym(lib_handle,"curl_easy_init");
				curl_easy_setopt = (CURLcode (*)(CURL *,CURLoption,...)) dlsym(lib_handle,"curl_easy_setopt");
				curl_easy_perform = (CURLcode (*)(CURL *)) dlsym(lib_handle,"curl_easy_perform");
				curl_easy_cleanup = (void (*)(CURL *)) dlsym(lib_handle,"curl_easy_cleanup");
				curl_slist_free_all = (void (*)(struct curl_slist*)) dlsym(lib_handle,"curl_slist_free_all");
				curl_slist_append = (curl_slist* (*)(struct curl_slist*,const char *)) dlsym(lib_handle,"curl_slist_append");
				curl_version_info = (curl_version_info_data* (*)(CURLversion)) dlsym(lib_handle,"curl_version_info");
				dlerr(err);
				if ( err.empty() ) {
					if ( curl_slist_append != NULL && curl_slist_free_all != NULL && curl_easy_init != NULL && curl_easy_setopt!=NULL && curl_easy_perform!=NULL && curl_easy_cleanup!=NULL) {
						loaded = true;
					}
				} else {
					//					string msg = err;
					//					*Logger::log << Log::debug << Log::LI << "HTTPFetch::startup() dlsym reported '" << err << "'." << Log::LO << Log::blockend;
				}
			} else {
				//				string msg = err;
				//				*Logger::log << Log::debug << Log::LI << "HTTPFetch::startup() dlopen reported '" << err << "'." << Log::LO << Log::blockend;
			}
		}
		return loaded;
	}
	
	bool HTTPFetch::shutdown() {											 //necessary IFF script uses pcre.
		if ( lib_handle != NULL ) {
			dlclose(lib_handle);
		}
		return true;
	}
	//	CURLE_SEND_FAIL_REWIND -- don't know what this is, or when it fails.
	void HTTPFetch::processErrorCode(CURLcode errorCode, string& errstr) {
		if (! had_error && errorCode != CURLE_OK && errorCode != CURLE_SEND_FAIL_REWIND) {
			errstr.append(errorBuf);
			had_error = true;
		}
	}
	
	void HTTPFetch::addHeader(std::string& headerString) {
		headers = curl_slist_append(headers, headerString.c_str());
	}
	
	// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html
	
	HTTPFetch::HTTPFetch(string& u,string& m,string& v,string& b,string& errstr) : headers(NULL),cookies(),body(b),handle(NULL),errorBuf(new char[CURL_ERROR_SIZE]),had_error(false) {
		errorBuf[0] = '\0'; 
		handle = curl_easy_init();
		assert(handle != NULL);
		//		processErrorCode(curl_easy_setopt(handle, CURLOPT_VERBOSE, true));
		string redirect_val,timeout_val;
		unsigned long maxRedirects = 10,timeout_seconds=30;
		if (ItemStore::get("REDIRECT_BREAK_COUNT",redirect_val)) {
			pair<long long,bool> enval = String::integer(redirect_val);
			maxRedirects = (unsigned long)enval.first;
		} 
		if (ItemStore::get("URL_TIMEOUT_SECS",timeout_val)) {
			pair<long long,bool> toval = String::integer(timeout_val);
			timeout_seconds = (unsigned long)toval.first;
		} 
		if (maxRedirects == 0) {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION,0), errstr);
		} else {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION,1), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_AUTOREFERER,1), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_MAXREDIRS,maxRedirects), errstr);
		}
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1), errstr);
		//		processErrorCode(curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout_milli_seconds));
		processErrorCode(curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout_seconds), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOBODY, 0), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_COOKIEFILE, ""), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorBuf), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_POSTFIELDS, NULL), errstr);
		if ( Logger::debugging() ) {
//			processErrorCode(curl_easy_setopt(handle, CURLOPT_DEBUGDATA, &body), errstr); //
			processErrorCode(curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, debugCallback), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_VERBOSE, 1), errstr);
		}
		processErrorCode(curl_easy_setopt(handle, CURLOPT_READFUNCTION, readMemoryCallback), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_READDATA, &body), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeMemoryCallback), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_URL, u.c_str()), errstr);
		if (m.compare("GET") == 0) {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_HTTPGET, 1), errstr);
		} else if (m.compare("POST") == 0) {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_POST, 1), errstr); //or CURLOPT_HTTPPOST ??
		} else {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, m.c_str()), errstr);
		}
		if (v.compare("HTTP/1.1") == 0) {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1), errstr);
		} else {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0), errstr); //or CURLOPT_HTTPPOST ??
		}
	}
	
	//- HTTPFetch methods -//
	HTTPFetch::HTTPFetch(std::string& errstr) : headers(NULL),cookies(),body(),handle(NULL),errorBuf(new char[CURL_ERROR_SIZE]),had_error(false) {
		Environment* env = Environment::service();
		errorBuf[0] = '\0'; 
		cookies = env->response_cookies(false); //We may want to think about this...
		handle = curl_easy_init();
		assert(handle != NULL);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_COOKIEFILE, ""), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorBuf), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeMemoryCallback), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0), errstr);
	}
	
	HTTPFetch::~HTTPFetch() {
		if (headers != NULL) { 
			curl_slist_free_all(headers); //crud - crashes if headers = NULL.
		}
		curl_easy_cleanup(handle);
		delete [] errorBuf;
		body.clear();
	}
	
	//used by OSI Request directly.
	bool HTTPFetch::doRequest(string& headerString,string& bodyString,string& errstr) {
		string redirect_val,timeout_val;
		unsigned long maxRedirects = 0,timeout_seconds=30;
		if ( Logger::debugging() ) {
			curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
			if (info != NULL) {
				*Logger::log << Log::debug << Log::LI << "libcurl/";
				*Logger::log << info->version << " " << info->ssl_version << " libz/" << info->libz_version << " " << info->libssh_version;
				*Logger::log << Log::LO;
			}
		}
		
		if (ItemStore::get("REDIRECT_BREAK_COUNT",redirect_val)) {
			pair<long long,bool> enval = String::integer(redirect_val);
			maxRedirects = (unsigned long)enval.first;
		} 
		if (ItemStore::get("URL_TIMEOUT_SECS",timeout_val)) {
			pair<long long,bool> toval = String::integer(timeout_val);
			timeout_seconds = (unsigned long)toval.first;
		} 
		if (maxRedirects == 0) {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION,0), errstr);
		} else {
			processErrorCode(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION,1), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_AUTOREFERER,1), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_MAXREDIRS,maxRedirects), errstr);
		}
		processErrorCode(curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout_seconds), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1), errstr);
		if (!body.empty()) {
			if ( Logger::debugging() ) {
				*Logger::log << Log::LI << "There is a body to process." << Log::LO;
			}
			string cl="Content-Length: ";
			cl.append(String::tostring((unsigned long long)body.size()));
			headers = curl_slist_append(headers, headerString.c_str());
			processErrorCode(curl_easy_setopt(handle, CURLOPT_POST, 1), errstr); //
			processErrorCode(curl_easy_setopt(handle, CURLOPT_READFUNCTION, readMemoryCallback), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_READDATA, &body), errstr);	//This is not a c_string- it's used by readMemoryCallback.
			processErrorCode(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, body.size()), errstr);
			processErrorCode(curl_easy_setopt(handle, CURLOPT_NOBODY, 0), errstr);
		}
		processErrorCode(curl_easy_setopt(handle, CURLOPT_FAILONERROR, false), errstr); //Prevents output on return values > 300
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &bodyString), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEHEADER, &headerString), errstr);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers), errstr);
		processErrorCode(curl_easy_perform(handle), errstr);
		curl_slist_free_all(headers); headers=NULL;
		size_t codepoint = headerString.find(' ') + 1;
		if (codepoint != string::npos) {
			string::const_iterator cp = headerString.begin() + codepoint;
			unsigned int code = String::natural(cp);
			if (code > 300) {
				errstr = errorBuf;
				if (errstr.empty()) { errstr  = "Request Error"; }
				had_error = true;
			}
		}
		if ( Logger::debugging() ) {
			*Logger::log << Log::blockend;
		}
		return ! had_error; //This does NOT return an error if the return code is > 300.
	}
	
	//used by space="url" indirectly
	bool HTTPFetch::retrievePage(const string& urlString, HTTPFetchHeader& header, string& my_page, string& my_errors) {
		std::string headerString;
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_COOKIELIST, cookies.c_str()), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_FAILONERROR, true), my_errors); //Prevents output on return values > 300
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOBODY, false), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_HTTPGET, true), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_URL, urlString.c_str()), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &my_page), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEHEADER, &headerString), my_errors);
		processErrorCode(curl_easy_perform(handle), my_errors);
		if(headerString.empty() && ! had_error ) {
			my_page.clear(); // important!
			processErrorCode(curl_easy_perform(handle), my_errors);
		}
		HTTPFetchHeadParser headerParser(headerString,cookies,header);
		headerParser();
		return ! had_error;
	}
	
	bool HTTPFetch::retrieveHeader(const std::string& urlString, HTTPFetchHeader& header, string& my_errors) {
		std::string headerString;
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_FAILONERROR, true), my_errors);	//This doesn't prevent redirects, and saves bacon on some odd lib. issues.
		processErrorCode(curl_easy_setopt(handle, CURLOPT_NOBODY, true), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_URL,urlString.c_str()), my_errors);
		processErrorCode(curl_easy_setopt(handle, CURLOPT_WRITEHEADER, &headerString), my_errors);
		processErrorCode(curl_easy_perform(handle), my_errors);
		if(headerString.empty() && ! had_error ) {
			processErrorCode(curl_easy_perform(handle), my_errors);
		}
		HTTPFetchHeadParser headerParser(headerString, cookies, header);
		headerParser();
		return ! had_error;
	}
	
	bool HTTPFetch::fetchHeader(std::string& my_page, HTTPFetchHeader& header, Redirects& redirects, std::string& errstr) {
		HTTPFetchHead fetcher(this, my_page, header, redirects, errstr);
		return fetcher();
	}
	
	bool HTTPFetch::fetchPage(std::string my_page, HTTPFetchHeader& header, Redirects& redirects, std::string& body_i, std::string& errmsg_i) {
		HTTPFetchPage fetcher(this, my_page, header, redirects, body_i, errmsg_i);
		return fetcher();
	}
	
} //namespace Fetch
