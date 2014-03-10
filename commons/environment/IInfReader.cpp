/* 
 * IIinfxxx edited by Ben Griffin
 * This file is derived from ImageInfo written by Alexandre Klaey
 * ImageInfo is derived from ImageInfo.java 1.3
 *
 * ImageInfo.java is written and maintained by Marco Schmidt <marcoschmidt@users.sourceforge.net>
 * C++ version of ImageInfo written and maintained by Alexandre Klaey <alkla80@hotmail.com>
 *
 * This derivation is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
 * The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt
 * Support for SWF written by Michael Aird.
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

#include "IInfGlobal.h"

using namespace ImageInfo;
using namespace std;

// Constructors

/** Construct a reader with the given file name as input stream. */
Reader::Reader( istream& ist ) : input(ist)  {
    bitPos = 0;
    bitBuf = 0;
}

/** Read an unsigned value from the given number of bits */
long Reader::readUBits( int numBits ) throw ( exception ) {
  if ( numBits == 0 ) return 0;
  int bitsLeft = numBits;
  long result  = 0;
  if ( bitPos == 0 ) { // no value in the buffer - read a byte
    bitBuf = readOne();
    bitPos = 8;
  }

  while( true ) {
    int shift = bitsLeft - bitPos;
    if ( shift > 0 ) {
      // Consume the entire buffer
      result |= bitBuf << shift;
      bitsLeft -= bitPos;

      // Get the next byte from the input stream
      bitBuf = readOne();
      bitPos = 8;
    } else {
      // Consume a portion of the buffer
      result |= bitBuf >> -shift;
      bitPos -= bitsLeft;
      bitBuf &= 0xff >> ( 8 - bitPos ); // mask off the consumed bits
      return result;
    }
  }
}

/** Read a signed value from the given number of bits */
int Reader::readSBits( int numBits ) throw ( exception ) {
  // Get the number as an unsigned value.
  long uBits = readUBits( numBits );
	if( numBits > 0 ) { 
  // Is the number negative?
	  if( ( uBits & ( 1L << (numBits - 1 ) ) ) != 0 ) {
		// Yes. Extend the sign.
		uBits |= -1L << numBits;
	  }
	}
  return static_cast< int >( uBits );
}  
   
/** Reads the next char or trait::eof() from the stream */
int Reader::readOne() throw ( exception ) {
  if ( input.fail() )
    throw exception();
  else return input.get();
}

/** Skips offset chars,
 * reads num chars from the stream and
 * put them into the array a
 * CAUTION: order of the parameters 2 and 3 changed from ImageIno.java */
bool Reader::read( char* a, streamsize num, int offset ) throw ( exception ) {
  if ( input.fail() )
    throw exception();
  else {
    input.ignore( offset );
    input.read( a, num );
    return ! input.fail();
  }
}

/** Reads a whole line from the stream and returns it as a string. */
string Reader::readLine() throw ( exception ) {
  char aChar=' ';
  string line;
  if ( input.fail() )
    throw exception();
  while ( true ) {
    input.get( aChar );
    if ( input.eof() || aChar == '\n' ) return line;
    if ( input.fail() )
      throw exception();
    line += aChar;
  }
}

/** Jumps num chars */
void Reader::skip( unsigned int num ) throw ( exception ) {
  if ( input.fail() )
    throw exception();
  else input.seekg( num , std::ios::cur );
}

/** Go back of num chars  */ 
void Reader::goBack() {
	input.unget();
}
