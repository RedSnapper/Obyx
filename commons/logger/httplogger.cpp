/* 
 * httplogger.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * httplogger.cpp is a part of Obyx - see http://www.obyx.org .
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

#include <iostream>
#include "commons/httphead/httphead.h"
#include "commons/string/strings.h"
#include "core/obyxelement.h"
#include "httplogger.h"

bool HTTPLogger::minititle = true;

void HTTPLogger::dofatal(std::string message) {
	Httphead* http = Httphead::service();
	if (! http->done() ) {
		http->addcustom("Obyx-Status","Fatal");
		http->addcustom("Obyx-Signature",title.c_str());
		http->addcustom("Obyx-File",path.c_str());
		http->addcustom("Obyx-Breakpoint",ObyxElement::breakpoint_str());
		http->addcustom("Obyx-Message",message);
		if ( logging_on	) {	
			http->setcode(200);	
		} else {
			http->setcode(500);	//There may be a good reason for not using 500 here, but I cannot remember it.
		}
		http->setmime("text/html; charset=utf-8");
		http->setdisposition("");
		http->doheader(); 
	}
	if (!topped) {
		topped=true;
		string top_str;
		if (should_report()) {
			top(top_str,true);
		} else {
			top(top_str,false);
		}
		*fo << top_str;
	}
}

void HTTPLogger::strip(string& basis) { 
	if ( ! basis.empty() && String::Regex::available() ) {
		String::Regex::replace("<!DOCTYPE([^>]+)>","<!--[DOCTYPE \\1]-->",basis);
		String::Regex::replace("<\\?xml([^>]+)\\?>","<!--[xml \\1]-->",basis);
		String::Regex::replace("<html([^>]+)>","<!--[html \\1]-->",basis);
		String::Regex::replace("<title([^>]+)>","<!--[title \\1]-->",basis);
		String::Regex::replace("<meta([^>]+)>","<!--[meta \\1]-->",basis);
		String::Regex::replace("<link([^>]+)>","<!--[link \\1]-->",basis);
		String::Regex::replace("<body([^>]+)>","<!--[body \\1]-->",basis);
	} 	
	String::fandr(basis,"</title>","<!--/title--></div>");
	String::fandr(basis,"</head>","<!--/head--></div>");
	String::fandr(basis,"</body>","<!--/body--></div>");
	String::fandr(basis,"</html>","<!--/html--></div>");
}

//top-tail are used to compose external log-reports as well.
void HTTPLogger::ltop(string& container,bool do_bits) {		//top log document
	container.clear();
	container.append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
	container.append("<html xmlns=\"http://www.w3.org/1999/xhtml\" ><head><title>");
	container.append(title);
	container.append("</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
	if (do_bits) {
		std::string logjs="<script type=\"text/javascript\" charset=\"utf-8\" >"
		"/* <![CDATA[ */\n"
		"function f(n,b){n=n.nextSibling;while(n!=null){if(n.nodeType==1&&n.nodeName.toLowerCase()=='li'){n.style.display=b;}n=n.nextSibling;}}\n"
		"function s(n){while(n!=null){if(n.nodeType==1){if(n.nodeName.toLowerCase()=='li'){if(n.firstChild!=null&&n.firstChild.nodeValue!=null){switch(n.firstChild.nodeValue.substring(0,1)){case'▶':f(n,'none');break;default:s(n.firstChild);break;}}}s(n.firstChild);}n=n.nextSibling;}}\n"
		"function d(n,a,b){n.firstChild.nodeValue=a+n.firstChild.nodeValue.substring(1);f(n,b);}\n"
		"function sh(n){x=n.firstChild.nodeValue.substring(0,1);switch(x){case'▼':d(n,'▶','none');break;case'▶':d(n,'▼','block');break;}}\n"		"/* ]]>*/\n"
		"</script>\n";
		std::string logstyle="<style type=\"text/css\">\n"
		"/* <![CDATA[ */\n"
		"ol,li,div {font-family: Gill Sans, Arial; font-size: 1em; display: block; padding:0; margin:0; padding-left:0.25em; }\n"
		"a {text-decoration: none;}\n"
		"a:hover { background-color: #FFF2B5; }\n"
		"li {display:block;}\n"
		"span {padding-right:2em;}\n"
		"span + span {padding-right:1em;}\n"
		".notify {font-weight:normal; color: #3F3F6F; background-color: #DDDDDD;}\n"
		".info {font-weight:normal; color:#00008B; background-color: #FFF;}\n"
		".redirect {font-weight:normal; color: #A6A6A6; background-color: #F2F2F2;}\n"
		".error   {color: #441100; background-color: #FFDDDD; border-color: #F80000; border-style: solid; border-width: 1px; padding-bottom:0.25em; margin-top: 2px; margin-bottom: 2px;}\n"
		".syntax  {color: #331100; background-color: #FFAAAA; border-color: #F80000; border-style: solid; border-width: 1px; padding-bottom:0.25em; margin-top: 2px; margin-bottom: 2px;}\n"
		".warning {color: #441100; background-color: #FFEEDD; border-color: #F87700; border-style: solid; border-width: 1px; padding-bottom:0.25em; margin-top: 2px; margin-bottom: 2px; }\n"
		".subhead  {font-weight: bold; color: #4F4F4F; background-color: #DDDDDD; padding-top:0.25em; padding-bottom:0.25em; }\n"
		".headline {font-family: Gill Sans, Arial; font-size: 1.1em; font-weight: bold; color: #000000; background-color: #FCFCFC; border-color:#CFCFCF; border-style: solid; border-width: 1px; }\n"
		".timing {color: #001199; background-color: #CCCCFF;}\n"
		".eo {font-weight:normal;}\n"
		".even {font-weight:normal; color:#1C1C1C; background-color: #F8F8F8;}\n"
		".odd {font-weight:normal; color:#4F4F4F; background-color: #FFFFFF;}\n"
		"li.xpath, li.filepath, li.breakpoint {font-weight:normal; color:#1F1F1F; background-color: #DDDDDD;}\n"
		"/* ]]>*/\n"
		"</style>";	
		container.append(logjs);
		container.append(logstyle);
		container.append("</head><body onload=\"s(document.body);\">");
	} else { 
		container.append("</head><body>");
	}
	if (!should_report()) {
		container.append("<h1>An internal error has occured.</h1>");
		if(syslogging) {
			container.append("<h3>We have been notified and will be fixing it as soon as possible.</h3>");
		}
	} else {
		container.append("<div class=\"headline\">");
		container.append(title);
		container.append("</div><div>");
	}
}

//top-tail are used to compose external log-reports as well.
void HTTPLogger::ltail(string& container) {		//tail log document
	container.clear();
	if (!should_report()) {
		container.append("</body></html>\n"); 
	} else {
		container = "</div><div class=\"headline\">";
		container.append(title);
		container.append("</div></body></html>\n"); 
	}	
}

void HTTPLogger::open() {	//This should always be called ..
	minititle = true;
	if ((debugflag || hadfatal) && !topped ) {
		string top_str;
		top(top_str,true);
		if (debugflag) {
			Httphead::init(fo);
			*o << top_str;
			*o << "<ol class=\"subhead\"><li>Debug is On</li></ol>";
		} else {
			*o << top_str;
		}
		topped=true;
	}
}

void HTTPLogger::close() { 
	if (topped && !tailed) {
		tailed=true;
		string tail_str;
		tail(tail_str);
		*o << tail_str;
	}
	minititle = false;
}

void HTTPLogger::extra(extratype t) {
//	if ( !logging_available() ) return;
	switch (t) {
		case br:  { 
			*o << "<br />";
		} break;
		case urli:  { 
			*o << "<a onclick=\"window.open(this.href,'_log'); return false;\"";
			switch ( type_stack.top()  ) {
				case Log::headline: { *o << " class=\"headline\""; } break;
				case Log::subhead: { *o << " class=\"subhead\""; } break;
				case Log::even: { 
					if ( evenodd ) 
						*o << " class=\"even\""; 
					else
						*o << " class=\"odd\""; 
				} break;
				case Log::warn: { *o << " class=\"warning\""; } break;
				case Log::timing: { *o << " class=\"timing\""; } break;  
//				case Log::debug: 
				case Log::info:  { *o << " class=\"info\""; } break;
				case Log::fatal: { *o << " class=\"error\""; } break;
				case Log::error: { *o << " class=\"error\""; } break;
				case Log::syntax: { *o << " class=\"syntax\""; } break;
				case Log::notify: { *o << " class=\"notify\""; } break;
				default: break;
			} 
			*o << " href=\"";
		} break;
		case urlt:  { 
			*o << "\">";
		} break;
		case urlo:  { 
			*o << "</a>";
		} break;
		case rule:  { 
			*o << "<hr />";
		} break;
	}
}					 

void HTTPLogger::bracket(bracketing bkt) {
//	if ( !logging_available() ) return;
	if (type_stack.empty() ) { 
		*o << "<ol class=\"error\"><li>Brackets should only occur inside message blocks</li></ol>";
	}
	switch (bkt) {
		case LI: {
			*o << "<li ";
			if (minititle) {
				*o << "onclick=\"sh(this);\" "; 
			}
			if (type_stack.top() == even) {
				if ( evenodd ) {
					*o << "class=\"even\" ";
				} else {
					*o << "class=\"odd\" ";
				}
				evenodd = ! evenodd;
			} 
			if (minititle) {
				if (type() == Log::fatal || type() == Log::error || type() == Log::warn) {
					unsigned long long int bp = ObyxElement::breakpoint(); 
					if ( bp != 0 ) {
						*o << ">▶ (" << bp << ") ";
					} else {
						*o << ">▶ ";
					}
				} else {
					*o << ">▶ ";
				}
				minititle=false;
			} else {
				*o << ">";
			}
		} break;
		case LIXP: {
			*o << "<span class=\"xpath\" >";
		} break;
		case LIFP: {
			*o << "<span class=\"filepath\" >";
		} break;
		case LIPP: {
			*o << "<span class=\"breakpoint\" >";
		} break;
		case LOXP:
		case LOFP:
		case LOPP: {
			*o << "</span>";
		} break;
		case LO: *o << "</li>";break;
		case II: { *o << "<span>"; }  break;
		case IO: { *o << "</span>"; } break;
		case RI: 
		case RO:
		default: break;
	}
}

void HTTPLogger::wrap(bool io) {
//	if ( !logging_available() ) return;
	if (io) {
		if (type_stack.size() == 2) {
			minititle = true;
		}
		switch ( type_stack.top() ) {
			case logger: break;	
			case timing:  { 
				*o << "<ol class=\"timing\" >";
			} break;
			case redirect:  { 
				*o << "<ol class=\"redirect\" >";
			} break;
			case headline:  { 
				*o << "<ol class=\"headline\" >";
				minititle = true;
			} break;
			case subhead:  { 
				*o << "<ol class=\"subhead\" >";
				minititle = true;
			} break;
			case thrown:
//			case debug: 
			case info:  { 
				*o << "<ol class=\"info\" >";
			} break;
			case even:  { 
				*o << "<ol class=\"eo\" >";
			} break;
			case notify: { 
				*o << "<ol class=\"notify\" >";
			} break;			
			case fatal:  
			case Log::error: { 
				*o << "<ol class=\"error\" >";
			} break;
			case syntax: { 
				*o << "<ol class=\"syntax\" >";
			} break;
			case warn:  {
				*o << "<ol class=\"warning\" >";
			} break;
			case blockend: break;
		}
	} else {
		*o << "</ol>" ;  
	}
}					 
