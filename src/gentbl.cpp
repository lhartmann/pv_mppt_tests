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
#include "mppt_mlam.h"
#include "pvgen.h"
#include "pvgen_sc.h"

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
	cout<<"<< MPPT error table calculations >>"<<endl;
	cout<<"   by Lucas V. Hartmann."<<endl;
	
	// Prepare MPPT tracker (Use nameplate-based model)
	mppt_mlam track;
	cout<<"Preparing MPP tracker (MLAM)... "<<flush;
	if (1) {
		// Generator model-number: KC130TM
		double Gr = 1000;
		double Tr = 25;
		// Pmax = 130W
		double Vmp = 17.6;
		double Imp = 7.39;
		double Voc = 21.9;
		double Isc = 8.02;
		int    Ns  = 36;
		
		double k  = 1.3806504e-23;
		double q  = 1.6021765e-19;
		double Vt = k*(Tr+273.16)/q;
		
		double Iphr = Isc;
		double mr   = (2*Vmp-Voc) / (Vt*log((Iphr-Imp)/Isc) + (Vt*Imp)/(Iphr-Imp));
		double Ior  = Isc / (exp(Voc/(mr*Vt)) - 1);
		double Rs   = (mr*Vt*log((Iphr-Imp-Ior)/Ior)-Vmp) / Imp;
		double Rp   = 1e300;
		
		track.Iphr = Iphr;
		track.mr   = mr;
		track.Ior  = Ior;
		track.Rs   = Rs;
		track.Rp   = Rp;
		track.Tr   = Tr;
		track.Ns   = Ns;
		
		debug_dump(track.Iphr);
		debug_dump(track.mr);
		debug_dump(track.Ior);
		debug_dump(track.Rs);
		debug_dump(track.Rp);
		debug_dump(track.Tr);
		
		// Configure lookup-table
		track.setMap(
			0, Iphr*1.2, 128, // minI, maxI, nI
			25, 100, 4        // minT, maxT, nT (CELSIUS)
		);
		if (!track) {
			cout<<"Error."<<endl;
			cerr<<"Error: Failed to configure MPPT trackers."<<endl;
			return 1;
		}
	}
	cout<<"Ok."<<endl;
	
	// Prepare generator model (use experimental model)
	cout<<"Preparing PV generator model... "<<flush;
	pvGenerator_sc gen;
	if (1) {
		// Should use experimental data here.
		double Gr    = 1000;
		double Tr    = track.Tr;
		double Iphr  = track.Iphr * 0.9;
		double mr    = track.mr   * 1.1;
		double Ior   = track.Ior  * 1.1;
		double Rs    = track.Rs   + 0.3;
		double Rp    = 120; // track.Rp;
		double Ns    = track.Ns;
		
		gen.setSeriesCellCount(Ns);
		gen.setSourceReference(Iphr, Gr);
		gen.setDiodeModel(Ior, Tr+273.16, mr); // Temperature in KELVIN here
		gen.setRs(Rs);
		gen.setRp(Rp);
		
		debug_dump(Iphr);
		debug_dump(mr);
		debug_dump(Ior);
		debug_dump(Rs);
		debug_dump(Rp);
		debug_dump(Tr);
	}
	cout<<"Ok."<<endl;
	
	// Settings for the error tables (may differ from the lookup table)
	int    nG = 181;
	double G0 = 100;
	double G1 = 1000;
	int    nT = 101;
	double T0 = 0;
	double T1 = 100;
	
	// Get some memory
	cout<<"Allocating output tables... "<<flush;
	double  *sv_G   = new double [nG];
	double  *sv_T   = new double [nT];
	double **sv_Vmp = new_array(nT,nG);
	double **sv_V   = new_array(nT,nG);
	double **sv_Pmp = new_array(nT,nG);
	double **sv_P   = new_array(nT,nG);
	if (!sv_G || !sv_T || !sv_Vmp || !sv_V || !sv_Pmp || !sv_P ) {
		cout<<"Error."<<endl;
		cerr<<"Out of memory."<<endl;
		return 1;
	}
	cout<<"Ok."<<endl;
	
	// Calculate error tables
	cout<<"Calculating error tables... "<<flush;
	for (int iG = 0; iG < nG; ++iG) {
		double G = G0 + (G1-G0) * iG / (nG-1);
		sv_G[iG] = G;
	}
	
	for (int iT = 0; iT < nT; ++iT) {
		double T = T0 + (T1-T0) * iT / (nT-1);
		sv_T[iT] = T;
		
		for (int iG = 0; iG < nG; ++iG) {
			double G = sv_G[iG];
			
			// Set operating condition
			gen.setTemperature(T + 273.16);
			gen.setInsolation(G);
			
			// Use experimental model to get the true-MPP
			double Vmp = 0;
			for (int i=0; i<10; ++i) {
				Vmp = gen.Vmp(gen.I(Vmp));
			}
			sv_Vmp[iT][iG] = Vmp;
			
			// Use tracker to see where it goes
			double V = 0;
			for (int i=0; i<10; ++i) {
				V = track(gen.I(V), T);
			}
			sv_V[iT][iG] = V;
		}
	}
	cout<<"Ok."<<endl;
	
	// Save everything
	cout<<"Writing output files... "<<flush;
	ofstream out("result.mat", ios::out|ios::binary|ios::trunc);
	if (!out) {
		cout<<"Error."<<endl;
		cerr<<"Failed to save result.mat."<<endl;
		return 1;
	}
	matv4_add(out, "G",   sv_G,   nG);
	matv4_add(out, "T",   sv_T,   nT);
	matv4_add(out, "Vmp", sv_Vmp, nT, nG);
	matv4_add(out, "V",   sv_V,   nT, nG);
	cout<<"Ok."<<endl;
	
	return false;
}
