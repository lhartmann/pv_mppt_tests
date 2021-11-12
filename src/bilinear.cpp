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

#include "bilinear.h"

void bilinear_interpolator::buildMap() {
	if (!nx || !ny || !f) return;
	
	// Memory Allocation
	map = new double*[ny];
	if (!map) return; // Failed
	for (int i=0; i<ny; ++i) {
		map[i] = new double[nx];
		if (!map[i]) { // Failed
			while (i--) delete [] map[i];
			delete [] map;
			return;
		}
	}
	
	// Map calculation
	for (int iy=0; iy<ny; ++iy) {
		for (int ix=0; ix<nx; ++ix) {
			map[iy][ix] = f(x0+dx*ix, y0+dy*iy, p);
		}
	}
}

void bilinear_interpolator::freeMap() {
	if (!map) return;
	
	for (int i=0; i<ny; ++i) delete [] map[i];
	delete [] map;
}

void bilinear_interpolator::setX(double nx0, double nx1, int n) {
	freeMap(); // Clear previous map
	if (nx1 < nx0) { // Ensure nx0<nx1
		double d = nx0;
		nx0 = nx1;
		nx1 = d;
	}
	x0 = nx0;
	nx = n;
	dx = (nx1-nx0)/(nx-1);
	buildMap(); // Build new map, if possible
}
void bilinear_interpolator::setY(double ny0, double ny1, int n) {
	freeMap(); // Clear previous map
	if (ny1 < ny0) { // Ensure ny0<right
		double d = ny0;
		ny0 = ny1;
		ny1 = d;
	}
	y0 = ny0;
	ny = n;
	dy = (ny1-ny0)/(ny-1);
	buildMap(); // Build new map, if possible
}


double bilinear_interpolator::operator() (double x, double y) const {
	if (!map) return 0;

#ifndef DEBUG
	// Silent extrapolation.
	int          ix = int((x-x0)/dx);
	if (ix<0)    ix = 0;
	if (ix>nx-2) ix = nx-2;
	int          iy = int((y-y0)/dy);
	if (iy<0)    iy = 0;
	if (iy>ny-2) iy = ny-2;
#else
	// Warn of extrapolation.
	bool is_out_of_range = false;
	int ix = int((x-x0)/dx);
	if (ix<0) {
		ix = 0;
		is_out_of_range = true;
	}
	if (ix>nx-2) {
		ix = nx-2;
		is_out_of_range = true;
	}
	int iy = int((y-y0)/dy);
	if (iy<0) {
		iy = 0;
		is_out_of_range = true;
	}
	if (iy>ny-2) {
		iy = ny-2;
		is_out_of_range = true;
	}
	
	// Warn only once to prevent flood.
	static bool had_out_of_range = false;
	if (is_out_of_range && !had_out_of_range) {
		had_out_of_range = true;
		debug_say("Bilinear interpolation limits exceeded. Results may be inacurate.");
	}
#endif

	double zy0 = (x-(x0+ix*dx))*(map[iy  ][ix+1]-map[iy  ][ix])/dx + map[iy  ][ix];
	double zy1 = (x-(x0+ix*dx))*(map[iy+1][ix+1]-map[iy+1][ix])/dx + map[iy+1][ix];
	return (y-(y0+iy*dy))*(zy1-zy0)/dy + zy0;
}
