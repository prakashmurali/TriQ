/*
 * optimize_1q.hpp
 *
 *  Created on: Sep 28, 2018
 *      Author: prakash
 */

#ifndef OPTIMIZE_1Q_HPP_
#define OPTIMIZE_1Q_HPP_

#include "headers.hpp"
#include "circuit.hpp"
#include "gate.hpp"

class URotate{
public:
	URotate(){};
	~URotate(){};
	int type;
	double theta;
	double phi;
	double lambda;
	void print(){
		std::setprecision(10);
		if(type == 1)
			cout << "U1(" << lambda << ")\n";
		else if(type == 2)
			cout << "U2(" << phi << "," << lambda << ")\n";
		else if(type == 3)
			cout << "U3(" << theta << "," << phi << "," << lambda << ")\n";
		else assert(0);
	}
};

class Optimize1QGates{
public:
	vector <Gate*> in_gates;
	Optimize1QGates(vector <Gate*> gateSeq){
		in_gates = gateSeq;
		decompose();
	}
	vector <URotate*> rot_seq;
	vector <Gate*> out_gates;
	void decompose();
	void optimize();
	vector <Gate*> get_optimized_gates();
	void merge_u3_gates(URotate* g1, URotate* g2);
	void yzy_to_zyz(double xi, double theta1, double theta2, double& thetap, double& phip, double& lambdap);

	URotate *create_U1_gate(double lambda);
	URotate *create_U2_gate(double phi, double lambda);
	URotate *create_U3_gate(double theta, double phi, double lambda);
};

class OptimizeSingleQubitOps{
public:
	Circuit *C;
	map<Gate*, vector<Gate*> > runs;
	OptimizeSingleQubitOps(Circuit *c){
		C = c;
	}
	void optimize();
	void extract_runs();
	Circuit* test_optimize();
	vector<Gate*> get_optimized_layer(vector <Gate*> gate_list);
};

#endif /* OPTIMIZE_1Q_HPP_ */
