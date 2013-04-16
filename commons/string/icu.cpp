/*
 * icu.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd
 * icu.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <dlfcn.h>
#include <string.h>
#include "icu.h"
#include "commons/environment/environment.h"
#include "commons/dlso.h"

#ifndef DISALLOW_ICU

namespace String {

	bool TransliterationService::loadattempted = false;
	bool TransliterationService::loaded = false;

	void* TransliterationService::i18n= NULL;
	void* TransliterationService::uc= NULL;
	void* TransliterationService::tu= NULL;
	
	UTransliterator* TransliterationService::transservice = NULL;
	UErrorCode TransliterationService::errcode;

	void (*TransliterationService::u_init)(UErrorCode*);
	void (*TransliterationService::u_cleanup)();
	const char* 	(*TransliterationService::u_errorName)(UErrorCode);
	
	UTransliterator* (*TransliterationService::utrans_openU)(const UChar*,int32_t,UTransDirection,const UChar*,int32_t,UParseError*,UErrorCode*);
	void (*TransliterationService::utrans_close)(UTransliterator *);
	void (*TransliterationService::utrans_transUChars)(const UTransliterator*,UChar*,int32_t*,int32_t,int32_t,int32_t*,UErrorCode*);
	
	void TransliterationService::dlerr(std::string& errstr) {
		const char *err = dlerror();
		if (err != NULL) {
			errstr.append(err);
			errstr.append("; ");
		}
	}
	bool TransliterationService::available(string& errors) {
		if (!loadattempted) {
			startup(errors);
		}
		return loaded;
	}
	bool TransliterationService::startup(string& errors) {
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string icupath,sufstr;
			if (Environment::getbenv("OBYX_LIBICUDIR",icupath)) {
				if (*icupath.rbegin() != '/') icupath.push_back('/');
			}
			Environment::getbenv("OBYX_LIBICUVS",sufstr);
			string i18nstr = SO(icupath,libicui18n); i18n = dlopen(i18nstr.c_str(),RTLD_GLOBAL | RTLD_NOW); dlerr(errors);
			string ucstr = SO(icupath,libicuuc);   	 uc = dlopen(ucstr.c_str(),RTLD_GLOBAL | RTLD_NOW); dlerr(errors);
			string tustr = SO(icupath,libicutu);   	 tu = dlopen(tustr.c_str(),RTLD_GLOBAL | RTLD_NOW); dlerr(errors);
			
			if (errors.empty() && uc != NULL && i18n != NULL && tu != NULL) {
				string init="u_init"+sufstr;
				string cleanup="u_cleanup"+sufstr;
				string open="utrans_openU"+sufstr;
				string openo="utrans_open"+sufstr;
				string close="utrans_close"+sufstr;
				string trans="utrans_transUChars"+sufstr;
				string errname="u_errorName"+sufstr;

				u_init = (void (*)(UErrorCode*)) dlsym(uc,init.c_str()); dlerr(errors);
				u_cleanup = (void (*)()) dlsym(uc,cleanup.c_str()); dlerr(errors);
				u_errorName = (const char* (*)(UErrorCode)) dlsym(tu,errname.c_str()); dlerr(errors);
				utrans_openU = (UTransliterator* (*)(const UChar*,int32_t,UTransDirection,const UChar*,int32_t,UParseError*,UErrorCode*)) dlsym(i18n,open.c_str()); dlerr(errors);
				utrans_close = (void (*)(UTransliterator *)) dlsym(i18n,close.c_str()); dlerr(errors);
				utrans_transUChars = (void (*)(const UTransliterator*, UChar*, int32_t*, int32_t, int32_t, int32_t*, UErrorCode*)) dlsym(i18n,trans.c_str()); dlerr(errors);
				
				if ( errors.empty() ) {
					loaded = true;
					u_init(&errcode);
					transservice = utrans_openU((const UChar*)(L"Any-NFKD;Any-Latin;Latin-ASCII;[\\u007F-\\uFB02] Remove;"),-1,UTRANS_FORWARD,NULL,0,NULL,&errcode);
					if (errcode != 0) {
						errors.append("Transliteration service found and loaded but the service failed with the error: ");
						errors.append(u_errorName(errcode));
					}
				}
			}
		}
		return loaded;
	}
	bool TransliterationService::shutdown() {		//necessary IFF script uses pcre.
		if (loaded && transservice != NULL) {
			utrans_close(transservice);
			u_cleanup();
		}
		if ( i18n != NULL ) {
			dlclose(i18n);
			dlclose(uc);
		}
		return true;
	}
	void TransliterationService::transliterate(u_str& basis,string& errstr) {
		if (available(errstr)) {
			UChar buffer[4*basis.size()+1]; 				//16 bit mem-array buffer single chars can transliterate to 4 ascii.
			memcpy(buffer,basis.c_str(),2*basis.size());	//memcpy uses 1 byte for size
			buffer[basis.size()] = 0;
			int32_t length=(int32_t)basis.size(),size=(int32_t)(4*basis.size()),limit=length;
			utrans_transUChars(transservice,buffer,&length,size,0,&limit,&errcode);
			if (errcode == U_ZERO_ERROR) {
				basis.clear();//
				basis.append(buffer,length);
			}
		} else {
			errstr.append("Transliteration service was not loaded or is not available.  ");
		}
	};
}

#endif
