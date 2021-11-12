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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include "straux.h"
#include <string>
#include <vector>
#include "matv4.h"
#include "progressbar.h"

// #define DEBUG
#include <debug.h>

using namespace std;

vector<string> remove_empty_entries(const vector<string> &in) {
	vector<string> out;
	for (int i=0; i<in.size(); ++i) {
		if (in[i] == "") continue;
		if (in[i] == "\r") continue;
		if (in[i] == "\n") continue;
		out.push_back(in[i]);
	}
	return out;
}

string fixname(string in) {
	string out;
	for (int i=0; i<in.length(); ++i) {
		if (isalnum(in[i])) out += in[i];
		else out += '_';
	}
	return out;
}

int main(int argc, char *argv[]) {
	string src, dst;
	if (argc == 2) {
		src = argv[1];
		dst = string(src, 0, src.find_last_of('.')) + ".mat";
	} else if (argc == 3) {
		src = argv[1];
		dst = argv[2];
	} else {
		cout<<"Use:"<<endl;
		cout<<"  dat2mat <file.dat> [file.mat]"<<endl;
		return 1;
	}
	debug_say("From "<<src<<" to "<<dst<<".");
	
	ifstream in(src.c_str(), ios::in);
	if (!in) {
		cout<<"Could not open specified input file."<<endl;
		return 1;
	}
	
	ofstream out(dst.c_str(), ios::out|ios::trunc|ios::binary);
	if (!out) {
		cout<<"Could not open specified output file."<<endl;
		return 1;
	}
	
	debug_say("File access is ok.");
	
	string line;
	getline(in, line);
	vector<string> var = remove_empty_entries(strSplit(line, ' '));
	int nv = var.size();
	
	debug_say("Number of variables: "<<nv);
	
	int npt = 0;
	while (getline(in, line)) ++npt;
	in.clear();
	in.seekg(0, ios::beg);
	
	debug_say("Number of rows: "<<npt);
	
	vector<unsigned long> hoff;
	vector<unsigned long> doff;
	hoff.resize(nv);
	doff.resize(nv);
	for (int v=0; v<nv; ++v) {
		debug_say("Variable "<<v<<" is \""<<var[v]<<"\".");
		hoff[v] = matv4_header(out, fixname(var[v]).c_str(), matv4_type(double()), false, npt, 1);
		doff[v] = out.tellp();
		double d = NAN;
//		for (int i=0; i<npt; ++i) {
//			out.write((char*)&d, sizeof(double));
//		}
		out.seekp(npt*sizeof(double), ios::cur);
	}
	
	getline(in, line); // Discard names
	progressBar pgb("dat2mat ", npt);
	for (int i=0; i<npt; ++i) {
		cout<<pgb(i);
		getline(in, line);
		vector<string> data = remove_empty_entries(strSplit(line, ' '));
		if (data.size() != nv) {
			cerr<<"Row "<<(i+1)<<" does not match the expected number of variables."<<endl;
			cerr<<"  Expected="<<nv<<", received="<<data.size()<<"."<<endl;
			cerr<<"  Line=\""<<line<<"\"."<<endl;
			return 1;
		}
		for (int v=0; v<nv; ++v) {
			double d = atof(data[v].c_str());
			out.seekp(doff[v]);
			out.write((char*)&d, sizeof(double));
			doff[v] = out.tellp();
		}
	}
	cout<<pgb();
	
	return 0;
}
