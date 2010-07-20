/* 
 * crypto.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * crypto.cpp is a part of Obyx - see http://www.obyx.org .
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
#include "crypto.h"
#include "commons/environment/environment.h"

namespace String {
	
	bool Digest::loadattempted = false;
	bool Digest::loaded = false;
	void* Digest::lib_handle = NULL;
	EVP_MD_CTX* Digest::context = NULL;
	const EVP_MD* Digest::md[16] = {NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL, NULL};
	
	void (*Digest::OpenSSL_add_all_digests)(void) = NULL;
	EVP_MD_CTX* (*Digest::EVP_MD_CTX_create)(void) = NULL;
	void (*Digest::EVP_MD_CTX_destroy)(EVP_MD_CTX*) = NULL;
	const EVP_MD* (*Digest::EVP_get_digestbyname)(const char*) = NULL;
	int (*Digest::EVP_DigestInit_ex)(EVP_MD_CTX*, const EVP_MD*,ENGINE*) = NULL;
	int (*Digest::EVP_DigestUpdate)(EVP_MD_CTX*, const void *,size_t) = NULL;
	int (*Digest::EVP_DigestFinal_ex)(EVP_MD_CTX*,unsigned char *,unsigned int*) = NULL;
	
	void Digest::dlerr(std::string& errstr) {
		const char *err = dlerror();
		if (err != NULL) {
			errstr.append(err);
			errstr.push_back(';');
		}
	}
	
	bool Digest::available(string& errors) {
		if (!loadattempted) { 
			startup(errors); 
		}
		return loaded;
	}
	
	bool Digest::startup(string& errors) {	
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string libstr;
			if (!Environment::getbenv("OBYX_LIBSSLSO",libstr)) libstr = "libssl.so";
			lib_handle = dlopen(libstr.c_str(),RTLD_GLOBAL | RTLD_NOW);
			dlerr(errors); //debug only.
			if (errors.empty() && lib_handle != NULL ) {
				OpenSSL_add_all_digests	=(void (*)(void)) dlsym(lib_handle,"OpenSSL_add_all_digests"); dlerr(errors);
				EVP_MD_CTX_create		=(EVP_MD_CTX* (*)(void)) dlsym(lib_handle,"EVP_MD_CTX_create"); dlerr(errors);
				EVP_MD_CTX_destroy		=(void (*)(EVP_MD_CTX*)) dlsym(lib_handle,"EVP_MD_CTX_destroy"); dlerr(errors);
				EVP_get_digestbyname	=(const EVP_MD* (*)(const char*)) dlsym(lib_handle,"EVP_get_digestbyname"); dlerr(errors);
				EVP_DigestInit_ex		=(int (*)(EVP_MD_CTX*,const EVP_MD*,ENGINE*)) dlsym(lib_handle,"EVP_DigestInit_ex"); dlerr(errors);
				EVP_DigestUpdate		=(int (*)(EVP_MD_CTX*,const void*,size_t)) dlsym(lib_handle,"EVP_DigestUpdate"); dlerr(errors);
				EVP_DigestFinal_ex		=(int (*)(EVP_MD_CTX*,unsigned char*,unsigned int*)) dlsym(lib_handle,"EVP_DigestFinal_ex"); dlerr(errors);
				if ( errors.empty() ) {
					loaded = true;
					OpenSSL_add_all_digests();
					context = EVP_MD_CTX_create();
					md[0] = EVP_get_digestbyname("md2");
					md[1] = EVP_get_digestbyname("md4");
					md[2] = EVP_get_digestbyname("md5");
					md[3] = EVP_get_digestbyname("sha");
					md[4] = EVP_get_digestbyname("sha1");
					md[5] = EVP_get_digestbyname("sha224");
					md[6] = EVP_get_digestbyname("sha256");
					md[7] = EVP_get_digestbyname("sha384");
					md[8] = EVP_get_digestbyname("sha512");
					md[9] = EVP_get_digestbyname("dss1");
					md[10] = EVP_get_digestbyname("mdc2");
					md[11] = EVP_get_digestbyname("ripemd160");
				} 
			}
		}
		return loaded;
	}
	
	bool Digest::shutdown() {											 //necessary IFF script uses pcre.
		if (loaded) {
			EVP_MD_CTX_destroy(context);
		}
		if ( lib_handle != NULL ) {
			dlclose(lib_handle);
		}
		return true;
	}
	
	void Digest::do_digest(const digest d,string& basis) {
		unsigned char md_value[EVP_MAX_MD_SIZE];
		unsigned int md_len;
		const EVP_MD* d_touse;
		switch (d) {
			case md2: { d_touse=md[0];} break;
			case md4: { d_touse=md[1];} break;
			case md5: { d_touse=md[2];} break;
			case sha: { d_touse=md[3];} break;
			case sha1: { d_touse=md[4];} break;
			case sha224: { d_touse=md[5];} break;
			case sha256: { d_touse=md[6];} break;
			case sha384: { d_touse=md[7];} break;
			case sha512: { d_touse=md[8];} break;
			case dss1: { d_touse=md[9];} break;
			case mdc2: { d_touse=md[10];} break;
			case ripemd160: { d_touse=md[11];} break;
		}
		EVP_DigestInit_ex(context, d_touse, NULL);
		EVP_DigestUpdate(context,basis.c_str(),basis.size());
		EVP_DigestFinal_ex(context, md_value, &md_len);
		basis = string((char *)md_value,md_len);
	}
}
