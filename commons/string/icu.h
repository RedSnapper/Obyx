/*
 * icu.h is authored and maintained by Ben Griffin of Red Snapper Ltd
 * icu.h is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is C Opyright (C) 2006-2010 Red Snapper Ltd. http://www.redsnapper.net
 * The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your  Option)
 * any later version.
 *
 * This program is distributed in the h Ope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a c Opy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Documents for this part of ICU can be found at http://icu-project.org/apiref/icu4c/utrans_8h.html
 */

#ifndef OBYX_ICU_H
#define OBYX_ICU_H

#ifndef DISALLOW_ICU

#include <map>
#include <vector>
#include <sstream>
#include <string>
#include <unicode/utrans.h>

namespace {
#ifndef u_str
	#include "xercesc/util/Xerces_autoconf_config.hpp"
	typedef std::basic_string<char16_t> u_str;
	typedef std::basic_string<XMLCh> x_str;
#endif
}

using namespace std;

namespace String {
		class TransliterationService {
		private:
			
			static void* i18n;
			static void* uc;
			static void* tu;

			static bool loadattempted;	//used to show if the service is up or down.
			static bool loaded;			//used to show if the service is up or down.
			static UTransliterator* asciiservice;
			static UErrorCode errcode;

			static void (*u_init)(UErrorCode*);
			static void (*u_cleanup)();
			static const char* 	(*u_errorName)(UErrorCode);

			//	UTransliterator * 	utrans_openU (const UChar *id, int32_t idLength, UTransDirection dir, const UChar *rules, int32_t rulesLength, UParseError *parseError, UErrorCode *pErrorCode)
			//	Open a custom transliterator, given a custom rules string OR a system transliterator, given its ID.
			static UTransliterator* (*utrans_openU)(const UChar*,int32_t,UTransDirection,const UChar*,int32_t,UParseError*,UErrorCode*);

			//	void 	utrans_close (UTransliterator *trans)
			//	Close a transliterator.
			static void (*utrans_close)(UTransliterator*);

			//	void 	utrans_transUChars (const UTransliterator *trans, UChar *text, int32_t *textLength, int32_t textCapacity, int32_t start, int32_t *limit, UErrorCode *status)
			//	Transliterate a segment of a UChar* string.
			static void (*utrans_transUChars)(const UTransliterator*, UChar *, int32_t *, int32_t, int32_t, int32_t*, UErrorCode*);
			
			static void dlerr(string&);
			
		public:
			static bool startup(string&);
			static bool available();
			static bool shutdown();
			static void ascii(u_str&,string&);
			static void transliterate(u_str&,u_str&,string&);
			
		};
}
#endif

#endif

