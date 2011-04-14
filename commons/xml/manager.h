/*
 * xercesmanager.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * xercesmanager.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_XMLXERCESMGR_H 
#define OBYX_XMLXERCESMGR_H

#include <string>
#include <istream>
#include <ostream>

#include "commons/xml/xml.h"
#include "commons/string/strings.h"

namespace XML {
	
	class XercesInitExn { };
	
	class Manager {
	private:
		typedef hash_map<const std::string,u_str, hash<const string&> > su_map_type;
		typedef hash_map<const u_str,std::string, hash<const u_str&> > us_map_type;

		friend class XMLObject;

		static su_map_type tsu;	//cache transcodes.
		static us_map_type tus;	//cache transcodes.
	    static std::ostream*			serial_ostream;
		
	public:
		static XML::Parser*				xparser;
		
		static void append(u_str&,const u_str&);
		static void transcode(const u_str&, std::string&);
		static void transcode(const std::string&,u_str&,const std::string = "UTF-8");
		static void to_ustr(const long,u_str&);

		static bool attribute(const DOMNode*,const std::string,std::string&);
		static bool attribute(const DOMNode*,const u_str,u_str&);
		static bool attribute(const DOMNode*,const u_str);
		static XML::Parser* parser() { return xparser; }
		static void resetDocPool() { xparser->parser->resetDocumentPool(); }
		static bool hadErrors();
		
		Manager();
		~Manager();
	};
	
	
} // namespace XML

#endif // xercesmanager_H
