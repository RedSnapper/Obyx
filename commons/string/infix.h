/* 
 * infix.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * infix.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_INFIX_H
#define OBYX_INFIX_H

#include <map>
#include <vector>
#include <sstream>
#include <cfloat>
#include <math.h>
#include <float.h>

using namespace std;

namespace String {
	namespace Infix {
		class Op {
		public:
			typedef enum {none, left, right} association;
			int precedence; //the order in which things are done. low = first.
			association assoc;
			size_t parms;
			Op(int prec,association a = left,size_t p = 2) : precedence(prec),assoc(a),parms(p) {}
			virtual char sig() const = 0;
			virtual long double evaluate(const long double,const long double,const long double) const { return NAN; }
			virtual ~Op() {}
		};
		class subtract : public Op {
		public:
			subtract() : Op(6){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '-';}
		};
		class add : public Op {
		public:
			add() : Op(6){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '+';}
		};
		class multiply : public Op {
		public:
			multiply() : Op(5){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '*';}
		};
		class divide : public Op {
		public:
			divide() : Op(5){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '/';}
		};
		class uminus : public Op {
		public:
			uminus() : Op(3,right,1){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '_';}
		};
		class modulo : public Op {
		public:
			modulo() : Op(5){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '%';}
		};
		class quotient : public Op {
		public:
			quotient() : Op(5){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return '\\';}
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
		class powfn : public Op {
		public:
			powfn() : Op(2){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class maxfn : public Op {
		public:
			maxfn() : Op(2){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class minfn : public Op {
		public:
			minfn() : Op(2){}
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class bandfn : public Op {
		public:
			bandfn() : Op(2){} //as a function = 2; as characters (10)
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class bxorfn : public Op {
		public:
			bxorfn() : Op(2){} //as a function = 2; as characters (11)
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class bnotfn : public Op {
		public:
			bnotfn() : Op(2,left,1) {} //as a function = 2; as characters (11)
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class borfn : public Op {
		public:
			borfn() : Op(2){} //as a function = 2; as characters (12)
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class roundfn : public Op {
		public:
			roundfn() : Op(2,left,1){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class floorfn : public Op {
		public:
			floorfn() : Op(2,left,1){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class ceilfn : public Op {
		public:
			ceilfn() : Op(2,left,1){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class absfn : public Op {
		public:
			absfn() : Op(2,left,1){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class rolfn : public Op { //number,shifts,wordsize(in bits)
		public:
			rolfn() : Op(2,left,3){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		class rorfn : public Op { //number,shifts,wordsize(in bits)
		public:
			rorfn() : Op(2,left,3){} //as a function = 2
			long double evaluate(const long double,const long double,const long double) const;
			char sig() const { return 'f';}
		};
		
		class Evaluate {
		private:
			typedef map<string, Op*> lut_t;
			typedef hash_map<const string,long double, hash<const string&> > parm_map_t;
			static lut_t lut;
			
			string expr;
			vector<const  Op*>  opstack;
			vector<long double>	valstack;
			parm_map_t parms;
			
			bool get(string::const_iterator&,long double&);
			size_t name(string::const_iterator&,string&);
			long double getvalue(const string&);
			char evalstack();
			const  Op* get(string::const_iterator&,size_t&);
			const  Op* get(const string);
			
		public:
			static void startup();
			static void shutdown();
			Evaluate() {}
			~Evaluate() {}
			long double process(string&);
			void set_expression(const string);
			void add_parm(const string,long double);
		};
	}
}

#endif
