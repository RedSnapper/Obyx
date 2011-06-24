/* 
 * document.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * document.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_CONTEXT_DOC_H
#define OBYX_CONTEXT_DOC_H

#include <string>
#include <utility>
#include <stack>
#include <map>
#include <ext/hash_map>
#include <xercesc/dom/DOMDocument.hpp>

#include "commons/string/strings.h"
#include "commons/xml/xml.h"
#include "commons/vdb/vdb.h"

#include "obyxelement.h"
#include "dataitem.h"
#include "itemstore.h"
#include "output.h"

//using namespace __gnu_cxx; //hashmap namespace.

class InputType;
class Document : public ObyxElement  {
	
private:
	friend class UnknownElement;
	friend class Output;
	friend class Instruction;
	friend class InputType;
	friend class Iteration;
	
	typedef hash_map<const string, DataItem*, hash<const string&> > type_parm_map;
	typedef hash_map<const string, ItemStore*, hash<const string&> > type_store_map;
	
	static XML::Manager* xmlmanager;
	static std::string curr_http_req;
	static type_store_map docstores; //Those stores that are identified by document signatures.
	static Document* root; 			 //This is the opening document.
	
	xercesc::DOMDocument* xdoc; //NOT copied - because we dont want to have to delete each case, and it's owned by parser.
	xercesc::DOMNode*	root_node;
	std::string filepath;
	std::string signature;		//Used for identifying a document.
	double doc_version;			//version of obyx that this document is written for.
	u_str ownprefix;			//used for really special cases.
	type_parm_map*					parm_map;
	ItemStore store;
	ItemStore* doc_store;
	
	static std::stack<u_str>		prefix_stack;
	static std::stack<std::string>  filepath_stack;
	
	void pushprefix(const u_str);
	void popprefix();
	void inner_list() const;

	bool getparm(const std::string&,const DataItem*&) const;
	
protected:
	friend class ObyxElement;
	friend class XMLNode;
	friend class XMLCData;
	friend class IKO;
	
	static size_t prefix_length;											//--cdata
	const ObyxElement* doc_par;	
	
public:
	typedef enum {File,URL,Main,URLText} load_type;	//how to load a file		
	Document(ObyxElement*,const Document*);//
	Document(DataItem*,load_type,std::string,ObyxElement* par = NULL,bool = true);
	
	virtual ~Document();
	
	static bool const prefix(u_str&);
	
	static std::string const currentname();
	static std::string const currenthttpreq();
	void setparms(type_parm_map*& ps) { parm_map = ps; }
	const u_str own_prefix() const { return ownprefix; }
	const std::string own_filepath() const { return filepath; }
	virtual const std::string name() const;
	const xercesc::DOMDocument* doc() const;
	
	bool metastore(const string,unsigned long long&);
	bool setstore(const DataItem*, DataItem*&, kind_type, Output::scope_type, std::string& );   //name#path
	bool setstore(u_str&,u_str&, DataItem*&, kind_type, Output::scope_type, std::string& );     //name,path
	bool storeexists(const u_str&,bool,string&);      			//name#path
	bool storeexists(const u_str&,const u_str&,bool,std::string&);		//name,path
	bool storefind(const string&,bool,string&);	
	bool storefind(const u_str&,bool,string&);
	bool storefind(const u_str&,const u_str&,bool,std::string&);

	void storekeys(const u_str&,set<string>&,string&);
	bool getstore(const u_str&,DataItem*&, bool, std::string&);			//name#path, container, release?, errstr
	bool getstore(const u_str&,const u_str&,DataItem*&, bool, bool, std::string&);	//name,path, container, expected? release?, errstr
	void liststore();
	
	bool getparm(const u_str&,const DataItem*&) const;
	bool parmexists(const u_str&) const;
	bool parmfind(const u_str&) const;
	void parmkeys(const u_str&,set<string>&) const;
	
	void list() const;
	bool eval();
	void evaluate(size_t,size_t) {}	
 	void process(xercesc::DOMNode*&,ObyxElement* = NULL);
	double version() const { return doc_version; }
	string version_str() const { return String::tostring(doc_version,8); }
	static void startup();
	static void init();
	static void finalise();
	static void shutdown();
};

#endif
