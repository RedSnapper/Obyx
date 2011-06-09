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
#include <dlfcn.h>
#include <string.h>

#ifndef DISALLOW_GMP

#include <gmp.h>
#include "commons/environment/environment.h"
#include "commons/logger/logger.h"
#include "commons/string/bitwise.h"

using namespace std;

namespace String {
	
	namespace Bit {

//mpz_set IS a copy operator.		
		
		Num::Num() { 
			GMP::mpz_init(value); 
		}
		Num::Num(const Num &x) {
			Num* p = const_cast<Num*>(&x);
			GMP::mpz_init(value); 
			GMP::mpz_set(value,p->value); 
		}
		Num::Num(mpz_t &x) {
			GMP::mpz_init(value); 
			GMP::mpz_set(value,x); 
		}
		Num::Num(const string str) { 
			GMP::mpz_init(value); 
			if (!str.empty()) {
				if ((str.size() > 2) && (str[0]=='0') && (str[1] == 'x' || str[1] =='X') ) {
					GMP::mpz_set_str(value,str.c_str(),0);
				} else {
					GMP::mpz_set_str(value,str.c_str(),16);
				}
			} else {
				GMP::mpz_set_ui(value,0);
			}
		}
		Num::Num(string::const_iterator& x) { //load a number from a string iterator.
			GMP::mpz_init(value); 
			if (*x == '0' && (*(x+1) == 'x' || *(x+1) == 'X')) {
				x++; x++;
			} 
			string::const_iterator y(x); //Start position as copy constructor.
			string valids("0123456789ABCDEFabcdef");
			while( valids.find(*x) != string::npos ) x++;
			if ( x == y ) { 
				GMP::mpz_set(value,GMP::zero); 
			} else {
				string hexvalstr(y,x);
				GMP::mpz_set_str(value,hexvalstr.c_str(),16); 
			}
		}	
		Num::~Num() { 
			GMP::mpz_clear(value); 
		}
		void Num::operator=(const Num &x) {
			Num* p = const_cast<Num*>(&x);
			GMP::mpz_set(value,p->value); 
		}

		string Num::str(unsigned int base_i) { 
			string result;
			unsigned int base = 16;
			if (base_i > 1 && base_i <= 36) base= base_i;
			char* tmp = GMP::mpz_get_str(NULL,base,value);
			if (tmp) { 
				result = tmp;
				free(tmp); tmp=NULL;
			}
			if (base == 16 && result.size() % 2 == 1) {
				result.insert(0,"0");
			}
			return result;
		}
		
		Evaluate::lut_t Evaluate::lut;
		
		//public static stuff
		void Evaluate::startup() {
			if ( GMP::available() ) {
				lut.insert(lut_t::value_type("(",new lb()));
				lut.insert(lut_t::value_type(")",new rb()));
				lut.insert(lut_t::value_type(",",new delim()));
				lut.insert(lut_t::value_type("-",new subtract()));
				lut.insert(lut_t::value_type("+",new add()));
				lut.insert(lut_t::value_type("*",new multiply()));
				lut.insert(lut_t::value_type("/",new quotient()));
				lut.insert(lut_t::value_type("\\",new quotient()));
				lut.insert(lut_t::value_type("%",new modulo()));
				lut.insert(lut_t::value_type("_",new uminus()));
				lut.insert(lut_t::value_type("xor",new bxorfn()));
				lut.insert(lut_t::value_type("and",new bandfn()));
				lut.insert(lut_t::value_type("or",new borfn()));
				lut.insert(lut_t::value_type("not",new bnotfn()));
				lut.insert(lut_t::value_type("abs",new absfn()));
				lut.insert(lut_t::value_type("pow",new powfn()));
				lut.insert(lut_t::value_type("max",new maxfn()));
				lut.insert(lut_t::value_type("min",new minfn()));
				lut.insert(lut_t::value_type("shl",new shlfn()));
				lut.insert(lut_t::value_type("shr",new shrfn()));
				lut.insert(lut_t::value_type("rol",new rolfn()));
				lut.insert(lut_t::value_type("ror",new rorfn()));
			}
		}
		void Evaluate::shutdown() {
			for (lut_t::iterator i = lut.begin(); i != lut.end(); i++) {
				if (i->second != NULL) {
					delete i->second; i->second = NULL;
				};
			}
		}
		
		//public stuff
		void Evaluate::set_expression(const string e) {
			expr = e;
		}
		//need to delete parms also..
		void Evaluate::add_parm(const string key,const string value) {
			Num mval(value);	
			parms.insert(parm_map_t::value_type(key,mval)); //copy constructor here *2, 1 destructor
		}
		string Evaluate::process(string& errs,unsigned int base) {
			const string wss = "\n\r\t ";
			valstack.clear();
			opstack.clear();
			string::const_iterator i=expr.begin();
			bool didfn  = true;			//starts true. remembers if the last thing to be eaten was an op or a value.
			while ( i != expr.end()) {
				while ( wss.find_first_of(*i) != string::npos ) { //skip over whitespace.
					i++;
				}
				string cur_name("");
				size_t eaten;
				const Op* curop = get(i,eaten); //second is for how many chars were eaten.
				if (curop == NULL) {
					eaten = name(i,cur_name);
					if (!cur_name.empty()) {
						char x = *i;
						if(x == '(') {
							curop = get(cur_name); //named fn.
							if ( curop == NULL) {
								errs =  "Error. Unknown function '" + cur_name + "' in " + expr;
							}
						} 
					}
				}
				if (curop != NULL) {
					char csig = curop->sig();
					if(csig == '(') {
						opstack.push_back(curop);
						didfn = true;
					} else { 
						if(csig == ')' ) {
							char ssig =' ';
							while(!opstack.empty() && ssig !='(') {
								ssig = evalstack();
								didfn = false;
							}
							//need to test the below with eg "4-3)"
							if( ssig !='(' ) {
								errs =  "Error. Unbalance bracket in expression " + expr;
							} else {
								if( !opstack.empty() && opstack.back()->sig() == 'f') {
									ssig = evalstack(); //was a function declaration. so do it.
									didfn = false;
								}
							}
						} else {
							if ( didfn && csig =='-' ) { //switch for unary minus.
								curop= get("_"); 
							}
							if(curop->assoc == Op::right) {
								//mul=5,add=6,neg=3; so if tok=neg, and stack=mul, do neg first.
								while(!opstack.empty() && (curop->precedence > opstack.back()->precedence)) {
									evalstack();
									didfn = false;
								}
							} else {
								//mul=5,add=6,neg=3; so if tok=add, and stack=mul, do mul first. if same, do stack first.
								while(!opstack.empty() && (curop->precedence >= opstack.back()->precedence)) {
									evalstack();
									didfn = false;
								}
							}
							opstack.push_back(curop);
							didfn = true;
						}
					}
				} else { //non-op - must be value.
					if (cur_name.empty() ) {
						Num value(i); // generate from string iterator.
						valstack.push_back(value);
					} else {
						Num value(GMP::zero); 
						getvalue(cur_name,value);
						valstack.push_back(value);
					}
					didfn = false;
				}
			}
			while(!opstack.empty()) {
				evalstack(); //discard result here. should be just 1 operation, normally..
			}
			return valstack.back().str(base);
		}
		size_t Evaluate::name(string::const_iterator& i,string& value) {
			string::const_iterator b(i); //Start position as copy constructor.
			if (isalpha(*i)) {
				while (isalnum(*++i));
				value=string(b,i);
			} else {
				value.clear();
			}
			return i - b;
		}
		const Op* Evaluate::get(string::const_iterator& i,size_t& eaten) {
			eaten = 0;
			const Op* retval(NULL);
			string possible_op;
			possible_op.push_back(*i);
			lut_t::const_iterator o = lut.find(possible_op);
			if( o != lut.end() ) {
				retval = o->second;
				eaten = 1;
				i++;
			} 
			return retval;
		}
		const Op* Evaluate::get(const string i) {
			const Op* retval(NULL);
			lut_t::const_iterator o = lut.find(i);
			if( o != lut.end() ) { retval = o->second; } 
			return retval;
		}
		char Evaluate::evalstack() {
			const Op* pop = opstack.back(); 
			char retval = pop->sig();
			opstack.pop_back();
			Num w,x,y;
			if (pop->parms > 0) {
				y = valstack.back(); 
				valstack.pop_back();
				if (pop->parms > 1) {
					x = valstack.back(); 
					valstack.pop_back();
					if (pop->parms > 2) {
						w = valstack.back(); 
						valstack.pop_back();
					}
				}
				Num result;
				pop->evaluate(result,w,x,y);
				valstack.push_back(result);
			}
			return retval;
		}
		
		void Evaluate::getvalue(const string& str,Num& retval) { //load a number from the parmslist
			parm_map_t::const_iterator o = parms.find(str);
			if( o != parms.end() ) { retval = o->second; } 
		}
		void subtract::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_sub(result.value,a.value,b.value);
		}
		void add::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_add(result.value,a.value,b.value);
		}
		void multiply::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_mul(result.value,a.value,b.value);
		}
		void quotient::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_tdiv_q(result.value,a.value,b.value);
		}
		void modulo::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_mod(result.value,a.value,b.value);
		}
		void borfn::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_ior(result.value,a.value,b.value);
		}
		void bandfn::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_and(result.value,a.value,b.value);
		}
		void bxorfn::evaluate(Num& result,Num,Num a,Num b) const {
			GMP::mpz_xor(result.value,a.value,b.value);
		}
		void bnotfn::evaluate(Num& result,Num,Num,Num b) const {
			GMP::mpz_com(result.value,b.value);
		}
		void uminus::evaluate(Num& result,Num,Num,Num b) const {
			GMP::mpz_neg(result.value,b.value);
		}
		void absfn::evaluate(Num& result,Num,Num,Num b) const {
			GMP::mpz_abs(result.value,b.value);
		}
		void shlfn::evaluate(Num& result,Num,Num a,Num b) const {
			if (GMP::mpz_fits_ulong_p(b.value) != 0) {
				unsigned long int exp = GMP::mpz_get_ui(b.value);					 
				GMP::mpz_mul_2exp(result.value,a.value,exp);
			}
		}
		void shrfn::evaluate(Num& result,Num,Num a,Num b) const {
			if (GMP::mpz_fits_ulong_p(b.value) != 0) {
				unsigned long int exp = GMP::mpz_get_ui(b.value);					 
				GMP::mpz_fdiv_q_2exp(result.value,a.value,exp);
			}
		}
		void rorfn::evaluate(Num& result,Num a,Num b,Num c) const {
			if ((GMP::mpz_fits_ulong_p(b.value) != 0) && (GMP::mpz_fits_ulong_p(c.value) != 0)) {
				unsigned long int wsiz = GMP::mpz_get_ui(c.value);
				unsigned long int bits = GMP::mpz_get_ui(b.value) % wsiz;
				if (wsiz == 0) { wsiz = 1+ GMP::mpz_sizeinbase(a.value,2); } //may be used to pick up msbit
				mpz_t mask; GMP::mpz_init(mask); GMP::mpz_set_ui(mask,1);
				GMP::mpz_mul_2exp(mask,mask,wsiz); GMP::mpz_sub_ui(mask,mask,1); //now we have a bitmask.
				mpz_t t1; GMP::mpz_init(t1);
//				r = (x >> b | (x << (w - b)));
				GMP::mpz_fdiv_q_2exp(t1,a.value,bits);					//t1 = a >> bits
				GMP::mpz_mul_2exp(result.value,a.value,wsiz - bits);	//r  = a << wsiz - bits
				GMP::mpz_ior(result.value,result.value,t1);				//r  = r | t1
				GMP::mpz_and(result.value,result.value,mask);			//r  = r & mask

				//r = (x & ~m) | r;
				GMP::mpz_com(t1,mask);					//t1 = ~mask.
				GMP::mpz_and(t1,t1,a.value);		//(x & ~m)
				GMP::mpz_ior(result.value,result.value,t1);		//
				
				
				GMP::mpz_clear(mask);  GMP::mpz_clear(t1); 
			}
		}
		void rolfn::evaluate(Num& result,Num a,Num b,Num c) const {
			if ((GMP::mpz_fits_ulong_p(b.value) != 0) && (GMP::mpz_fits_ulong_p(c.value) != 0)) {
				unsigned long int wsiz = GMP::mpz_get_ui(c.value);
				unsigned long int bits = GMP::mpz_get_ui(b.value) % wsiz;
				if (wsiz == 0) { wsiz = 1+ GMP::mpz_sizeinbase(a.value,2); } //may be used to pick up msbit
				mpz_t mask; GMP::mpz_init(mask); GMP::mpz_set_ui(mask,1);
				GMP::mpz_mul_2exp(mask,mask,wsiz); GMP::mpz_sub_ui(mask,mask,1); //now we have a bitmask.
				mpz_t t1; GMP::mpz_init(t1);
//				r = (x << b | (x >> (w - b)));
				GMP::mpz_mul_2exp(t1,a.value,bits);
				GMP::mpz_fdiv_q_2exp(result.value,a.value,wsiz - bits);
				GMP::mpz_ior(result.value,result.value,t1);
				GMP::mpz_and(result.value,result.value,mask);
				
				//r = (x & ~m) | r;
				GMP::mpz_com(t1,mask);					//t1 = ~mask.
				GMP::mpz_and(t1,t1,a.value);		//(x & ~m)
				GMP::mpz_ior(result.value,result.value,t1);		//

				
				GMP::mpz_clear(mask); 
				GMP::mpz_clear(t1); 
			}
		}
		void powfn::evaluate(Num& result,Num,Num a,Num b) const {
			if (GMP::mpz_fits_ulong_p(b.value) != 0) {
				unsigned long int exp = GMP::mpz_get_ui(b.value);					 
				GMP::mpz_pow_ui(result.value,a.value,exp);
			}
		}
		void maxfn::evaluate(Num& result,Num,Num a,Num b) const {
			if (GMP::mpz_cmp(a.value,b.value) > 0) {
				GMP::mpz_set(result.value,a.value); 
			} else {
				GMP::mpz_set(result.value,b.value); 
			}
		}
		void minfn::evaluate(Num& result,Num,Num a,Num b) const {
			if (GMP::mpz_cmp(a.value,b.value) < 0) {
				GMP::mpz_set(result.value,a.value); 
			} else {
				GMP::mpz_set(result.value,b.value); 
			}
		}
		
		mpz_t GMP::zero;
		
		bool GMP::loadattempted = false;
		bool GMP::loaded = false;
		void* GMP::lib_handle = NULL;
		void (*GMP::mpz_init)(mpz_t) = NULL;
		void (*GMP::mpz_clear)(mpz_t) = NULL;
		void (*GMP::mpz_set)(mpz_t, mpz_t)= NULL;
		void (*GMP::mpz_set_ui)(mpz_t, unsigned long int)= NULL;
		void (*GMP::mpz_set_d)(mpz_t, double)= NULL;
		void (*GMP::mpz_set_str)(mpz_t,const char*,int)= NULL;
		char* (*GMP::mpz_get_str)(char*,int,mpz_t)= NULL;
		unsigned long int (*GMP::mpz_get_ui)(mpz_t)= NULL;					 
		void (*GMP::mpz_mul_2exp)(mpz_t,mpz_t,unsigned long int)= NULL;		
		void (*GMP::mpz_fdiv_q_2exp)(mpz_t,mpz_t,unsigned long int)= NULL;
		void (*GMP::mpz_and)(mpz_t,mpz_t,mpz_t)= NULL;	//Function Set rop to op1 logical-and op2.
		void (*GMP::mpz_ior)(mpz_t,mpz_t,mpz_t)= NULL;	//Function Set rop to op1 inclusive-or op2.
		void (*GMP::mpz_xor)(mpz_t,mpz_t,mpz_t)= NULL;	//Function Set rop to op1 exclusive-or op2.
		void (*GMP::mpz_com)(mpz_t,mpz_t)= NULL;		//Function Set rop to the one's complement of op.
		void (*GMP::mpz_add)(mpz_t,mpz_t,mpz_t)= NULL;
		void (*GMP::mpz_add_ui)(mpz_t,mpz_t,unsigned long int)= NULL;
		void (*GMP::mpz_sub)(mpz_t,mpz_t,mpz_t)= NULL;
		void (*GMP::mpz_sub_ui)(mpz_t,mpz_t,unsigned long int)= NULL;
		void (*GMP::mpz_mul)(mpz_t,mpz_t,mpz_t)= NULL;
		void (*GMP::mpz_neg)(mpz_t,mpz_t)= NULL;
		void (*GMP::mpz_abs)(mpz_t,mpz_t)= NULL;
		void (*GMP::mpz_tdiv_q)(mpz_t,mpz_t,mpz_t)= NULL;	
		void (*GMP::mpz_tdiv_r)(mpz_t,mpz_t,mpz_t)= NULL;	
		void (*GMP::mpz_mod)(mpz_t,mpz_t,mpz_t)= NULL;
		size_t (*GMP::mpz_size)(mpz_t)= NULL;
		int (*GMP::mpz_cmp)(mpz_t,mpz_t)= NULL;
		int (*GMP::mpz_fits_ulong_p)(mpz_t)= NULL;
		size_t (*GMP::mpz_sizeinbase)(mpz_t,int)= NULL;		//may be used to pick up msbit
		void (*GMP::mpz_pow_ui)(mpz_t,mpz_t,unsigned long int)= NULL;
		int (*GMP::mpz_root)(mpz_t,mpz_t,unsigned long int)= NULL;
		
		bool GMP::available() {
			if (!loadattempted) startup();
			return loaded;
		}
		
		bool GMP::startup() {	
			std::string err=""; //necessary IFF script uses pcre.
			if ( ! loadattempted ) {
				loadattempted = true;
				loaded = false;
				string libname;
				if (!Environment::getbenv("OBYX_LIBGMPSO",libname)) libname = "libgmp.so";
				lib_handle = dlopen(libname.c_str(),RTLD_GLOBAL | RTLD_NOW);
				dlerr(err); //debug only.
				if (err.empty() && lib_handle != NULL ) {
					mpz_init		= (void (*)(mpz_t))							dlsym(lib_handle,"__gmpz_init"); dlerr(err);
					mpz_clear		= (void (*)(mpz_t))							dlsym(lib_handle,"__gmpz_clear"); dlerr(err);
					mpz_set			= (void (*)(mpz_t,mpz_t))					dlsym(lib_handle,"__gmpz_set"); dlerr(err);
					mpz_set_d		= (void (*)(mpz_t,double))					dlsym(lib_handle,"__gmpz_set_d"); dlerr(err);
					mpz_set_str		= (void (*)(mpz_t,const char*,int))			dlsym(lib_handle,"__gmpz_set_str"); dlerr(err);
					mpz_get_str		= (char* (*)(char*,int,mpz_t))				dlsym(lib_handle,"__gmpz_get_str"); dlerr(err);
					mpz_set_ui		= (void (*)(mpz_t,unsigned long int))		dlsym(lib_handle,"__gmpz_set_ui"); dlerr(err);
					mpz_get_ui		= (unsigned long int (*)(mpz_t))			dlsym(lib_handle,"__gmpz_get_ui"); dlerr(err);		 
					mpz_mul_2exp	= (void (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_mul_2exp"); dlerr(err);	
					mpz_fdiv_q_2exp	= (void (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_fdiv_q_2exp"); dlerr(err);
					mpz_and			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_and"); dlerr(err);
					mpz_ior			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_ior"); dlerr(err);
					mpz_xor			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_xor"); dlerr(err);
					mpz_com			= (void (*)(mpz_t,mpz_t))					dlsym(lib_handle,"__gmpz_com"); dlerr(err);
					mpz_add			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_add"); dlerr(err);
					mpz_add_ui		= (void (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_add_ui"); dlerr(err);
					mpz_sub			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_sub"); dlerr(err);
					mpz_sub_ui		= (void (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_sub_ui"); dlerr(err);
					mpz_mul			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_mul"); dlerr(err);
					mpz_neg			= (void (*)(mpz_t,mpz_t))					dlsym(lib_handle,"__gmpz_neg"); dlerr(err);
					mpz_abs			= (void (*)(mpz_t,mpz_t))					dlsym(lib_handle,"__gmpz_abs"); dlerr(err);
					mpz_tdiv_q		= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_tdiv_q"); dlerr(err);
					mpz_tdiv_r		= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_tdiv_r"); dlerr(err);
					mpz_mod			= (void (*)(mpz_t,mpz_t,mpz_t))				dlsym(lib_handle,"__gmpz_mod"); dlerr(err);
					mpz_size		= (size_t (*)(mpz_t))						dlsym(lib_handle,"__gmpz_size"); dlerr(err);
					mpz_cmp			= (int (*)(mpz_t,mpz_t))					dlsym(lib_handle,"__gmpz_cmp"); dlerr(err);
					mpz_fits_ulong_p= (int (*)(mpz_t))							dlsym(lib_handle,"__gmpz_fits_ulong_p"); dlerr(err);
					mpz_sizeinbase	= (size_t (*)(mpz_t,int))					dlsym(lib_handle,"__gmpz_sizeinbase"); dlerr(err);
					mpz_pow_ui		= (void (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_pow_ui"); dlerr(err);
					mpz_root		= (int (*)(mpz_t,mpz_t,unsigned long int))	dlsym(lib_handle,"__gmpz_root"); dlerr(err);
					
					if ( err.empty() ) {
						mpz_init(zero);
						mpz_set_ui(zero,0);
						
						/*					
						 mpz_t foo,bar;		
						 mpz_init(foo);
						 mpz_init(bar);
						 mpz_set_str(foo,"53415315768646535895341531576864653589534153157686465358953415315768646535895341531576864653589534153157686465358953415315768646535895341531576864653589",16);
						 mpz_set_str(bar,"01010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101",16);
						 mpz_and(foo,foo,bar);
						 char* baz = mpz_get_str(NULL,16,foo);
						 if (baz) { free(baz); baz=NULL;}
						 char* bim = mpz_get_str(NULL,16,bar);
						 if (bim) { free(bim); bim=NULL;}
						 mpz_clear(foo);
						 mpz_clear(bar);
						 */
						loaded = true;
					} 
				}
			}
			return loaded;
		}
		
		void GMP::dlerr(std::string& container) {
			const char *err = dlerror();
			if (err != NULL) {
				container.append(err);
			}
		}
		
		bool GMP::shutdown() {											 //necessary IFF script uses pcre.
			if ( lib_handle != NULL ) {
				mpz_clear(zero);
				dlclose(lib_handle);
			}
			return true;
		}
		
	}
	
}

#endif
