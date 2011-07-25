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

#include "commons/logger/logger.h"

using namespace ImageInfo;
using std::auto_ptr;
using std::out_of_range;
using std::string;

Info::Info(  istream& ist ) : fileReader( ist ), img( 0 ) {
  collectComments = false;
  countNumberOfImages = false;
}

bool Info::check() {
	if (Logger::debugging()) {
		*Logger::log << Log::info << Log::LI << "Checking to identify if file is an image we recognise." << Log::LO << Log::blockend;
	}
  try {
	int b1 = fileReader.readOne();
	int b2 = fileReader.readOne();
	int b3 = fileReader.readOne();

	if ( b1 == 0x47 && b2 == 0x49 && b3 == 0x46 ) { 
		if (Logger::debugging()) {
			*Logger::log << Log::info << Log::LI << "Looks like Gif" << Log::LO << Log::blockend;
		}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageGif( &fileReader) );
	} else if ( b1 == 0x89 && b2 == 0x50 && b3 == 0x4e ) { 
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Png" << Log::LO << Log::blockend;	}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImagePng( &fileReader) );
    } else if ( b1 == 0x8a && b2 == 0x4d && b3 == 0x4e ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Mng" << Log::LO << Log::blockend;	}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageMng( &fileReader) );
    } else if ( b1 == 0x8B && b2 == 0x4a && b3 == 0x4e ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Jng" << Log::LO << Log::blockend;	}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageJng( &fileReader) );
    } else if ( b1 == 0xff && b2 == 0xd8 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Jpeg" << Log::LO << Log::blockend;}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageJpeg( &fileReader) );
    } else if ( b1 == 0x42 && b2 == 0x4d ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Bmp" << Log::LO << Log::blockend;	}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageBmp( &fileReader) );
    } else if ( b1 == 0x0a && b2 <  0x06 && b3 == 0x01 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Pcx" << Log::LO << Log::blockend;	}
		AUTO_PTR_ASSIGN( ImageBase, img, new ImagePcx( &fileReader) );
    } else if ( b1 == 0x46 && b2 == 0x4f && b3 == 0x52 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Iff" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageIff( &fileReader) );
    } else if ( b1 == 0x59 && b2 == 0xa6 && b3 == 0x6a ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Ras" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageRas( &fileReader) );
    } else if ( b1 == 0x50 && b2 >= 0x31 && b2 <= 0x36 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Pnm" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImagePnm( &fileReader) );
    } else if ( b1 == 0x38 && b2 == 0x42 && b3 == 0x50 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Psd" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImagePsd( &fileReader) );
    } else if ( b1 == 0x46 && b2 == 0x57 ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Swf" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageSwf( &fileReader) );
    } else if ( b1 == 0x00 && b2 == 0x00 && ( b3 == 0x01 || b3 == 0x02 ) ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Ico" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageIco( &fileReader) );
    } else if ( ( b2 == 0x01 && ( b3 == 0x01 || b3 == 0x09 || b3 == 0x20 || b3 == 0x21 ) ) ||
                ( b2 == 0x00 && ( b3 == 0x02 || b3 == 0x03 || b3 == 0x0A || b3 == 0x0B ) ) ) {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "Looks like Tga" << Log::LO << Log::blockend;}	
		AUTO_PTR_ASSIGN( ImageBase, img, new ImageTga( &fileReader) );
    } else {
		if (Logger::debugging()) {*Logger::log << Log::info << Log::LI << "It's not a media file that we can recognise." << Log::LO << Log::blockend;}	
      return false;
    }
    img->setCollectComments( collectComments ); //these are both false by default.
    img->setCountNumberOfImages( countNumberOfImages );
	if (Logger::debugging()) {
		*Logger::log << Log::info << Log::LI << "Turned off comment collection and image counting. (default)" << Log::LO << Log::blockend;	
		*Logger::log << Log::info << Log::LI << "Now about to look deeper into the image." << Log::LO << Log::blockend;	
	}
    return img->check();
  } catch ( std::exception e ) {
	if (Logger::debugging()) {
		*Logger::log << Log::info << Log::LI << "General image check - exception happened: " << e.what() << Log::LO << Log::blockend;
	}
	return false;
  }
}

int Info::getHeight() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getHeight();
}

int Info::getWidth() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getWidth();
}

int Info::getFormat() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getFormat();
}

string Info::getFormatName() {
  if ( img.get() == 0 ) return "";
  else                  return img->getFormatName();
}

string Info::getMimeType() {
  if ( img.get() == 0 ) return "";
  else                  return img->getMimeType();
}

int Info::getBitsPerPixel() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getBitsPerPixel();
}

size_t Info::getNumberOfComments() {
  if ( img.get() == 0 ) return 0;
  else                  return img->getNumberOfComments();
}

string Info::getComment( unsigned int index ) throw ( out_of_range ) {
  if ( img.get() == 0 ) throw out_of_range( "check() was not called. No comments exctracted." );
  else                  return img->getComment( index );
}

void Info::setCollectComments( bool enable ) {
  collectComments = enable;
}

int Info::getNumberOfImages() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getNumberOfImages();
}

void Info::setCountNumberOfImages( bool enable ) {
  countNumberOfImages = enable;
}

int Info::getPhysicalHeightDpi() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getPhysicalHeightDpi();
}

float Info::getPhysicalHeightInch() {
  if ( img.get() == 0 ) return static_cast< float >( INVALID );
  else                  return img->getPhysicalHeightInch();
}

int Info::getPhysicalWidthDpi() {
  if ( img.get() == 0 ) return INVALID;
  else                  return img->getPhysicalWidthDpi();
}

float Info::getPhysicalWidthInch() {
  if ( img.get() == 0 ) return static_cast< float >( INVALID );
  else                  return img->getPhysicalWidthInch();
}
