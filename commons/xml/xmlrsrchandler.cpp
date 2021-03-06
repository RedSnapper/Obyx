/*
 * xmlrsrchandler.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlrsrchandler.cpp is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#include "commons/filing/filing.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"

using namespace Log;
using namespace xercesc;

namespace XML {
		
	GrammarRecord::GrammarRecord(const u_str& n,const u_str& s,const u_str& p,const string& f,Grammar::GrammarType t) : 
	inp(nullptr),mem(nullptr),key(n),gra(f),grx(nullptr),typ(t),use(false) {
		XMLByte* xmlraw = (XMLByte*)(gra.c_str());
		inp = ((DOMImplementationLS*)Manager::parser()->impl)->createLSInput();	
		mem = new MemBufInputSource(xmlraw,gra.size(),pcx(key.c_str()),false);
		mem->setCopyBufToStream(false);
		inp->setByteStream(mem);
		inp->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
		if (t == Grammar::DTDGrammarType) {
			inp->setPublicId(pcx(p.c_str()));
			inp->setSystemId(pcx(s.c_str()));
			mem->setPublicId(pcx(p.c_str()));
			mem->setSystemId(pcx(s.c_str()));
		}
	}
	GrammarRecord::~GrammarRecord() {
		if (grx != nullptr) {
			grx = nullptr; //don't delete it! it appears to be dealt with by grammarpool
		}
		if (inp != nullptr && !use) {
			inp->release(); 
			delete mem; 
		}
		mem=nullptr; inp=nullptr;
		key.clear();
		gra.clear();
	}
	XMLResourceHandler::XMLResourceHandler() : DOMLSResourceResolver(),the_grammar_map() {}
	XMLResourceHandler::~XMLResourceHandler() {
		for(grammar_map_type::iterator i = the_grammar_map.begin(); i != the_grammar_map.end(); i++) {
			if (!i->second.first && i->second.second != nullptr) {
				delete i->second.second; 
				i->second.second=nullptr;
			}
		}
		the_grammar_map.clear();
	}
	void XMLResourceHandler::installGrammar(const u_str& name) {	
		grammar_map_type::iterator it = the_grammar_map.find(name);
		if (it != the_grammar_map.end()) {
			GrammarRecord* grec =  it->second.second;
			if (! grec->used()) {
				bool tmp = Manager::parser()->errorHandler->getGrammar();
				Manager::parser()->errorHandler->setGrammar(true);
				grec->grx = Manager::parser()->parser->loadGrammar(grec->inp, grec->typ, true);
				Manager::parser()->errorHandler->setGrammar(tmp);
				if ( Manager::parser()->errorHandler->hadErrors() ) {
					string err_name; Manager::transcode(name.c_str(),err_name);
					*Logger::log << Log::error << Log::LI << "loading grammar:" << err_name << " with contents:" << Log::LO;
					*Logger::log << Log::LI << grec->gra << Log::LO;
					*Logger::log << Log::blockend;
					Manager::parser()->errorHandler->resetErrors();
				} else {
					grec->setused();
				}
			}
		}
	}
	void XMLResourceHandler::setGrammar(const string grammar,const u_str& name,Grammar::GrammarType type,bool loadit) {
		if ( !grammar.empty() ) {
			grammar_map_type::iterator it = the_grammar_map.find(name);
			if (it == the_grammar_map.end() ) {
				u_str sysIDstr,pubIDstr;
				if( type == Grammar::DTDGrammarType) { // Ideally, we would access the SystemID / PublicID here using grx.
					if ( String::Regex::available() ) { // && Logger::logging_available()
						size_t s = String::Regex::after("\\s+SYSTEM\\s*=?\\s*[\"']",grammar);
						if ( s == string::npos ) {
							string err_name; Manager::transcode(name.c_str(),err_name);
							if (Logger::log != nullptr) {
								*Logger::log << Log::error << Log::LI << "When loading the DTD:" << err_name << " there was no xml comment found " <<  Log::LO;
								*Logger::log << Log::LI << " at the beginning of the document indicating the SYSTEM ID." << Log::LO;
								*Logger::log << Log::blockend;
							} else {
								cerr << "No namespace xml comment found in " << err_name << " indicating the PUBLIC ID" ;
							}
						} else {
							string::size_type q = grammar.find(grammar[s-1],s); //Get the end of the trick.
							string sysIdDecl=grammar.substr(s,q-s);
							Manager::transcode(sysIdDecl,sysIDstr);
						}
						size_t p = String::Regex::after("\\s+PUBLIC\\s*=?\\s*[\"']",grammar);
						if ( p == string::npos ) {
							string err_name; Manager::transcode(name.c_str(),err_name);
							if (Logger::log != nullptr) {
								*Logger::log << Log::error << Log::LI << "When loading the DTD:" << err_name << " there was no xml comment found " <<  Log::LO;
								*Logger::log << Log::LI << " at the beginning of the document indicating the PUBLIC ID." << Log::LO;
								*Logger::log << Log::blockend;
							} else {
								cerr << "No namespace xml comment found in " << err_name << " indicating the PUBLIC ID" ;
							}
						} else {
							string::size_type q = grammar.find(grammar[p-1],p); //Get the end of the trick.
							string pubIdDecl=grammar.substr(p,q-p);
							Manager::transcode(pubIdDecl,pubIDstr);
						}
					} 
				}
				GrammarRecord* record = new GrammarRecord(name,sysIDstr,pubIDstr,grammar,type);
				if (loadit) {
					record->setused();
					Manager::parser()->grammar_reading_on();
					record->grx = Manager::parser()->parser->loadGrammar(record->inp, type, true); 
					Manager::parser()->grammar_reading_off();
					if ( Manager::parser()->errorHandler->hadErrors() ) {
						string err_name; Manager::transcode(name.c_str(),err_name);
						if (Logger::log != nullptr) {					
							*Logger::log << Log::error << Log::LI << "loading grammar:" << err_name << " with contents:" << Log::LO;
							*Logger::log << Log::LI << grammar << Log::LO;
							*Logger::log << Log::blockend;
						} else {
							cerr << "Errors were found while loading Grammar " << err_name;							
						}
						Manager::parser()->errorHandler->resetErrors();
					} else {
						the_grammar_map.insert(grammar_map_type::value_type(name,gmap_entry_type(false,record)));
						if( type == Grammar::DTDGrammarType) { 
							the_grammar_map.insert(grammar_map_type::value_type(sysIDstr,gmap_entry_type(true,record)));
						}
					}
				} else {
					the_grammar_map.insert(grammar_map_type::value_type(name,gmap_entry_type(false,record)));
					if( type == Grammar::DTDGrammarType) { 
						the_grammar_map.insert(grammar_map_type::value_type(sysIDstr,gmap_entry_type(true,record)));
					}
				}
			} 
		}
	}
	void XMLResourceHandler::getGrammar(string& grammarfile,const string name,bool release) {
		if (!name.empty()) {
			u_str gname; Manager::transcode(name,gname);
			grammar_map_type::iterator it = the_grammar_map.find(gname);
			if (it != the_grammar_map.end() ) {
				GrammarRecord* gr =  it->second.second;
				grammarfile = gr->gra;
				if (release ) {
					if (!it->second.first) {
						delete it->second.second;
						it->second.second= nullptr;	
						//					type == Grammar::DTDGrammarType
					}
					the_grammar_map.erase(it);
				}
			} 
		}
	}
	bool XMLResourceHandler::existsGrammar(const string name,bool release) {
		bool retval = false;
		if (!name.empty()) {
			u_str gname; Manager::transcode(name,gname);
			grammar_map_type::iterator it = the_grammar_map.find(gname);
			if (it != the_grammar_map.end()) {
				retval = true;
				if (release) {
					if (!it->second.first) {
						delete it->second.second;
						it->second.second= nullptr;	
						//					type == Grammar::DTDGrammarType
					}
					the_grammar_map.erase(it);
				}
			}
		}
		return retval;
	}
 
	//With DTD, we must bind our namespace to the publicId.
	//This means we must extract the publicId during grammar load.
	DOMLSInput* XMLResourceHandler::resolveResource(
		const XMLCh* const,  //resourceType
		const XMLCh* const namespaceUri , 
		const XMLCh* const publicId, //This IS differentiated on a document load.
		const XMLCh* const systemId, 
		const XMLCh* const ) { //
		DOMLSInput* retval = nullptr;
		const XMLCh* grammarkey = nullptr;
		grammar_map_type::iterator it = the_grammar_map.end();
		if (publicId != nullptr && XMLString::stringLen(publicId) != 0) {
			it = the_grammar_map.find(pcu(publicId)); //Pick up eg XHTML1.0
		}
		if (it == the_grammar_map.end()) {
			if ( namespaceUri != nullptr ) {
				grammarkey = namespaceUri;
			} 
			if ( grammarkey == nullptr || XMLString::stringLen(grammarkey) == 0 ) { // test for length!
				if (systemId != nullptr) { grammarkey = systemId; }
			}
			it = the_grammar_map.find(pcu(grammarkey));
		}
		if (it == the_grammar_map.end()) {
			string a_nsu="-",a_pid="-",a_sid="-";
			if (namespaceUri != nullptr) Manager::transcode(pcu(namespaceUri),a_nsu);
			if (publicId != nullptr ) Manager::transcode(pcu(publicId),a_pid);  //Currently we only bind on publicID.
			if (systemId != nullptr ) Manager::transcode(pcu(systemId),a_sid);
//			if (Logger::log != nullptr) {
				*Logger::log << Log::error << Log::LI << "Remote Grammar Required. Ensure that you have a local grammar file for this." << Log::LO;
				*Logger::log << Log::LI << "Grammar details follow:" << Log::LO; 
				*Logger::log << Log::LI << a_nsu << Log::LO << Log::LI << a_pid << Log::LO << Log::LI << a_sid << Log::LO; 
				*Logger::log << Log::LO << Log::blockend;
//			} else {
//				cerr << "Remote Grammar Required. Grammar details: " << a_nsu << " " << a_pid << " " << a_sid ;
//			}
		} else {
			GrammarRecord* grec =  it->second.second;
			if (! grec->used()) {
				grec->setused();
				retval = grec->inp;
			}
		}
		return retval; // DOMLSInput* 
	}
}
