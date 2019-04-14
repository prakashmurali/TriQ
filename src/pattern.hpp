/*
 * pattern.hpp
 *
 *  Created on: Sep 4, 2018
 *      Author: prakash
 */

#ifndef PATTERN_HPP_
#define PATTERN_HPP_

#include "headers.hpp"
#include "circuit.hpp"

class GateStub{
public:
	string gate_type;
	vector<int> qubit_order;
	string params;
	void print_stub(){
		cout << "Type:" << gate_type << " Qubit order: ";
		for(int i=0; i<qubit_order.size(); i++) cout << qubit_order[i] << " ";
		cout << params << endl;
	}
};


class Action{
public:
	int id;
	string name;
	virtual void act(Circuit *C, vector<Gate*> *in_gates) = 0;
};

class ReplaceAction : public Action{
public:
	vector <GateStub*> repl;
	void act(Circuit *C, vector<Gate*> *in_gates);

};

class Pattern{
public:
	int id;
	string name;
	string gate_type; //TODO patterns can be of many types: gate strings, gate motifs etc.
	Action *action;
	Pattern(){}
};


class PatternProcessor{
public:
	vector <Pattern*> patterns;
	void load_test_patterns();
	void load_patterns_from_file(string fname);
	void load_one_pattern(ifstream &inFile);
	void apply(Circuit *C);
	GateStub* get_gate_stub(string in_str);
};
#endif /* PATTERN_HPP_ */
