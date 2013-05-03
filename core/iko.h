/* 
 * iko.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * iko.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_IKO_H
#define OBYX_CONTEXT_IKO_H

#include <string>
#include "commons/xml/xml.h"
#include "commons/logger/logger.h"
#include "obyxelement.h"
#include "json.h"

using namespace obyx;

class IKO : public ObyxElement {
public:
	typedef enum {immediate,none,store,file,error,xmlnamespace,xmlgrammar,cookie,field,sysparm,sysenv,url,fnparm } inp_space;
private:
	friend class Function;
	typedef enum { c_object, c_name, c_request, c_response, c_osi_response, c_ts, c_time, c_timing, c_version, c_vnumber, c_point, c_cookies, c_scratch, c_xpaths } current_type;	//what kind of dataItem
	typedef std::map< u_str, current_type > current_type_map; 
//ok - problem is that xpath is u_str, whereas keys are string (so that we can use regex without having to transliterate every key in a map).
	bool existsinspace(u_str&,const inp_space,const bool,const bool);
	bool sigfromspace(const u_str&,const inp_space,const bool,const kind_type,DataItem*&);
	bool foundinspace(const u_str&,const inp_space,const bool);
	void log(const Log::msgtype,const std::string) const;
	void doerrspace(const u_str&) const;
	void setfilepath(const string&,string&) const;
	bool httpready() const;
	bool legalsysenv(const u_str&) const;
	
protected:
	typedef std::map<u_str, inp_space > inp_space_map;
	friend class Document;
	friend class ObyxElement;
	void process_encoding(DataItem*&);
	static Json					json;
	static kind_type_map		kind_types;
	static enc_type_map			enc_types;
	static inp_space_map		ctx_types; //subset of input types.
	static current_type_map		current_types;
	kind_type kind;				//derived from the kind attribute
	enc_type  encoder;			//derived from the encoder attribute
	inp_space  context;			//derived from the context attribute
	inp_space  xcontext;		//derived from the xcontext attribute
	process_t process;			//derived from the process attribute
	bool	  wsstrip;			//referring to wsstrip attribute.
	bool	  exists;		    //a value exists - is inp_space or has a context != none
	u_str     name_v;			//name value - used for tracing etc.
    u_str 	  xpath;			//xpath used as am object modifier.
	
	//            input    release eval, is_context name/ref  container 
	void evaltype(inp_space, bool, bool, bool, kind_type, DataItem*&,DataItem*&); 
	void keysinspace(const u_str&,const inp_space,set<string>&);	//gather them keys.
	bool valuefromspace(u_str&,const inp_space,const bool,const bool,const kind_type,DataItem*&);
	
public:
	static void init(); 
	static void finalise();
	static void startup(); 
	static void shutdown();	
	static enc_type str_to_encoder(const u_str);
	bool currentenv(const u_str&,const usage_tests,const IKO*,DataItem*&); ///why was this static?
	bool getexists() const {return exists;}
	bool found() const {return exists;}
	virtual void evaluate(size_t,size_t)=0;
	IKO(ObyxElement*,const IKO*); 
	IKO(xercesc::DOMNode* const&,ObyxElement* = NULL, elemtype = endqueue);	
	virtual ~IKO(); 
};

#endif

