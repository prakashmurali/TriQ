/*
 * targetter.hpp
 *
 *  Created on: Sep 14, 2018
 *      Author: prakash
 */

#ifndef TARGETTER_HPP_
#define TARGETTER_HPP_

#include "headers.hpp"
#include "circuit.hpp"
#include "backtrack.hpp"
#include "machine.hpp"

enum SwapDirection{
	ForwardSwap, ReverseSwap
};

enum LinkDirection{
	Predecessor, Successor
};

class Targetter{
public:
	Machine *M;
	Circuit *C;

	map<Qubit*, int> *initial_map;
	map<Qubit*, int> final_map;
	vector<Gate*> *gate_order;
	map<Gate*, BacktrackSolution*> bsol;

	unordered_map<std::type_index, string> gate_print_ibm;
	unordered_map<std::type_index, string> gate_print_rigetti;
	unordered_map<std::type_index, string> gate_print_tion;

	map<int, int> aspen1_map;
	map<int, int> aspen3_map;

	Targetter(Machine *m, Circuit *c, map<Qubit*, int> * initialMap, vector<Gate*> *gateOrder, map<Gate*, BacktrackSolution*> bSol){
		M = m;
		C = c;
		initial_map = initialMap;
		gate_order = gateOrder;
		bsol = bSol;
		init_gate_print_maps();
	}
	Circuit* map_and_insert_swap_operations();
	Circuit *map_to_trapped_ion();
	void implement_gate_for_trapped_ion(Gate *g, Circuit *I);

	void init_gate_print_maps();
	void map_single_qubit_gate(Gate *g, map<Qubit*, int> current_map);
	void map_two_qubit_gate_no_swap(Gate *g, map<Qubit*, int> current_map);
	void map_two_qubit_gate_to_swaps(Gate *g, BacktrackSolution *bs, Circuit *I, map<Qubit*, int> *current_map);
	void insert_swap_chain(vector<Gate*> &gate_seq, vector<int> *swap_seq, SwapDirection dir);
	void insert_two_qubit_op(vector<Gate*> &gate_seq, int ctrl, int targ);
	void copy_gate_connections_to_chain(Gate *g, vector<Gate*> &gate_seq, Circuit *I);
	void map_swaps_to_cnots(Gate *g, Circuit *I);
	void map_cnots_to_cz(Gate *g, Circuit *I);
	void implement_hardware_directions_for_cnots(Gate *g, Circuit *I);
	void print_code(Circuit *I, string fname);
	void print_one_qubit_gate(Gate *g, ofstream &out_file, int is_last_gate);
	void print_two_qubit_gate(Gate *g, ofstream &out_file, int is_last_gate);
	void print_header(ofstream &out_file);
	void print_measure_ops(ofstream &out_file);
	void print_footer(ofstream &out_file);
	void _external_link(Gate *g, vector<Gate*> gate_list, Circuit *I, int link_direction);
};

#endif /* TARGETTER_HPP_ */
