/*
 * json.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * json.cpp is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2009-2010 Red Snapper Ltd. http://www.redsnapper.net
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


#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "dataitem.h"
#include "json.h"

using namespace Log;
using namespace XML;
using namespace std;

/*********************************** JSON ***************************************/
const u_str Json::k_object	= UCS2(L"object");
const u_str Json::k_array	= UCS2(L"array");
const u_str Json::k_name	= UCS2(L"name");
const u_str Json::k_value	= UCS2(L"value");
const u_str Json::k_type	= UCS2(L"type");
const u_str Json::kt_string	= UCS2(L"string");
const u_str Json::kt_number	= UCS2(L"number");
const u_str Json::kt_null	= UCS2(L"null");
const u_str Json::kt_bool	= UCS2(L"bool");
bool Json::encode(DataItem** basis,const kind_type,string& err_str) const {
	bool retval = true; DataItem* orig = *basis; 
	if (orig != NULL) {
		ostringstream result,errs;
		xercesc::DOMDocument* d = *orig;
		if (d != NULL) {
			xercesc::DOMNode* n=d->getFirstChild();
			compose(n,result,errs);
		} else {
			errs << "value was not a legal object for json encoding";
		}
		err_str = errs.str();
		if (err_str.empty()) {
			*basis = DataItem::factory(result.str(),di_text);
			delete orig; orig = NULL; 
		} else {
			retval = false;
		}
	}
	return retval;
}
void Json::nextel(const xercesc::DOMNode*& n) const {
	if (n != NULL) {
		n = n->getNextSibling();
		while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE) {
			n = n->getNextSibling(); 
		}
	}
}
void Json::compose(const xercesc::DOMNode* n ,ostringstream& o,ostringstream& e) const {
	if ( n != NULL && n->getNodeType() == DOMNode::ELEMENT_NODE) {
		u_str attr,eln = n->getLocalName();
		const xercesc::DOMNode* p = n->getParentNode();
		if (p != NULL && p->getNodeType() == DOMNode::ELEMENT_NODE) {
			u_str pn = p->getLocalName();
			if (pn.compare(k_object) ==0 ) {
				Manager::attribute(n,k_name,attr);
				if (! attr.empty()) {
					string name;
					XML::Manager::transcode(attr.c_str(),name);
					o << "\"" << name << "\":";
				} else {
					o << "\"\":";
				}
			}
		}
		if (eln.compare(k_object) == 0) {
			o << "{";
			n=n->getFirstChild();
			if (n != NULL) {
				if (n->getNodeType() != DOMNode::ELEMENT_NODE) {
					nextel(n);
				}
				if (n != NULL) { compose(n,o,e); }
				nextel(n);
				while (n != NULL) { 
					o << ",";
					compose(n,o,e);
					nextel(n);
				}
			}
			o << "}";
		} else {
			if (eln.compare(k_array) == 0) {
				o << "[";
				n=n->getFirstChild();
				if (n != NULL) {
					if (n->getNodeType() != DOMNode::ELEMENT_NODE) {
						nextel(n);
					}
					if (n != NULL) { compose(n,o,e); }
					nextel(n);
					while (n != NULL) { 
						o << ",";
						compose(n,o,e);
						nextel(n);
					}
				}
				o << "]";
			} else {
				if (eln.compare(k_value) == 0) {
					type_t el_type = t_string;
					u_str txt,a_type;
					Manager::attribute(n,k_type,a_type);
					if (! a_type.empty()) {
						if (a_type.compare(kt_number) == 0) {
							el_type = t_number;
						} else {
							if (a_type.compare(kt_bool) == 0) {
								el_type = t_bool;
							} else {
								if (a_type.compare(kt_null) == 0) {
									el_type = t_null;
								} else {
									if (a_type.compare(kt_string) != 0) {
										string err; 
										XML::Manager::transcode(a_type.c_str(),err);
										e << "type attribute value '" << err <<"' is not allowd.";
									}
								}
							}
						}
					}
					if (n->getTextContent() != NULL) {
						txt = n->getTextContent();
					}
					if (! txt.empty()) {
						switch (el_type) {
							case t_string: { 
								string value; 
								XML::Manager::transcode(txt.c_str(),value);
								String::fandr(value,"\\","\\\\"); //backslash.
								String::fandr(value,"\r","\\r");
								String::fandr(value,"\n","\\n");
								String::fandr(value,"\t","\\t");
								String::fandr(value,"\"","\\\"");
								o << "\"" << value << "\""; 
							} break;
							case t_number: { 
								string value; 
								XML::Manager::transcode(txt.c_str(),value);
								const size_t n = value.size();
								size_t i = 0;
								if (n > 0) {
									if (value[i] == '-') { o << "-"; i++; }
									if (value[i] == '0') { o << "0"; i++; } else {
										while (i < n && value[i] >= '0' && value[i] <= '9') {
											o << value[i]; i++;
										}
									}
									if (i < n) {
										if (value[i] == '.') {
											o << value[i]; i++;
											while (i < n && value[i] >= '0' && value[i] <= '9') {
												o << value[i]; i++;
											}
										}
										if (i < n) {
											if (value[i] == 'e' || value[i] == 'E') {
												o << value[i]; i++;
												if (value[i] == '-' || value[i] == '+' ) { o << value[i]; i++; }
												while (i < n && value[i] >= '0' && value[i] <= '9') {
													o << value[i]; i++;
												}
											}
										}
									}
								} else {
									o << "0"; 
								}
								if (i != n) {
									e << "Number " << value << " appears to be malformed.";
								}
							} break;
							case t_bool: { 
								if (txt.compare(UCS2(L"true")) == 0) {
									o << "true"; 
								} else {
									o << "false"; 
								}
							} break;
							case t_null: { o << "null"; } break;
						}
					} else {
						switch (el_type) {
							case t_string: {
								o << "\"\""; 
							} break;
							case t_number: { o << "0"; } break;
							case t_bool: { 
								o << "true"; //default value?
							} break;
							case t_null: { o << "null"; } break;
						}
					}
				} else {
					string err; 
					XML::Manager::transcode(eln.c_str(),err);
					e << "Element named '" << err <<"' is not known. ";
				}
			}
		}
	}
}
bool Json::decode(DataItem** basis,const kind_type,string& err_str) const {
	//decode: turn from being a json thing to being an xml thing.
	ostringstream errors;
	bool retval = true; DataItem* orig = *basis; 
	if (orig != NULL) {
		string encoded = *orig;
		ostringstream result,errs;
		size_t offset = encoded.find_first_of("[{");	//skip any wrap.
		if (offset != string::npos) {
			string::const_iterator i = encoded.begin() + offset;
			const string::const_iterator e = encoded.end();
			if (! encoded.empty() ) {
				retval = do_value(i,e,result,false,errors);
				if ( retval ) {
					string file = result.str();
					size_t i = file.find('>');
					file.insert(i," xmlns=\"http://www.obyx.org/json\"");
					*basis = DataItem::factory(file,di_object);
					delete orig; orig = NULL; 
				}
				err_str = errors.str();
				if (!err_str.empty()) {
					retval = false;
				}
			}
		} else {
			err_str = "json must include a containing array or object";
			retval = false;
		}
	}
	return retval;
}
bool Json::do_qv(string::const_iterator& i,const string::const_iterator& end, string& qv, ostringstream& err) const {
	bool retval = true;
	while (*i != '"' && i < end) { i++; } //skip over whitespace
	if (i < end) { //*i = "
		if (*i == '"') {
			i++; // jump over the "
			while (*i != '"' && i < end) { 
				if (*i == '\\') { 
					i++; 
					switch (*i) {
						case 'b': 
						case 'f': { //pass over bell and formfeed.
							i++;
						} break;
						case 'n': {
							qv.push_back('\n');
							i++;
						} break;
						case 'r': {
							qv.push_back('\r');
							i++;
						} break;
						case 't': {
							qv.push_back('\t');
							i++;
						} break;
						case 'u': {
							if (i+5 < end) {
								string hex_err;
								string uval = "0x"+string(i+1,i+5);
								i+=5;
								pair<long long,bool> ucs4 = String::integer(uval);
								if (ucs4.second) {
									uval = String::UCS4toUTF8(ucs4.first);
									qv.append(uval);
								} else {
									err << " Unexpected hex conversion of Unicode character.";
									retval = false;
								}
							} else {
								err << " Unexpected finish within utf-8 encoding.";
								retval = false;
							}
						} break;							
						default: {
							qv.push_back(*i);
							i++;
						} break;
					}
				} else {
					if ( i < end ) {
						qv.push_back(*i);
						i++; 
					}
				}
			}
			if (i < end) {
				XMLChar::encode(qv);
			} else {
				err << " Terminal quote not found while decomposing quoted-value.";
				retval = false;
			}
			i++; // jump over the "
		} else {
			retval = false;
			//this is the END of an object or array.
		}
	} else {
		err << " Initial quote not found while decomposing quoted-value.";
		retval = false;
	}
	return retval;	
}
bool Json::do_nullbool(string::const_iterator& i,const string::const_iterator&,ostringstream& result,const string& name,ostringstream& err) const {
	bool retval = true;
	string value;
	switch (*i) {
		case 'n': {
			value = string(i,i+4);
			if (value.compare("null") == 0) {
				result << "<value type=\"null\"" << name << "></value>";
			} else {
				err << " 'null' expected but '" << value << "' found.";
				retval = false;
			}
		} break;
		case 't': {
			value = string(i,i+4);
			if (value.compare("true") == 0) {
				result << "<value type=\"bool\"" << name << ">true</value>";
			} else {
				err << " 'true' expected but '" << value << "' found.";
				retval = false;
			}
		} break;
		case 'f': {
			value = string(i,i+5);
			if (value.compare("false") == 0) {
				result << "<value type=\"bool\"" << name << ">false</value>";
			} else {
				err << " 'false' expected but '" << value << "' found.";
				retval = false;
			}
		} break;
	}
	return retval;
}
bool Json::do_number(string::const_iterator& i,const string::const_iterator& end,ostringstream& result,const string& name,ostringstream& err) const {
	bool retval = true;
	string::const_iterator b = i;
	const string number_chars="-+0.123456789Ee"; //this is a quick hack.
	while (i < end && number_chars.find(*i) != string::npos) { i++; }
	if (b < i) {
		result << "<value type=\"number\"" << name << ">" << string(b,i) << "</value>";
	} else {
		err << " Value decomposition failed while gathering a numeric value.";
		retval = false;
	}
	return retval;
}
bool Json::do_string(string::const_iterator& i,const string::const_iterator& end,ostringstream& result,const string& name,ostringstream& err) const {
	bool retval = true;
	string value;
	if (do_qv(i,end,value,err)) {
		result << "<value" << name << ">" << value << "</value>"; // type=\"string\" = default so not necessary.
	} else {
		err << " Value decomposition failed while gathering a string value.";
		retval = false;
	}
	return retval;
}
bool Json::do_name(string::const_iterator& i,const string::const_iterator& end, ostringstream& name, ostringstream& err) const {
	bool retval = true;
	string nameval;
	if (do_qv(i,end,nameval,err)) {
		name << " name=\"" << nameval << "\"";
	} else {
		retval = false;
		err << " Name decomposition failed while gathering name.";
	}
	if (retval) {
		while (*i != ':' && i < end) { i++; }
		if (i == end) { 
			err << " Name-value delimiter ':' was missing.";
			retval = false;
		} else {
			i++;
		}
	}
	return retval;
}
bool Json::do_vlist(string::const_iterator& i,const string::const_iterator& end, ostringstream& result, bool doname, ostringstream& err) const {
	bool moretodo = true,retval = true;
	while (i < end && (*i == ' ' || *i == '\r' || *i == '\n' || *i == '\t')) { i++; } //skip over whitespace
	if ((doname && *i == '}') || (!doname && *i == ']')) {
		moretodo = false;
	}
	while (retval && moretodo) {
		retval = do_value(i,end,result,doname,err);
		while (retval && *i != ',' && *i != ']'  && *i != '}' &&i < end) { i++; } //skip over everything but delimiters and end.
		if (*i != ',') {
			moretodo = false;
		} else {
			i++;
		}
	}
	return retval;
}
bool Json::do_object(string::const_iterator& i,const string::const_iterator& end,ostringstream& result,const string& name, ostringstream& err) const {
	bool retval = true;
	i++;	//skip over brace.
	result << "<object" << name << ">";
	retval = do_vlist(i,end,result,true,err);
	result << "</object>";
	if (*i != '}') {
		err << " Object decomposition failed - terminating bracket was missing.";
		retval = false;
	} else { 
		i++; 
	}
	return retval;
}
bool Json::do_array(string::const_iterator& i,const string::const_iterator& end,ostringstream& result,const string& name, ostringstream& err) const {
	bool retval = true;
	i++;	//skip over brace.
	result << "<array" << name << ">";
	retval = do_vlist(i,end,result,false,err);
	result << "</array>";
	if (*i != ']') {
		err << " Array decomposition failed - terminating brace was missing.";
		retval = false;
	} else { 
		i++; 
	}
	return retval;
}
bool Json::do_value(string::const_iterator& i,const string::const_iterator& end, ostringstream& result, bool doname, ostringstream& err) const {
	string name;
	bool retval = true;
	if (doname) {
		ostringstream name_str;
		if (!do_name(i,end,name_str,err) ) {
			retval = false;
			err << " Name decomposition failed while gathering a named value.";
		} else {
			name=name_str.str();
		}
	}
	if (retval) {
		bool found=false;
		while (!found && retval) {
			found = true;
			switch (*i) {
				case '{': { //object
					retval = do_object(i,end,result,name,err);
				} break;
				case '[': { //array
					retval = do_array(i,end,result,name,err);
				} break;
				case '"': { //string
					retval = do_string(i,end,result,name,err);
				} break;
				case 'n':
				case 't': 
				case 'f': { //bool or null
					retval = do_nullbool (i,end,result,name,err);
				} break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '-': { //number
					retval = do_number(i,end,result,name,err);
				} break;
				case '\t':
				case '\r':
				case '\n':
				case ' ': {
					i++;
					found = false;
				} break;
				default: {
					err << " Unexpected character: '" << *i << "' found.";
					i++;
					retval = false;
				}  break;
			}
		}
	}
	return retval;
}
