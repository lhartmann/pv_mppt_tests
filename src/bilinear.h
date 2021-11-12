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

#ifndef BILINEAR_H
#define BILINEAR_H

class bilinear_interpolator {
	typedef double (*builder_fcn)(double X, double Y, void *p);
	
	double x0, dx, y0, dy;
	int nx, ny;
	double **map;
	
	builder_fcn f;
	void *p;
	
	void freeMap();
	void buildMap();
	
	public:
	bilinear_interpolator() : nx(0), ny(0), map(0), f(0), x0(0), y0(0), dx(0), dy(0) {}
	~bilinear_interpolator() { freeMap(); }
	
	void setFunction(builder_fcn fcn, void *par) {
		freeMap();
		f = fcn;
		p = par;
		buildMap();
	}
	builder_fcn getFunction() const { return f; }
	void setX(double nx0, double nx1, int n);
	void setY(double ny0, double ny1, int n);
	double operator () (double x, double y) const;
	operator bool () const { return map; }
};

#endif
