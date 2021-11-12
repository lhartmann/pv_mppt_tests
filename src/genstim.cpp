/***************************************************************************
 *   Copyright (C) 2010 by Lucas V. Hartmann <lucas.hartmann@gmail.com>    *
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

/***************************************************************************
 *   CAUTION: TEMPERATURE HANDLING                                         *
 *                                                                         *
 *   Temperatures are handled diferently by different classes. The tracker *
 *   class, mppt_mlam, uses temperatures in Celsius as they are easier to  *
 *   read. The PV generator model class, pvgen, uses temperatures in       *
 *   KELVIN in order to match the papers where the model was presented.    *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include "matv4.h"
#include "pvgen.h"
#include "pvgen_sc.h"
#include "regexpp.h"
#include "load_dat.h"
#include "save_dat.h"

//#define DEBUG
#include "debug.h"

using namespace std;

// 2d-array allocation: Returns a pointer if ok, or 0 on error.
double **new_array(int rows, int cols) {
	double **p = new double*[rows];
	if (!p) return 0;
	p[0] = new double[rows*cols];
	if (!p[0]) {
		delete [] p;
		return 0;
	}
	for (int i=1; i<rows; ++i) p[i] = p[0] + cols*i;
	return p;
}

// 2d-array freeing
void delete_array(double **p) {
	delete [] p[0];
	delete [] p;
}

// Main
int main(int argc, char *argv[]) {
	cout<<"<< Stimuli Converter >>"<<endl;
	cout<<"   by Lucas V. Hartmann."<<endl;
	string src, dst, sIsc, sVoc, sT;
	
	regularExpression reAssignIsc ("^Isc=([A-Za-z0-9:\\(\\)]+)$");
	regularExpression reAssignVoc ("^Voc=([A-Za-z0-9:\\(\\)]+)$");
	regularExpression reAssignT   ("^t=([A-Za-z0-9:\\(\\)]+)$");
	regularExpression reSourceFile("^in:(.*\\.dat)$");
	regularExpression reDestFile  ("^out:(.*\\.dat)$");
	
	for (int i=1; i<argc; ++i) {
		// Check variable assignments in the format
		//   Vin=V(V1:1)
		if (reAssignIsc(argv[i])) {
			sIsc = reAssignIsc[1];
		} else if (reAssignVoc(argv[i])) {
			sVoc = reAssignVoc[1];
		} else if (reAssignT(argv[i])) {
			sT = reAssignT[1];
		} else if (reSourceFile(argv[i])) {
			src = reSourceFile[1];
		} else if (reDestFile(argv[i])) {
			dst = reDestFile[1];
		} else {
			cout<<"Error parsing parameter \""<<argv[i]<<"\"."<<endl;
			return 1;
		}
	}
	if (src.empty() || dst.empty() || sIsc.empty() || sVoc.empty() || sT.empty()) {
		cout<<"Required parameter missing. Required are:"<<endl;
		cout<<"    in:file.dat"<<endl;
		cout<<"    out:file.dat"<<endl;
		cout<<"    t=variable"<<endl;
		cout<<"    Isc=variable"<<endl;
		cout<<"    Voc=variable"<<endl;
		return 1;
	}
	
	debug_say("From "<<src<<" to "<<dst<<".");
	debug_say("t   = "<<sT);
	debug_say("Voc = "<<sVoc);
	debug_say("Isc = "<<sIsc);
	
	// Load data file
	cout<<"Loading data file... "<<flush;
	map<string, vector<double> > data = load_dat(src);
	if (data.empty()) {
		cout<<"Error, file is empty or could not be opened."<<endl;
		return 1;
	}
	// Check for existence of all assignments
	map<string, vector<double> >::iterator datait;
	//   Isc
	datait = data.find(sIsc);
	if (datait == data.end()) {
		cout<<"Error, \"" + sIsc + "\" does not exist in the data file."<<endl;
		return 1;
	}
	vector<double> Isc = datait->second;
	//   Voc
	datait = data.find(sVoc);
	if (datait == data.end()) {
		cout<<"Error, \"" + sVoc + "\" does not exist in the data file."<<endl;
		return 1;
	}
	vector<double> Voc = datait->second;
	//   t
	datait = data.find(sT);
	if (datait == data.end()) {
		cout<<"Error, \"" + sT + "\" does not exist in the data file."<<endl;
		return 1;
	}
	vector<double> t = datait->second;
	cout<<data.size()<<" curves with "<<t.size()<<" data points loaded."<<endl;
	
	// Prepare generator model (use experimental model)
	cout<<"Preparing PV generator model... "<<flush;
	pvGenerator_sc gen;
	if (1) {
/*		// Generator model-number: KC130TM
		double Gr = 1000;
		double Tr = 25 + 273.16;
		// Pmax = 130W
		double Vmp = 17.6;
		double Imp = 7.39;
		double Voc = 21.9;
		double Isc = 8.02;
		int    Ns  = 36;
		
		double k  = 1.3806504e-23;
		double q  = 1.6021765e-19;
		double Vt = k*Tr/q;
		
		double Iphr = Isc;
		double mr   = (2*Vmp-Voc) / (Vt*log((Iphr-Imp)/Isc) + (Vt*Imp)/(Iphr-Imp));
		double Ior  = Isc / (exp(Voc/(mr*Vt)) - 1);
		double Rs   = (mr*Vt*log((Iphr-Imp-Ior)/Ior)-Vmp) / Imp;
		double Rsh  = 120; //1e300;*/
		
		// Experimental data
		int Ns = 36;
		double Iphr = 9.189668074289505;     // Fitted
		double Ior  = 4.217457490309515e-06; // Fitted
		double mr   = 52.434864865090077;    // Fitted
		double Rs   = 0.244345347877704;     // Fitted
		double Rsh  = 99.841834105583771;    // Fitted
		double Tr   = 273.16 + 39.2;         // Measured
		double Gr   = Iphr/8.02 * 1000;      // Estimated
		
		gen.setSeriesCellCount(Ns);
		gen.setSourceReference(Iphr, Gr);
		gen.setDiodeModel(Ior, Tr, mr);
		gen.setRs(Rs);
		gen.setRp(Rsh);
		
		debug_dump(Iphr);
		debug_dump(mr);
		debug_dump(Ior);
		debug_dump(Rs);
		debug_dump(Rp);
		debug_dump(Tr);
	}
	cout<<"Ok."<<endl;
	
	// Change Voc and Isc into G and T
	cout<<"Conversion ongoing... "<<flush;
	vector<double> G(t.size()), T(t.size());
	double oG=1000, oT=273.16;
	bool bad = false;
	for (int i=0; i<t.size(); ++i) {
		double dt   = t[i];
		double dVoc = Voc[i];
		double dIsc = Isc[i];
		
		int iter = 1000;
		do {
			gen.setInsolation(oG);
			gen.setTemperature(oT);
			double eVoc = dVoc - gen.V(0, dVoc);
			double eIsc = dIsc - gen.I(0, dIsc);
			if (isnan(eVoc) || isnan(eIsc)) {
				cout<<"Error, pvgen is not working properly."<<endl;
				
				debug_dump(iter);;
				debug_dump(dt);
				debug_dump(dVoc);
				debug_dump(dIsc);
				debug_dump(oG);;
				debug_dump(oT);;
				debug_dump(eVoc);
				debug_dump(eIsc);
				return 1;
			}
			double dG   = +100.0 * eIsc;
			double dT   =  -10.0 * eVoc;
			oG += dG;
			oT += dT;
			debug_say("i="<<i<<", Iter="<<iter<<", eIsc="<<eIsc<<", eVoc="<<eVoc<<".");
			if (fabs(dG) < 1e-5 && fabs(dT) < 1e-5) break;
		} while (--iter);
		if (!iter && !bad) {
			cout<<"Results may not be good... "<<flush;
			bad = true;
		}
		
		G[i] = oG;
		T[i] = oT;
	}
	cout<<"Done."<<endl;
	
	// Save everything
	cout<<"Saving data... "<<flush;
	data.clear();
	data["Time"] = t;
	data["G"]    = G;
	data["T"]    = T;
	data["Voc"]  = Voc;
	data["Isc"]  = Isc;
	if (save_dat(dst, data)) {
		cout<<"Error."<<endl;
		return 1;
	}
	cout<<"OK."<<endl;
	
	return false;
}
