/*
 * pattern.cpp
 *
 *  Created on: Sep 4, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "pattern.hpp"
#include "circuit.hpp"


void delete_gate_from_vec(vector <Gate*> &vec, Gate *g){
	vec.erase(std::remove(vec.begin(), vec.end(), g), vec.end());
}

GateStub* PatternProcessor::get_gate_stub(string in_str){
	GateStub *gs = new GateStub;
	vector<string> tokens;
	stringstream check1(in_str);
	string intermediate;
	while (getline(check1, intermediate, ',')) {
		tokens.push_back(intermediate);
	}
	gs->gate_type = tokens[0];
	int paramIdx;
	for (int i = 1; i < tokens.size(); i++){
		if(tokens[i].find("params") == string::npos){
			gs->qubit_order.push_back(stoi(tokens[i]));
		}else{
			paramIdx = i;
		}
	}
	gs->params = tokens[paramIdx];
	gs->params.erase(0, 7);
	gs->params.erase(gs->params.find(';'));
	return gs;
}

void PatternProcessor::load_one_pattern(ifstream &inFile){
	string tmp;
	inFile >> tmp;
	if(tmp.find("Pattern") == string::npos){
		return;
	}

	while(1){
		inFile >> tmp;
		if(tmp.find("Search") != string::npos) break;
	}

	string search_string;
	inFile >> search_string;
	cout << "Pattern for " << search_string << endl;
	inFile >> tmp; //Action
	inFile >> tmp; //Replace

	Pattern *P = new Pattern;
	P->gate_type = search_string;

	while(1){
		inFile >> tmp;
		if(tmp == "End") break;
		GateStub *gs = get_gate_stub(tmp);
		gs->print_stub();
	}
}

void PatternProcessor::load_patterns_from_file(string fname){
	ifstream inFile;
	inFile.open(fname.c_str());
	cout << "Reading " << fname << endl;
	while(!inFile.eof())
		this->load_one_pattern(inFile);
}

void PatternProcessor::load_test_patterns(){
	//load pattern for H gate
	Pattern *p1 = new Pattern();
	p1->id = 0;
	p1->name = "H Pattern";
	p1->gate_type = "H"; //search for H
	ReplaceAction *a1 = new ReplaceAction();
	a1->id = 0;
	a1->name = "H Implementation";
	a1->repl.push_back(get_gate_stub("RY,0,params:+2;"));
	a1->repl.push_back(get_gate_stub("RX,0,params:+1;"));
	p1->action = a1;

	patterns.push_back(p1);

	Pattern *p2 = new Pattern();
	p2->id = 1;
	p2->name = "CNOT Pattern";
	p2->gate_type = "CNOT"; //search for H
	ReplaceAction *a2 = new ReplaceAction();
	a2->id = 0;
	a2->name = "CNOT Implementation";
	a2->repl.push_back(get_gate_stub("RY,0,params:+2;"));
	a2->repl.push_back(get_gate_stub("XX,0,1,params:+4;"));
	a2->repl.push_back(get_gate_stub("RY,0,params:-2;"));
	a2->repl.push_back(get_gate_stub("RX,1,params:-2;"));
	a2->repl.push_back(get_gate_stub("RZ,0,params:-2;"));
	p2->action = a2;

	patterns.push_back(p2);

	Pattern *p3 = new Pattern();
	p3->id = 1;
	p3->name = "X Pattern";
	p3->gate_type = "X"; //search for H
	ReplaceAction *a3 = new ReplaceAction();
	a3->id = 2;
	a3->name = "X Implementation";
	a3->repl.push_back(get_gate_stub("RX,0,params:+1;"));
	p3->action = a3;

	patterns.push_back(p3);
}

void PatternProcessor::apply(Circuit *C){
	int i;
	for(i=0; i<patterns.size(); i++){
		Pattern *p = patterns[i];
		cout << "Applying " << p->name << endl;
		int s = C->gates.size();

		vector<Gate*> remove_gates;
		for(int j=0; j<s; j++){
			if(C->gates[j]->printStr == p->gate_type){
				cout << "found a gate " << p->gate_type << endl;
				vector<Gate*> target_gates = {C->gates[j]};
				p->action->act(C, &target_gates);
				remove_gates.push_back(C->gates[j]);
			}
		}

		for(int j=0; j<remove_gates.size(); j++){
			delete_gate_from_vec(C->gates, remove_gates[j]);
		}
	}
}

void ReplaceAction::act(Circuit *C, vector<Gate*> *in_gates){
	cout << "applying action" << endl;
	assert(in_gates->size() == 1);
	vector<Gate*> out_gates;
	Gate *in_g = (*in_gates)[0];
	if(repl.size() == 0) assert(0); //delete not allowed in this function
	for(int i=0; i<repl.size(); i++){
		Gate *tmp = Gate::create_new_gate(repl[i]->gate_type);
		cout << "created gate of type" << repl[i]->gate_type << endl;
		tmp->params = repl[i]->params;
		for(int j=0; j<tmp->nvars; j++){
			tmp->vars.push_back(in_g->vars[repl[i]->qubit_order[j]]);
		}
		out_gates.push_back(tmp);
	}
	//wire in dependencies to first gate
	Gate *first_gate = out_gates[0];
	first_gate->pred = in_g->pred;
	//wite out dependencies to last gate
	Gate *last_gate = out_gates[out_gates.size()-1];
	last_gate->succ = in_g->succ;
	//delete the node in_g
	//change connections of predecessors
	for(unsigned int i=0; i<in_g->pred.size(); i++){
		Gate *pred_g = in_g->pred[i];
		delete_gate_from_vec(pred_g->succ, in_g);
		pred_g->succ.push_back(first_gate);
	}
	for(unsigned int i=0; i<in_g->succ.size(); i++){
		Gate *succ_g = in_g->succ[i];
		delete_gate_from_vec(succ_g->pred, in_g);
		succ_g->pred.push_back(last_gate);
	}
	//delete_gate_from_vec(C->gates, in_g);
	for(unsigned int i=0; i<out_gates.size()-1; i++){
		Gate *current = out_gates[i];
		Gate *next = out_gates[i+1];
		current->succ.push_back(next);
		next->pred.push_back(current);
	}
	for(int i=0; i<out_gates.size(); i++){
		C->gates.push_back(out_gates[i]);
	}
}
