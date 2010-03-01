/* 
 * osiapp.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * osiapp.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <unistd.h>
#include <string>
#include <fstream>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"
#include "commons/httpfetch/httpfetch.h"
#include "commons/filing/filing.h"

#include "osiapp.h"
#include "osimessage.h"
#include "document.h"
#include "dataitem.h"

using namespace Log;
using namespace FileUtils;
using namespace Fetch;
using namespace xercesc;

const std::string OsiAPP::crlf		= "\r\n";
const std::string OsiAPP::crlfcrlf	= "\r\n\r\n";
const std::string OsiAPP::boundary	= "Message_Boundary_";
std::string OsiAPP::last_response	= "";	//

unsigned int OsiAPP::counter = 1;

bool OsiAPP::request(const xercesc::DOMNode* n,DataItem*& the_result) {
	bool request_result = true;
	std::string elname;
	XML::transcode(n->getLocalName(),elname);
	if (elname.compare("http") == 0) {
		n=n->getFirstChild();		//this is one of request|response - but we only support request here.
		while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  {
			n = n->getNextSibling();
		}
		XML::transcode(n->getLocalName(),elname);
		if (elname.compare("request") == 0) {
			string req_url,req_method,req_version;
			if ( ! XML::Manager::attribute(n,"url",req_url)) {
				*Logger::log << Log::error << Log::LI << "Error. OSI 'http' must have a URL attribute." << Log::LO << Log::blockend;
				request_result=false;
			} else {
				if ( ! XML::Manager::attribute(n,"method",req_method))  req_method="GET";
				if ( ! XML::Manager::attribute(n,"version",req_version)) req_version="HTTP/1.1";
				string body;		//we need to initialise this before initializing the HTTPFetch.
				string req_errors;
				if ( HTTPFetch::available() ) {
					HTTPFetch my_req(req_url,req_method,req_version,&body,req_errors);
					vector<std::string> heads;
					DOMNode* msg=n->getFirstChild();		//this is one of request|response - but we only support request here.
					while ( msg != NULL && msg->getNodeType() != DOMNode::ELEMENT_NODE)  {
						msg = msg->getNextSibling();
					}
					if (msg != NULL) {
						decompile_message(msg,heads,body);		//this is a message...
					}
					//---------------- COMMENT  when NOT debugging
					/*					
					 string logb = Environment::ScratchDir();
					 logb.append("osi.log");
					 ofstream logFile(logb.c_str()); 
					 for (unsigned int i=0; i < heads.size(); i++) {
					 logFile << heads[i] << crlf;
					 }
					 logFile << crlf << body; 
					 logFile.close(); 
					 */
					//---------------- COMMENT  when NOT debugging
					for (unsigned int i=0; i < heads.size(); i++) {
						my_req.addHeader(heads[i]);
					}
					last_response.clear();
					string response_head,response_body;	//what was returned by remote server...
					if (! my_req.doRequest(response_head,response_body,req_errors) ) {
						compile_http_response(response_head,response_body,last_response);
						*Logger::log << Log::error << Log::LI << Log::II << req_url << Log::IO << Log::II << req_errors << Log::IO << Log::LO << Log::blockend;
						request_result=false;
					} else {
						compile_http_response(response_head,response_body,last_response);
						the_result = DataItem::factory(last_response,qxml::di_object);
					}
				} else {
					*Logger::log << Log::error << Log::LI<< "Error. OSI request failed for url:" << req_url << " - HTTPFetch not available." << Log::LO << Log::blockend;				
				}
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. OSI 'http' only supports request for outwards initiation." << Log::LO << Log::blockend;
			request_result=false;
		}
	} else {
		if (elname.compare("mta") == 0) { //mail transfer agent - using sendmail
			n=n->getFirstChild();		  //we only support send here.
			while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  {
				n = n->getNextSibling();
			}
			XML::transcode(n->getLocalName(),elname);
			if (elname.compare("send") == 0) {
				string send_path,env_sender;
				if (XML::Manager::attribute(n,"sender",env_sender)) {
					String::mailencode(env_sender);
				}
				if ( ! Environment::getenv("OBYX_MTA",send_path)) {
					*Logger::log << Log::error << Log::LI << "Error. OBYX_MTA must be defined which is a path to the sendmail binary." << Log::LO << Log::blockend;
					request_result=false;
				} else {
					string body;
					vector<std::string> heads;
					n=n->getFirstChild();		  //we only support send here.
					while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  {
						n = n->getNextSibling();
					}
					decompile_message(n,heads,body);		//this is a message...
					string mfil = Environment::ScratchDir();
					mfil.append(Environment::ScratchName());
					string resf = mfil;
					mfil.append("osimail.file");
					resf.append("osimail.res");
					ofstream outFile(mfil.c_str()); 
					for (unsigned int i=0; i < heads.size(); i++) {
						outFile << heads[i] << crlf;
					}
					outFile << crlf << body; 
					outFile.close();
					String::strip(send_path);
					ostringstream cmd;
					cmd << "cat " << mfil << " | " << send_path;
					if (! env_sender.empty() ) {
						cmd << " -r \"" << env_sender << "\"";
					}
					cmd << " -t > " << resf;  
					system(cmd.str().c_str());
					FileUtils::File file(resf);
					if (file.exists()) {
						off_t flen = file.getSize();
						if ( flen != 0 ) {
							string file_content;
							file.readFile(file_content);
							the_result = DataItem::factory(file_content);
						}
						file.removeFile();
					}
					FileUtils::File srcfile(mfil);
					if (srcfile.exists()) {
						srcfile.removeFile();
					}					
					request_result=true;
//---------------- COMMENT  when NOT debugging
					/*
						string logb = Environment::ScratchDir();
						logb.append("osi.log");
						ofstream logFile(logb.c_str()); 
						for (unsigned int i=0; i < heads.size(); i++) {
							logFile << heads[i] << crlf;
						}
						logFile << crlf << body; 
						logFile.close(); 
					 */
//---------------- COMMENT  when NOT debugging
				}
			} else {
				*Logger::log << Log::error << Log::LI << "Error. OSI 'mta' only supports send for the time being." << Log::LO << Log::blockend;
				request_result=false;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. OSI application " << elname << " is not supported." << Log::LO << Log::blockend;
			request_result=false;
		}
	}
	return request_result;
}

void OsiAPP::compile_http_response(string& head, string& body, string& the_result) {
	ostringstream res;
	string res_vals[3];
	res << "<osi:http xmlns:osi=\"http://www.obyx.org/osi-application-layer\">";
	size_t splitpoint = 0; //when curl does redirects - it queues up each response, and we only want the final response.
	splitpoint = head.rfind(crlfcrlf);
	if (splitpoint != string::npos) {
		splitpoint = head.rfind(crlfcrlf,splitpoint-4);
		if (splitpoint < string::npos ) { 
			head = head.substr(splitpoint+4,string::npos);
		}
	}

	size_t hd_endLinePos = head.find(crlf);
	string res_val = head.substr(0,hd_endLinePos);
	if(hd_endLinePos != string::npos) {
		head.erase(0, hd_endLinePos+2);
	} else {
		head.erase();
	}
	pair<string,string> res_vs;
	String::split(' ',res_val,res_vs);
	res_vals[0] = res_vs.first;
	res_val = res_vs.second;
	String::split(' ',res_val,res_vs);
	res_vals[1] = res_vs.first;
	res_vals[2] = res_vs.second;
	res << "<osi:response";
	if ( !res_vals[0].empty()) res << " version=\"" << res_vals[0] << "\"";
	if ( !res_vals[1].empty()) res << " code=\"" << res_vals[1] << "\"";
	if ( !res_vals[2].empty()) res << " reason=\"" << res_vals[2] << "\"";
	res << ">";
	string msg = head+body;
	compile_message(msg,res);
	res << "</osi:response></osi:http>";
	the_result = res.str();
}

void OsiAPP::compile_http_request(string& head, string& body, string& the_result) {
	ostringstream res;
	string res_vals[3];
	res << "<osi:http xmlns:osi=\"http://www.obyx.org/osi-application-layer\">";
	size_t hd_endLinePos = head.find(crlf);
//	size_t hd_endLinePos = head.find(crlf);
	string res_val = head.substr(0,hd_endLinePos);
	if(hd_endLinePos != string::npos) {
		head.erase(0, hd_endLinePos+2);
	} else {
		head.erase();
	}
	pair<string,string> res_vs;
	String::split(' ',res_val,res_vs);
	res_vals[0] = res_vs.first;
	res_val = res_vs.second;
	String::split(' ',res_val,res_vs);
	res_vals[1] = res_vs.first;
	res_vals[2] = res_vs.second;
/*	
	size_t nextPos, pos = 0; unsigned int n=0;
	do {
		nextPos = head.find(' ', pos);
		if(nextPos == string::npos || nextPos > hd_endLinePos ) {
			if(hd_endLinePos != string::npos) {
				res_vals[n++] = head.substr(pos, hd_endLinePos - pos);
			} else {
				res_vals[n++] = head.substr(pos, hd_endLinePos);
			}
		} else {
			res_vals[n++] = head.substr(pos, nextPos-pos);
			pos = ++nextPos; 
		}
	} while (nextPos != string::npos && nextPos < hd_endLinePos);
	if(hd_endLinePos != string::npos) {
		head.erase(0, hd_endLinePos+2);
	} else {
		head.erase(0, hd_endLinePos);
	}
*/	
	res << "<osi:request";
	String::xmlencode(res_vals[1]);
	if ( !res_vals[0].empty()) res << " method=\"" << res_vals[0] << "\"";
	if ( !res_vals[1].empty()) res << " url=\"" << res_vals[1] << "\"";
	if ( !res_vals[2].empty()) res << " version=\"" << res_vals[2] << "\"";
	res << ">";
	string msg = head+crlfcrlf+body;
	compile_message(msg,res);
	res << "</osi:request></osi:http>";
	the_result = res.str();
}

void OsiAPP::compile_message(string& msg_str, ostringstream& res, bool do_namespace) {
	OsiMessage msg;
	msg.compile(msg_str,res,do_namespace);
}

//Take an xml osi message and turn it into an RFC standard message.
//n must point to root message element. (only partially works with non-crlf messages)
void OsiAPP::decompile_message(const xercesc::DOMNode* n,vector<std::string>& heads, string& body,bool addlength) {
	std::string head,encoded_s,angled_s;
	if ( n != NULL && n->getNodeType() == DOMNode::ELEMENT_NODE) {
		std::string elname;
		XML::transcode(n->getLocalName(),elname);
		if (elname.compare("message") == 0) {
			n=n->getFirstChild();		  //first element of message == header OR body OR NULL OR Whitespace.
			while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  { //Skip whitespace.
				n = n->getNextSibling();
			}
			//Now this is either header or body or NULL.
			if ( n != NULL ) {
				XML::transcode(n->getLocalName(),elname);
				//Handle the (multiple) header elements.
				//Header is a single line: name: value; subvalue="foo"; subvue="bar";
				while (n != NULL && n->getNodeType() == DOMNode::ELEMENT_NODE && elname.compare("header") == 0) {	//basically - headers
					std::string name;
					XML::Manager::attribute(n,"name",name); //mandatory
					head.append(name); head.append(": ");
					std::string header_value;
					if ( XML::Manager::attribute(n,"value",header_value)) { //optional
						if( XML::Manager::attribute(n,"urlencoded",encoded_s)) { //subhead value
							if ( encoded_s.compare("true") == 0 ) String::urldecode(header_value);
						}
						if( XML::Manager::attribute(n,"angled",angled_s)) { //subhead value
							if ( angled_s.compare("true") == 0 ) header_value = '<' + header_value + '>';
						}
						if (name.compare("Received") != 0) {
							head.append(header_value); //now finished with the main part of the line.
						}
					}
					
					//Header may have sub-heads...
					DOMNode* ch=n->getFirstChild();
					while (ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE) ch=ch->getNextSibling();
					if ( ch != NULL ) {
						if ( ! header_value.empty() ) {
							if (name.compare("Received") != 0) {
								head.push_back(';');
								head.push_back(' ');
							}
						} 
						string subhead;
						XML::transcode(ch->getLocalName(),subhead);
						if (subhead.compare("subhead") == 0 || subhead.compare("address") == 0) {
							while (ch != NULL && (subhead.compare("subhead") == 0 || subhead.compare("address") == 0)) {
								std::string shvalue,shnote,shname;
								bool url_encoded=false;
								if (XML::Manager::attribute(ch,"name",shname)) head.append(shname);
								if( XML::Manager::attribute(ch,"urlencoded",encoded_s)) { //subhead value
									if ( encoded_s.compare("true") == 0 ) url_encoded=true;
								}
								if(! XML::Manager::attribute(ch,"value",shvalue)) { //subhead value
									DOMNode* shvo=ch->getFirstChild();
									while ( shvo != NULL ) {
										if (shvo->getNodeType() == DOMNode::ELEMENT_NODE) {
											string sh; XML::Manager::parser()->writenode(shvo,sh);
											shvalue.append(sh);
										}
										shvo=shvo->getNextSibling();
									}
								}
								if (! shvalue.empty() || ! shnote.empty() ) {
									if (url_encoded) String::urldecode(shvalue);
									if (subhead.compare("address") == 0) {
										XML::Manager::attribute(ch,"note",shnote);
										if (!shnote.empty()) {
											head.append(shnote);
											head.push_back(' ');
										}
										head.push_back('<');
										head.append(shvalue);
										head.push_back('>');
									} else {
										if( XML::Manager::attribute(n,"angled",angled_s)) { //subhead value
											if ( angled_s.compare("true") == 0 ) shvalue = '<' + shvalue + '>';
										}
										if (name.compare("Received") == 0) {
											head.push_back(' ');head.append(shvalue);	
										} else {
											head.append("=\"");head.append(shvalue);head.push_back('"');	
										}
									}
								}
								ch=ch->getNextSibling();
								while (ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE) ch=ch->getNextSibling();
								if (ch != NULL) {
									if (name.compare("Received") != 0) {
										head.push_back(';');
									}
									head.push_back(' ');
									XML::transcode(ch->getLocalName(),subhead);
								} else {
									subhead.clear();
								}
							}
						} else {
							for ( DOMNode* hv=n->getFirstChild(); hv != NULL; hv=hv->getNextSibling()) {
								if (hv->getNodeType() == DOMNode::ELEMENT_NODE) {
									string sh; XML::Manager::parser()->writenode(hv,sh);
									head.append(sh);
								}
							}
						}
					}
					if (name.compare("Received") == 0) {
						head.append("; ");
						head.append(header_value); //now finished with the main part of the line.
					}
					heads.push_back(head); head.clear();
					do { n = n->getNextSibling(); } while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE);						
					if (n != NULL) {
						XML::transcode(n->getLocalName(),elname);
					}
				} 
				//Now handle the (single) body element.
				if ( n != NULL && n->getNodeType() == DOMNode::ELEMENT_NODE && elname.compare("body") == 0 ) { //this is a body..
					std::string mechanism,type,subtype,encoded;
					XML::Manager::attribute(n,"mechanism",mechanism); //optional
					XML::Manager::attribute(n,"type",type);           //optional
					XML::Manager::attribute(n,"subtype",subtype);     //optional
					XML::Manager::attribute(n,"urlencoded",encoded);  //optional, NOT with multiparts..
					if (!mechanism.empty()) {
						head.append("Content-Transfer-Encoding: ");
						head.append(mechanism); 
						heads.push_back(head); head.clear();
					}
					// Content-Type: type=multipart/mixed; boundary=boundary-002
					if ( ! type.empty() ) {
						head.append("Content-Type: ");
						head.append(type); head.push_back('/'); head.append(subtype); 
						if (type.compare("multipart") == 0) { // need to compose a set of messages..
							vector<std::string> messages;
							string loc_boundary(boundary);
							loc_boundary.append(String::tofixedstring(6,counter++));
							DOMNode* ch=n->getFirstChild();		  //first element of message == header OR body OR NULL
							while ( ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE)  {
								ch = ch->getNextSibling();
							}			
							while (ch!=NULL) {
								string msgname;
								XML::transcode(ch->getLocalName(),msgname); //textnodes must be discarded..
								if (msgname.compare("message") == 0) {
									vector<string> tmphds;
									string tmpmsg,tmpbody;
									decompile_message(ch,tmphds,tmpbody,false);
									for (unsigned int i=0; i < tmphds.size(); i++) {
										tmpmsg.append(tmphds[i]);
										tmpmsg.append(crlf);
									} 
									tmpmsg.append(crlf); 
									tmpmsg.append(tmpbody);
									bool boundary_clash = (tmpmsg.find(loc_boundary) != string::npos);
									while (boundary_clash) {
										loc_boundary=boundary;
										loc_boundary.append(String::tofixedstring(6,counter++));
										for (unsigned int x=0; x < messages.size(); x++) {
											boundary_clash = messages[x].find(loc_boundary) != string::npos;
											if (boundary_clash) break;
										}
										if (!boundary_clash) { 
											boundary_clash = (tmpmsg.find(loc_boundary) != string::npos);
										}
									}
									messages.push_back(tmpmsg);
								}
								ch = ch->getNextSibling();
								while ( ch != NULL && ch->getNodeType() != DOMNode::ELEMENT_NODE)  {
									ch = ch->getNextSibling();
								}
							}
							//we now have a vector of messages and we have a non-clashing boundary...
							//quotes on the boundary subhead appear to be very poorly supported.
							//						head.append("; boundary=\"");
							//						head.append(loc_boundary);
							//						head.push_back('"');
							head.append("; boundary=");
							head.append(loc_boundary);
							heads.push_back(head); head.clear();
							
							for (unsigned int x=0; x < messages.size(); x++) {
								body.append("--");
								body.append(loc_boundary);
								body.append(crlf);
								body.append(messages[x]);
								body.append(crlf);
							}
							body.append("--");
							body.append(loc_boundary);
							body.append("--");
						} else {
							heads.push_back(head); head.clear();
							DOMNode* ch=n->getFirstChild();
							while (ch != NULL) {
 								string sh; XML::Manager::parser()->writenode(ch,sh);
								body.append(sh);
								ch=ch->getNextSibling();
							}
							if (encoded.compare("true") == 0 ) String::urldecode(body);
							if (addlength) {
								head.append("Content-Length: ");
								head.append(String::tostring((long long unsigned int)body.size())); 
								heads.push_back(head); head.clear();
							}
						}
					} else {
						DOMNode* ch=n->getFirstChild();
						while (ch != NULL) {
							string sh; XML::Manager::parser()->writenode(ch,sh);
							body.append(sh);
							ch=ch->getNextSibling();
						}
						if (encoded.compare("true") == 0 ) String::urldecode(body);
						if (addlength) {
							head.append("Content-Length: ");
							head.append(String::tostring((long long unsigned int)body.size())); 
							heads.push_back(head); head.clear();
						}
					}
				}
			} 
			else {
				*Logger::log << Log::error << Log::LI << "Error. OSI application, message was empty. Nothing was constructed." << Log::LO << Log::blockend;
			}
		} else {
			*Logger::log << Log::error << Log::LI << "Error. OSI application, message was expected, not '" << elname << "'." << Log::LO << Log::blockend;
		}
	} else {
		*Logger::log << Log::error << Log::LI << "Error. OSI application, element node 'message' was missing." << Log::LO << Log::blockend;
	}
}
