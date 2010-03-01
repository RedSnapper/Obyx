/* 
 * device.h is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * device.h is a part of Obyx - see http://www.obyx.org .
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

#ifndef OBYX_FILE_DEVICE_H
#define OBYX_FILE_DEVICE_H

//-------------------------------------------------------------------
// System includes
//-------------------------------------------------------------------
#include <string>

	namespace FileUtils {
		using std::string;

		class Device {
		protected:
			string	device;				//eg C:
			string	root_separator;

		public:
			Device();
			Device(const string &newdevice);
			Device(const Device &newdevice);	
			virtual ~Device();

			void init();
			void clear();

			void setRootSeparator(const string &separator);
			const string getRootSeparator() const;
			void setDevice(const string &newdevice);
			const string getDevice() const;
			const string getDeviceName(bool abs = false) const;

			virtual const string output(bool abs = false) const;
		};

	}	// namespace FileUtils

#endif


