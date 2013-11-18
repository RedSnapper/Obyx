/*
 * xmlparser.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlparser.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef xmlparser_H
#define xmlparser_H

#include <string>
#include <vector>

#include "commons/xml/xml.h"

using namespace xercesc;

namespace XML {
	class Parser {

	private:
		friend class GrammarRecord;
		friend class Manager;
		friend class XMLResourceHandler;
		static const char*  SourceId;
		static const u_str memfile;

		static std::string  xmlxsd;
		static std::string  oalxsd;
		static std::string  messagexsd;
		static std::string  obyxxsd;
		static std::string  obyxdepxsd;		
		static std::string  soapxsd;
		static std::string  soapencodingxsd;
		static std::string  xlinkxsd;
		static std::string	xhtml1dtd;
		static std::string	xhtml1xsd;
		static std::string	svgxsd;
		static std::string	xhtml5xsd;
		static std::string	wsdlmimexsd;
		static std::string	wsdlxsd;
		static std::string	jsonxsd;
		

		XML::XMLErrorHandler*	errorHandler;
		DOMImplementation*		impl;
		DOMLSSerializer*		writer;
		XMLFormatTarget*        xfmt;
		DOMLSParser*			parser; //xercesc 3.0
		bool					srv_validation; //srv_validation default value.
		bool					validation; 	//instance value.
		void makeReader();
		void makeWriter();
		
		void do_mustbetext_following_gap(DOMNode*&,const u_str&);
		void do_mustbetext_preceding_gap(DOMNode*&,const u_str&);
		void do_mustbetext_set(DOMNode*&,const u_str&);

		void do_maybenode_child_gap(DOMDocument*&,DOMNode*&,const u_str&);
		void do_maybenode_preceding_gap(DOMDocument*&,DOMNode*&,const u_str&);
		void do_maybenode_following_gap(DOMDocument*&,DOMNode*&,const u_str&);
		void do_maybenode_set(DOMDocument*&,DOMNode*&,const u_str&);
		void do_attr_namespace_kludge(DOMDocument*&,DOMNode*&);

		void do_mustbetext_following_gap(DOMNode*&,DOMNode*);
		void do_mustbetext_preceding_gap(DOMNode*&,DOMNode*);
		void do_mustbetext_set(DOMNode*&, DOMNode*);
		void do_maybenode_child_gap(DOMDocument*&,DOMNode*&,const DOMNode*);
		void do_maybenode_preceding_gap(DOMDocument*&,DOMNode*&,const DOMNode*);
		void do_maybenode_following_gap(DOMDocument*&,DOMNode*&,const DOMNode*);
		void do_maybenode_set(DOMDocument*&,DOMNode*&,const DOMNode*);

	public:
		XML::XMLResourceHandler* resourceHandler;
		Parser();
		~Parser();
		void makerw();		
		void validation_set(bool,bool=false);
		void validation_on();
		void validation_off();
		void grammar_reading_on();
		void grammar_reading_off();
		void resetErrors() { errorHandler->resetErrors(); }
		bool hadErrors() { return errorHandler->hadErrors(); }
		void setGrammar(const std::string&,const u_str&,Grammar::GrammarType = Grammar::SchemaGrammarType);  //use the grammer inside the string.
		void getGrammar(std::string&,const std::string&,bool);  //use the grammer inside the string.
		bool existsGrammar(const std::string&,bool);  //use the grammer inside the string.
		DOMDocument* loadDoc(const std::string&);
		DOMDocument* loadDoc(const u_str&);
		DOMDocument* newDoc(const DOMNode*);
		DOMDocumentFragment* newDocFrag(DOMDocument*);
		u_str xpath(DOMNode* );
		void insertContext(DOMDocument*&,DOMNode*&,const u_str&,DOMLSParser::ActionType);
		void insertContext(DOMDocument*&,DOMNode*&,DOMNode* const,DOMLSParser::ActionType);
		bool loadURI(const std::string&, DOMDocument*&);
		void writenode(const DOMNode* const& ,std::string&);
		void writedoc(const DOMDocument* const& ,std::string&);
		void writenode(const DOMNode* const& ,u_str&);
		void writedoc(const DOMDocument* const& ,u_str&);
	};	
}


#endif
