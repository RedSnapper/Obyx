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

#ifndef GLOBAL_H
#define GLOBAL_H 1

#define NB_FORMATS 18

/** Return value of most methods if check() fails. */
#define INVALID -1
#define FORMAT_JPEG 0
#define FORMAT_GIF 1
#define FORMAT_PNG 2
#define FORMAT_BMP 3
#define FORMAT_PCX 4
#define FORMAT_IFF 5
#define FORMAT_RAS 6
#define FORMAT_PBM 7
#define FORMAT_PGM 8
#define FORMAT_PPM 9
#define FORMAT_PSD 10
#define FORMAT_SWF 11
#define FORMAT_ICO 12
#define FORMAT_CUR 13
#define FORMAT_TGA 14
#define FORMAT_MNG 15
#define FORMAT_JNG 16

#define XMALLOC(type, num)  ( (type *) malloc ( (num) * sizeof(type) ) )
#define AUTO_PTR_ASSIGN( type, auto_p, c_p ) { auto_ptr<type> tmp_p( c_p ); auto_p = tmp_p; }

#include <memory>
#include <string>
#include <stdexcept>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace ImageInfo {
//-----------------------------------------------------------------------------	
//------------- Reader --------------------------------------------------------	
	class Reader { // A class with various methods to read from an input stream.
public:
		Reader();
		Reader(  istream& ist );
		long readUBits( int numBits ) throw ( std::exception );
		int  readSBits( int numBits ) throw ( std::exception );
		int readOne() throw ( std::exception );
		bool read( char* a, std::streamsize num, int offset = 0 ) throw ( std::exception);
		std::string readLine() throw ( std::exception );
		void skip( unsigned int num ) throw ( std::exception );
		void goBack();
		
private:
		int bitPos;
		int bitBuf;
		istream& input;
		Reader( const Reader & );
		Reader &operator=( const Reader & );
	};
//------------- ImageBase ----------------------------------------------------	
class ImageBase {
public:
		ImageBase( Reader* const input );
		virtual ~ImageBase();
		virtual bool check() throw ( std::exception ) = 0;
		int getWidth();
		int getHeight();
		int getFormat();
		std::string getFormatName();
		std::string getMimeType();
		int getBitsPerPixel();
		size_t getNumberOfComments();
		std::string getComment( unsigned int index ) throw ( std::out_of_range );
		void setCollectComments( bool enable );
		int getNumberOfImages();
		void setCountNumberOfImages( bool enable );
		int getPhysicalHeightDpi();
		float getPhysicalHeightInch();
		int getPhysicalWidthDpi();
		float getPhysicalWidthInch();
		
protected:
			int format;
		int width;
		int height;
		int bitsPerPixel;
		int physicalHeightDpi;
		int physicalWidthDpi;
		int numberOfImages;
		bool countNumberOfImages;
		bool supportMultipleImages;
		std::vector< std::string > comments;
		bool collectComments;
		bool supportComments;
		Reader* const in;
		
		bool equals( char* a1, unsigned int offs1, char* a2, unsigned int offs2, unsigned int num );
		int getIntBigEndian(      char* a, unsigned int offs );
		int getIntLittleEndian(   char* a, unsigned int offs );
		int getShortBigEndian(    char* a, unsigned int offs );
		int getShortLittleEndian( char* a, unsigned int offs );
	};	
//--------------ImageBase leaf classes ---------------------------------------
class ImageJpeg : public ImageBase {
public:
	ImageJpeg( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageGif : public ImageBase {
public:
	ImageGif( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageBmp : public ImageBase {
public:
	ImageBmp( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImagePcx : public ImageBase {
public:
	ImagePcx( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageIff: public ImageBase {
public:
	ImageIff( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageRas : public ImageBase {
public:
	ImageRas( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImagePnm : public ImageBase {
public:
	ImagePnm( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImagePsd : public ImageBase {
public:
	ImagePsd( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageSwf : public ImageBase {
public:
	ImageSwf( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageIco : public ImageBase {
public:
	ImageIco( Reader* const input );
	virtual bool check() throw ( std::exception );
};

class ImageTga : public ImageBase {
public:
	ImageTga( Reader* const input );
	virtual bool check() throw ( std::exception );
};
//--------------Info ---------------------------------------------------------
class Info {
public:
    Info( istream& ist );
    bool check();
    int getWidth();
    int getHeight();
    int getFormat();
	
    std::string getFormatName();
    std::string getMimeType();
	
    int getBitsPerPixel();
    size_t getNumberOfComments();
    std::string getComment( unsigned int index ) throw ( std::out_of_range );
    void setCollectComments( bool enable );
    int getNumberOfImages();
    void setCountNumberOfImages( bool enable );
    int getPhysicalHeightDpi();
    float getPhysicalHeightInch();
    int getPhysicalWidthDpi();
    float getPhysicalWidthInch();
	
private:
		Reader fileReader;
    std::auto_ptr< ImageBase > img;
	
    bool collectComments;
    bool countNumberOfImages;
	
};
//--------------NetworkGraphics ImageBase leaf classes -----------------------
class NetworkGraphics : public ImageBase {
	
public:
    NetworkGraphics( Reader* const input );
    virtual bool check() throw ( std::exception ) = 0;
	
protected:
	static char MAGIC[5];
    static char HDR[3];  // (subpart of the) header
    static char IDAT[4]; // image data
    static char pHYs[4]; // png physical
    static char pHYg[4]; // global physical
    static char IEND[4]; // png ending
    static char MEND[4]; // mng ending
    unsigned int handleComments( unsigned int length, char type_a[4] );
	
private:
	static char tEXt[4]; // plain text
    static char zTXt[4]; // compressed text
    static char iTXt[4]; // UTF-8 text
    std::string uncompress( const std::string compr_str );
};

class ImagePng : public NetworkGraphics {
public:
    ImagePng( Reader* const input );
    virtual bool check() throw ( std::exception );
};

class ImageMng : public NetworkGraphics {
public:
	ImageMng( Reader* const input );
    virtual bool check() throw ( std::exception );
};

class ImageJng : public NetworkGraphics {
public:
    ImageJng( Reader* const input );
    virtual bool check() throw ( std::exception );
};

//--------------Info ---------------------------------------------------------
//--------------Info ---------------------------------------------------------
//-----------------------------------------------------------------------------	
};


#endif // GLOBAL_H
