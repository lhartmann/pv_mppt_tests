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

#ifndef PID_FILE_HANDLER_H
#define PID_FILE_HANDLER_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>

/// Simple class for handling a Process-ID file creation and deletion.
class pid_file_handler {
	std::string fn;
	std::ofstream fs;
	std::ostream *out;
public:
	/// Creates the PID file.
	/** @param name  is the PID file name to create.
	 *  @param out   is a pointer to a iostream where to print log messages.
	 *  @returns     true on success or false on error.
	 * 
	 * Creates a file containing the process identification number as returned by getPID().
	 * If @param out is specified it is used for log messages. If @param out is zero then
	 * no log messages are output.
	 * 
	 * WARNING: Operation is not atomic, therefore the PID file must not be considered a lock.
	 */
	bool create(const char *name, std::ostream *out=0) {
		if (out) (*out)<<"Writing PID file... "<<std::flush;
		if (access(name, F_OK)) {
			if (out) (*out)<<"ERROR."<<std::endl;
			std::cerr<<"Error: PID file \""<<name<<"\" already exists."<<std::endl;
			return false;
		}
		std::ofstream pidfile(name, std::ios::out|std::ios::trunc);
		if (!pidfile) {
			if (out) (*out)<<"ERROR."<<std::endl;
			std::cerr<<"Error: Could not open \""<<name<<"\" for writing."<<std::endl;
			return false;
		}
		pidfile<<getpid();
		fn = name;
		if (out) (*out)<<"Ok."<<std::endl;
		return true;
	}
	
	/// Removes the PID file.
	void remove() {
		if (fn.empty()) return;
		if (out) (*out)<<"Removing PID file... "<<std::flush;
		unlink(fn.c_str());
		if (out) (*out)<<"Ok."<<std::endl;
	}
	
	/// Constructor.
	/** Constructor for @class pid_file_handler. PID file is NOT created. */
	pid_file_handler() : out(0) {};
	
	/// Destructor.
	/** Destructor for @class pid_file_handler. Automatically removes PID file if existing. */
	~pid_file_handler() { remove(); };
};

#endif
