/*
 * circuit.hpp
 *
 *  Created on: Sep 2, 2018
 *      Author: prakash
 */

#ifndef CIRCUIT_HPP_
#define CIRCUIT_HPP_

#include "headers.hpp"
#include "qubit.hpp"
#include "gate.hpp"
#include "machine.hpp"
#include "backtrack.hpp"

class BacktrackSolution;

class Moment{
public:
	int timestamp;
	vector <Gate*> gates;
	void operate();
	void print_moment();
	void generate_code();
};

class Circuit{
public:
	vector <Qubit*> qubits;
	vector <Gate*> gates;
	vector <Moment*> moments;

	vector <pair<Gate*, Gate*> > sched_depends;

	void load_from_file(string fname);
	void load_from_openqasm(string fname);
	void create_moments();
	void duplicate_circuit(Circuit *C, map<Gate*, Gate*> *old2new);

	Circuit *rewrite_with_mapping(std::map<Qubit*, int> *qubit_map, Machine *M);
	vector<Gate*> *topological_ordering();
	set<Gate*> descendants(Gate *g);
	set<Gate*> ancestors(Gate *g);
	set<Gate*> can_overlap_set(Gate *g);
	set<Gate*> require_dependency_edge(Gate *g, vector<Gate*>* gate_order);
	void enforce_topological_ordering(vector<Gate*> *gate_order);
	void add_scheduling_dependency(vector<Gate*> *gate_order, map<Gate*, BacktrackSolution*> bsol);
	void _add_dependency_for_gate(Gate *g, BacktrackSolution *bs);
	void optimize_1q_gate_sequence();
	void replace_with_chain(vector <Gate*> in_gates, vector <Gate*> out_gates);

	void print_sched_depends();
	int check_use_qubit(Gate *g, Qubit *q);
	int is_descendant(Gate *g1, Gate *g2);
	void print_dot_file(string fname);

	void print_gates();
	void print_moments();
	void generate_code();
	~Circuit();
};
#endif /* CIRCUIT_HPP_ */
