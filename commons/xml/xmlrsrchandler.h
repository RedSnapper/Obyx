/*
 * xmlrsrchandler.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xmlrsrchandler.h is a part of Obyx - see http://www.obyx.org .
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

// xmlrsrchandler.h
#ifndef XML_RSRC_HANDLER_H
#define XML_RSRC_HANDLER_H

#include <map>
#include <string>
#include "commons/xml/xml.h"

namespace XML {
	
	using namespace xercesc;
	using namespace std;
	
	class GrammarRecord {
	private:
		friend class XMLResourceHandler;
		DOMLSInput*				inp; //
		MemBufInputSource*		mem;
		u_str					key; //namespaceUri for a schema or systemID for a DTD.
		std::string 			gra; //original grammar
//		XMLCh*					gky; //key as XMLCh;
		Grammar*				grx; //Xercesc Grammar object that was loaded.
		Grammar::GrammarType	typ; //
		GrammarRecord(const u_str&,const string&,Grammar::GrammarType);		
	public:
		~GrammarRecord();
	};
	
	typedef map<const u_str,GrammarRecord*> grammar_map_type;

	class XMLResourceHandler : public DOMLSResourceResolver {
	private:
		friend class Parser;
		grammar_map_type the_grammar_map;
			
	public:
		void getGrammar(string&,const string,bool);
		bool existsGrammar(const string,bool);
		void setGrammar(const string,const u_str&,Grammar::GrammarType type);
		DOMLSInput* resolveResource(const XMLCh* const,const XMLCh* const,const XMLCh* const,const XMLCh* const, const XMLCh* const);
		
		XMLResourceHandler();
		virtual ~XMLResourceHandler();
	};
	
}
#endif
