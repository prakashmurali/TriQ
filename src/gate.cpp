/*
 * gate.cpp
 *
 *  Created on: Sep 3, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "gate.hpp"

int Gate::id_gen = 0;

Gate* Gate::create_new_gate(string type){
	Gate *pGate;
	if(type == "H"){
		pGate = new H();
	}else if(type == "CNOT"){
		pGate = new CNOT();
	}else if(type == "MeasZ"){
		pGate = new MeasZ();
	}else if(type == "X"){
		pGate = new X();
	}else if(type == "Y"){
		pGate = new Y();
	}else if(type == "Z"){
		pGate = new Z();
	}else if(type == "T"){
		pGate = new T();
	}else if(type == "Tdag"){
		pGate = new Tdag();
	}else if(type == "S"){
		pGate = new S();
	}else if(type == "Sdag"){
		pGate = new Sdag();
	}else if(type == "RX"){
		pGate = new RX();
	}else if(type == "RY"){
		pGate = new RY();
	}else if(type == "RZ"){
		pGate = new RZ();
	}else if(type == "XX"){
		pGate = new XX();
	}else if(type == "SWAP"){
		pGate = new SWAP();
	}else if(type == "U1"){
		pGate = new U1();
	}else if(type == "U2"){
		pGate = new U2();
	}else if(type == "U3"){
		pGate = new U3();
	}else if(type == "CZ"){
		pGate = new CZ();
	}
	else{
		assert(0);
	}
	pGate->id = id_gen;
	id_gen++;
	return pGate;
}
void Gate::print() {
	cout << id << " " << printStr << " " << "Qubits: ";
	for (int i = 0; i < nvars; i++)
		cout << vars[i]->id << " ";
	cout << "Depends on: ";
	for (unsigned int i = 0; i < pred.size(); i++)
		cout << pred[i]->id << " ";
	cout << "Succeeded by: ";

	for (unsigned int i = 0; i < succ.size(); i++)
		cout << succ[i]->id << " ";
	cout << params;

	cout << endl;
	if (printStr == "U1") {
		cout << "lambda=" << lambda << "\n";
	}
	if (printStr == "U2" || printStr == "U3") {
		cout << "theta=" << theta << "\n";
		cout << "phi=" << phi << "\n";
		cout << "lambda=" << lambda << "\n";

	}
}
