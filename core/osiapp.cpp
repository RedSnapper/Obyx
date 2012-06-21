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
const std::string OsiAPP::crlft		= "\r\n\t";
const std::string OsiAPP::boundary	= "Message_Boundary_";
std::string OsiAPP::last_response	= "";	//

//int max_redirects = 33, timeout_secs = 30
bool OsiAPP::request(const xercesc::DOMNode* n,int max_redirects,int timeout_secs,DataItem*& the_result) {
	Environment* env = Environment::service();
	std::string elname = "";
	bool request_result = true;
	XML::Manager::transcode(n->getLocalName(),elname);
	if (elname.compare("http") == 0) {
		n=n->getFirstChild();		//this is one of request|response - but we only support request here.
		while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  {
			n = n->getNextSibling();
		}
		XML::Manager::transcode(n->getLocalName(),elname);
		if (elname.compare("request") == 0) {
			string req_url,req_method,req_version;
			if ( ! XML::Manager::attribute(n,UCS2(L"url"),req_url)) {
				*Logger::log << Log::error << Log::LI << "Error. OSI 'http' request must have a url attribute." << Log::LO << Log::blockend;
				request_result=false;
			} else {
				if ( ! XML::Manager::attribute(n,UCS2(L"method"),req_method)) { 
					*Logger::log << Log::error << Log::LI << "Error. OSI 'http' request must have a method attribute. Normally GET or POST." << Log::LO << Log::blockend;
					request_result=false;
				}
				String::toupper(req_method);
				if ( ! XML::Manager::attribute(n,UCS2(L"version"),req_version)) { 
					req_version="HTTP/1.0";
				}
				string body;		//we need to initialise this before initializing the HTTPFetch.
				string req_errors;
				if ( HTTPFetch::available() ) {
					vector<std::string> heads;
					DOMNode* msg=n->getFirstChild();		//this is one of request|response - but we only support request here.
					while ( msg != NULL && msg->getNodeType() != DOMNode::ELEMENT_NODE)  {
						msg = msg->getNextSibling();
					}
					if (msg != NULL) {
						OsiMessage::decompile(msg,heads,body);		//this is a message...
					}
//					int max_redirects = 33, timeout_secs = 30;
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
					HTTPFetch my_req(req_url,req_method,req_version,body,max_redirects,timeout_secs,req_errors);
					for (unsigned int i=0; i < heads.size(); i++) {
						String::fandr(heads[i],crlf,crlft);		//crlf in heads need a tab after them to indicate that they are not heads.
						my_req.addHeader(heads[i]);
					}
					last_response.clear();
					string response_head,response_body;	//what was returned by remote server...
					if (! my_req.doRequest(response_head,response_body, max_redirects, timeout_secs, req_errors) ) {
						compile_http_response(response_head,response_body,last_response);
						*Logger::log << Log::error << Log::LI << Log::II << req_url << Log::IO << Log::II << req_errors << Log::IO << Log::LO << Log::blockend;
						request_result=false;
					} else {
						compile_http_response(response_head,response_body,last_response);
						the_result = DataItem::factory(last_response,obyx::di_object);
					}
				} else {
					*Logger::log << Log::error << Log::LI<< "Error. OSI request failed for url:" << req_url << " - HTTPFetch not available." << Log::LO << Log::blockend;				
					request_result=false;
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
			XML::Manager::transcode(n->getLocalName(),elname);
			if (elname.compare("send") == 0) {
				string send_path,env_sender;
				if (XML::Manager::attribute(n,UCS2(L"sender"),env_sender)) {
					String::mailencode(env_sender);
				}
				if ( ! env->getenv("OBYX_MTA",send_path)) {
					*Logger::log << Log::error << Log::LI << "Error. OBYX_MTA must be defined which is a path to the sendmail binary." << Log::LO << Log::blockend;
					request_result=false;
				} else {
					n=n->getFirstChild();		  //we only support send here.
					while ( n != NULL && n->getNodeType() != DOMNode::ELEMENT_NODE)  {
						n = n->getNextSibling();
					}
					string mfil = env->ScratchDir();
					mfil.append(env->ScratchName());
					string resf = mfil;
					mfil.append("osimail.file");
					resf.append("osimail.res");
					ofstream outFile(mfil.c_str()); 
					OsiMessage::decompile(n,outFile,false,true);
					outFile.close();
					String::strip(send_path);
					ostringstream cmd;
					cmd << "cat " << mfil << " | " << send_path;
					if (! env_sender.empty() ) {
						cmd << " -f \"" << env_sender << "\""; //-r is now deprecated by sendmail.org
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
	if ( !res_vals[0].empty()) { 
		res << " version=\"" << res_vals[0] << "\""; 
	} else {
		res << " version=\"HTTP/1.0\"";
	}
	if ( !res_vals[1].empty()) {
		res << " code=\"" << res_vals[1] << "\"";
	} else {
		res << " code=\"200\"";
	}
	if ( !res_vals[2].empty()) { 
		res << " reason=\"" << res_vals[2] << "\"";
	} else {
		res << " reason=\"OK\"";
	}
	res << ">";
	string msg_str = head+body;
	OsiMessage msg;
	msg.compile(msg_str,res,true);
	res << "</osi:response></osi:http>";
	the_result = res.str();
}

void OsiAPP::compile_http_request(string& head, string& body, string& the_result) {
	ostringstream res;
	string res_vals[3];
	res << "<osi:http xmlns:osi=\"http://www.obyx.org/osi-application-layer\">";
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
	res << "<osi:request";
	String::xmlencode(res_vals[1]);
	if ( !res_vals[0].empty()) { res << " method=\"" << res_vals[0] << "\""; }
	if ( !res_vals[1].empty()) { res << " url=\"" << res_vals[1] << "\""; }
	if ( !res_vals[2].empty()) { res << " version=\"" << res_vals[2] << "\""; }
	res << ">";
	string msg_str;
	size_t head_crlf = head.size() - 2;
	if (head_crlf > 0 && head.find(crlf,head_crlf) == head_crlf) {
		msg_str = head+crlf+body; 
	} else {
		msg_str = head+crlfcrlf+body; 
	}
	OsiMessage msg;
	msg.compile(msg_str,res,true);
	res << "</osi:request></osi:http>";
	the_result = res.str();
}

