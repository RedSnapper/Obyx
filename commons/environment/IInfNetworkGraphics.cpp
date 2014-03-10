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
using std::string;
using std::exception;

#if ( ! NO_PNG | ! NO_MNG | ! NO_JNG )

/** Deletion of input is under the caller's responsablity */
NetworkGraphics::NetworkGraphics( Reader* const input ) : ImageBase( input ) {
  supportComments = true;
}

// Protected static fields

char NetworkGraphics::MAGIC[5] = {0x47, 0x0d, 0x0a, 0x1a, 0x0a};
char NetworkGraphics::HDR[3]   = {0x48, 0x44, 0x52};       // (subpart of the) header
char NetworkGraphics::IDAT[4]  = {0x49, 0x44, 0x41, 0x54}; // image data
char NetworkGraphics::pHYs[4]  = {0x70, 0x48, 0x59, 0x73}; // png physical
char NetworkGraphics::pHYg[4]  = {0x70, 0x48, 0x59, 0x67}; // global physical
char NetworkGraphics::IEND[4]  = {0x49, 0x45, 0x4e, 0x44}; // png ending
char NetworkGraphics::MEND[4]  = {0x4D, 0x45, 0x4e, 0x44}; // mng ending


// Private static fields

char NetworkGraphics::tEXt[4] = {0x74, 0x45, 0x58, 0x74}; // plain text
char NetworkGraphics::zTXt[4] = {0x7a, 0x54, 0x58, 0x74}; // compressed text
char NetworkGraphics::iTXt[4] = {0x69, 0x54, 0x58, 0x74}; // UTF-8 text

unsigned int NetworkGraphics::handleComments( unsigned int length, char type_a[4] ) {

  if ( ! collectComments ) return 0;
  else {

    bool plain_text = equals( type_a, 0, tEXt, 0, 4 );
    bool compr_text = equals( type_a, 0, zTXt, 0, 4 );
    bool UTF_8_text = equals( type_a, 0, iTXt, 0, 4 );

    if ( plain_text || compr_text || UTF_8_text ) {
      // text chunk
      bool compressed = false;
      unsigned int byteCount = 0;
      string keyword;
      string text;

      // extract the keyword
      while ( byteCount < length ) {
        char c = in->readOne();
		byteCount++;
        if ( c == 0x00 ) break;
        keyword += static_cast< char >( c );
      }

      if ( keyword != "Comment" || byteCount >= length ) return byteCount;

      if ( compr_text ) {
        compressed = true;
        byteCount++;
        if ( in->readOne() != 0x00 ) {
          // Invalid compression method. Only "deflate/inflate" is allowed.
          return byteCount;
        }
      } else if ( UTF_8_text ) {
        int compr_flag = in->readOne();
        byteCount++;
        if ( compr_flag == 0x00 ) {
          // Not compressed
          compressed = false;
        } else if ( compr_flag == 0x01 ) {
          // Compressed
          compressed = true;
        } else {
          // Invalid compression flag
          return byteCount;
        }
        byteCount++;
        if ( in->readOne() != 0x00 && compressed ) {
          // Invalid compression method. Only "deflate/inflate" is allowed.
          return byteCount;
        }
        if ( byteCount >= length ) return byteCount;

        // extract the language
        while ( byteCount < length ) {
          char c = in->readOne();
          byteCount++;
          if ( c == 0x00 ) break;
        }
        if ( byteCount >= length ) return byteCount;

        // skip the translated keyword
        while ( byteCount < length ) {
          byteCount++;
          if ( in->readOne() == 0x00 ) break;
        }
        if ( byteCount >= length ) return byteCount;
      }

      // extract the text
      while ( byteCount < length ) {
        byteCount++;
        text += static_cast< char >( in->readOne() );
      }

      if ( compressed ) text = uncompress( text );
      comments.push_back( text );

      return length; // all the data field has been consumed
    }
  }
  return 0;
}

string NetworkGraphics::uncompress(const string) {
// TODO
  return "*** compressed ***";
}

#endif // ( ! NO_PNG | ! NO_MNG | ! NO_JNG )


#ifndef NO_PNG

// Constructor

/** Deletion of input is under the caller's responsablity */
ImagePng::ImagePng( Reader* const input ) : NetworkGraphics( input ) {}

// Public methods

/** */
bool ImagePng::check() throw ( exception ) {
  const unsigned int a_size( 23 );
  char a[a_size];
  char type_a[4];
  char length_a[4];
  int  length;
  bool found_IDAT = false;

  if ( ! in->read( a, a_size ) )       return false;
  if ( ! equals( a, 0, MAGIC, 0, 5 ) ) return false;

  width  = getIntBigEndian( a, 13 );
  height = getIntBigEndian( a, 17 );
  if ( width < 1 || height < 1 ) return false;

  bitsPerPixel  = a[21] & 0xff;
  int colorType = a[22] & 0xff;
  if ( colorType == 2 || colorType == 6 ) bitsPerPixel *= 3;

  format = FORMAT_PNG;

  length = 3; // the remaining #bytes in the data block of the IHDR chunk

  // Attempt to find the pHYs chunk and, if needed, loop for comments
  while( true ) {
    in->skip( length + 4 ); // skip the data and the CRC fields
    if ( ! in->read( length_a, 4 ) ) return false;
    if ( ! in->read( type_a,   4 ) ) return false;

    if ( equals( type_a, 0, IEND, 0, 4 ) ) {
      // reached the end of the image
      break;
    }

    length = getIntBigEndian( length_a, 0 );
    if ( length == 0 ) continue; // field data is empty

    if ( ! found_IDAT && equals( type_a, 0, pHYs, 0, 4 ) ) {
      // physical information chunk
      if ( ! in->read( a, 9 ) ) return false;

      if ( a[8] == 0x01 ) { // unit is in meter
        // pixels per inch on X axis
        int ppi = (int)(getIntBigEndian( a, 0 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalWidthDpi = static_cast<int>( ppi );
        if ( ppi - physicalWidthDpi >= 0.5) physicalWidthDpi++;

        // pixels per inch on Y axis
        ppi = (int)(getIntBigEndian( a, 4 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalHeightDpi = static_cast<int>( ppi );
        if ( ppi - physicalHeightDpi >= 0.5) physicalHeightDpi++;
      }
      if ( ! collectComments ) break;
	  length -= 9; // already consumed 9 bytes from the data field

    } else if ( equals( type_a, 0, IDAT, 0, 4 ) ) {
      // image data chunk
      found_IDAT = true;
      if ( ! collectComments ) break;

    }

    length -= handleComments( length, type_a );

  }
  return true;
}

#endif // NO_PNG


#ifndef NO_MNG

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageMng::ImageMng( Reader* const input ) : NetworkGraphics( input ) {
  supportMultipleImages = true;
  numberOfImages = 0;
}

// Public methods

/** Similar to the PNG format */
bool ImageMng::check() throw ( exception ) {
  const unsigned int a_size( 41 );
  char a[a_size];
  char type_a[4];
  char length_a[4];
  int  length;

  if ( ! in->read( a, a_size ) )       return false;
  if ( ! equals( a, 0, MAGIC, 0, 5 ) ) return false;

  format = FORMAT_MNG;

  width  = getIntBigEndian( a, 13 );
  height = getIntBigEndian( a, 17 );

  // simplicity equals 0x01 if this is a Very Low Complexity mng stream
  int simplicity = getIntBigEndian( a, 37 ) & 0x00000217;

  if ( width == 0 || height == 0 ) { // no visible images in this mng stream
    countNumberOfImages = false; // no use to look for images
  } else if ( simplicity == 0x01 ) {
    numberOfImages = getIntBigEndian( a, 25 ) - 1;
    countNumberOfImages = false; // no use to look for images
  }

  length = 0; // the remaining #bytes in the data block of the MHDR chunk

  // Attempt to find the pHYs chunk and, if needed, loop for comments
  while( true ) {
    in->skip( length + 4 ); // skip the data and the CRC fields
    if ( ! in->read( length_a, 4 ) ) return false;
    if ( ! in->read( type_a,   4 ) ) return false;

    length = getIntBigEndian( length_a, 0 );

    if ( equals( type_a, 0, MEND, 0, 4 ) ) {
      // reached the end of the MNG stream
      break;
    } else if ( equals( type_a, 0, pHYg, 0, 4 ) ) {
      // physical information chunk
      if ( length == 0 ) {
        physicalWidthDpi  = INVALID ;
        physicalHeightDpi = INVALID ;
		continue;
      }
      if ( ! in->read( a, 9 ) ) return false;

      if ( a[8] == 0x01 ) { // unit is in meter
        // pixels per inch on X axis
        float ppi = (float)(getIntBigEndian( a, 0 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalWidthDpi = static_cast<int>( ppi );
        if ( ppi - physicalWidthDpi >= 0.5) physicalWidthDpi++;

        // pixels per inch on Y axis
        ppi = (float)(getIntBigEndian( a, 4 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalHeightDpi = static_cast<int>( ppi );
        if ( ppi - physicalHeightDpi >= 0.5) physicalHeightDpi++;
      }
      if ( ! collectComments && ! countNumberOfImages ) break;
      length -= 9; // already consumed 9 bytes from the data field

    }

    if ( length == 0 ) continue; // the data field is empty

    if ( countNumberOfImages && equals( type_a, 1, HDR, 0, 3 ) &&
         ( type_a[0] == 0x49 || type_a[0] == 0x4a ) ) {
      numberOfImages++;
    }

    length -= handleComments( length, type_a );

  }
  return true;
}

#endif // NO_MNG


#ifndef NO_JNG

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageJng::ImageJng( Reader* const input ) : NetworkGraphics( input ) {}

// Public methods

/** Similar to the PNG format */
bool ImageJng::check() throw ( exception ) {
  const unsigned int a_size( 21 );
  char a[a_size];
  char type_a[4];
  char length_a[4];
  int  length;

  if ( ! in->read( a, a_size ) )       return false;
  if ( ! equals( a, 0, MAGIC, 0, 5 ) ) return false;

  format = FORMAT_JNG;

  width  = getIntBigEndian( a, 13 );
  height = getIntBigEndian( a, 17 );

  length = 8; // the remaining #bytes in the data block of the JHDR chunk

  // Attempt to find the pHYs chunk and, if needed, loop for comments
  while( true ) {
    in->skip( length + 4 ); // skip the data and the CRC fields
    if ( ! in->read( length_a, 4 ) ) return false;
    if ( ! in->read( type_a,   4 ) ) return false;


    if ( equals( type_a, 0, IEND, 0, 4 ) ) {
      // reached the end of the JNG stream
      break;
    }

    length = getIntBigEndian( length_a, 0 );
    if ( length == 0 ) continue; // the data field is empty

    if ( equals( type_a, 0, pHYs, 0, 4 ) ) {
      // physical information chunk
      if ( ! in->read( a, 9 ) ) return false;

      if ( a[8] == 0x01 ) { // unit is in meter
        // pixels per inch on X axis
        float ppi = (float)(getIntBigEndian( a, 0 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalWidthDpi = static_cast<int>( ppi );
        if ( ppi - physicalWidthDpi >= 0.5) physicalWidthDpi++;

        // pixels per inch on Y axis
        ppi = (float)(getIntBigEndian( a, 4 ) * 0.0254); //64 bit to 32 bit conversion...
        physicalHeightDpi = static_cast<int>( ppi );
        if ( ppi - physicalHeightDpi >= 0.5) physicalHeightDpi++;
      }
      if ( ! collectComments ) break;
      length -= 9; // already consumed 9 bytes from the data field
    }

    length -= handleComments( length, type_a );

  }
  return true;
}

#endif // NO_JNG
