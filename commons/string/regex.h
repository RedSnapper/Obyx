/* 
 * regex.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * regex.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_STRING_REGEX_H
#define OBYX_STRING_REGEX_H

#include <string>
#include <map>
#include <ext/hash_map>
#include <pcre.h>
using namespace std;
using namespace __gnu_cxx; //hashmap namespace.

extern "C" {
	typedef struct real_pcre pcre;
}

namespace String {
	class Regex {
	private:
		typedef pair<pcre*,pcre_extra*> pccache_item;
		typedef hash_map<const string, pccache_item, hash<const string&> > type_regex_cache;
		static type_regex_cache regex_cache;
		
		static void * pcre_lib_handle;
		static bool loadattempted;	//used to show if the service is up or down.
		static bool loaded;	//used to show if the service is up or down.
		
		//The pcre object(s) that we use.		
		static int re_options;
		static int mt_options;
		static bool compile(const string &, pcre*&, pcre_extra*&);
		static size_t matcher(const string&, const string&,const int, int*&,int&);
		static void reporterror(int);
		static void dlerr(std::string&);
		
		//The pcre API that we use.		
		static pcre* (*pcre_compile)(const char*, int, const char**, int*,const unsigned char*);
		static int (*pcre_exec)(const pcre*,const pcre_extra*,PCRE_SPTR,int,int,int,int*,int);
		static int (*pcre_config)(int,void *);
		static void (*pcre_free)(void *);
		static pcre_extra* (*pcre_study)(const pcre*,int,const char**);
		//		static void (*pcre_free)(void *); // appears to be causing crashes.
		
	public:
		static const string xml_doctype_prolog;
		static const string xml_namespace_prolog;
		static const string xml_schema_prolog;
		
		static bool startup(string&);
		static bool available();
		static bool shutdown();
		
		static bool field(const string &,const string &,unsigned int,string &);
		static size_t replace(const string &,const string &,string &,bool=false);  //replace uses pcre - bbedit like search/replace stuff.
		static bool match(const string &,const string &);               //match substring using pcre
		static size_t first(const string &,const string &);				//first char position of first match.
		static size_t after(const string &,const string &);				//return position of first char after first match.
		static bool fullmatch(const string &,const string &);           //match entire string using pcre
	};
}

#endif

