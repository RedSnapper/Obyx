/* 
 * fast.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * fast.cpp is a part of Obyx - see http://www.obyx.org .
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
#include "commons/logger/logger.h"
#include "commons/environment/environment.h"
#include "fast.h"

FCGX_Request Fast::request;
streambuf* Fast::cin_streambuf  = NULL;
streambuf* Fast::cout_streambuf = NULL;
streambuf* Fast::cerr_streambuf = NULL;
fcgi_streambuf* Fast::fcin_str = NULL;
fcgi_streambuf* Fast::fcout_str = NULL;
fcgi_streambuf* Fast::fcerr_str = NULL;
bool Fast::ready(ostream*& out_ptr,char**& env) { //outp = 'final_out' where everything must end up being put.
	bool retval = false;
	if (fcin_str != NULL) {delete fcin_str; fcin_str= NULL;}
	if (fcout_str != NULL) {delete fcout_str; fcout_str= NULL;}
	if (fcerr_str != NULL) {delete fcerr_str; fcerr_str= NULL;}
	retval = (FCGX_Accept_r(&request) >= 0);
	if (retval) {
		fcin_str =  new fcgi_streambuf(request.in);
		fcout_str = new fcgi_streambuf(request.out);
		fcerr_str = new fcgi_streambuf(request.err);
		cin.rdbuf(fcin_str);
		cout.rdbuf(fcout_str);
		cerr.rdbuf(fcerr_str);
		env = request.envp;
	}
	out_ptr = &cout;
	return retval;
}

void Fast::startup() {	
	cin_streambuf  = cin.rdbuf();
	cout_streambuf = cout.rdbuf();
	cerr_streambuf = cerr.rdbuf();
	FCGX_Init();
    FCGX_InitRequest(&request,0,0);	
}

void Fast::shutdown() {
	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);
}
