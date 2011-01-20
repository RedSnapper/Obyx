/* 
 * output.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * output.cpp is a part of Obyx - see http://www.obyx.org .
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
#include <xercesc/dom/DOMNode.hpp>
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"
#include "commons/vdb/vdb.h"
#include "commons/httphead/httphead.h"
#include "commons/filing/filing.h"

#include "xmlobject.h"
#include "dataitem.h"
#include "itemstore.h"
#include "iteration.h"
#include "obyxelement.h"
#include "iko.h"
#include "function.h"
#include "output.h"
#include "document.h"

using namespace Log;
using namespace obyx;

output_type_map Output::output_types;
Output::part_type_map Output::part_types;
Output::http_line_type_map Output::httplinetypes;

Output::Output(xercesc::DOMNode* const& n,ObyxElement* par, elemtype el): IKO(n,par,el),type(out_immediate),part(value),errowner(true),errs(NULL) {
	errs = new ostringstream();
	u_str str_esc,str_encoder,str_process,str_type,str_value,str_part;
	
	if ( XML::Manager::attribute(n,UCS2(L"type"),str_type)  ) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: attribute 'type' should be 'space'" << Log::LO;
		trace();
		*Logger::log  << Log::blockend;
	}
	if ( XML::Manager::attribute(n,UCS2(L"space"),str_type)  ) {
		output_type_map::const_iterator j = output_types.find(str_type);
		if( j != output_types.end() ) {
			type = j->second; 
		} else {
			string err_type; transcode(str_type.c_str(),err_type);
			*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: space '" << err_type << "'  not recognised needs to be one of:";
			*Logger::log << "immediate,discard,cookie,store." << Log::LO;
			trace();
			*Logger::log  << Log::blockend;
		}
	}
	
	if ( XML::Manager::attribute(n,UCS2(L"part"),str_part)  ) {
		if (type == out_cookie) {
			part_type_map::const_iterator k = part_types.find(str_part);
			if( k != part_types.end() ) {
				part = k->second; 
			} else {
				string err_msg; transcode(str_part.c_str(),err_msg);
				*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: part '" << err_msg << "'  not recognised needs to be one of:";
				*Logger::log << "expires,value,domain,value." << Log::LO;
				trace();
				*Logger::log  << Log::blockend;
			}
		}
	}
	
	if ((type == out_immediate || type == out_none) && context != immediate) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: context attribute cannot be used for immediate or discard output spaces." << Log::LO;
		trace();
		*Logger::log << Log::blockend;
	}
	if ( type == out_none && encoder != e_none ) {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: escape attribute cannot be used with discard." << Log::LO ;
		trace();
		*Logger::log << Log::blockend;
	}
	Function* i = dynamic_cast<Function *>(p);	
	if ( i != NULL ) {
		i->outputs.push_back(this);
	} else {
		*Logger::log << Log::syntax << Log::LI << "Syntax Error. Output: outputs can only belong to flow-functions." << Log::LO ;
		trace();
		*Logger::log << Log::blockend;
	}
}
Output::Output(ObyxElement* par,const Output* orig) : IKO(par,orig),type(orig->type),part(orig->part),errowner(false),errs(orig->errs) { 
}
Output::~Output() {
	if (errowner) { //this is why actual body has to be the last iteration.
		delete errs;	
	}
}
void Output::sethttp(const http_line_type line_type,const string& value) {
	Httphead* http = Httphead::service();	
	switch ( line_type ) {
		case cache:					http->setcache(value); break;
		case code:					http->setcode(String::natural(value)); break;
		case connection:			http->setconnection(value); break;
		case content_disposition:	http->setdisposition(value); break;
		case content_length:		http->setlength(String::natural(value)); break; 
		case content_type:			http->setmime(value); break;
		case date:					http->setdate(value); break;
		case h_expires:				http->setexpires(value); break;
		case location:				http->setlocation(value); break;
		case p3p:					http->setp3p(value); break;
		case pragma:				http->setpragma(value); break;
		case range:					http->setrange(value); break;
		case server:				http->setserver(value); break;
		case custom: { 
			pair<string,string> nvpair;
			if ( String::split(':',value, nvpair) ) {
				http->addcustom(nvpair.first,nvpair.second); 
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Output: http custom must be name-value separated by a colon 'name:value'" << Log::LO;
				trace();
				*Logger::log << Log::blockend;
			}
		} break;
		case remove_date: {
			if ( value.compare("true") == 0) {
				http->nodates(true); //ie remove the date headerlines.
			} else {
				if ( value.compare("false") == 0) {
					http->nodates(false); //ie remove the date headerlines.
				} else {
					*Logger::log << Log::error << Log::LI << "Error. Output: http remove_date must be 'true' or 'false'" << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
			}
		} break;
		case remove_http: {
			if (value.compare("true") == 0) {
				http->noheader(true); //ie remove the header
			} else {
				if ( value.compare("false") == 0) {
					http->noheader(false); //ie remove the header
				} else {
					*Logger::log << Log::error << Log::LI << "Error. Output: http remove_http must be 'true' or 'false'" << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
			}
		} break;
		case privacy: {
			if (value.compare("true") == 0) {
				http->setprivate(true);  //ie add privacy
			} else {
				if ( value.compare("false") == 0) {
					http->setprivate(false); //ie remove the header
				} else {
					*Logger::log << Log::error << Log::LI << "Error. Output: http privacy must be 'true' or 'false'" << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
			}
		} break;
		case remove_nocache: {
			if (value.compare("true") == 0) {
				http->nocache(false); //ie remove the nocache headerlines.
			} else {
				if ( value.compare("false") == 0) {
					http->nocache(true); //ie remove the header
				} else {
					*Logger::log << Log::error << Log::LI << "Error. Output: http remove_nocache must be 'true' or 'false'" << Log::LO;
					trace();
					*Logger::log << Log::blockend;
				}
			}
		} break;
		default: {
			*Logger::log << Log::error << Log::LI << "Error. Http output '" << value << "' not implemented." << Log::LO;	
			trace();
			*Logger::log << Log::blockend;
		} break;
	}
}
void Output::evaluate(size_t out_num,size_t out_count) {
	results.undefer();
	prep_breakpoint();
	results.evaluate();
	DataItem* name_part = NULL; 
	DataItem* value_part = NULL; 
	switch ( context ) {
		case immediate: {
			results.takeresult(name_part); 
		} break;
		default: {
			DataItem* context_part = NULL; 
			results.takeresult(context_part);
			//       type    release eval					   name/ref    container 
			evaltype(context, false, false, di_auto, context_part,name_part);
			delete context_part;
			context = immediate;
		} break;
	}
	switch ( type ) {
		case out_immediate: {
			if ( p->results.result() != NULL ) { // &&  p->results.final() surely this is tested already?
				DataItem* pe = NULL;
				if (out_num == out_count) {
					p->results.takeresult(pe);
				} else {
					p->results.result()->copy(pe);
				}
				if ( encoder != e_none ) { //immediate output - just encode, or something.
					if (wsstrip) { 
						pe->trim(); 
						if ( pe->empty() ) {
							delete pe;
							pe = NULL;
						}
					}
					if ( pe != NULL && encoder != e_none ) { 
						process_encoding(pe);
					}
				} 
				results.setresult(pe);	//This is right, because we may not want to strip or encode every output!!
			}
		} break;
		case out_none: {
			results.takeresult(value_part); 
			if (value_part != NULL) {
				delete value_part;
				value_part = NULL;
			}
		} break;
		default: {		//into some special container
			if ( name_part != NULL && ! name_part->empty() ) {
				name_part->trim();
			}
			if ( name_part!= NULL && ! name_part->empty() ) { //are we in the right scope
				DataItem* value_comp= NULL;
				if (out_num == out_count) {
					p->results.takeresult(value_comp);
				} else {
					if ( p->results.result() != NULL) {
						p->results.result()->copy(value_comp);
					}
				}
				process_encoding( value_comp );	
				bool deleteval = true;	
				name_v = *name_part; //stored.
				switch ( type ) {
					case out_xmlnamespace: { 
						if (name_v.find(':') != string::npos) {
							string val_value = *value_comp;
							string err_msg; transcode(name_v.c_str(),err_msg);
							*Logger::log << Log::error << Log::LI << "Error while outputting to namespace. ";
							*Logger::log << Log::error << "Signature " << err_msg << " for namespace " << val_value << " cannot use colons!" << Log::LO;	
							trace();
							*Logger::log << Log::blockend;
						} else {
							ItemStore::setns(name_part,value_comp);
						}
					} break;
					case out_xmlgrammar: { //what to do when value_comp is null?
						ItemStore::setgrammar(name_part,value_comp);
					} break;
					case out_store: {             //0123456789
						string errstring;
						if ( ItemStore::set(name_part,value_comp,kind,errstring) ) { //returns true if all of it is used.
							value_comp = NULL; //taken by object. 
						}
						if (!errstring.empty()) {
							string err_msg; transcode(name_v.c_str(),err_msg);
							*Logger::log << Log::error << Log::LI << "Error while outputting to store with " << err_msg << Log::LO;	
							*Logger::log << Log::LI << errstring << Log::LO;	
							if (value_comp != NULL) {
								string val_value = *value_comp;
								*Logger::log << Log::LI << val_value << Log::LO;	
							}
							trace();
							*Logger::log << Log::blockend;
						}								
					} break;
					case out_error: { 
						string error_stuff;
						error_stuff = errs->str();
						if (! error_stuff.empty() ) {
							ostringstream err_report;
							string top_s,tail_s,tmptitle,errstring;
							Logger::get_title(tmptitle);
							Logger::set_title("Caught Error");
							Logger::top(top_s,false);
							Logger::tail(tail_s);
							err_report << top_s << error_stuff << tail_s;
							string err_result = err_report.str();
							String::normalise(err_result);
							Logger::set_title(tmptitle);
							DataItem* err_doc = new XMLObject(err_result);
							ItemStore::set(name_part, err_doc, di_object,errstring);
							if (!errstring.empty()) {
								string err_msg; transcode(name_v.c_str(),err_msg);
								*Logger::log << Log::error << Log::LI << "Error while outputting an error space to store with " << err_msg << Log::LO;	
								*Logger::log << Log::LI << errstring << Log::LO;	
								trace();
								*Logger::log << Log::blockend;
							}
							//									cout << "-----------------\n";
							//									cout << err_result;
							//									cout << "-----------------\n";
						}
					} break;
					case out_file: {
						Environment* env = Environment::service();
						string root(env->getpathforroot());
						string wd(FileUtils::Path::wd());
						if (wd.empty()) wd = root;
						string filename; if (name_part != NULL) { filename =  *name_part; }
						string filetext; if (value_comp != NULL) { filetext = *value_comp; }
						if (filename[0] == '/') { //we don't want to use file root, but site root.
							filename = root + filename;
						} else {
							filename = wd + '/' + filename;
						}
						FileUtils::Path destination; destination.cd(filename);
						string actual_path = destination.output(true);
						if ( actual_path.find(root) == 0 ) {
							FileUtils::File file(filename);
							bool file_written = file.writeFile(filetext);
							if ( ! file_written ) {
								if ( wd.find(root) == 0 ) {
									wd.erase(0,root.length());
									if ( wd.empty() ) wd = "/";
								}
								actual_path.erase(0,root.length());
								*Logger::log << Log::error << Log::LI << "Error. " <<  name() << ":file " << actual_path << " failed to be output. wd:" << wd << Log::LO;
								trace();
								*Logger::log << Log::blockend;
							}
						} else {
							*Logger::log << Log::error << Log::LI << "Error. " <<  name() << ":file " << filename << " failed to be output because " << actual_path << " points outside of " << root << Log::LO;
							trace();
							*Logger::log << Log::blockend;
						}
					} break;
					case out_http: { 
						u_str oname; 
						if (name_part != NULL) { oname = *name_part; }							
						http_line_type line_type;
						http_line_type_map::const_iterator i = httplinetypes.find(oname);
						if( i != httplinetypes.end() ) {
							line_type = i->second;
							if (line_type == http_object) {
								if (value_comp != NULL) {
									xercesc::DOMDocument* doc = *value_comp;
									if (doc != NULL) {
										xercesc::DOMNode* obj_node = doc->getDocumentElement();
										if (obj_node != NULL) {
											ostringstream* suppressor = new ostringstream();
											Logger::set_stream(suppressor);
											Httphead* http = Httphead::service();	
											http->objectparse(obj_node); //ie remove the date headerlines.
											Logger::unset_stream();
											if (!suppressor->str().empty()) {
												string errdoc; 
												XML::Manager::parser()->writenode(obj_node,errdoc);
												*Logger::log << Log::error << Log::LI << "Error. Http object parse failed." << Log::LO;	
												trace();
												*Logger::log << Log::LI << "The document that failed is:" << Log::LO;
												*Logger::log << Log::LI << Log::info << Log::LI << errdoc << Log::LO << Log::blockend << Log::LO; 
												*Logger::log << Log::blockend; //Error
											}
											delete suppressor;
										} else {
											*Logger::log << Log::error << Log::LI << "Error. Http object had no root element." << Log::LO;	
											trace();
											*Logger::log << Log::blockend;
										}
									} else {
										*Logger::log << Log::error << Log::LI << "Error. Http object couldn't be parsed." << Log::LO;	
										trace();
										*Logger::log << Log::blockend;
									}
								} else {
									*Logger::log << Log::error << Log::LI << "Error. Http object was NULL." << Log::LO;	
									trace();
									*Logger::log << Log::blockend;
								}
							} else {
								string value=""; 
								if (value_comp != NULL) { value = *value_comp; }
								sethttp(line_type,value); 
							}
						} else {
							string err_type; transcode(oname.c_str(),err_type);
							*Logger::log << Log::error << Log::LI << "Error. Http line name '" << err_type << "' not recognised." << Log::LO;	
							trace();
							*Logger::log << Log::blockend;
						}
					} break;
					case out_cookie: {
						Environment* env = Environment::service();
						switch (part) {
							case value: {
								string oname; if (name_part != NULL) { oname =  *name_part; }
								string value; if (value_comp != NULL) { value = *value_comp; }
								env->setcookie_res(oname,value); 
							} break;
							case domain: {
								string oname; if (name_part != NULL) { oname =  *name_part; }
								string value; if (value_comp != NULL) { value = *value_comp; }
								env->setcookie_res_domain(oname,value);
							} break;
							case expires: {
								string oname; if (name_part != NULL) { oname =  *name_part; }
								string value; if (value_comp != NULL) { value = *value_comp; }
								env->setcookie_res_expires(oname,value);
							} break;
							case path: {
								string oname; if (name_part != NULL) { oname =  *name_part; }
								string value; if (value_comp != NULL) { value = *value_comp; }
								env->setcookie_res_path(oname,value);
							} break;
						}
					} break;
					default: {
						*Logger::log << Log::error << Log::LI << "Error. Output: unknown output type!" << Log::LO;							
						trace();
						*Logger::log << Log::blockend;
					} break; //out_immediate and out_none are dealt with already.
				}
				if ( deleteval && value_comp != NULL ) {
					delete value_comp;
					value_comp = NULL;
				}
				delete name_part;
				name_part = NULL;
			} else {
				*Logger::log << Log::error << Log::LI << "Error. Output: non-immediate, value-holding outputs must have a name!" << Log::LO;							
				trace();
				*Logger::log << Log::blockend;
			}
		} break;
	}
	do_breakpoint();
}
void Output::startup() {
	part_types.insert(part_type_map::value_type(UCS2(L"value"), value));
	part_types.insert(part_type_map::value_type(UCS2(L"path"), path));
	part_types.insert(part_type_map::value_type(UCS2(L"domain"), domain));
	part_types.insert(part_type_map::value_type(UCS2(L"expires"), expires));
	output_types.insert(output_type_map::value_type(UCS2(L"store"), out_store));
	output_types.insert(output_type_map::value_type(UCS2(L"immediate"), out_immediate));
	output_types.insert(output_type_map::value_type(UCS2(L"file"), out_file));
	output_types.insert(output_type_map::value_type(UCS2(L"none"), out_none));
	output_types.insert(output_type_map::value_type(UCS2(L"cookie"), out_cookie));
	output_types.insert(output_type_map::value_type(UCS2(L"http"), out_http)); //elements of http header. use name for line.
	output_types.insert(output_type_map::value_type(UCS2(L"error"), out_error));
	output_types.insert(output_type_map::value_type(UCS2(L"namespace"), out_xmlnamespace));
	output_types.insert(output_type_map::value_type(UCS2(L"grammar"), out_xmlgrammar));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Code"), code));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Date"), date));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Server"), server));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Expires"), h_expires));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Private"), privacy));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Cache-Control"), cache));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Pragma"), pragma));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Accept-Ranges"), range));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Content-Type"), content_type));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Location"), location));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Content-Length"), content_length));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Content-Disposition"), content_disposition));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Connection"), connection));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"P3P"), p3p));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"nocache"), nocache));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"remove_http"), remove_http));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"remove_nocache"), remove_nocache));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"remove_date"), remove_date));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"Custom"), custom));
	httplinetypes.insert(http_line_type_map::value_type(UCS2(L"object"), http_object));
}
void Output::shutdown() {
	part_types.clear();
	output_types.clear();
	httplinetypes.clear();
}
