/* 
 * bitwise.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * bitwise.h is a part of Obyx - see http://www.obyx.org .
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
 */

#ifndef OBYX_BITWISE_H
#define OBYX_BITWISE_H

#include <map>
#include <vector>
#include <sstream>
#include <cfloat>
#include <math.h>
#include <float.h>

#ifndef DISALLOW_GMP

#include <gmp.h>
	namespace String {
		
		namespace Bit {
			
			class Num {
			public:
				mpz_t	value;
				Num(); 
				~Num(); 
				Num(string::const_iterator&);
				Num(const string);
				Num(const Num &); 
				Num(mpz_t &);
				void operator=(const Num &);
				string str(unsigned int base_i = 16); 
			};
			
			class Op {
			public:
				typedef enum {none, left, right} association;
				int precedence; //the order in which things are done. low = first.
				association assoc;
				size_t parms;
				Op(int prec,association a = left,size_t p = 2) : precedence(prec),assoc(a),parms(p) {}
				virtual char sig() const = 0;
				virtual void evaluate(Num&,Num,Num,Num) const { }
				virtual ~Op() {}
			};
			class subtract : public Op {
			public:
				subtract() : Op(6){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '-';}
			};	
			class add : public Op {
			public:
				add() : Op(6){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '+';}
			};	
			class multiply : public Op {
			public:
				multiply() : Op(5){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '*';}
			};	
			class quotient : public Op {
			public:
				quotient() : Op(5){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '/';}
			};	
			class modulo : public Op {
			public:
				modulo() : Op(5){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '/';}
			};	
			class lb : public Op {
			public: 
				lb() : Op(1000,none,0) {}
				char sig() const { return '(';}
			};
			class rb : public Op {
			public: 
				rb() : Op(1000,none,0) {}
				char sig() const { return ')';}
			};
			class delim : public Op {
			public: 
				delim() : Op(999,none,0) {}
				char sig() const { return ',';}
			};
			class bandfn : public Op {
			public:
				bandfn() : Op(2){} //as a function = 2; as characters (10)
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class bxorfn : public Op {
			public:
				bxorfn() : Op(2){} //as a function = 2; as characters (11)
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class bnotfn : public Op {
			public:
				bnotfn() : Op(2,left,1) {} //as a function = 2; as characters (11)
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class borfn : public Op {
			public:
				borfn() : Op(2){} //as a function = 2; as characters (12)
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class uminus : public Op {
			public:
				uminus() : Op(3,right,1){}
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return '_';}
			};
			class absfn : public Op {
			public:
				absfn() : Op(2,left,1){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class powfn : public Op {
			public:
				powfn() : Op(2){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class maxfn : public Op {
			public:
				maxfn() : Op(2){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class minfn : public Op {
			public:
				minfn() : Op(2){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class shlfn : public Op {
			public:
				shlfn() : Op(2){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class shrfn : public Op {
			public:
				shrfn() : Op(2){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class rolfn : public Op {
			public:
				rolfn() : Op(2,left,3){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			class rorfn : public Op {
			public:
				rorfn() : Op(2,left,3){} //as a function = 2
				void evaluate(Num&,Num,Num,Num) const;
				char sig() const { return 'f';}
			};
			
			class Evaluate {
			private:
				typedef map<string, Op*> lut_t;
				typedef hash_map<const string,Num,hash<const string&> > parm_map_t;
				static lut_t lut;
				
				string expr;
				vector<const  Op*>  opstack;
				vector<Num>	valstack;
				parm_map_t parms;
				
				void getvalue(const string&,Num&);		//load a number from the parmslist
				size_t name(string::const_iterator&,string&);
				char evalstack();
				const  Op* get(string::const_iterator&,size_t&);
				const  Op* get(const string);
				
			public:
				static void startup();
				static void shutdown();
				Evaluate() {}
				~Evaluate() {}
				string process(string&,unsigned int = 16);
				void set_expression(const string);
				void add_parm(const string,const string);
			};
					
			class GMP {
			private:
				
				//Library loading things
				static void*	lib_handle;
				static bool		loadattempted;	//used to show if the service is up or down.
				static bool		loaded;	//used to show if the service is up or down.
				static void		dlerr(std::string&);
				
			public:
				//The API that we use.	
				static void (*mpz_init)(mpz_t);
				static void (*mpz_clear)(mpz_t);
				static void (*mpz_set)(mpz_t, mpz_t);
				static void (*mpz_set_ui)(mpz_t, unsigned long int);
				static void (*mpz_set_d)(mpz_t, double);
				static void (*mpz_set_str)(mpz_t,const char*,int);
				static char* (*mpz_get_str)(char*,int,mpz_t);
				static unsigned long int (*mpz_get_ui)(mpz_t);					//long uint from mpz 
				static void (*mpz_mul_2exp)(mpz_t,mpz_t,unsigned long int);		//left shift
				static void (*mpz_fdiv_q_2exp)(mpz_t,mpz_t,unsigned long int);  //2s cmp. right shift
				static void (*mpz_and)(mpz_t,mpz_t,mpz_t);	//Function Set rop to op1 logical-and op2.
				static void (*mpz_ior)(mpz_t,mpz_t,mpz_t);	//Function Set rop to op1 inclusive-or op2.
				static void (*mpz_xor)(mpz_t,mpz_t,mpz_t);	//Function Set rop to op1 exclusive-or op2.
				static void (*mpz_com)(mpz_t,mpz_t);		//Function Set rop to the one's complement of op.
				static void (*mpz_add)(mpz_t,mpz_t,mpz_t);
				static void (*mpz_add_ui)(mpz_t,mpz_t,unsigned long int);
				static void (*mpz_sub)(mpz_t,mpz_t,mpz_t);
				static void (*mpz_sub_ui)(mpz_t,mpz_t,unsigned long int);
				static void (*mpz_mul)(mpz_t,mpz_t,mpz_t);
				static void (*mpz_neg)(mpz_t,mpz_t);
				static void (*mpz_abs)(mpz_t,mpz_t);
				static void (*mpz_tdiv_q)(mpz_t,mpz_t,mpz_t);	//q = quotient(n/d)
				static void (*mpz_tdiv_r)(mpz_t,mpz_t,mpz_t);	//r = remainder(n/d)
				static void (*mpz_mod)(mpz_t,mpz_t,mpz_t);
				static size_t (*mpz_size)(mpz_t);
				static int (*mpz_cmp)(mpz_t,mpz_t);
				static int (*mpz_fits_ulong_p)(mpz_t);
				static size_t (*mpz_sizeinbase)(mpz_t,int);		//may be used to pick up msbit
				static void (*mpz_pow_ui)(mpz_t,mpz_t,unsigned long int);
				static int (*mpz_root)(mpz_t,mpz_t,unsigned long int);
				static mpz_t zero;
				
				static bool startup();
				static bool available();
				static bool shutdown();
				//		static bool field(const string &,const string &,unsigned int,string &);
			};
			
		}
	}
#endif

#endif

