/* 
 * clilogger.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * clilogger.cpp is a part of Obyx - see http://www.obyx.org .
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

#include "clilogger.h"

void CLILogger::open() {
	if (! isopened) {
		logging_on = true;
		size_t padsize = (80 - 12 - title.length()) / 2;
		string pad; pad.assign(padsize, '-');
		*o << "\n" << pad << "BEGIN " << title << " BEGIN" << pad << "\n";
		isopened = true;
	}
}

void CLILogger::close() {
	if (isopened && logging_on) {
		size_t padsize = ((80 - 8) - title.length()) / 2;
		string pad; pad.assign(padsize, '-');
		*o << "\n" << pad << "END " << title << " END" << pad << "\n" << flush;
	}
}

void CLILogger::extra(extratype t) {
	switch (t) {
		case br:  { 
			*o << "\n";
		} break;
		case rule:  {
			*o << "\n================================================================================\n";
		} break;
		case urli:  {
			*o << "[";  
		} break;
		case urlt:  {
			*o << "][";  
		} break;
		case urlo:  {
			*o << "]";  
		} break;
	}
}

void CLILogger::bracket(bracketing bkt) {
	switch (bkt) {
		case LI:
		case LIFP:
		case LIPP:
		case LIXP: {
			string margin; 
			margin.assign( type_stack.size() - 1,'.');
			*o << "|" << margin;
			switch ( type_stack.top()  ) {
//				case debug: { 
//					*o << "D:";
//				} break;
				case thrown:
				case info: { 
					*o << "+:";
				} break;
				case headline: { 
					*o << "H:";
				} break;  
				case notify: { 
					*o << "N:";
				} break;  
				case subhead: { 
					*o << "+:";
				} break;  
				case error: { 
					*o << "E:";
				} break;
				case syntax: { 
					*o << "S:";
				} break;
				case warn:  {
					*o << "W:";  
				} break;
				case fatal: {
					*o << "F:";  
				} break;
				case timing: {
					*o << "T:";  
				} break;
				case even: {
					if ( evenodd ) {
						*o << "*:";
					} else {
						*o << "-:";
					}
					evenodd = ! evenodd;
				} break;
				case redirect: {
					*o << "R:";  
				} break;
				case blockend: break;
				case logger: break;
			}
		} break;
		case LOFP:
		case LOPP:
		case LOXP:
		case RO:  
		case LO: { 
			*o << "\n";
		} break;
		case RI: 
		  break;  			
		case II: { 
			*o << "[";
		} break;  			
		case IO: { 
			*o << "]";
		} break;
			
	}
}

//this is called after the push and before the pop - 
void CLILogger::wrap(bool io) {
	if (!isopened) {
		open();
	}
	if ( io ) {
		switch ( type_stack.top() ) {
//			case raw: break;
			case thrown:  { 
				*o <<			"+----------------------------------  THROWN -------------------------------------+\n";
			} break;
			case timing:  { 
				*o <<			"+----------------------------------  TIMING -------------------------------------+\n";
			} break;
			case redirect:  { 
				*o <<			"+----------------------------------- REDIRECT -----------------------------------+\n";
			} break;
			case even:  { 
				evenodd = true;
				*o <<			"+----------------------------------- EVEN ---------------------------------------+\n";
			} break;
			case notify:  { 
				*o <<			"+----------------------------------- NOTIFY -------------------------------------+\n";
			} break;
//			case debug:  { 
//				*o <<			"+----------------------------------- DEBUG --------------------------------------+\n";
//			} break;
			case info:  { 
				*o <<			"+----------------------------------- INFO ---------------------------------------+\n";
			} break;
			case headline:  { 
				*o <<			"+----------------------------------- HEADLINE -----------------------------------+\n";
			} break;
			case subhead:  { 
				*o <<			"+----------------------------------- SUBHEAD ------------------------------------+\n";
			} break;
			case fatal:  { 
				*o <<			"+----------------------------------- FATAL --------------------------------------+\n";
			} break;
			case error:  { 
				*o <<			"+----------------------------------- ERROR --------------------------------------+\n";
			} break;
			case syntax:  { 
				*o <<			"+----------------------------------- SYNTAX -------------------------------------+\n";
			} break;
			case warn:  { 
				*o <<			"+----------------------------------- WARN ---------------------------------------+\n";
			} break;
			case blockend:
			case logger: break;
		}
	} else { 
		switch ( type_stack.top() ) {
//			case raw: break;
			case thrown:  { 
				*o <<			"+------------------------------- FIN THROWN -------------------------------------+\n";
			} break;
			case timing:  { 
				*o <<			"+------------------------------- FIN TIMING -------------------------------------+\n";
			} break;
			case redirect:  { 
				*o <<			"+------------------------------- FIN REDIRECT -----------------------------------+\n";
			} break;
			case even:  { 
				*o <<			"+------------------------------- FIN EVEN ---------------------------------------+\n";
			} break;
			case notify:  { 
				*o <<			"+------------------------------- FIN NOTIFY -------------------------------------+\n";
			} break;
//			case debug:  { 
//				*o <<			"+------------------------------- FIN DEBUG --------------------------------------+\n";
//			} break;
			case info:  { 
				*o <<			"+------------------------------- FIN INFO ---------------------------------------+\n";
			} break;
			case headline:  { 
				*o <<			"+------------------------------- FIN HEADLINE -----------------------------------+\n";
			} break;
			case subhead:  { 
				*o <<			"+------------------------------- FIN SUBHEAD ------------------------------------+\n";
			} break;
			case fatal:  { 
				*o <<			"+------------------------------- FIN FATAL --------------------------------------+\n";
			} break;
			case error:  { 
				*o <<			"+------------------------------- FIN ERROR --------------------------------------+\n";
			} break;
			case syntax:  { 
				*o <<			"+------------------------------- FIN SYNTAX -------------------------------------+\n";
			} break;
			case warn:  { 
				*o <<			"+------------------------------- FIN WARN ---------------------------------------+\n";
			} break;
			case blockend:
			case logger: break;
		}
	}
}					 
