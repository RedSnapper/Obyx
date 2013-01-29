/* 
 * regex.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * regex.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <string>
#include <pcre.h>
#include <dlfcn.h>
#include <string.h>

#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "regex.h"
#include "convert.h"

using namespace std;

namespace String {

	const string Regex::xml_doctype_prolog="\\A\\s*(<\\?xml(?:[^?]|\\?(?!>))+\\?>)?(?:\\s+|<\\?(?:[^?]|\\?(?!>))+\\?>|<!--(?:[^-]|\\-(?!-))+-->)*<!DOCTYPE\\s+(\\w+)\\s+(?:[^][<>]|\\[[^][]+\\])+>\\s*<\\w*:?(\\2)\\s+";
	const string Regex::xml_namespace_prolog="\\A\\s*(?:<\\?xml(?:[^?]|\\?(?!>))+\\?>)?(?:\\s+|<\\?(?:[^?]|\\?(?!>))+\\?>|<!--(?:[^-]|\\-(?!-))+-->)*(?:<!DOCTYPE(?:[^][<>]|\\[[^][]+\\])+>)?\\s*<(\\w*):?\\w+[^>]+xmlns:?\\1\\s*=\\s*\"([^\"]+)\"";
	const string Regex::xml_schema_prolog="<(\\w*):?schema[^>]+xmlns:?\\1=\"http://www.w3.org/2001/XMLSchema\"";
	
	bool Regex::loadattempted = false;
	bool Regex::loaded = false;
	void* Regex::pcre_lib_handle = NULL;
	
	int Regex::re_options = ( PCRE_EXTENDED | PCRE_UTF8 | PCRE_NO_UTF8_CHECK ) & 0x1FFFFFF;
	int Regex::mt_options = ( PCRE_NO_UTF8_CHECK ) & 0x1FFFFFF;
	pcre* (*Regex::pcre_compile)(const char*, int, const char**, int*, const unsigned char*) = NULL;
	int (*Regex::pcre_exec)(const pcre*,const pcre_extra*,PCRE_SPTR,int,int,int,int*,int) = NULL;
	int (*Regex::pcre_config)(int,void *)= NULL;
	void (*Regex::pcre_free)(void *) = NULL;

	pcre_extra* (*Regex::pcre_study)(const pcre*,int,const char**) = NULL;
	//	void (*Regex::pcre_free)(void *) = NULL;
	
	Regex::type_regex_cache Regex::regex_cache;
	
	bool Regex::available() {
		if (!loadattempted) startup();
		return loaded;
	}
	
	bool Regex::startup() {	
		std::string err=""; //necessary IFF script uses pcre.
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string pcrelib;
			if (!Environment::getbenv("OBYX_LIBPCRESO",pcrelib)) pcrelib = "libpcre.so";
			pcre_lib_handle = dlopen(pcrelib.c_str(),RTLD_GLOBAL | RTLD_NOW);
			dlerr(err); //debug only.
			if (err.empty() && pcre_lib_handle != NULL ) {
				pcre_compile = (pcre* (*)(const char*, int, const char**, int*,const unsigned char*)) dlsym(pcre_lib_handle,"pcre_compile"); dlerr(err);
				pcre_exec = (int (*)(const pcre*,const pcre_extra*,PCRE_SPTR,int,int,int,int*,int)) dlsym(pcre_lib_handle,"pcre_exec"); dlerr(err);
				pcre_config = (int (*)(int,void *)) dlsym(pcre_lib_handle,"pcre_config");  dlerr(err);
				pcre_study = (pcre_extra* (*)(const pcre*,int,const char**)) dlsym(pcre_lib_handle,"pcre_study");  dlerr(err);
				pcre_free = (void (*)(void *)) dlsym(pcre_lib_handle,"pcre_free");  dlerr(err);
				if ( err.empty() ) {
					if ( pcre_config != NULL && pcre_compile != NULL && pcre_exec!=NULL) {
						int locr = 0;
						int dobo = pcre_config(PCRE_CONFIG_UTF8, &locr);
						if (locr != 1 || dobo != 0) { //dobo means that the flag is not supported...
							if (Logger::debugging()) {
								*Logger::log << Log::info << "Regex::startup() The pcre library was loaded, but utf-8 appears not to be supported." << Log::LO << Log::blockend;
							}
						}
						loaded = true;
					}
				} 
			}
		}
		return loaded;
	}
	
	void Regex::dlerr(std::string& container) {
		const char *err = dlerror();
		if (err != NULL) {
			container.append(err);
		}
	}
	
	
	bool Regex::shutdown() {											 //necessary IFF script uses pcre.
		loadattempted=false;
		loaded = false;
		if (! regex_cache.empty() ) {
			type_regex_cache::iterator it = regex_cache.begin();
			while ( it != regex_cache.end()) {
				pccache_item* item = &(it->second);
				if (item != NULL) {
					if (item->first != NULL) {
						(*pcre_free)(item->first);
					}
					if (item->second != NULL) {
						delete item->second;
					}
				}
				it++;
			}
			regex_cache.clear();
		}
		if ( pcre_lib_handle != NULL ) {
			dlclose(pcre_lib_handle);
			pcre_lib_handle = NULL;
		}
		return true;
	}
	
	//• --------------------------------------------------------------------------
	//•	Return the number of replaces that took place.
	//•	eg find "^foo([a-z]+)" against "foobar woobar" using substitute "\1 is bar" ==> "bar is bar"
	size_t Regex::replace(const string &pattern,const string &substitute,string &scope,bool do_all) {
		size_t count = 0;
		if ( ! scope.empty() ) {
			int* ov = NULL;		// size must be a multiple of 3.
			int ovc = 90;   	// size of ovector
			ov = new int[ovc];
			memset((int *) ov, 0, sizeof(ov));
			size_t lastend = string::npos;
			string basis = scope;
			size_t start = 0;
			size_t base_len = basis.length();
			//			size_t sub_len = substitute.length();
			scope.clear();		
			while (start <= base_len) {
				size_t matches = matcher(pattern, basis, (int)start, ov, ovc);
				if (matches <= 0) {
					if (start == 0) scope=basis; //nothing changed at all.
					break;	//we are finished - either with nothing, or all done.
				}
				size_t matchstart = ov[0], matchend = ov[1];
				if (matchstart == matchend && matchstart == lastend) {
					matchend = start + 1;
					if (start+1 < base_len && basis[start] == '\r' && basis[start+1] == '\n' ) {
						matchend++;
					}
					while (matchend < base_len && (basis[matchend] & 0xc0) == 0x80) { 
						matchend++; 
					}
					if (matchend <= base_len) { 
						scope.append(basis, start, matchend - start);
					}
					start = matchend;
				} else {
					enum {u,U,l,L,E,n} transform = n;
					scope.append(basis, start, matchstart - start);
					size_t o = 0, s = 0 ;
					do {
						s = substitute.find('\\',o);
						string newbit = substitute.substr(o,s-o);
						if (! newbit.empty() ) {
							switch(transform) {
								case n: break;
								case u: newbit[0] = std::toupper(newbit[0]);transform=n; break;
								case U: String::toupper(newbit); break;
								case l: newbit[0] = std::tolower(newbit[0]);transform=n; break;
								case L: String::tolower(newbit); break;
								case E: transform=n; break;
							}
							scope.append(newbit);
						}
						if ( s != string::npos) {
							o = s+2;
							int c = substitute[s+1];
							if ( isdigit(c) ) {
								int sn = (c - '0');
								int st = ov[2 * sn];
								if (st >= 0) {
									string newxbit;
									const char *bb = basis.c_str() + st;
									int be = ov[2 * sn + 1];
									if (be > 0 ) { 
										newxbit.append(bb, be - st);
									}									
									if (! newxbit.empty() ) {
										switch(transform) {
											case n: break;
											case u: newxbit[0] = std::toupper(newxbit[0]);transform=n; break;
											case U: String::toupper(newxbit); break;
											case l: newxbit[0] = std::tolower(newxbit[0]);transform=n; break;
											case L: String::tolower(newxbit); break;
											case E: transform=n; break;
										}
										scope.append(newxbit);
									}
								}
							} else if (c == '\\') {
								scope.push_back('\\');
							} else { 
								/* maybe it's a case modifier. 
								 \u Make the next character uppercase
								 \U Make all following characters uppercase until reaching another case specifier (\u, \L, \l ) or \E
								 \l Make the next character lowercase
								 \L Make all following characters lowercase until reaching another case specifier (\u, \U, \l ) or \E
								 \E End case transformation opened by \U or \L
								 */
								switch (c) {
									case 'u': transform=u; break;
									case 'U': transform=U; break;
									case 'l': transform=l; break;
									case 'L': transform=L; break;
									case 'E': transform=E; break;
									default: break;
								}
							}
						}
					} while ( s != string::npos );
					start = matchend;
					lastend = matchend;
					count++;
				}
				count++;
				if (! do_all) {
					break;
				}
			}
			if (count != 0 && start < base_len) {
				scope.append(basis, start, base_len - start);
			}
			delete [] ov;
		}
		return count;
	}
	
	bool Regex::field(const string &pattern,const string &basis,unsigned int fieldnum,string &scope) {
		bool retval=false;
		if ( ! basis.empty() ) {
			int* ov = NULL; int ovc = 90;		// size must be a multiple of 3. //ovc size of ovector
			ov = new int[ovc];
			memset((int *) ov, 0, sizeof(ov));
			size_t matches = matcher(pattern, basis, 0, ov, ovc);
			if (matches > 0) {
				//				size_t matchstart = ov[0], matchend = ov[1];
				int st = ov[2 * fieldnum];
				if (st >= 0) {
					string newxbit;
					const char *bb = basis.c_str() + st;
					int be = ov[2 * fieldnum + 1];
					if (be > 0 ) { 
						scope.append(bb, be - st);
						retval = true;
					}
				}
			}
			delete [] ov;
		}
		return retval;
	}
	
	size_t Regex::first(const string &pattern, const string &scope) {
		size_t retval = string::npos;
		if ( !pattern.empty() ) {
			int* ov;	   // size must be a multiple of 3.
			int ovc = 90;  // size of ovector
			ov = new int[ovc];
			size_t result = matcher(pattern, scope, 0, ov, ovc);
			if (result > 0) {
				retval = ov[0];
			}
			delete [] ov;
		}
		return retval;
	}
	
	size_t Regex::after(const string &pattern, const string &scope) {
		size_t retval = string::npos;
		if ( !pattern.empty() ) {
			int* ov;	   // size must be a multiple of 3.
			int ovc = 90;  // size of ovector
			ov = new int[ovc];
			size_t result = matcher(pattern, scope, 0, ov, ovc);
			if (result > 0) {
				retval = ov[1];
			}
			delete [] ov;
		}
		return retval;
	}
	
	bool Regex::match(const string &pattern, const string &scope) {         //match substring using pcre
		bool retval = false;
		if ( pattern.empty() ) {
			retval = true;
		} else {
			int* ov;	// size must be a multiple of 3.
			int ovc = 90;  // size of ovector
			ov = new int[ovc];
			size_t result = matcher(pattern, scope, 0, ov, ovc);
			retval = result > 0 ? true : false;
			delete [] ov;
		}
		return retval;
	}
	
	bool Regex::fullmatch(const string &pattern, const string &scope) {     //match entire string using pcre
		bool retval = false;
		if ( pattern.empty() ) {
			retval = scope.empty() ? true : false;
		} else {
			int* ov;		// size must be a multiple of 3.
			int ovc = 90;  // size of ovector
			ov = new int[ovc];
			size_t result = matcher(pattern, scope, 0, ov, ovc);
			if (result > 0 ) {  //at least one match was found.
				size_t matchstart = ov[0];
				size_t matchend = ov[1];
				if (matchstart == 0 && matchend == scope.length() ) { 
					retval = true;
				}
			}
			delete [] ov;
		}
		return retval;
	}
	
	//• Regex private methods compile(), matcher(), reporterror() ------
	size_t Regex::matcher(const string& pattern,const string& scope,const int offset,int*& ov,int& ovc) {
		size_t inner_result = 0;
		int exec_result = 0;
		pcre* re = NULL;
		pcre_extra* rx = NULL;
		type_regex_cache::const_iterator it = regex_cache.find(pattern);
		if (it != regex_cache.end()) {
			re = ((*it).second.first);
			rx = ((*it).second.second);
			exec_result = pcre_exec(re,rx,scope.c_str(),(unsigned int)scope.length(),offset,mt_options,ov,ovc);
		} else {
			if ( compile(pattern,re,rx) ) {
				regex_cache.insert(type_regex_cache::value_type(pattern,pair<pcre*,pcre_extra*>(re,rx)));
				exec_result = pcre_exec(re,rx,scope.c_str(),(unsigned int)scope.length(),offset,mt_options,ov,ovc);
			}
		}
		if ( exec_result < 1 ) { 
			if (exec_result != -1) reporterror(exec_result);
			inner_result = 0;
		} else {
			inner_result = exec_result;
		}
		return inner_result;
	}
	
	bool Regex::compile(const string& pattern,pcre*& container,pcre_extra*& extra) {
		const char *error = NULL;
		int erroffset=0;
		bool retval = true;
		container = pcre_compile(pattern.c_str(),re_options,&error,&erroffset,NULL);
		if ( container == NULL) {
			string errmsg;
			if (error != NULL) { 
				errmsg = error;
			} else {
				errmsg = "Unspecified compile error";
			}
			if (Logger::log) {
				*Logger::log << Log::error << "Regex::compile_re() " << errmsg;
				if (erroffset > 0) *Logger::log << " at offset " << erroffset; 			
				*Logger::log << " while compiling pattern '" << pattern << "'." << Log::LO << Log::blockend; 
			}
			retval = false;	//could report the actual error here.
		} else {
			extra = pcre_study(container,0,&error);	
			if (error != NULL) {
				*Logger::log << Log::error << "Regex, while studying pattern '" << pattern << "' : " << error << Log::LO << Log::blockend; ;
				retval = false;	//could report the actual error here.
			}
		}
		return retval;
	}
	
	void Regex::reporterror(int errnum) {
		if (Logger::logging_available()) {
			string msg;
			switch(errnum) {
				case  0: msg="Internal Error: ovector is too small"; break;
				case -2: msg="Internal Error: Either 'code' or 'subject' was passed as NULL, or ovector was NULL and ovecsize was not zero."; break;
				case -3: msg="Internal Error: An unrecognized bit was set in the options argument."; break;
				case -4: msg="Internal Error: Endian test magic number was missing. Probably a junk pointer was passed."; break;
				case -5: msg="Internal Error: An unknown item in the compiled pattern was encountered during match."; break;
				case -6: msg="Internal Error: Internal library malloc failed."; break;
				case -8: msg="The backtracking limit (as specified by the default match_limit field) was reached."; break;
				case -10: msg="A domain that contains an invalid UTF-8 byte sequence was passed."; break;
				case -11: msg="Internal Error: The UTF-8 domain was valid, but the value of startoffset did not point to the beginning of a UTF-8 character."; break;
				case -12: msg="The domain did not match, but it did match partially"; break;
				case -13: msg="Internal Error: The PARTIAL option was used with a pattern containing items that are not supported for partial matching."; break;
				case -14: msg="Internal Error: An unexpected internal error has occurred."; break;
				case -15: msg="Internal Error: The value of the ovecsize argument was negative."; break;
				case -21: msg="Recursion Limit reached. The internal recursion vector of size 1000 was not enough"; break;
				default: msg="Unknown Error"; break;		
			}
			*Logger::log << Log::error << Log::LI << "Regex::match() error. Error " << errnum << ". " << msg << Log::LO << Log::blockend;
		}
	}
	
	//• --------------------------------------------------------------------------
}
