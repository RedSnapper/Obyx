/* 
 * obyxelement.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * obyxelement.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_ELEMENT_H
#define OBYX_CONTEXT_ELEMENT_H

#include <utility>
#include <string>
#include <set>
#include <map>
#include <deque>

#include <xercesc/dom/DOMNode.hpp>

#include "commons/vdb/vdb.h"
#include "dataitem.h"
#include "pairqueue.h"

namespace qxml {

	typedef enum { encode,decode } process_t;	//what sort of process

	typedef enum { any,all } scope_t;	//what sort of scope (comparison)
	
	typedef enum { e_sql,e_url,e_xml,e_name,e_digits,e_none,e_base64,e_hex,e_message,e_qp } enc_type;	//
	typedef std::map<u_str, enc_type > enc_type_map; 

	typedef enum { flowfunction,parm,defparm,other } elemclass;	//what sort of object
	
	typedef enum { endqueue,iteration,control,body,instruction,comparison,output,input,comparate,ontrue,onfalse,mapping,domain,match,key,comment,xmlnode,xmldocument,shortsequence} elemtype;	//
	typedef std::map<u_str, elemtype > nametype_map; 
	
	typedef enum { out_immediate,out_none,out_store,out_file,out_error,out_xmlnamespace,out_xmlgrammar,out_cookie,out_http,out_cookie_expiry,out_cookie_path,out_cookie_domain } output_type;	//
	typedef std::map<u_str, output_type > output_type_map; 
	
	typedef enum {immediate,none,store,file,error,xmlnamespace,xmlgrammar,cookie,field,sysparm,sysenv,url,fnparm } inp_type;	//cookie_expiry,cookie_path,cookie_domain -- cannot be retrieved from server.. 
	typedef std::map<u_str, inp_type > inp_type_map;
	
	//four flow-functions..	
	typedef enum {it_sql,it_while,it_while_not,it_repeat} it_type;
	typedef std::map<u_str, it_type > it_type_map; 
	
	typedef enum { move,append,substring,position,length,left,right,reverse,upper,lower,kind,add,subtract,multiply,divide,maximum,minimum,remainder,quotient,shell_command,query_command,function} op_type;	//transform
	typedef std::map<u_str, op_type > op_type_map; 

	typedef enum { equivalent_to,exists,is_empty,substring_of,significant,cmp_true,cmp_or,cmp_and,cmp_xor,less_than,greater_than,natural,email} cmp_type;	//
	typedef std::map<u_str, std::pair< cmp_type, bool > > cmp_type_map; 

	typedef enum { m_substitute, m_switch, m_state } map_type;	//
	typedef std::map<u_str, map_type > map_type_map; 
	
	typedef std::map<unsigned long, std::string > long_map; 
	
	typedef enum { ut_value, ut_existence, ut_significant } usage_tests;
	typedef enum { c_object, c_name, c_request, c_response, c_time, c_timing, c_version, c_http, c_point, c_cookies } current_type;	//what kind of dataItem
	typedef std::map< std::string, current_type > current_type_map; 
	
}
using namespace qxml;
class Document;
class Iteration;
class ObyxElement {
private:
	static long_map ce_map;
	static nametype_map ntmap;
	void do_alloc();
	void do_dealloc();

protected:
	friend class Document;
	friend class Environment;
	friend class Function;
	friend class Iteration;
	friend class IKO;
	static unsigned long long int eval_count;
	static unsigned long long int break_point;
	static Vdb::ServiceFactory*	dbsf;		//this is managed by main.
	static Vdb::Service*		dbs;		//this is managed by the factory.
	static Vdb::Connection*		dbc;		//this is generated at startup.
	const Document* owner;					//so we can find stuff out about the document.
	ObyxElement* p;							//not const, as we append to it's results!
	xercesc::DOMNode* node;					//should be a const (but we manipulate it in breakpoint)
	void do_breakpoint();

	//statics
public:
	PairQueue results;
	qxml::elemclass wottype;													//what elemclass is this.
	qxml::elemtype  wotzit;														//what elemtype is this.

	ObyxElement(ObyxElement*,const ObyxElement*);// : p(par) { copy(orig); doalloc(); }
	ObyxElement(ObyxElement*,qxml::elemtype,qxml::elemclass,xercesc::DOMNode*);
	virtual ~ObyxElement();

	void trace() const;
	
	virtual void explain() { results.explain(); }			//
	virtual const string name() const;	
	virtual bool evaluate(size_t=0,size_t=0) = 0 ;	

	static ObyxElement* Factory(xercesc::DOMNode* const&,ObyxElement*);
	static void init(Vdb::ServiceFactory*&);
	static void finalise();
};
class XMLNode : public ObyxElement {
	bool doneit;
public:
	XMLNode(ObyxElement* par,const XMLNode* orig) : ObyxElement(par,orig),doneit(orig->doneit) {}
	XMLNode(xercesc::DOMNode* const&,ObyxElement *);
	bool evaluate(size_t =0,size_t=0) { return doneit; }
	virtual ~XMLNode();
};

#endif