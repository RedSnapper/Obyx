/*
 *  infix.cpp
 *  obyx
 *
 *  Created by Ben on 13/10/2010.
 *  Copyright 2010 Red Snapper Ltd. All rights reserved.
 *
 */

#include "commons/environment/environment.h"
#include "commons/string/strings.h"
#include <limits>

namespace String {
	
	namespace Infix {
		Evaluate::lut_t Evaluate::lut;
		
		//public static stuff
		void Evaluate::startup() {
			lut.insert(lut_t::value_type("(",new lb()));
			lut.insert(lut_t::value_type(")",new rb()));
			lut.insert(lut_t::value_type(",",new delim()));
			lut.insert(lut_t::value_type("=",new same()));
			lut.insert(lut_t::value_type("-",new subtract()));
			lut.insert(lut_t::value_type("+",new add()));
			lut.insert(lut_t::value_type("*",new multiply()));
			lut.insert(lut_t::value_type("/",new divide()));
			lut.insert(lut_t::value_type("\\",new quotient()));
			lut.insert(lut_t::value_type("_",new uminus()));
			lut.insert(lut_t::value_type("%",new modulo()));
			lut.insert(lut_t::value_type("lt",new ltfn()));
			lut.insert(lut_t::value_type("lte",new ltefn()));
			lut.insert(lut_t::value_type("gt",new gtfn()));
			lut.insert(lut_t::value_type("gte",new gtefn()));
			lut.insert(lut_t::value_type("pow",new powfn()));
			lut.insert(lut_t::value_type("max",new maxfn()));
			lut.insert(lut_t::value_type("min",new minfn()));
			lut.insert(lut_t::value_type("xor",new bxorfn()));
			lut.insert(lut_t::value_type("and",new bandfn()));
			lut.insert(lut_t::value_type("or",new borfn()));
			lut.insert(lut_t::value_type("not",new bnotfn()));
			lut.insert(lut_t::value_type("log2",new log2fn()));
			lut.insert(lut_t::value_type("round",new roundfn()));
			lut.insert(lut_t::value_type("floor",new floorfn()));
			lut.insert(lut_t::value_type("ceil",new ceilfn()));
			lut.insert(lut_t::value_type("abs",new absfn()));
			lut.insert(lut_t::value_type("rol",new rolfn()));
			lut.insert(lut_t::value_type("ror",new rorfn()));
			lut.insert(lut_t::value_type("if",new iftrue()));
		}
		void Evaluate::shutdown() {
			for (lut_t::iterator i = lut.begin(); i != lut.end(); i++) {
				if (i->second != nullptr) {
					delete i->second; i->second = nullptr;
				};
			}
		}
		
		//public stuff
		void Evaluate::set_expression(const string e) {
			expr = e;
		}
		void Evaluate::add_parm(const string key, long double value) {
			parms.insert(parm_map_t::value_type(key,value));
		}
		long double Evaluate::process(string& errs) {
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
				if (curop == nullptr) {
					name(i,cur_name);
					if (!cur_name.empty()) {
						char x = *i;
						if(x == '(') {
							curop = get(cur_name); //named fn.
							if ( curop == nullptr) {
								errs =  "Error. Unknown function '" + cur_name + "'.";
							}
						} 
					}
				}
				if (curop != nullptr) {
					char csig = curop->sig();
					if(csig == '(') {
						opstack.push_back(curop);
						didfn = true;
					} else { 
						if(csig == ')' ) {
							char ssig =' ';
							while(!opstack.empty() && ssig !='(') {
								ssig = evalstack(errs);
								didfn = false;	//evalstack pushes to val.
							}
							//need to test the below with eg "4-3)"
							if( ssig !='(' ) {
								errs =  "Error. Unbalanced bracket.";
							} else {
								if( !opstack.empty() && opstack.back()->sig() == 'f') {
									evalstack(errs); //was a function declaration. so do it.
									didfn = false;	//evalstack pushes to val.
								}
							}
						} else {
							if ( didfn && csig =='-' ) { //switch for unary minus.
								curop= get("_"); 
								didfn = true;
							}
							if(curop->assoc == Op::right) {
								//mul=5,add=6,neg=3; so if tok=neg, and stack=mul, do neg first.
								while(!opstack.empty() && (curop->precedence > opstack.back()->precedence)) {
									evalstack(errs);
									didfn = false;	//evalstack pushes to val.
								}
							} else {
								//mul=5,add=6,neg=3; so if tok=add, and stack=mul, do mul first. if same, do stack first.
								while(!opstack.empty() && (curop->precedence >= opstack.back()->precedence)) {
									evalstack(errs);
									didfn = false;	//evalstack pushes to val.
								}
							}
							opstack.push_back(curop);
						}
					}
				} else { //non-op - must be value.
					long double value = nanl(""); //concrete number.
					if (cur_name.empty() ) {
						if ( get(i,value) ) {
							valstack.push_back(value);
						}
					} else {
						value = getvalue(cur_name);
						valstack.push_back(value);
					}
					didfn = false;
				}
			}
			while(!opstack.empty()) {
				evalstack(errs); //discard result here. should be just 1 operation, normally..
			}
			if (!errs.empty()) {
				errs.append(" Expression was '");
				errs.append(expr);
				errs.append("'.");
			}
			if (!valstack.empty()) {
				return valstack.back();
			} else {
				errs.append(" Stack Underflow Error. Expression was '");
				errs.append(expr);
				errs.append("'.");
				return nanl("");
			}
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
			const Op* retval(nullptr);
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
			const Op* retval(nullptr);
			lut_t::const_iterator o = lut.find(i);
			if( o != lut.end() ) { retval = o->second; } 
			return retval;
		}
		char Evaluate::evalstack(string& errs) {
			char retval = '?';
			if (!opstack.empty()) {
				const Op* pop = opstack.back();
				if (pop->parms <= valstack.size()) {
					retval = pop->sig();
					opstack.pop_back();
					long double iw(0),ix(0),iy(0);
					if (pop->parms > 0) {
						iy = valstack.back();
						valstack.pop_back();
						if (pop->parms > 1) {
							ix = valstack.back();
							valstack.pop_back();
							if (pop->parms > 2) {
								iw = valstack.back();
								valstack.pop_back();
							}
						}
						long double result = pop->evaluate(iw,ix,iy);
						valstack.push_back(result);
					}
				} else {
					errs =  "Error. Stack underflow. Too few values.";
					valstack.push_back(nanl(""));
				}
			}
			return retval;
		}
		long double Evaluate::getvalue(const string& str) {
			long double retval(nanl(""));
			parm_map_t::const_iterator o = parms.find(str);
			if( o != parms.end() ) { retval = o->second; } 
			return retval;
		}
		bool Evaluate::get(string::const_iterator& x,long double& v) {
			bool retval(false);
			if (*x == '0' && *(x+1) == 'x') {
				double hexresult;
				x++; x++;
				String::hex(x,hexresult);
				v = hexresult;
				retval = true;
			} else {
				string::const_iterator y(x); //Start position as copy constructor.
				string valids("0123456789.");
				while( valids.find(*x) != string::npos ) x++;
				if ( x == y ) { 
					v = nanl("");
				} else {
					retval = true;
					istringstream ist(string(y,x));
					ist >> v;
				}
			}
			return retval;
		}	
		long double subtract::evaluate(const long double,const long double p,const long double q) const {
			return p - q;
		}
		long double add::evaluate(const long double,const long double p,const long double q) const {
			return p + q;
		}
		long double multiply::evaluate(const long double,const long double p,const long double q) const {
			return p * q;
		}
		long double divide::evaluate(const long double,const long double p,const long double q) const {
			return p / q;
		}
		long double uminus::evaluate(const long double,const long double,const long double q) const {
			return -q;
		}
		long double modulo::evaluate(const long double,const long double p,const long double q) const {
			return (long long)p % (long long)q;
		}
		long double quotient::evaluate(const long double,const long double p,const long double q) const {
			return floor( p / q);
		}
		long double powfn::evaluate(const long double,const long double p,const long double q) const {
			return pow(p,q);
		}
		long double maxfn::evaluate(const long double,const long double p,const long double q) const {
			return p > q ? p : q;
		}
		long double minfn::evaluate(const long double,const long double p,const long double q) const {
			return p < q ? p : q;
		}
		long double bandfn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : ((long long)p & (long long)q) == 0 ? 0: 1;
//			return ((long long)p & (long long)q) == 0 ? 0: 1;
		}
		long double bxorfn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : ((long long)p ^ (long long)q) == 0 ? 0: 1;
//			return ((long long)p ^ (long long)q) == 0 ? 0: 1;
		}
		long double bnotfn::evaluate(const long double,const long double,const long double q) const {
			return std::isnan(q) ? nanl("") : (long long)q == 0 ? 1: 0;
//			return (long long)q == 0 ? 1: 0;
		}
		long double borfn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : ((long long)p | (long long)q) == 0 ? 0: 1;
//			return ((long long)p | (long long)q) == 0 ? 0: 1;
		}
		long double log2fn::evaluate(const long double,const long double,const long double q) const {
			return log2l(q);
		}
		long double roundfn::evaluate(const long double,const long double,const long double q) const {
			return round(q);
		}
		long double floorfn::evaluate(const long double,const long double,const long double q) const {
			return floor(q);
		}
		long double ceilfn::evaluate(const long double,const long double,const long double q) const {
			return ceil(q);
		}
		long double absfn::evaluate(const long double,const long double,const long double q) const {
			return q < 0 ? -q : q;
		}
		// false = 0 here.
		long double same::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : p == q ? 1:0;
//			return p == q ? 1:0;
		}
		long double ltfn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : p < q ? 1:0;
//			return p < q ? 1:0;
		}
		long double ltefn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : p <= q ? 1:0;
//			return p <= q ? 1:0;
		}
		long double gtfn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : p > q ? 1:0;
//			return p > q ? 1:0;
		}
		long double gtefn::evaluate(const long double,const long double p,const long double q) const {
			return std::isnan(p) || std::isnan(q) ? nanl("") : p >= q ? 1:0;
//			return p >= q ? 1:0;
		}
		long double iftrue::evaluate(const long double o,const long double p,const long double q) const {
			return (o != 0 && !std::isnan(o)) ? p : q;
		}
		long double rolfn::evaluate(const long double o,const long double p,const long double q) const {
			unsigned long long x = (unsigned long long)o;	 //number to change
			unsigned long long b = (unsigned long long)(q > 0 ? q : sizeof(unsigned long long) << 3);	 //wordsize
			unsigned long long s = (long long) p % b;	//number of shifts to make is mod q.
			unsigned long long m = (unsigned long long)(pow(2,q) - 1);			//mask
			unsigned long long r = (x << s | (x >> (b - s)));
			r = (x & ~m) | (m > 0 ? r & m :r);
			return (long double) r;
		}
		long double rorfn::evaluate(const long double o,const long double p,const long double q) const {
			unsigned long long x = (unsigned long long)o;	 //number to change
			unsigned long long b = (unsigned long long)(q > 0 ? q : sizeof(unsigned long long) << 3);	 //wordsize
			unsigned long long s = (long long) p % b;	//number of shifts to make is mod q.
			unsigned long long m = (unsigned long long)(pow(2,q) - 1);			//mask
			unsigned long long r = (x >> s | (x << (b - s)));
			r = (x & ~m) | (m > 0 ? r & m :r);
			return (long double) r;
		}
		
	}
	
}
