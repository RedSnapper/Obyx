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
 * This file is Copyright (C) 2006-2010 Red Snapper Ltd. http://www.redsnapper.net
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
#include <memory>
//#include <stdlib>
#include "commons/logger/logger.h"
#include "commons/string/strings.h"
#include "IInfGlobal.h"

using namespace ImageInfo;
using std::auto_ptr;
using std::string;
using std::exception;

#ifndef NO_JPEG

ImageJpeg::ImageJpeg( Reader* const input ) : ImageBase( input ) {
  supportComments = true;
}

bool ImageJpeg::check() throw ( exception ) {
  char APP0_ID[5] = { 0x4a, 0x46, 0x49, 0x46, 0x00 };
  const unsigned int a_size( 12 );
  char data[a_size];
  in->goBack(); // unget one char
  while ( true ) {
    if ( ! in->read( data, 4 ) ) return false;
    int marker = getShortBigEndian( data, 0 );
    int size   = getShortBigEndian( data, 2 );
    if ( ( marker & 0xff00 ) != 0xff00 ) return false; // not a valid marker
	  if (Logger::debugging()) {
		  *Logger::log << Log::info << Log::LI << "Looks like deeply like Jpeg" << Log::LO << Log::blockend;	
	  }
    if ( marker == 0xffe0 ) { // APPx
      // APPx header must be larger than 14 bytes
      if ( size < 14 )              return false;
      if ( ! in->read( data, 12 ) ) return false;
      if ( equals( APP0_ID, 0, data, 0, 5 ) ) {
        if        ( data[7] == 1 ) {
          physicalWidthDpi  = getShortBigEndian( data, 8  );
          physicalHeightDpi = getShortBigEndian( data, 10 );
        } else if ( data[7] == 2 ) {
          int x = getShortBigEndian( data, 8  );
          int y = getShortBigEndian( data, 10 );
          physicalWidthDpi  = static_cast< int >( x * 2.54f );
          physicalHeightDpi = static_cast< int >( y * 2.54f );
        }
      }
      in->skip( size - 14 );
    } else if ( collectComments && size > 2 && marker == 0xfffe ) { // comment
      size -= 2;
		char* chars = new char(size);
		if ( ! in->read( chars, size )) {
			delete chars;
			return false;
		} else {
      		string comment( chars );
      		comments.push_back( comment );
			delete chars;
		}
    } else if ( marker >= 0xffc0 && marker <= 0xffcf && marker != 0xffc4 && marker != 0xffc8) {
      if ( ! in->read( data, 6 ) ) return false;
      format = FORMAT_JPEG;
      bitsPerPixel = ( data[0] & 0xff ) * ( data[5] & 0xff );
      width  = getShortBigEndian( data, 3 );
      height = getShortBigEndian( data, 1 );
      return ( width > 0 && height > 0 );
    } else in->skip(size - 2);
  }
}

#endif // NO_JPEG


#ifndef NO_GIF
/** Deletion of input is under the caller's responsablity */
ImageGif::ImageGif( Reader* const input ) : ImageBase( input ) {
  supportComments = true;
  supportMultipleImages = true;
}

bool ImageGif::check() throw ( exception ) {
  const unsigned int a_size( 10 );
  char a[a_size];
  char GIF_MAGIC_87A[3] = {0x38, 0x37, 0x61};
  char GIF_MAGIC_89A[3] = {0x38, 0x39, 0x61};

  if ( ! in->read( a, a_size ) ) return false;
  if ( ( ! equals( a, 0, GIF_MAGIC_89A, 0, 3 ) ) &&
       ( ! equals( a, 0, GIF_MAGIC_87A, 0, 3 ) ) ) {
    return false;
  }
  format = FORMAT_GIF;
  width  = getShortLittleEndian( a, 3 );
  height = getShortLittleEndian( a, 5 );
  if ( width < 1 || height < 1 ) return false;

  int flags = a[7] & 0xff;
  bitsPerPixel = ( ( flags >> 4 ) & 0x07 ) + 1;

  if ( ! countNumberOfImages && ! collectComments ) return true;
  if ( ( flags & 0x80 ) != 0 ) {
    int tableSize = ( 1 << ( ( flags & 7 ) + 1 ) ) * 3;
	in->skip( tableSize );
  }
  numberOfImages = 0;
  int blockType;
  do {
    blockType = in->readOne();
    switch ( blockType ) {
    case 0x2c : { // image separator
      if ( ! in->read( a, 9 ) ) return false;
      flags = a[8] & 0xff;
      int localBitsPerPixel = ( flags & 0x07 ) + 1;
      if ( localBitsPerPixel > bitsPerPixel )
        bitsPerPixel = localBitsPerPixel;
      if ( ( flags & 0x80 ) != 0 )
        in->skip( ( 1 << localBitsPerPixel ) * 3 );
      in->skip( 1 ); // initial code length
      int n;
      do {
        n = in->readOne();
        if      ( n >  0  ) in->skip( n );
        else if ( n == -1 ) return false;
      } while ( n > 0 );
      if ( countNumberOfImages ) numberOfImages++;
      break;
    } case 0x21 : { // extension
      int extensionType = in->readOne();
      if ( collectComments && extensionType == 0xfe ) {
        string str;
        int n = in->readOne();
        while ( n > 0 ) {
          for ( int i = 0; i < n; i++ ) {
            int ch = in->readOne();
            if ( ch == -1 ) return false;
            str += static_cast< char >( ch );
          }
          n = in->readOne();
        }
        comments.push_back( str );
      } else {
        int n;
        do {
          n = in->readOne();
          if      ( n >  0  ) in->skip( n );
          else if ( n == -1 ) return false;
        } while ( n > 0 );
      }
      break;
    } case 0x3b : // end of file
      break;
    }
  } while ( blockType != 0x3b );

  return true;
}

#endif // NO_GIF


#ifndef NO_BMP

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageBmp::ImageBmp( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** */
bool ImageBmp::check() throw ( exception ) {
  const unsigned int a_size( 43 );
  char a[a_size];

  if ( ! in->read( a, a_size ) ) return false;

  width  = getIntLittleEndian( a, 15 );
  height = getIntLittleEndian( a, 19 );
  if ( width < 1 || height < 1 ) return false;

  bitsPerPixel = getShortLittleEndian( a, 25 );
  if ( bitsPerPixel != 1  && bitsPerPixel != 4  &&
       bitsPerPixel != 8  && bitsPerPixel != 16 &&
       bitsPerPixel != 24 && bitsPerPixel != 32 ) {
    return false;
  }

  // pixels per inch on X axis
  double ppi = getIntLittleEndian( a, 35 ) * 0.0254; //64 bit to 32 bit conversion...
  if ( ppi != 0.0 ) {
    physicalWidthDpi = static_cast<int>( ppi );
    if ( ppi - physicalWidthDpi >= 0.5) physicalWidthDpi++;
  }

  // pixels per inch on Y axis
  ppi = getIntLittleEndian( a, 39 ) * 0.0254; //64 bit to 32 bit conversion...
  if ( ppi != 0.0 ) {
    physicalHeightDpi = static_cast<int>( ppi );
    if ( ppi - physicalHeightDpi >= 0.5) physicalHeightDpi++;
  }

  format = FORMAT_BMP;
  return true;
}

#endif // NO_BMP

#ifndef NO_PCX

// Constructor

/** Deletion of input is under the caller's responsablity */
ImagePcx::ImagePcx( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** */
bool ImagePcx::check() throw ( exception ) {
  const unsigned int a_size( 63 );
  char a[a_size];

  if ( ! in->read( a, a_size ) ) return false;

  // width / height
  int x1 = getShortLittleEndian( a, 1 );
  int y1 = getShortLittleEndian( a, 3 );
  int x2 = getShortLittleEndian( a, 5 );
  int y2 = getShortLittleEndian( a, 7 );
  if ( x1 < 0 || x2 < x1 || y1 < 0 || y2 < y1 ) return false;
  width  = x2 - x1 + 1;
  height = y2 - y1 + 1;

  // color depth
  int bits   = a[0];
  int planes = a[62];
  if ( planes == 1 &&
       ( bits == 1 || bits == 2 || bits == 4 || bits == 8 ) ) {
    bitsPerPixel = bits;  // paletted
  } else if ( planes == 3 && bits == 8 ) {
    bitsPerPixel = 24;    // RGB truecolor
  } else return false;

  physicalWidthDpi  = getShortLittleEndian( a, 9 );
  physicalHeightDpi = getShortLittleEndian( a, 9 );
  format = FORMAT_PCX;
  return true;
}

#endif // NO_PCX


#ifndef NO_IFF

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageIff::ImageIff( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** */
bool ImageIff::check() throw ( exception ) {
  const unsigned int a_size( 9 );
  char a[a_size];

  // read remaining 2 bytes of file id, 4 bytes file size 
  // and 4 bytes IFF subformat
  if ( ! in->read( a, a_size ) ) return false;
  if ( a[0] != 0x4d )            return false;
  int type = getIntBigEndian( a, 5 );
  if ( type != 0x494c424d &&  // type must be ILBM...
       type != 0x50424d20 ) { // ...or PBM
    return false;
  }
  // loop chunks to find BMHD chunk
  do {
    if ( ! in->read( a, 8 ) ) return false;
    int chunkId = getIntBigEndian( a, 0 );
    int size    = getIntBigEndian( a, 4 );
    if ( ( size & 1 ) == 1 ) size++;
	if ( chunkId == 0x424d4844 ) { // BMHD chunk
	  if ( ! in->read( a, 9 ) ) return false;

	  format = FORMAT_IFF;
      width  = getShortBigEndian( a, 0 );
      height = getShortBigEndian( a, 2 );
      if ( width < 1 || height < 1 ) return false;

	  bitsPerPixel = a[8] & 0xff;
      return ( bitsPerPixel > 0 && bitsPerPixel < 33 );

    } else in->skip( size );
  } while ( true );
}

#endif // NO_IFF


#ifndef NO_RAS

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageRas::ImageRas( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** */
bool ImageRas::check() throw ( exception ) {
  const unsigned int a_size( 13 );
  char a[a_size];

  if ( ! in->read( a, a_size ) ) return false;
  if ( a[0] != char( 0x95 ) )    return false;

  width  = getIntBigEndian( a, 1 );
  height = getIntBigEndian( a, 5 );
  if ( width < 1 || height < 1 ) return false;

  bitsPerPixel = getIntBigEndian( a, 9 );

  format = FORMAT_RAS;
  return ( bitsPerPixel > 0 && bitsPerPixel <= 24 );
}

#endif // NO_RAS


#ifndef NO_PNM

// Constructor

/** Deletion of input is under the caller's responsablity */
ImagePnm::ImagePnm( Reader* const input ) : ImageBase( input ) {
  supportComments = true;
}

// Public methods

/** */
bool ImagePnm::check() throw ( exception ) {
  in->goBack(); 
  in->goBack(); 

  int id2 = in->readOne() - '0';

  if ( id2 < 1 || id2 > 6 ) return false;
  const int PNM_FORMATS[3] = { FORMAT_PBM, FORMAT_PGM, FORMAT_PPM };
  format = PNM_FORMATS[( id2 - 1 ) % 3];
  bool hasPixelResolution = false;
  string s;
  while ( true ) {
    s = in->readLine();
    if ( s.empty() ) continue;
    if ( s.at(0) == '#' ) { // comment
      if ( collectComments && s.length() > 1 ) comments.push_back( s.substr( 1 ) );
      continue;
    }
    if ( ! hasPixelResolution ) { // split "343 966" into width=343, height=966
      size_t spaceIndex = s.find( ' ' );
      if ( spaceIndex == string::npos ) return false;
      string widthString = s.substr( 0, spaceIndex );

      spaceIndex = s.find_last_of( ' ' );
      if ( spaceIndex == string::npos ) return false;
      string heightString = s.substr( spaceIndex + 1 );
		width = String::natural(widthString);
		height = String::natural(heightString);
      if ( width < 1 || height < 1 ) return false;
      if ( format == FORMAT_PBM ) {
        bitsPerPixel = 1;
        return true;
      }
      hasPixelResolution = true;
    } else {
		int maxSample =  String::natural(s);
		if ( maxSample < 0 ) return false;
		for ( int i = 0; i < 25; ) {
			if ( maxSample < ( 1 << ++i ) ) {
				bitsPerPixel = i;
				if ( format == FORMAT_PPM ) bitsPerPixel *= 3;
				return true;
			}
		}
		return false;
    }
  }	
}

#endif // NO_PNM


#ifndef NO_PSD

// Constructor

/** Deletion of input is under the caller's responsablity */
ImagePsd::ImagePsd( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** */
bool ImagePsd::check() throw ( exception ) {
  const unsigned int a_size( 23 );
  char a[a_size];

  if ( ! in->read( a, a_size ) )  return false;
  if ( a[0] != 0x53 )             return false;

  width  = getIntBigEndian( a, 15 );
  height = getIntBigEndian( a, 11 );
  if ( width < 1 || height < 1 ) return false;

  int channels = getShortBigEndian( a, 9 );
  int depth    = getShortBigEndian( a, 19 );
  bitsPerPixel = channels * depth;

  format = FORMAT_PSD;
  return ( bitsPerPixel > 0 && bitsPerPixel <= 64 );
}

#endif // NO_PSD


#ifndef NO_SWF

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageSwf::ImageSwf( Reader* const input ) : ImageBase( input ) {}

// Public methods

/** Java version written by Michael Aird. */
bool ImageSwf::check() throw ( exception ) {
  // Get rid of the last byte of the signature,
  // the byte of the version and 4 bytes of the size
  const unsigned int a_size( 5 );
  char a[a_size];
  if ( ! in->read( a, a_size ) ) return false;
  format = FORMAT_SWF;
  int bitSize = static_cast< int >( in->readUBits( 5 ) );
//  int minX = in->readSBits( bitSize );
  int maxX = in->readSBits( bitSize );
//  int minY = in->readSBits( bitSize );
  int maxY = in->readSBits( bitSize );
  width  = maxX / 20; //cause we're in twips
  height = maxY / 20; //cause we're in twips
  physicalWidthDpi  = 72;
  physicalHeightDpi = 72;
  return true;
}

#endif // NO_SWF


#ifndef NO_ICO

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageIco::ImageIco( Reader* const input ) : ImageBase( input ) {
  supportMultipleImages = true;
}

// Public methods

/** If bitsPerPixel equals to 32, this means your facing a Windows XP icon
 * ( True colors AND transparency) */
bool ImageIco::check() throw ( exception ) {
  const unsigned int a_size( 12 );
  char a[a_size];

  in->goBack(); // unget 1 char
  if ( ! in->read( a, a_size ) ) return false;

  int typeID = getShortLittleEndian( a, 0 );
  if      ( typeID == 0x01 ) format = FORMAT_ICO;
  else if ( typeID == 0x02 ) format = FORMAT_CUR;
  else                       return false;

  if ( countNumberOfImages ) numberOfImages = getShortLittleEndian( a, 2 );

  // information extracted only from the first image present in the file!
  width  = a[4]; // 7th byte
  height = a[5]; // 8th byte
  if ( width < 1 || height < 1 ) return false;

  // According to the cursor specification, the byte containing bit per pixel
  // value is reserved and must be 0 
  if ( format == FORMAT_CUR ) {
    bitsPerPixel = INVALID;
	return true;
  } else if ( a[6] != 0x00 ) { // 9th byte (icon only)
    bitsPerPixel = ( ( a[6] >> 4 ) & 0x07 ) + 1;
  } else { // 12th and 13th bytes (icon only)
    bitsPerPixel = getShortLittleEndian( a, 10 );
  }

  return ( bitsPerPixel > 0 && bitsPerPixel <= 32 );
}

#endif // NO_ICO


#ifndef NO_TGA

// Constructor

/** Deletion of input is under the caller's responsablity */
ImageTga::ImageTga( Reader* const input )
  : ImageBase( input ) {}

// Public methods

/** An easy one... */
bool ImageTga::check() throw ( exception ) {
  const unsigned int a_size( 14 );
  char a[a_size];

  if ( ! in->read( a, a_size ) ) return false;

  width  = getShortLittleEndian( a, 9  ); // bytes 12 & 13
  height = getShortLittleEndian( a, 11 ); // bytes 14 & 15

  if ( width < 1 || height < 1 ) return false;

  format = FORMAT_TGA;

  bitsPerPixel = a[13]; // 17th byte

  return ( bitsPerPixel > 0 && bitsPerPixel <= 32 );
}

#endif // NO_TGA
