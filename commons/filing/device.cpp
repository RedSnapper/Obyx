/* 
 * device.cpp is authored and maintained by Ben Griffin of Red Snapper Ltd 
 * device.cpp is a part of Obyx - see http://www.obyx.org .
 * Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd.
 * This file is Copyright (C) 2006-2014 Red Snapper Ltd. http://www.redsnapper.net
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

#include "device.h"

	namespace FileUtils {
		//-------------------------------------------------------------------------
		// Basic constructor
		//-------------------------------------------------------------------------
		Device::Device() :  device(),root_separator("/") { init(); }

		//-------------------------------------------------------------------------
		// Constructs a new device with the specified device name
		// e.g. "C:", "/"
		//-------------------------------------------------------------------------
		Device::Device(const string &newdevice) : device(newdevice),root_separator("/") { init(); }

		//-------------------------------------------------------------------------
		// Constructs a new device using a specified device
		//-------------------------------------------------------------------------
		Device::Device(const Device &newdevice) : device(),root_separator() {
			init();
			device = newdevice.device;
			root_separator = newdevice.root_separator;
		}

		//-------------------------------------------------------------------------
		// Destructor
		//-------------------------------------------------------------------------
		Device::~Device()
		{
		}

		//-------------------------------------------------------------------------
		// Default initialisation to set up OS defaults
		//-------------------------------------------------------------------------
		void Device::init() {
		#ifdef WIN32
			root_separator = "\\";
		#else
			root_separator = "/";
		#endif
		}


		//-------------------------------------------------------------------------
		// Clears the device name
		//-------------------------------------------------------------------------
		void Device::clear()
		{
			device = "";
		}

		//-------------------------------------------------------------------------
		// Sets a new root separator
		//-------------------------------------------------------------------------
		void Device::setRootSeparator(const string &separator)  {
			root_separator = separator;
		}


		//-------------------------------------------------------------------------
		// Gets the current root separator 
		//-------------------------------------------------------------------------
		const string Device::getRootSeparator() const {
			return root_separator;
		}


		//-------------------------------------------------------------------------
		// Sets a new device name
		//-------------------------------------------------------------------------
		void Device::setDevice(const string &newdevice)
		{
			device = newdevice;
		}

		//-------------------------------------------------------------------------
		// Gets the current device name
		//-------------------------------------------------------------------------
		const string Device::getDevice() const {
			return device;
		}


		//-------------------------------------------------------------------------
		// Gets a formatted device name
		// e.g. "C:\", "/"
		//
		// abs = true, forces an absolute device to be returned
		//-------------------------------------------------------------------------
		const string Device::getDeviceName(bool abs) const {
			string result;
 			if (!device.empty()) {
				if (device.compare(root_separator) == 0)
					result = root_separator;
				else {
					result = device;
					result.append(root_separator);
				}
			} else { 
				if (abs) {
					result = root_separator;
				}
			}
			return result;
		}

		//-------------------------------------------------------------------------
		// Gets a formatted device name, see getDeviceName() 
		//-------------------------------------------------------------------------
		const string Device::output(bool abs) const {
			return getDeviceName(abs);
		}


	}	// namespace FileUtils

