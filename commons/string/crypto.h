/* 
 * crypto.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * crypto.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CRYPTO_H
#define OBYX_CRYPTO_H

#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zlib.h>
using namespace std;

namespace String {
	class Deflate {
	private:
		static void* lib_handle;
		static bool loadattempted;	//used to show if the service is up or down.
		static bool loaded;			//used to show if the service is up or down.
		static void dlerr(string&);
		//used / loaded functions
		static const char* (*obyx_zlibVersion)(void);
		static int (*obyx_deflateInit)(z_streamp,int,const char*,int);
		static int (*obyx_deflate)(z_streamp,int);
		static int (*obyx_deflateEnd)(z_streamp);
		static int (*obyx_inflateInit)(z_streamp,const char*,int);
		static int (*obyx_inflate)(z_streamp,int);
		static int (*obyx_inflateEnd)(z_streamp);
		static uLong (*obyx_deflateBound)(z_streamp,uLong);

	
		//specific private functions
		static string libversion;
		static int zsize;
		static int level;
		static bool noerrs(int,int,string&);
		
	public:
		static bool startup(string&);
		static bool available(string&);
		static bool shutdown();
		static void deflate(string&,string&);
		static void inflate(string&,string&);
		
	};

	class Digest {
	private:
		static void* lib_handle;
		static bool loadattempted;	//used to show if the service is up or down.
		static bool loaded;			//used to show if the service is up or down.
		
		static void (*OpenSSL_add_all_digests)(void);
		static EVP_MD_CTX* (*EVP_MD_CTX_create)(void);
		static void (*EVP_MD_CTX_destroy)(EVP_MD_CTX*);
		static const EVP_MD* (*EVP_get_digestbyname)(const char*);
		static int (*EVP_DigestInit_ex)(EVP_MD_CTX*, const EVP_MD*,ENGINE*);
		static int (*EVP_DigestUpdate)(EVP_MD_CTX*, const void *,size_t);
		static int (*EVP_DigestFinal_ex)(EVP_MD_CTX*,unsigned char *,unsigned int*);
	
		static int (*RAND_bytes)(unsigned char*,int);
		static int (*RAND_pseudo_bytes)(unsigned char*,int);
		
		static unsigned char* (*HMAC)(const EVP_MD *,const void *,int,const unsigned char *,int,unsigned char *,unsigned int *);
		
		static EVP_MD_CTX* context;
		static const EVP_MD* md[16];	//the different digests
		
		static void dlerr(string&);
		
	public:
		typedef enum {md2,md4,md5,sha,sha1,sha224,sha256,sha384,sha512,dss1,mdc2,ripemd160} digest;	//cookie_expiry,cookie_path,cookie_domain -- cannot be retrieved from server.. 
		static bool startup(string&);
		static bool available(string&);
		static bool shutdown();
		static void hmac(const digest,const string,const string,string&);
		static void do_digest(const digest,string&);
		static void random(string&,unsigned short);

	};
}
#endif
