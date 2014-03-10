/* 
 * crypto.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * crypto.cpp is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#include <string.h>
#include <dlfcn.h>
#include "crypto.h"
#include "commons/dlso.h"
#include "commons/environment/environment.h"

namespace String {
	//--------------------------------------------- DIGEST ------------------------------------	
	
	bool Digest::loadattempted = false;
	bool Digest::loaded = false;
	void* Digest::lib_handle = nullptr;
	EVP_MD_CTX* Digest::context = nullptr;
	const EVP_MD* Digest::md[16] = {nullptr,nullptr,nullptr,nullptr,nullptr, nullptr,nullptr,nullptr,nullptr,nullptr, nullptr,nullptr,nullptr,nullptr,nullptr, nullptr};
	
	void (*Digest::OpenSSL_add_all_digests)(void) = nullptr;
	EVP_MD_CTX* (*Digest::EVP_MD_CTX_create)(void) = nullptr;
	void (*Digest::EVP_MD_CTX_destroy)(EVP_MD_CTX*) = nullptr;
	const EVP_MD* (*Digest::EVP_get_digestbyname)(const char*) = nullptr;
	int (*Digest::EVP_DigestInit_ex)(EVP_MD_CTX*, const EVP_MD*,ENGINE*) = nullptr;
	int (*Digest::EVP_DigestUpdate)(EVP_MD_CTX*, const void *,size_t) = nullptr;
	int (*Digest::EVP_DigestFinal_ex)(EVP_MD_CTX*,unsigned char *,unsigned int*) = nullptr;
	int (*Digest::RAND_bytes)(unsigned char*,int) = nullptr;
	int (*Digest::RAND_pseudo_bytes)(unsigned char*,int) = nullptr;
	unsigned char* (*Digest::HMAC)(const EVP_MD *,const void *,int,const unsigned char *,int,unsigned char *,unsigned int *) = nullptr;

	
	void Digest::dlerr(std::string& errstr) {
		const char *err = dlerror();
		if (err != nullptr) {
			errstr.append(err);
			errstr.push_back(';');
		}
	}
	bool Digest::available(string& errors) {
		if (!loadattempted) {
			string digest_errors;
			startup(digest_errors);
			errors.append(digest_errors);
		}
		return loaded;
	}
	bool Digest::startup(string& errors) {	
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string libdir,libstr;
			if (!Environment::getbenv("OBYX_LIBSSLSO",libstr)) { 	//legacy method
				if (Environment::getbenv("OBYX_LIBSSLDIR",libdir)) {
					if (!libdir.empty() && *libdir.rbegin() != '/') libdir.push_back('/');
				}
				libstr = SO(libdir,libssl);
			}
			lib_handle = dlopen(libstr.c_str(),RTLD_GLOBAL | RTLD_NOW);
			dlerr(errors); //debug only.
			if (errors.empty() && lib_handle != nullptr ) {
				OpenSSL_add_all_digests	=(void (*)(void)) dlsym(lib_handle,"OpenSSL_add_all_digests"); dlerr(errors);
				EVP_MD_CTX_create		=(EVP_MD_CTX* (*)(void)) dlsym(lib_handle,"EVP_MD_CTX_create"); dlerr(errors);
				EVP_MD_CTX_destroy		=(void (*)(EVP_MD_CTX*)) dlsym(lib_handle,"EVP_MD_CTX_destroy"); dlerr(errors);
				EVP_get_digestbyname	=(const EVP_MD* (*)(const char*)) dlsym(lib_handle,"EVP_get_digestbyname"); dlerr(errors);
				EVP_DigestInit_ex		=(int (*)(EVP_MD_CTX*,const EVP_MD*,ENGINE*)) dlsym(lib_handle,"EVP_DigestInit_ex"); dlerr(errors);
				EVP_DigestUpdate		=(int (*)(EVP_MD_CTX*,const void*,size_t)) dlsym(lib_handle,"EVP_DigestUpdate"); dlerr(errors);
				EVP_DigestFinal_ex		=(int (*)(EVP_MD_CTX*,unsigned char*,unsigned int*)) dlsym(lib_handle,"EVP_DigestFinal_ex"); dlerr(errors);
				RAND_bytes				=(int (*)(unsigned char*,int)) dlsym(lib_handle,"RAND_bytes"); dlerr(errors);
				RAND_pseudo_bytes		=(int (*)(unsigned char*,int)) dlsym(lib_handle,"RAND_pseudo_bytes"); dlerr(errors);
				HMAC					=(unsigned char* (*)(const EVP_MD*,const void*,int,const unsigned char*,int,unsigned char*,unsigned int*)) dlsym(lib_handle,"HMAC"); dlerr(errors);
				
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

					unsigned char *ibuff = new unsigned char[16];
					RAND_pseudo_bytes(ibuff,16); //err = 1 on SUCCESS.
					unsigned int seed = *(unsigned int *)ibuff;
					srand((unsigned)time(0)+seed); 
					delete[] ibuff;

				} 
			}
		}
		return loaded;
	}
	bool Digest::shutdown() {											 //necessary IFF script uses pcre.
		if (loaded) {
			EVP_MD_CTX_destroy(context);
		}
		if ( lib_handle != nullptr ) {
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
		EVP_DigestInit_ex(context, d_touse, nullptr);
		EVP_DigestUpdate(context,basis.c_str(),basis.size());
		EVP_DigestFinal_ex(context, md_value, &md_len);
		basis = string((char *)md_value,md_len);
	}
	
	void Digest::hmac(const digest d,const string key,const string data, string& basis) {
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
		const unsigned char* dataptr = (const unsigned char*)data.c_str();
		HMAC(d_touse,key.c_str(),(int)key.length(),dataptr,(int)data.length(),md_value,&md_len);	
		basis = string((char *)md_value,md_len);
	}
	
	void Digest::random(string& result,unsigned short size) {
		for (size_t i=0; i < size; i++) {
			result.push_back((unsigned char)(rand() & 0xFF ));
		}
		//The following seems to weigh slightly to the lower side. not sure why.
//		unsigned char *ibuff = new unsigned char[size];
//		RAND_pseudo_bytes(ibuff,size); //err = 1 on SUCCESS.
//		result = string((const char *)ibuff,0,size);
//		delete ibuff;
	}
	
//--------------------------------------------- DEFLATE ------------------------------------	
	string Deflate::libversion ="";
	int Deflate::level = Z_DEFAULT_COMPRESSION;
	int Deflate::zsize=(int)sizeof(z_stream);
	bool Deflate::loadattempted = false;
	bool Deflate::loaded = false;
	
	void* Deflate::lib_handle = nullptr;
//loaded methods
	const char* (*Deflate::obyx_zlibVersion)(void) = nullptr;
	int (*Deflate::obyx_deflateInit)(z_streamp,int,const char*,int) = nullptr;
	int (*Deflate::obyx_deflate)(z_streamp,int) = nullptr;
	int (*Deflate::obyx_deflateEnd)(z_streamp) = nullptr;
	int (*Deflate::obyx_inflateInit)(z_streamp,const char*,int) = nullptr;
	int (*Deflate::obyx_inflate)(z_streamp,int) = nullptr;
	int (*Deflate::obyx_inflateEnd)(z_streamp) = nullptr;
	uLong (*Deflate::obyx_deflateBound)(z_streamp,uLong) = nullptr;
	
	void Deflate::dlerr(std::string& errstr) {
		const char *err = dlerror();
		if (err != nullptr) {
			errstr.append(err);
			errstr.push_back(';');
		}
	}
	bool Deflate::available() {
		if (!loadattempted) {
			string errors;
			startup(errors);
		}
		return loaded;
	}
	bool Deflate::startup(string& startup_errors) {
		string errors;
		if ( ! loadattempted ) {
			loadattempted = true;
			loaded = false;
			string libdir,libstr;
			if (!Environment::getbenv("OBYX_LIBZIPSO",libstr)) { 	//legacy method
				if (Environment::getbenv("OBYX_LIBZIPDIR",libdir)) {
					if (!libdir.empty() && *libdir.rbegin() != '/') libdir.push_back('/');
				}
				libstr = SO(libdir,libz);
			}
			lib_handle = dlopen(libstr.c_str(),RTLD_GLOBAL | RTLD_NOW);
			dlerr(errors); //debug only.
			if (errors.empty() && lib_handle != nullptr ) {
				//load dl functions .
				obyx_zlibVersion  =(const char* (*)(void)) dlsym(lib_handle,"zlibVersion"); dlerr(errors);
				obyx_deflateInit =(int (*)(z_streamp,int,const char*,int)) dlsym(lib_handle,"deflateInit_"); dlerr(errors);
				obyx_deflate =(int (*)(z_streamp,int)) dlsym(lib_handle,"deflate"); dlerr(errors);
				obyx_deflateEnd =(int (*)(z_streamp)) dlsym(lib_handle,"deflateEnd"); dlerr(errors);
				obyx_inflateInit =(int (*)(z_streamp,const char*,int)) dlsym(lib_handle,"inflateInit_"); dlerr(errors);
				obyx_inflate =(int (*)(z_streamp,int)) dlsym(lib_handle,"inflate"); dlerr(errors);
				obyx_inflateEnd =(int (*)(z_streamp)) dlsym(lib_handle,"inflateEnd"); dlerr(errors);
				obyx_deflateBound = (uLong (*)(z_streamp,uLong)) dlsym(lib_handle,"deflateBound"); dlerr(errors);

				if ( errors.empty() ) {
					loaded = true;
					const char* zlv = obyx_zlibVersion();
					if (zlv != nullptr) { libversion = zlv; }
				}
			}
			startup_errors.append(errors);
		}
		return loaded;
	}
	bool Deflate::shutdown() {											 //necessary IFF script uses pcre.
		if (loaded) {
// do any teardown functions here.
		}
		loaded=false;
		loadattempted = false;
		if ( lib_handle != nullptr ) {
			dlclose(lib_handle);
			lib_handle=nullptr;
		}
		return true;
	}
	bool Deflate::noerrs(int err,int expected,string& errstr) {
		bool retval = true;
		if (err != expected) {
			retval = false;
			switch (err) {
				case Z_ERRNO: {
					errstr.append(" read/write error; ");
				} break;
				case Z_STREAM_ERROR: {
					errstr.append(" invalid compression level; ");
				} break;
				case Z_DATA_ERROR: {
					errstr.append(" invalid or incomplete deflate data; ");
				} break;
				case Z_MEM_ERROR: {
					errstr.append(" out of memory; ");
				} break;
				case Z_VERSION_ERROR: {
					errstr.append(" zlib version mismatch; ");
				} break;
				default: {
					retval = true;
				} break;
			}
		}
		return retval;
	}

	void Deflate::deflate(string& context,string& errstr) {
		if (!context.empty()) {
			uLong bsize = (uLong)context.size();
			z_stream strm;
			strm.zalloc = nullptr;
			strm.zfree  = nullptr;
			strm.opaque = nullptr;
			Byte *ibuff = new Byte[bsize];
			memcpy((char *)ibuff,context.c_str(),bsize);
			context.clear();
			int err = Z_OK;
			if (noerrs(obyx_deflateInit(&strm,level,libversion.c_str(),zsize),Z_OK,errstr)) {
				uLong destiny = obyx_deflateBound(&strm,bsize);
				Byte *obuff = new Byte[destiny];
				strm.next_out = obuff;
				strm.next_in = ibuff;
				strm.avail_in =  (uInt)bsize;
				strm.avail_out = (uInt)destiny; //this is what we are deflating
				do {
					err = obyx_deflate(&strm,Z_FINISH);
					if (noerrs(err,Z_OK,errstr)) {
						if (obuff < strm.next_out ) {
							context.append(string(obuff,strm.next_out - 1));
						} else {
							err = Z_STREAM_END;
						}
						strm.avail_out = (uInt)destiny; //this is what we are deflating
						strm.next_out = obuff;
					}
				} while (err != Z_STREAM_END );
				if (noerrs(obyx_deflateEnd(&strm),Z_OK,errstr)) {
					if (obuff < strm.next_out ) {
						context.append(string(obuff,strm.next_out - 1));
					}
				}
				delete[] obuff;
			}
			delete[] ibuff;
		} 
	}
	
	void Deflate::inflate(string& context,string& errstr) {
		if (!context.empty()) {
			z_stream strm;
			uLong bsize = (uLong)context.size();
			strm.zalloc = nullptr;
			strm.zfree  = nullptr;
			strm.opaque = nullptr;
			Byte *ibuff = new Byte[context.size()];
			memcpy((char *)ibuff,context.c_str(),bsize);
			context.clear();
			int err = Z_OK;
			if (noerrs(obyx_inflateInit(&strm,libversion.c_str(),zsize),Z_OK,errstr)) {
				uLong destiny = obyx_deflateBound(&strm,bsize);
				Byte *obuff = new Byte[destiny];
				strm.next_out = obuff;
				strm.next_in = ibuff;
				strm.avail_in =  (uInt)bsize;
				strm.avail_out = (uInt)destiny; //this is what we are deflating
				do {
					err = obyx_inflate(&strm,Z_FINISH);
					if (noerrs(err,Z_OK,errstr)) {
						if (obuff < strm.next_out ) {
							context.append(string(obuff,strm.next_out));
						} else {
							err = Z_STREAM_END;
						}
						strm.avail_out = (uInt)destiny; //this is what we are deflating
						strm.next_out = obuff;
					} else {
						err = Z_STREAM_END;						
					}
				} while (err != Z_STREAM_END );
				if (noerrs(obyx_inflateEnd(&strm),Z_OK,errstr)) {
					if (obuff < strm.next_out ) {
						context.append(string(obuff,strm.next_out));
					}
				}
				delete[] obuff;
			}
			delete[] ibuff;
		}
	}
}
