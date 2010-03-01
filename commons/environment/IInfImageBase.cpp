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

#include "IInfGlobal.h"

using namespace ImageInfo;
using std::string;

static const char* FORMAT_NAMES[NB_FORMATS] = { "", "JPEG", "GIF", "PNG", "BMP","PCX", "IFF", "RAS", "PBM", "PGM","PPM", "PSD", "SWF", "ICO", "CUR","TGA", "MNG", "JNG" };

static const char* FORMAT_MIMES[NB_FORMATS] =
  { "", "image/jpeg", "image/gif", "image/png", "image/bmp", "image/pcx",
    "image/iff", "image/ras", "image/x-portable-bitmap",
    "image/x-portable-graymap", "image/x-portable-pixmap", "image/psd",
    "application/x-shockwave-flash", "image/x-icon", "image/x-cursor",
    "image/tga", "video/x-mng", "image/x-jng" };

ImageBase::ImageBase( Reader* const input ) : in( input ) {
  format            = INVALID;
  width             = INVALID;
  height            = INVALID;
  bitsPerPixel      = INVALID;
  physicalHeightDpi = INVALID;
  physicalWidthDpi  = INVALID;
  numberOfImages = 1;
  countNumberOfImages = false;
  supportMultipleImages = false;
  comments.clear();
  collectComments = false;
  supportComments = false;
}

ImageBase::~ImageBase() {}


int ImageBase::getWidth() { return width; }
int ImageBase::getHeight() { return height; }
int ImageBase::getFormat() { return format; }
string ImageBase::getFormatName() { return FORMAT_NAMES[ format + 1 ]; }
string ImageBase::getMimeType() { return FORMAT_MIMES[ format + 1 ]; }
int ImageBase::getBitsPerPixel() { return bitsPerPixel; }
size_t ImageBase::getNumberOfComments() { return comments.size(); }
string ImageBase::getComment( unsigned int index ) throw ( std::out_of_range ) {
  if ( comments.empty() || index >= comments.size() ) {
    throw std::out_of_range( "Given index is not a valid comment index." );
  }
  return comments[ index ];
}
void ImageBase::setCollectComments( bool enable ) {
  collectComments = supportComments && enable;
}
int ImageBase::getNumberOfImages() { return numberOfImages; }
void ImageBase::setCountNumberOfImages( bool enable ) {
  countNumberOfImages = supportMultipleImages && enable;
}

int ImageBase::getPhysicalHeightDpi() { return physicalHeightDpi; }
float ImageBase::getPhysicalHeightInch() {
  int h  = getHeight();
  int ph = getPhysicalHeightDpi();
  if ( h <= 0 || ph <= 0  )
    return static_cast< float >( INVALID );
  else
    return static_cast< float >( h ) / static_cast< float >( ph );
}
int ImageBase::getPhysicalWidthDpi() { return physicalWidthDpi; }
float ImageBase::getPhysicalWidthInch() {
  int w = getWidth();
  int pw = getPhysicalWidthDpi();
  if ( w <= 0 || pw <= 0 )
    return static_cast< float >( INVALID );
  else
    return static_cast< float >( w ) / static_cast< float >( pw );
}

bool ImageBase::equals( char* a1, unsigned int offs1,
                        char* a2, unsigned int offs2, unsigned int num ) {
  while ( num-- > 0 ) {
    if ( a1[offs1++] != a2[offs2++] ) {
      return false;
	}
  }
  return true;
}

int ImageBase::getIntBigEndian( char* a, unsigned int offs ) {
  return (( a[offs] & 0xff )    << 24) | (( a[offs + 1] & 0xff ) << 16) |
         (( a[offs + 2] & 0xff) << 8 ) | (a[offs + 3] & 0xff);
}

int ImageBase::getIntLittleEndian( char* a, unsigned int offs ) {
  return ( (a[offs + 3] & 0xff ) << 24) | (( a[offs + 2] & 0xff ) << 16) |
         ( (a[offs + 1] & 0xff ) << 8)  | (a[offs] & 0xff);
}

int ImageBase::getShortBigEndian( char* a, unsigned int offs ) {
  return (( a[offs] & 0xff ) << 8) | ( a[offs + 1] & 0xff );
}

int ImageBase::getShortLittleEndian( char* a, unsigned int offs ) {
  return ( a[offs] & 0xff ) | (( a[offs + 1] & 0xff ) << 8);
}
