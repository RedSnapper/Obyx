/*
 * xmlrsrchandler.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlrsrchandler.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "commons/filing/filing.h"
#include "commons/logger/logger.h"
#include "commons/xml/xml.h"
#include "commons/environment/environment.h"

using namespace Log;
using namespace xercesc;

namespace XML {

	GrammarRecord::GrammarRecord(const u_str& n,const string& f,Grammar::GrammarType t) : 
		inp(NULL),mem(NULL),key(n),gra(f),grx(NULL),typ(t) {
		XMLByte* xmlraw = (XMLByte*)(gra.c_str());
		inp = ((DOMImplementationLS*)Manager::parser()->impl)->createLSInput();	
		mem = new MemBufInputSource(xmlraw,gra.size(),key.c_str());
		mem->setCopyBufToStream(false);
		inp->setByteStream(mem);
		inp->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
	}
	
	GrammarRecord::~GrammarRecord() {
		inp->release();
		delete mem;
		key.clear();
		gra.clear();
	}
	
	XMLResourceHandler::XMLResourceHandler() : DOMLSResourceResolver(),the_grammar_map() {}
	XMLResourceHandler::~XMLResourceHandler() {
		the_grammar_map.clear();
	}
	
	void XMLResourceHandler::setGrammar(const string grammar,const u_str& name,Grammar::GrammarType type) {
		if ( !grammar.empty() ) {
			grammar_map_type::iterator it = the_grammar_map.find(name);
			if (it == the_grammar_map.end() ) {
				GrammarRecord* record = new GrammarRecord(name,grammar,type);
				record->grx = Manager::parser()->parser->loadGrammar(record->inp, type, true); 
				if ( Manager::parser()->errorHandler->hadErrors() ) {
					string err_name; transcode(name.c_str(),err_name);
					*Logger::log << Log::error << Log::LI << "loading grammar:" << err_name << " with contents:" << Log::LO;
					*Logger::log << Log::LI << grammar << Log::LO;
					*Logger::log << Log::blockend;
					Manager::parser()->errorHandler->resetErrors();
				} else {
					pair<grammar_map_type::iterator, bool> ins = the_grammar_map.insert(grammar_map_type::value_type(name,record));
					if (record->grx != NULL) {
						if( type == Grammar::DTDGrammarType) { // Ideally, we would access the SystemID / PublicID here using grx.
							if ( String::Regex::available() ) {
								size_t p = String::Regex::after("\\s+PUBLIC\\s*=?\\s*[\"']",grammar);
								if ( p == string::npos ) {
									string err_name; transcode(name.c_str(),err_name);
									*Logger::log << Log::error << Log::LI << "When loading the DTD:" << err_name << " there was no xml comment found " <<  Log::LO;
									*Logger::log << Log::LI << " at the beginning of the document indicating the PUBLIC ID." << Log::LO;
									*Logger::log << Log::blockend;
								} else {
									string::size_type q = grammar.find(grammar[p-1],p); //Get the end of the trick.
									string pubIdDecl=grammar.substr(p,q-p);
									u_str publicIDstr; transcode(pubIdDecl,publicIDstr);
									pair<grammar_map_type::iterator, bool> ins = the_grammar_map.insert(grammar_map_type::value_type(publicIDstr,record));
								}
							} 
						}
					}
				}
			} 
		}
	}

	void XMLResourceHandler::getGrammar(string& grammarfile,const string name,bool release) {
		if (!name.empty()) {
			u_str gname; transcode(name,gname);
			grammar_map_type::iterator it = the_grammar_map.find(gname);
			if (it != the_grammar_map.end() ) {
				GrammarRecord* gr =  it->second;
				grammarfile = gr->gra;
				if (release) {
					delete gr; gr= NULL;
					the_grammar_map.erase(it);
				}
			} 
		}
	}
	
	bool XMLResourceHandler::existsGrammar(const string name,bool release) {
		bool retval = false;
		if (!name.empty()) {
			u_str gname; transcode(name,gname);
			grammar_map_type::iterator it = the_grammar_map.find(gname);
			if (it != the_grammar_map.end()) {
				retval = true;
				if (release) {
					GrammarRecord* gr =  it->second;
					delete gr; gr= NULL;				
					the_grammar_map.erase(it);
				}
			}
		}
		return retval;
	}
	
//	XMLEntityResolver
	
	//With DTD, we must bind our namespace to the publicId.
	//This means we must extract the publicId during grammar load.
	DOMLSInput* XMLResourceHandler::resolveResource(
			const XMLCh* const,  //resourceType
			const XMLCh* const namespaceUri , 
			const XMLCh* const publicId, 
			const XMLCh* const systemId, 
			const XMLCh* const /*baseURI*/ ) { //
		DOMLSInput* retval=NULL;
		const XMLCh* grammarkey = NULL;
		if ( namespaceUri != NULL ) {
			grammarkey = namespaceUri;
		} 
		if ( grammarkey == NULL || XMLString::stringLen(grammarkey) == 0 ) { // test for length!
			if (publicId != NULL) { grammarkey = publicId; }
		}
		if ( grammarkey == NULL || XMLString::stringLen(grammarkey) == 0 ) { // test for length!
			if (systemId != NULL) { grammarkey = systemId; }
		}
		grammar_map_type::iterator it = the_grammar_map.find(grammarkey);
		if (it != the_grammar_map.end()) {
			retval = NULL;
/*			
			GrammarRecord* grec =  it->second;
			string r_gra = grec->gra;
			XMLByte* xmlraw = (XMLByte*)(r_gra.c_str());
			retval = ((DOMImplementationLS*)Manager::parser()->impl)->createLSInput();	
			MemBufInputSource* mem = new MemBufInputSource(xmlraw,r_gra.size(),grammarkey);
			mem->setCopyBufToStream(false);
			retval->setByteStream(mem);
			retval->setEncoding(XMLUni::fgUTF8EncodingString); //This must be done.
 */
		} else {
			string a_nsu="-",a_pid="-",a_sid="-";
			if (namespaceUri != NULL) transcode(namespaceUri,a_nsu); 
			if (publicId != NULL ) transcode(publicId,a_pid);  //Currently we only bind on publicID.
			if (systemId != NULL ) transcode(systemId,a_sid);
			*Logger::log << Log::error << Log::LI << "Remote Grammar Required. Ensure that you have a local grammar file for this." << Log::LO;
			*Logger::log << Log::LI << Log::subhead << Log::LI << "Grammar details follow:" << Log::LO; 
			*Logger::log << Log::LI << a_nsu << Log::LO << Log::LI << a_pid << Log::LO << Log::LI << a_sid << Log::LO; 
			*Logger::log << Log::blockend << Log::LO << Log::blockend;
		 }
		return retval;
	}

}
