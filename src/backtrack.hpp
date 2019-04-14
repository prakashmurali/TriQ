/*
 * backtrack.hpp
 *
 *  Created on: Sep 10, 2018
 *      Author: prakash
 */

#ifndef BACKTRACK_HPP_
#define BACKTRACK_HPP_

#include "headers.hpp"
#include "gate.hpp"
#include "qubit.hpp"
#include "machine.hpp"
#include "circuit.hpp"

class Circuit;

enum swap_decision{
	CtrlRestore, TargRestore, CtrlNotRestore, TargNotRestore
};


class BacktrackNode{
public:
	int gate_idx;
	swap_decision decision;
	BacktrackNode* parent;
	vector<BacktrackNode*> child;
	std::map<Qubit*, int> qubit_map; //this is a post-perm i.e., the decision has been applied
	std::map<int, Qubit*> hw_map;
	float reliab;
	SwapInfo *s;
};


struct BacktrackSolution{
	std::map<Qubit*, int> pre_perm;
	std::map<Qubit*, int> post_perm;
	SwapInfo *sinfo;
	swap_decision decision;
	//BacktrackNode *bnode;
};


class Backtrack{
public:
	Circuit *C;
	Machine *M;
	vector<Gate*> gate_order;
	BacktrackNode *root;

	BacktrackNode *best_leaf;
	float best_reliab;
	std::map<Qubit*, int> *orig_mapping;
	vector<BacktrackNode*> all_nodes;

	Backtrack(Circuit *pC, Machine *pM){
		C = pC;
		M = pM;
	}
	~Backtrack(){

	}
	void init(std::map<Qubit*, int> *initial_map, vector<Gate*> *top_order);
	int is_leaf(BacktrackNode *n);
	void solve(BacktrackNode *n, int consider_measurements);
	BacktrackNode* create_child(BacktrackNode *parent, swap_decision option);
	void print_trace(BacktrackNode *n, BacktrackNode *root);
	void print_perm(map<Qubit*, int> *qmap);
	map<Gate*, BacktrackSolution*> get_solution(int apply_first_gate_check);
	int check_if_backtracking_required(BacktrackNode *n);
	void cleanup(){
		for(auto v : all_nodes){
			delete v;
		}
	}
};

/* take care of cnot empty chunks */
class BacktrackFiniteLookahead{
public:
	Circuit *C;
	Machine *M;
	vector<Gate*> input_gate_order;
	vector<Gate*> cx_order;
	map<Qubit*, int> qubit_mapping;
	map<Gate*, BacktrackSolution*> final_solution;
	int chunk_size;

	BacktrackFiniteLookahead(Circuit *pC, Machine *pM, vector<Gate*> GateOrder, std::map<Qubit*, int> InitialMapping, int chunkSize=20){
		C = pC;
		M = pM;
		input_gate_order = GateOrder;
		qubit_mapping = InitialMapping;
		chunk_size = chunkSize;
	}

	map<Gate*, BacktrackSolution*> solve();
};
#endif /* BACKTRACK_HPP_ */
