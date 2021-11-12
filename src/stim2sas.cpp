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
#include "pvgen_models.h"
#include "pvgen_mpp_I.h"
#include "pvgen_setup.h"
#include "pvgen_nominal_model.h"
#include "regexpp.h"
#include "load_dat.h"
#include "save_dat.h"
#include "progressbar.h"

#define DEBUG
#include "debug.h"

using namespace std;

double pvgen_mpp_V(pvGenerator &r, double Vl, double Vh, double eMax);

// Main
int main(int argc, char *argv[]) {
	cout<<"<< Stimuli 2 SAS Converter >>"<<endl;
	cout<<"   by Lucas V. Hartmann."<<endl;
	string src, dst, gen, sG="G", sT="T";
	unsigned iNp=1, iNs=1;
	
	regularExpression reAssignG   ("^G=([A-Za-z0-9:\\(\\)]+)$");
	regularExpression reAssignT   ("^T=([A-Za-z0-9:\\(\\)]+)$");
	regularExpression reAssignNp  ("^Np=([0-9]+)$");
	regularExpression reAssignNs  ("^Ns=([0-9]+)$");
	regularExpression reSourceFile("^in:(.*\\.dat)$");
	regularExpression reDestFile  ("^out:(.*\\.dat)$");
	regularExpression reGenerator ("^gen:([A-Za-z0-9]+)$");
	
	for (int i=1; i<argc; ++i) {
		// Check variable assignments in the format
		//   Vin=V(V1:1)
		if (reAssignG(argv[i])) {
			sG = reAssignG[1];
		} else if (reAssignT(argv[i])) {
			sT = reAssignT[1];
		} else if (reAssignNs(argv[i])) {
			iNs = atoi(reAssignNs[1].c_str());
		} else if (reAssignNs(argv[i])) {
			iNp = atoi(reAssignNp[1].c_str());
		} else if (reSourceFile(argv[i])) {
			src = reSourceFile[1];
		} else if (reDestFile(argv[i])) {
			dst = reDestFile[1];
		} else if (reGenerator(argv[i])) {
			gen = reGenerator[1];
		} else {
			cout<<"Error parsing parameter \""<<argv[i]<<"\"."<<endl;
			return 1;
		}
	}
	if (src.empty() || dst.empty() || gen.empty()) {
		cout<<"Required parameter missing. Available options are:"<<endl;
		cout<<"    in:file.dat    Input file name."<<endl;
		cout<<"    out:file.dat   Output file name."<<endl;
		cout<<"    gen:name       Generator model."<<endl;
		cout<<"    G=variable     Variable name on input dat file from which to read solar irradiation. Default: G=G"<<endl;
		cout<<"    T=variable     Variable name on input dat file from which to read temperature. Default: T=T"<<endl;
		cout<<"    Ns=number      Number of generators in series. Default: Ns=1"<<endl;
		cout<<"    Np=number      Number of generators in parallel. Default: Np=1"<<endl;
		return 1;
	}
	
	debug_say("From "<<src<<" to "<<dst<<".");
	debug_say("gen = "<<gen);
	debug_say("G = "<<sG);
	debug_say("T = "<<sT);
	debug_say("Ns = "<<iNs);
	debug_say("Np = "<<iNp);
	
	// Load data file
	cout<<"Loading data file... "<<flush;
	map<string, vector<double> > data = load_dat(src);
	if (data.empty()) {
		cout<<"Error, file is empty or could not be opened."<<endl;
		return 1;
	}
	// Check for existence of all assignments
	map<string, vector<double> >::iterator datait;
	//   G
	datait = data.find(sG);
	if (datait == data.end()) {
		cout<<"Error, \"" + sG + "\" does not exist in the data file."<<endl;
		return 1;
	}
	vector<double> G = datait->second;
	//   T
	datait = data.find(sT);
	if (datait == data.end()) {
		cout<<"Error, \"" + sT + "\" does not exist in the data file."<<endl;
		return 1;
	}
	vector<double> T = datait->second;
	cout<<data.size()<<" curves with "<<G.size()<<" data points loaded."<<endl;
	
	// Prepare generator model (use experimental model)
	cout<<"Preparing PV generator model... "<<flush;
	pvGenerator_sc g;
	if (1) {
		const pvGenerator::parameters_t *param;
		param = generator_by_name(gen.c_str());
		if (!param) {
			cout<<"Error: Unknown."<<endl;
			return 1;
		}
		
		pvGenerator::model_parameters_t m;
		m = pvgen_nominal_model(param->nameplate);
		pvgen_setup(g, m);
		
		debug_dump(m.Ns);
		debug_dump(m.Iph);
		debug_dump(m.m);
		debug_dump(m.I0);
		debug_dump(m.Rs);
		debug_dump(m.Rp);
		debug_dump(m.T);
	}
	cout<<"Ok."<<endl;
	
	// Change (G,T) into (Voc, Isc, Vmp, Imp)
	vector<double> Voc(G.size()), Isc(G.size()), Vmp(G.size()), Imp(G.size());
	progressBar pgb("Conversion ongoing... ",G.size());
	vector<double> time;
	for (int i=0; i<G.size(); ++i) {
		time.push_back(2*i);
		cout<<pgb++;
		g.setInsolation(G[i]);
		g.setTemperature(T[i]+273.16);
		
		Voc[i] = g.V(0,20) * iNs;
		Isc[i] = g.I(0)    * iNp;
		
		double Vl=0, Vh=Voc[i];
		do {
			double Vm1 = (2*Vl+Vh)/3;
			double Im1 = g.I(Vm1);
			double Vm2 = (Vl+2*Vh)/3;
			double Im2 = g.I(Vm2);
			if (Vm1*Im1 > Vm2*Im2) Vh = Vm2;
			else Vl = Vm1;
		} while (Vh-Vl > 1e-6);
		
		Vmp[i] = (Vl+Vh)/2   * iNs;
		Imp[i] = g.I(Vmp[i]) * iNp;
	}
	cout<<pgb()<<endl;
	
	// Save everything
	cout<<"Saving data... "<<flush;
	data.clear();
	data["Time"] = time;
	data["G"]    = G;
	data["T"]    = T;
	data["Voc"]  = Voc;
	data["Isc"]  = Isc;
	data["Vmp"]  = Vmp;
	data["Imp"]  = Imp;
	if (save_dat(dst, data)) {
		cout<<"Error."<<endl;
		return 1;
	}
	cout<<"OK."<<endl;
	
	return false;
}

typedef double (*bisect_max_eval_fcn)(double, void*);
static double bisect_max(double x0, double x1, double ex, bisect_max_eval_fcn eval, void *p) {
	double X[5] = {
		x0,
		0,
		(x0+x1)/2,
		0,
		x1
	};
	double Y[5] = {
		eval(X[0],p),
		0,
		eval(X[2],p),
		0,
		eval(X[4],p)
	};
	
	while (fabs(X[4]-X[0]) > ex) {
		X[1] = (X[0]+X[2])/2;
		Y[1] = eval(X[1],p);
		X[3] = (X[2]+X[4])/2;
		Y[3] = eval(X[3],p);
		if (Y[1]>Y[2] && Y[1]>Y[3]) { // Y1 is closer to max
			X[4]=X[2]; Y[4]=Y[2];     //   Move 2 -> 4
			X[2]=X[1]; Y[2]=Y[1];     //   Move 1 -> 2
		} else if (Y[2]>Y[3]) {       // PY is closer to max
			X[0]=X[1]; Y[0]=Y[1];     //   Move 1 -> 0
			X[4]=X[3]; Y[4]=Y[3];     //   Move 3 -> 4
		} else {                      // Y3 is closer to max
			X[0]=X[2]; Y[0]=Y[2];     //   Move 2 -> 0
			X[2]=X[3]; Y[2]=Y[3];     //   Move 3 -> 2
		}
	}
	
	return X[2];
}

static double pvgen_mpp_V_aux(double V, void *r) {
	return ((pvGenerator*)r)->I(V);
}

double pvgen_mpp_V(pvGenerator &r, double Vl, double Vh, double eMax) {
	return bisect_max(Vl, Vh, eMax, &pvgen_mpp_V_aux, &r);
}
