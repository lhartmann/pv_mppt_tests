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

#include <unistd.h>
#include "denis_sensors.h"
#include <cstdlib>
#include <vector>
#include "straux.h"
#include <cmath>

bool denis_sensors::read(double *G, double *T1, double *T2) {
	if (!*this) return true;
	
	// Default to NAN
	if (G ) *G  = NAN;
	if (T1) *T1 = NAN;
	if (T2) *T2 = NAN;
	
	// Create and connect the socket
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s==-1) {
		debug_say("Failed to create socket.");
		return false;
	}
	if (connect(s, (sockaddr*)&addr, sizeof(addr))) {
		debug_say("Failed to connect socket.");
		return false;
	}
	
	// Prepare for reading data
/*	fd_set set;
	FD_ZERO(&set);
	FD_SET(s, &set);
	timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=500000;
	do {
		int i=select(s+1, &set, 0, 0, &timeout);
		if (i<0) {
			if (errno = EINTR) continue;
			return true;
		}
		if (!i) return true;
	} while (false);*/
	
	// Read data
	char data[1024];
	int n = 0;
	do {
		int d = ::read(s, data+n, 1024-n);
		if (d<=0) {
			debug_say("Failed to read from socket.");
			return false;
		}
		n += d;
	} while (data[n-1] != '\n');
	close(s);
	data[n-1] = 0;
	
	// Denis might have sent me data using UTF16....
	if (data[0] == 0) {
		for (int i=0; i<n/2; ++i)
			data[i] = data[2*i+1];
		n = n/2;
	}
	
	// Split the string into a vector
	std::vector<std::string> v = strSplit(data, ' ');
	if (v.size() < 5) {
		debug_say("Bad data read from socket.");
		debug_dump(n);
		debug_dump(v.size());
		debug_dump(data);
		return false;
	}
	
	if (G ) *G  = std::atof(v[0].c_str());
	if (T1) *T1 = std::atof(v[1].c_str());
	if (T2) *T2 = std::atof(v[2].c_str());
	
	return true;
}
