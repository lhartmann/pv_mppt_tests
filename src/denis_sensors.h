/***************************************************************************
 *   Copyright (C) 2008 by Lucas V. Hartmann <lucas.hartmann@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef READ_DENIS_SENSORS_H
#define READ_DENIS_SENSORS_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "debug.h"

class denis_sensors {
	sockaddr_in addr;
	
public:
	bool read(double *G, double *T1, double *T2);
	bool open(uint32_t IP, uint16_t port = htons(1800)) {
		// Sensor address specified, set connection parameters
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = IP;
		
		// Test run
		if (!read(0,0,0)) {
			addr.sin_addr.s_addr = 0;
			return false;
		}
		return true;
	}
	bool open(const char *address, uint16_t port = htons(1800)) {
		return open(inet_addr(address), port);
	}
	operator bool () const { return addr.sin_addr.s_addr; }
};

#endif