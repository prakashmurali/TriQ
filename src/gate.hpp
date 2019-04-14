/*
 * gate.hpp
 *
 *  Created on: Sep 2, 2018
 *      Author: prakash
 */

#ifndef GATE_HPP_
#define GATE_HPP_

#include "headers.hpp"
#include "qubit.hpp"

class Gate{
public:
	static int id_gen;
	int id;
	int nvars;
	string printStr;
	std::vector <Qubit*> vars;
	std::vector <Gate*> pred;
	std::vector <Gate*> succ;
	string params;

	double angle;
	double theta;
	double phi;
	double lambda;

	virtual void operate() = 0;
	virtual ~Gate(){};
	Gate(){
		id = 0;
		params = "";
	}
	void print();
	static Gate* create_new_gate(string type);
};

class SingleQubitGate : public Gate{
public:
	SingleQubitGate(){
		nvars = 1;
	}
};

class TwoQubitGate : public Gate{
public:
	TwoQubitGate(){
		nvars = 2;
	}
};

class H : public SingleQubitGate{
public:
	H(){
		printStr = "H";
	}
	void operate(){
		std::cout << "H gate\n";
	}
};

class X : public SingleQubitGate{
public:
	X(){
		printStr = "X";
	}
	void operate(){
			std::cout << "X gate\n";
	}
};

class Y : public SingleQubitGate{
public:
	Y(){
		printStr = "Y";
	}
	void operate(){
			std::cout << "Y gate\n";
	}
};

class Z : public SingleQubitGate{
public:
	Z(){
		printStr = "Z";
	}
	void operate(){
			std::cout << "Z gate\n";
	}
};

class T : public SingleQubitGate{
public:
	T(){
		printStr = "T";
	}
	void operate(){
			std::cout << "T gate\n";
	}
};

class Tdag : public SingleQubitGate{
public:
	Tdag(){
		printStr = "Tdag";
	}
	void operate(){
			std::cout << "Tdag gate\n";
	}
};

class S : public SingleQubitGate{
public:
	S(){
		printStr = "S";
	}
	void operate(){
			std::cout << "S gate\n";
	}
};

class Sdag : public SingleQubitGate{
public:
	Sdag(){
		printStr = "Sdag";
	}
	void operate(){
			std::cout << "Sdag gate\n";
	}
};

class U1 : public SingleQubitGate{
public:
	U1(){
		printStr = "U1";
	}
	void operate(){
		cout << "U1(" << lambda << ")\n";
	}
};

class U2 : public SingleQubitGate{
public:
	U2(){
		printStr = "U2";
	}
	void operate(){
		cout << "U2(" << phi << "," << lambda << ")\n";
	}
};

class U3 : public SingleQubitGate{
public:
	U3(){
		printStr = "U3";
	}
	void operate(){
		cout << "U3(" << theta << "," << phi << "," << lambda << ")\n";
	}
};


class RX : public SingleQubitGate{
public:
	RX(){
		printStr = "RX";
	}
	void operate(){
		cout << "RX" << params.at(0) << vars[0]->id << params.at(1) << endl;
	}
};

class RY : public SingleQubitGate{
public:
	RY(){
		printStr = "RY";
	}
	void operate(){
		cout << "RY" << params.at(0) << vars[0]->id << params.at(1) << endl;

	}
};

class RZ : public SingleQubitGate{
public:
	RZ(){
		printStr = "RZ";
	}
	void operate(){
		cout << "RZ" << params.at(0) << vars[0]->id << params.at(1) << endl;

	}
};


class MeasZ : public SingleQubitGate{
public:
	MeasZ(){
		printStr = "MeasZ";
	}
	void operate(){
		cout << "MeasZ " << vars[0]->id  << endl;

	}
};

class CNOT : public TwoQubitGate{
public:
	CNOT(){
		printStr = "CNOT";
	}
	void operate(){
		std::cout << "CNOT gate\n";
	}
};

class XX : public TwoQubitGate{
public:
	XX(){
		printStr = "XX";
	}
	void operate(){
		cout << "XX" << params.at(0) << vars[0]->id << vars[1]->id << params.at(1) << endl;
	}
};

class SWAP : public TwoQubitGate{
public:
	SWAP(){
		printStr = "SWAP";
	}
	void operate(){
		cout << "SWAP" << vars[0]->id << vars[1]->id << endl;
	}
};

class CZ : public TwoQubitGate{
public:
	CZ(){
		printStr = "CZ";
	}
	void operate(){
		cout << "CZ" << vars[0]->id << vars[1]->id << endl;
	}
};

#endif /* GATE_HPP_ */
