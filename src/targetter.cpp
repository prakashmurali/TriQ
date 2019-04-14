/*
 * targetter.cpp
 *
 *  Created on: Sep 14, 2018
 *      Author: prakash
 */

#include "targetter.hpp"

void print_gate_seq(vector<Gate*> *v){
	for(auto tmp : *v){
		tmp->print();
	}
}
void Targetter::map_single_qubit_gate(Gate *g, map<Qubit*, int> current_map){
	assert(g->vars.size() == 1);
	int hw_qubit_id = current_map[g->vars[0]];
	Qubit *hwq = M->qubits[hw_qubit_id];
	g->vars[0] = hwq;
}

void Targetter::map_two_qubit_gate_no_swap(Gate *g, map<Qubit*, int> current_map){
	assert(g->vars.size() == 2);
	int hw_qubit_id1 = current_map[g->vars[0]];
	int hw_qubit_id2 = current_map[g->vars[1]];
	Qubit *hwq1 = M->qubits[hw_qubit_id1];
	Qubit *hwq2 = M->qubits[hw_qubit_id2];
	g->vars[0] = hwq1;
	g->vars[1] = hwq2;
}

void Targetter::insert_swap_chain(vector<Gate*> &gate_seq,
		vector<int> *swap_seq, SwapDirection dir) {
	if(swap_seq->size() <= 1) return;
	if (dir == ForwardSwap) {
		for (unsigned int i = 0; i < swap_seq->size()-1; i++) {
			Gate *g = Gate::create_new_gate("SWAP");
			g->vars.push_back(M->qubits[swap_seq->at(i)]);
			g->vars.push_back(M->qubits[swap_seq->at(i + 1)]);
			gate_seq.push_back(g);
		}
	} else if (dir == ReverseSwap) {
		for (unsigned int i = swap_seq->size()-1; i > 0; i--) {
			Gate *g = Gate::create_new_gate("SWAP");
			g->vars.push_back(M->qubits[swap_seq->at(i)]);
			g->vars.push_back(M->qubits[swap_seq->at(i - 1)]);
			gate_seq.push_back(g);
		}
	}
}

void Targetter::insert_two_qubit_op(vector<Gate*> &gate_seq, int ctrl, int targ){
	Gate *g = Gate::create_new_gate("CNOT");
	g->vars.push_back(M->qubits[ctrl]);
	g->vars.push_back(M->qubits[targ]);
	gate_seq.push_back(g);

}

void Targetter::copy_gate_connections_to_chain(Gate *g, vector<Gate*> &gate_seq, Circuit *I){
	assert(gate_seq.size() >= 1);
	Gate *front = gate_seq.at(0);
	Gate *back = gate_seq.at(gate_seq.size()-1);
	//add front back connections
	front->pred = g->pred;
	for(auto d : g->pred){
		d->succ.erase(remove(d->succ.begin(), d->succ.end(), g), d->succ.end());
		d->succ.push_back(front);
	}
	back->succ = g->succ;
	for(auto d : g->succ){
		d->pred.erase(remove(d->pred.begin(), d->pred.end(), g), d->pred.end());
		d->pred.push_back(back);
	}
	//interlink each one
	Gate *g1, *g2;
	for(int i=0; i<int(gate_seq.size())-1; i++){
		g1 = gate_seq.at(i);
		g2 = gate_seq.at(i+1);
		g1->succ.push_back(g2);
		g2->pred.push_back(g1);
	}
	//how to handle sched_depends?
	//For the front gate - check if g is the target of any sched dependency
	//if yes, change g to front
	//For the back gate - check if g is the source of any sched dependency
	//if yes, change g to back
	vector<pair<Gate*, Gate*> > tmp_list = I->sched_depends;
	I->sched_depends.clear();
	for(auto p : tmp_list){
		g1 = p.first;
		g2 = p.second;
		if(g == g1){
			I->sched_depends.push_back(make_pair(back, g2));
		}else if(g == g2){
			I->sched_depends.push_back(make_pair(g1, front));
		}else{
			I->sched_depends.push_back(make_pair(g1, g2));
		}
	}
}

void Targetter::map_two_qubit_gate_to_swaps(Gate *g, BacktrackSolution *bs, Circuit *I, map<Qubit*, int> *current_map){
	assert((*current_map)[g->vars[0]] == bs->sinfo->c);
	assert((*current_map)[g->vars[1]] == bs->sinfo->t);
	for(auto qmap : *current_map){
		int my_mapping = qmap.second;
		int expected_mapping = bs->pre_perm[qmap.first];
		assert(my_mapping == expected_mapping);
	}
	SwapInfo *sinfo = bs->sinfo;
	vector<Gate*> gate_seq;

#if 0
	g->print();
	cout << "C" << bs->sinfo->c << endl;
	cout << "T" << bs->sinfo->t << endl;
	cout << "Dec" << bs->decision << endl;
	cout << "CE" << sinfo->ce << endl;
	cout << "TE" << sinfo->te << endl;
#endif

	switch(bs->decision){
	case CtrlRestore:
		insert_swap_chain(gate_seq, sinfo->c2te_path, ForwardSwap);
		insert_two_qubit_op(gate_seq, sinfo->te, sinfo->t);
		insert_swap_chain(gate_seq, sinfo->c2te_path, ReverseSwap);
		break;
	case TargRestore:
		insert_swap_chain(gate_seq, sinfo->t2ce_path, ForwardSwap);
		insert_two_qubit_op(gate_seq, sinfo->c, sinfo->ce);
		insert_swap_chain(gate_seq, sinfo->t2ce_path, ReverseSwap);
		break;
	case CtrlNotRestore:
		insert_swap_chain(gate_seq, sinfo->c2te_path, ForwardSwap);
		insert_two_qubit_op(gate_seq, sinfo->te, sinfo->t);
		break;
	case TargNotRestore:
		insert_swap_chain(gate_seq, sinfo->t2ce_path, ForwardSwap);
		insert_two_qubit_op(gate_seq, sinfo->c, sinfo->ce);
		break;
	default:
		assert(0);
		break;
	}
/*
	//find set of qubits that this chain uses
	set<Qubit*> overlap_set;
	for(auto Gi : gate_seq){
		for(auto v : Gi->vars){
			overlap_set.insert(v);
		}
	}

	//on these qubits, check for any parallel operations
	auto anc_gates = I->ancestors(g);

	//for each parallel operation, add a dependency to the head of this chain
*/

	//add new gates to circuit I
	copy_gate_connections_to_chain(g, gate_seq, I);
	for(auto tmp_gate : gate_seq){
		I->gates.push_back(tmp_gate);
	}
	I->gates.erase(remove(I->gates.begin(), I->gates.end(), g), I->gates.end());
	for(auto qmap : bs->post_perm){
		int expected_mapping = bs->post_perm[qmap.first];
		(*current_map)[qmap.first] = expected_mapping;
	}
}

void Targetter::map_swaps_to_cnots(Gate *g, Circuit *I){
	vector<Gate*> gate_seq;
	Gate *g1, *g2, *g3;
	g1 = Gate::create_new_gate("CNOT");
	g2 = Gate::create_new_gate("CNOT");
	g3 = Gate::create_new_gate("CNOT");

	int qid1 = g->vars[0]->id;
	int qid2 = g->vars[1]->id;
	vector< pair<int,int> > *dir = &(M->hw_cnot_directions);
	auto chk_1_2 = find(dir->begin(), dir->end(), make_pair(qid1, qid2));
	auto chk_2_1 = find(dir->begin(), dir->end(), make_pair(qid2, qid1));
	if(chk_1_2 != dir->end()){
		g1->vars.push_back(g->vars[0]);
		g1->vars.push_back(g->vars[1]);
		g2->vars.push_back(g->vars[1]);
		g2->vars.push_back(g->vars[0]);
		g3->vars.push_back(g->vars[0]);
		g3->vars.push_back(g->vars[1]);
	}else{
		g1->vars.push_back(g->vars[1]);
		g1->vars.push_back(g->vars[0]);
		g2->vars.push_back(g->vars[0]);
		g2->vars.push_back(g->vars[1]);
		g3->vars.push_back(g->vars[1]);
		g3->vars.push_back(g->vars[0]);
	}
	gate_seq.push_back(g1);
	gate_seq.push_back(g2);
	gate_seq.push_back(g3);
	copy_gate_connections_to_chain(g, gate_seq, I);
	for(auto tmp_gate : gate_seq){
		I->gates.push_back(tmp_gate);
	}
	I->gates.erase(remove(I->gates.begin(), I->gates.end(), g), I->gates.end());
}

void Targetter::map_cnots_to_cz(Gate *g, Circuit *I){
	vector<Gate*> gate_seq;
	Gate *g1, *g2, *g3;
	g1 = Gate::create_new_gate("H");
	g2 = Gate::create_new_gate("CZ");
	g3 = Gate::create_new_gate("H");

	g1->vars.push_back(g->vars[1]);
	g2->vars.push_back(g->vars[0]);
	g2->vars.push_back(g->vars[1]);
	g3->vars.push_back(g->vars[1]);

	gate_seq.push_back(g1);
	gate_seq.push_back(g2);
	gate_seq.push_back(g3);
	copy_gate_connections_to_chain(g, gate_seq, I);
	for(auto tmp_gate : gate_seq){
		I->gates.push_back(tmp_gate);
	}
	I->gates.erase(remove(I->gates.begin(), I->gates.end(), g), I->gates.end());
}


void Targetter::_external_link(Gate *g, vector<Gate*> gate_list, Circuit *I, int link_direction){
	vector <Gate*> common_list;
	common_list = gate_list;
	for(auto p : I->sched_depends){
		Gate *check_gate;
		Gate *add_gate;
		if(link_direction == Predecessor){
			check_gate = p.second;
			add_gate = p.first;
		}else{
			check_gate = p.first;
			add_gate = p.second;
		}
		if(check_gate == g){
			if(find(common_list.begin(), common_list.end(), add_gate) == common_list.end()){
				common_list.push_back(add_gate);
			}
		}
	}

	vector <Gate*> to_link;
	if(common_list.empty()){	//link to all gates in gate_list
		to_link = gate_list;
	}else{
		to_link = common_list;
	}
	for(auto friend_gate : to_link){
		if(link_direction == Predecessor){
			g->pred.push_back(friend_gate);
			friend_gate->succ.push_back(g);
		}else{
			g->succ.push_back(friend_gate);
			friend_gate->pred.push_back(g);
		}
	}
}

void Targetter::implement_hardware_directions_for_cnots(Gate *g, Circuit *I){
	int qid1 = g->vars[0]->id;
	int qid2 = g->vars[1]->id;
	vector< pair<int,int> > *dir = &(M->hw_cnot_directions);
	auto chk_1_2 = find(dir->begin(), dir->end(), make_pair(qid1, qid2));
	auto chk_2_1 = find(dir->begin(), dir->end(), make_pair(qid2, qid1));
	if(chk_1_2 != dir->end()){
		return; //this cnot doesnt need modification
	}else{
		assert(chk_2_1 != dir->end());

		Gate *h1 = Gate::create_new_gate("H");
		Gate *h2 = Gate::create_new_gate("H");
		Gate *h3 = Gate::create_new_gate("H");
		Gate *h4 = Gate::create_new_gate("H");
		Gate *rev_cx = Gate::create_new_gate("CNOT");
		h1->vars.push_back(g->vars[0]);
		h2->vars.push_back(g->vars[1]);
		rev_cx->vars.push_back(g->vars[1]);
		rev_cx->vars.push_back(g->vars[0]);
		h3->vars.push_back(g->vars[0]);
		h4->vars.push_back(g->vars[1]);

		//setup internal dependency
		h1->succ.push_back(rev_cx);
		h2->succ.push_back(rev_cx);
		rev_cx->pred.push_back(h1);
		rev_cx->pred.push_back(h2);

		rev_cx->succ.push_back(h3);
		rev_cx->succ.push_back(h4);
		h3->pred.push_back(rev_cx);
		h4->pred.push_back(rev_cx);

		vector<pair<Gate*, Gate*> > tmp_list = I->sched_depends;
		I->sched_depends.clear();
		for(auto p : tmp_list){
			Gate *g1 = p.first;
			Gate* g2 = p.second;
			if(g == g1){ //g is source
				I->sched_depends.push_back(make_pair(h3, g2));
				I->sched_depends.push_back(make_pair(h4, g2));
			}else if(g == g2){ //g is target
				I->sched_depends.push_back(make_pair(g1, h1));
				I->sched_depends.push_back(make_pair(g1, h2));
			}else{
				I->sched_depends.push_back(make_pair(g1, g2));
			}
		}

		//setup external dependency
		_external_link(h1, g->pred, I, Predecessor);
		_external_link(h2, g->pred, I, Predecessor);
		_external_link(h3, g->succ, I, Successor);
		_external_link(h4, g->succ, I, Successor);

		for(auto pred_gate : g->pred){
			pred_gate->succ.erase(remove(pred_gate->succ.begin(), pred_gate->succ.end(), g), pred_gate->succ.end());
		}
		for(auto succ_gate : g->succ){
			succ_gate->pred.erase(remove(succ_gate->pred.begin(), succ_gate->pred.end(), g), succ_gate->pred.end());
		}

		I->gates.push_back(h1);
		I->gates.push_back(h2);
		I->gates.push_back(rev_cx);
		I->gates.push_back(h3);
		I->gates.push_back(h4);
		I->gates.erase(remove(I->gates.begin(), I->gates.end(), g), I->gates.end());
	}
}

Circuit* Targetter::map_and_insert_swap_operations(){
	//C->print_sched_depends();

	Circuit *I = new Circuit();
	map<Gate*, Gate*> *old2new = new map<Gate*, Gate*>;
	C->duplicate_circuit(I, old2new);
	map<Qubit*, int> current_map = *initial_map;

	for(auto old_gate : *gate_order){
		Gate *new_gate = (*old2new)[old_gate];
		if(old_gate->vars.size() == 1){
			map_single_qubit_gate(new_gate, current_map);
		}else if(old_gate->vars.size() == 2){
			BacktrackSolution *bs = bsol[old_gate];
			map_two_qubit_gate_to_swaps(new_gate, bs, I, &current_map);
		}
	}

	final_map = current_map;

	//cout << "I dependencies:\n";
	//I->print_sched_depends();

	delete old2new;

	Circuit *J = new Circuit();
	old2new = new map<Gate*, Gate*>;
	I->duplicate_circuit(J, old2new);

	for(auto g : I->gates){
		if(typeid(*g) == typeid(SWAP)){
			map_swaps_to_cnots((*old2new)[g], J);
		}
	}


	//cout << "J dependencies:\n";
	//J->print_sched_depends();

	Circuit *K = new Circuit();
	delete old2new;
	old2new = new map<Gate*, Gate*>;
	J->duplicate_circuit(K, old2new);

	//cout << "K initial dependencies:\n";
	//K->print_sched_depends();

	//if ibmqx5:,

	for(auto g : J->gates){
		if(typeid(*g) == typeid(CNOT)){
			//cout << "CNT=" << cnt << endl << endl;
			if(M->machine_name == "ibmqx4" || M->machine_name == "ibmqx5" || M->machine_name == "ibmq_16_melbourne"){
				implement_hardware_directions_for_cnots((*old2new)[g], K);
			}else if(M->machine_name == "agave" || M->machine_name == "Aspen1" || M->machine_name == "Aspen3"){
				map_cnots_to_cz((*old2new)[g], K);
			}else;
			//stringstream fname;
		}
	}
	K->qubits = M->qubits;


	delete I;
	delete J;
	return K;
}

void Targetter::implement_gate_for_trapped_ion(Gate *g, Circuit *I){
	vector <Gate*> pred_interface;
	vector <Gate*> succ_interface;
	vector <Gate*> new_gates;
	if(typeid(*g) == typeid(H)){
		Gate *g1 = Gate::create_new_gate("RZ");
		Gate *g2 = Gate::create_new_gate("RY");
		g1->angle = PI;
		g2->angle = PI/2;
		g1->vars.push_back(g->vars[0]);
		g2->vars.push_back(g->vars[0]);
		g1->succ.push_back(g2);
		g2->pred.push_back(g1);
		pred_interface.push_back(g1);
		succ_interface.push_back(g2);
		new_gates.push_back(g1);
		new_gates.push_back(g2);
	}else if(typeid(*g) == typeid(X)){
		Gate *g1 = Gate::create_new_gate("RX");
		g1->angle = PI;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(Y)){
		Gate *g1 = Gate::create_new_gate("RY");
		g1->angle = PI;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(Z)){
		Gate *g1 = Gate::create_new_gate("RZ");
		g1->angle = PI;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(S)){
		Gate *g1 = Gate::create_new_gate("RZ");
		g1->angle = PI/2;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(Sdag)){
		Gate *g1 = Gate::create_new_gate("RZ");
		g1->angle = -PI/2;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(T)){
		Gate *g1 = Gate::create_new_gate("RZ");
		g1->angle = PI/4;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(Tdag)){
		Gate *g1 = Gate::create_new_gate("RZ");
		g1->angle = -PI/4;
		g1->vars.push_back(g->vars[0]);
		pred_interface.push_back(g1);
		succ_interface.push_back(g1);
		new_gates.push_back(g1);
	}else if(typeid(*g) == typeid(CNOT)){
		if(1){
		Gate *g1 = Gate::create_new_gate("RY");
		Gate *g2 = Gate::create_new_gate("XX");
		Gate *g3 = Gate::create_new_gate("RY");
		Gate *g4 = Gate::create_new_gate("RX");
		Gate *g5 = Gate::create_new_gate("RZ");
		g1->angle = PI/2;
		g2->angle = PI/4;
		g3->angle = -PI/2;
		g4->angle = -PI/2;
		g5->angle = -PI/2;
		g1->vars.push_back(g->vars[0]);
		g2->vars.push_back(g->vars[0]);
		g2->vars.push_back(g->vars[1]);
		g3->vars.push_back(g->vars[0]);
		g4->vars.push_back(g->vars[1]);
		g5->vars.push_back(g->vars[0]);

		g1->succ.push_back(g2);
		g2->pred.push_back(g1);
		g2->succ.push_back(g3);
		g3->pred.push_back(g2);
		g2->succ.push_back(g4);
		g4->pred.push_back(g2);
		g3->succ.push_back(g5);
		g5->pred.push_back(g4);

		pred_interface.push_back(g1);
		pred_interface.push_back(g2);
		succ_interface.push_back(g4);
		succ_interface.push_back(g5);
		new_gates.push_back(g1);
		new_gates.push_back(g2);
		new_gates.push_back(g3);
		new_gates.push_back(g4);
		new_gates.push_back(g5);
		}
		if(0){
			//buggy, not working
			Gate *g1 = Gate::create_new_gate("CNOT");
			g1->vars.push_back(g->vars[0]);
			g1->vars.push_back(g->vars[1]);
			pred_interface.push_back(g1);
			succ_interface.push_back(g1);
			new_gates.push_back(g1);
		}
	}

	for (auto front_gate : pred_interface)
		_external_link(front_gate, g->pred, I, Predecessor);
	for (auto back_gate : succ_interface)
		_external_link(back_gate, g->succ, I, Successor);
	for(auto pred_gate : g->pred)
		pred_gate->succ.erase(remove(pred_gate->succ.begin(), pred_gate->succ.end(), g), pred_gate->succ.end());
	for(auto succ_gate : g->succ)
		succ_gate->pred.erase(remove(succ_gate->pred.begin(), succ_gate->pred.end(), g), succ_gate->pred.end());
	for(auto ngate : new_gates)
		I->gates.push_back(ngate);
	I->gates.erase(remove(I->gates.begin(), I->gates.end(), g), I->gates.end());
}

Circuit* Targetter::map_to_trapped_ion(){
	Circuit *I = new Circuit();
	map<Gate*, Gate*> *old2new = new map<Gate*, Gate*>;
	C->duplicate_circuit(I, old2new);
	map<Qubit*, int> current_map = *initial_map;

	for(auto old_gate : *gate_order){
		Gate *new_gate = (*old2new)[old_gate];
		if(old_gate->vars.size() == 1){
			map_single_qubit_gate(new_gate, current_map);
		}else if(old_gate->vars.size() == 2){
			BacktrackSolution *bs = bsol[old_gate];
			map_two_qubit_gate_no_swap(new_gate, current_map);
		}
	}

	for(auto old_gate : *gate_order){
		Gate *new_gate = (*old2new)[old_gate];
		if(typeid(*new_gate) != typeid(MeasZ)){
			implement_gate_for_trapped_ion(new_gate, I);
		}
	}
	return I;
}

void Targetter::print_header(ofstream &out_file){
	if(M->machine_name == "ibmqx5"){
		out_file << "OPENQASM 2.0;\n";
		out_file << "include \"qelib1.inc\";\n";
		out_file << "qreg q[16];\n";
		out_file << "creg c[16];\n";
	}
	if(M->machine_name == "ibmqx4"){
		out_file << "OPENQASM 2.0;\n";
		out_file << "include \"qelib1.inc\";\n";
		out_file << "qreg q[5];\n";
		out_file << "creg c[5];\n";
	}
	if(M->machine_name == "ibmq_16_melbourne"){
		out_file << "OPENQASM 2.0;\n";
		out_file << "include \"qelib1.inc\";\n";
		out_file << "qreg q[14];\n";
		out_file << "creg c[14];\n";
	}
	if(M->machine_name == "agave"){
		out_file << "from pyquil import Program, get_qc\n";
		out_file << "from pyquil.gates import *\n";
		out_file <<	"import numpy as np\n";
		out_file << "from collections import defaultdict\n";
		out_file << "qc = get_qc(\"Aspen-1-16Q-A\", as_qvm=True)\n";
		out_file << "p = Program()\n";
	}
	if(M->machine_name == "Aspen1"){
			out_file << "from pyquil import Program, get_qc\n";
			out_file << "from pyquil.gates import *\n";
			out_file <<	"import numpy as np\n";
			out_file << "from collections import defaultdict\n";
			out_file << "qc = get_qc(\"Aspen-1-16Q-A\", as_qvm=True)\n";
			out_file << "p = Program()\n";
	}
	if(M->machine_name == "Aspen3"){
		out_file << "from pyquil import Program, get_qc\n";
		out_file << "from pyquil.gates import *\n";
		out_file <<	"import numpy as np\n";
		out_file << "from collections import defaultdict\n";
		out_file << "qc = get_qc(\"Aspen-3-14Q-A\", as_qvm=True)\n";
		out_file << "p = Program()\n";
	}
	if(M->machine_name == "Aer"){
		out_file << "import pickle\n";
		out_file << "from qiskit import QuantumRegister, ClassicalRegister\n";
		out_file << "from qiskit import QuantumCircuit, execute, Aer, IBMQ\n";
		out_file << "from qiskit.providers.aer import noise\n";
		out_file << "from qiskit.providers.aer.noise import NoiseModel\n";
		out_file << "from qiskit.providers.models import BackendProperties\n";
		out_file << "from qiskit.providers.models.backendproperties import *\n";
		out_file << "from numpy.random import choice\n";
		out_file << "noise_data = pickle.load(open(\"/home/pmurali/code/aer_interface/Aer16.1_Sim.pkl\", 'rb'))\n";
		out_file << "properties = noise_data[0]\n";
		out_file << "coupling_map = noise_data[1]\n";
		out_file << "noise_model = noise.device.basic_device_noise_model(properties, thermal_relaxation=False)\n";
		out_file << "basis_gates = noise_model.basis_gates\n";
		out_file << "q = QuantumRegister(16)\n";
		out_file << "c = ClassicalRegister(16)\n";
		out_file << "qc = QuantumCircuit(q, c)\n";

	}
}

void Targetter::print_footer(ofstream &out_file) {
	if (M->machine_name == "agave") {
		//out_file << "x=qpu.run(p, [0, 1, 2, 3], 1024)\n";
		//out_file << "print(x)\n";
		out_file << "p.wrap_in_numshots_loop(10)\n";
		out_file << "executable = qc.compile(p, to_native_gates=False, optimize=False)\n";
		out_file << "print(executable)\n";
		out_file << "bitstrings = qc.run(executable)\n";
		out_file << "print(bitstrings)\n";
	}
	if (M->machine_name == "Aspen1" || M->machine_name == "Aspen3") {
		out_file << "count = 8192\n";
		out_file << "p.wrap_in_numshots_loop(count)\n";
		out_file << "executable = qc.compile(p, to_native_gates=False, optimize=False)\n";
		out_file << "print(executable)\n";
		out_file << "bs = qc.run(executable)\n";
		out_file << "outs = {}\n";
		out_file << "for item in bs:\n";
		out_file << "  thisstr = \"\"\n";
		out_file << "  for i in item:\n";
		out_file << "    thisstr += str(i)\n";
		out_file << "  if not thisstr in outs:\n";
		out_file << "    outs[thisstr] = 1\n";
		out_file << "  else:\n";
		out_file << "    outs[thisstr] += 1\n";
		out_file << "for key in outs:\n";
		out_file << "  print(key, float(outs[key])/count)\n";
	}
	if (M->machine_name == "Aer"){
		out_file << "backend = Aer.get_backend('qasm_simulator')\n";
	    out_file << "job_sim = execute(qc, backend, coupling_map=coupling_map, noise_model=noise_model, basis_gates=basis_gates, shots=1024)\n";
	    out_file << "sim_result = job_sim.result()\n";
	    out_file << "print(sim_result.get_counts(qc))\n";
	}
}
void Targetter::print_measure_ops(ofstream &out_file){
	if (M->machine_name == "ibmqx5" || M->machine_name == "ibmqx4" || M->machine_name == "ibmq_16_melbourne") {
		for (auto mapping : final_map) {
			Qubit *q = mapping.first;
			int hwq = mapping.second;
			out_file << "measure " << "q[" << hwq << "] -> c[" << q->id
					<< "];\n";
		}
	}else if(M->machine_name == "agave" || M->machine_name == "Aspen1"|| M->machine_name == "Aspen3"){
		int num_qubits = C->qubits.size();
		out_file << "ro = p.declare(\'ro\', \'BIT\'," << num_qubits << ")\n";

		for (auto mapping : final_map) {
			Qubit *q = mapping.first;
			int hwq = mapping.second;
			if(M->machine_name == "Aspen1") hwq = aspen1_map[hwq];
			if(M->machine_name == "Aspen3") hwq = aspen3_map[hwq];

			out_file << "p += MEASURE(" << hwq << ", ro[" << q->id << "])\n";
		}
	}else if(M->machine_name == "Aer"){
		for (auto mapping : final_map) {
			Qubit *q = mapping.first;
			int hwq = mapping.second;
			out_file << "qc.measure(" << "q[" << hwq << "], c[" << q->id << "])\n";
		}
	}
}
void Targetter::print_code(Circuit *I, string fname){
	ofstream out_file;
	out_file.open(fname.c_str(), ofstream::out);
	cout << "fname:" << fname << endl;
	vector<Gate*>* torder = I->topological_ordering();
	print_header(out_file);

	int cx_count=0;

	Gate *last_gate = torder->back();
	int is_last_gate=0;
	for(auto g : *torder){
		if(M->machine_name == "tion"){
			if(g == last_gate)
			is_last_gate = 1;
		}
		if(g->vars.size() == 1){
			print_one_qubit_gate(g, out_file, is_last_gate);
		}else if(g->vars.size() == 2){
			print_two_qubit_gate(g, out_file, is_last_gate);
			cx_count++;
		}else{
			assert(0);
		}
	}
	cout << "Num CX:" << cx_count << endl;
	print_measure_ops(out_file);
	print_footer(out_file);
}

void Targetter::print_one_qubit_gate(Gate *g, ofstream &out_file, int is_last_gate){
	int qid = g->vars[0]->id;
	string gate_name;
	string tion_sep = "\n";
	if(typeid(*g) == typeid(MeasZ)){
		return;
	}
	if(M->machine_name == "Aer"){
		gate_name = gate_print_ibm[typeid(*g)];
		stringstream var_name;
		var_name << "q";
		var_name << "[";
		var_name << qid;
		var_name << "]";
		out_file << "qc." << gate_name << "(" << var_name.str() << ")\n";
	}
	else if(M->machine_name == "ibmqx5" || M->machine_name == "ibmqx4" || M->machine_name == "ibmq_16_melbourne"){
		if(typeid(*g) == typeid(U1) && g->lambda == 0){
			return;
		}
		gate_name = gate_print_ibm[typeid(*g)];
		out_file << gate_name;
		if(typeid(*g) == typeid(U1)){
			out_file << setprecision(15) << "(" << g->lambda << ")";
		}else if(typeid(*g) == typeid(U2)){
			out_file << setprecision(15) << "(" << g->phi << "," << g->lambda << ")";
		}else if(typeid(*g) == typeid(U3)){
			out_file << setprecision(15) << "(" << g->theta << "," << g->phi << "," << g->lambda << ")";
		}else;
		out_file << " ";
		stringstream var_name;
		var_name << "q";
		var_name << "[";
		var_name << qid;
		var_name << "]";
		out_file << var_name.str() << ";\n";
	}
	else if(M->machine_name == "agave" || M->machine_name == "Aspen1" || M->machine_name == "Aspen3"){
		if(M->machine_name == "Aspen1"){
			qid = aspen1_map[qid];
		}
		if(M->machine_name == "Aspen3"){
			qid = aspen3_map[qid];
		}

		if(typeid(*g) == typeid(U1)){
			out_file << setprecision(15) << "p += RZ(" << g->lambda << "," << qid << ")\n";
		}else if(typeid(*g) == typeid(U2)){
			double ang1, ang2, ang3;
			ang1 = g->phi + PI/2;
			ang2 = PI/2;
			ang3 = g->lambda - PI/2;
			out_file << setprecision(15) << "p += RZ(" << ang3 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RX(" << ang2 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RZ(" << ang1 << "," << qid << ")\n";
		}else if(typeid(*g) == typeid(U3)){
			double ang1, ang2, ang3, ang4, ang5;
			ang1 = g->phi + 3*PI;
			ang2 = PI/2;
			ang3 = g->theta + PI;
			ang4 = PI/2;
			ang5 = g->lambda;
			out_file << setprecision(15) << "p += RZ(" << ang5 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RX(" << ang4 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RZ(" << ang3 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RX(" << ang2 << "," << qid << ")\n";
			out_file << setprecision(15) << "p += RZ(" << ang1 << "," << qid << ")\n";
		}else{
			gate_name = gate_print_rigetti[typeid(*g)];
			out_file << "p += " << gate_name;
			out_file << "(" << qid << ")\n";
		}
	}
	else if(M->machine_name == "tion"){
		double fthresh = 0.0001;
		if(typeid(*g) == typeid(U1) || typeid(*g) == typeid(U2) || typeid(*g) == typeid(U3)){
			double pitheta = remainder(g->theta/(PI), 2);
			double piphi = remainder(g->phi/(PI), 2);
			double pilambda = remainder(g->lambda/(PI), 2);
			//g->print();
			if(typeid(*g) == typeid(U1)){
				//Z(lambda)
				if (pilambda != 0 && fabs(pilambda) >= fthresh) {
					out_file << "AZ" << qid + 1;
					if (pilambda >= 0)
						out_file << "+";
					else
						out_file << "-";
					out_file << std::fixed << setprecision(4) << abs(pilambda);
					if(!is_last_gate) out_file << tion_sep;
				}
			}else{
				//RZ(lambda)
				if (pilambda != 0 && fabs(pilambda) >= fthresh) {
					out_file << "AZ" << qid + 1;
					if (pilambda >= 0)
						out_file << "+";
					else
						out_file << "-";
					out_file << std::fixed << setprecision(4) << abs(pilambda);
					out_file << tion_sep;
				}
				//RY(theta)
				if (pitheta != 0 && fabs(pitheta) >= fthresh) {
					out_file << "RA" << qid + 1;
					if (pitheta >= 0)
						out_file << "+";
					else
						out_file << "-";
					out_file << std::fixed << setprecision(4) << abs(pitheta) << ","
							<< 0.5;
					out_file << tion_sep;
				}
				//RZ(phi)
				if (piphi != 0 && fabs(piphi) >= fthresh) {
					out_file << "AZ" << qid + 1;
					if (piphi >= 0)
						out_file << "+";
					else
						out_file << "-";
					out_file << std::fixed << setprecision(4) << abs(piphi);
					if(!is_last_gate) out_file << tion_sep;
				}
			}
		} else {
			cout << "Warning: unoptimized trapped ion code printed\n";
			gate_name = gate_print_tion[typeid(*g)];
			out_file << gate_name;
			int pi_div_factor = floor(PI / g->angle);
			if (pi_div_factor >= 0) {
				out_file << "+";
			} else {
				out_file << "-";
			}
			out_file << qid + 1;
			out_file << abs(pi_div_factor) << tion_sep;
		}
	}else;

}

void Targetter::print_two_qubit_gate(Gate *g, ofstream &out_file, int is_last_gate){
	int qid1 = g->vars[0]->id;
	int qid2 = g->vars[1]->id;
	string gate_name;
	string tion_sep = "\n";
	if(M->machine_name == "Aer"){
		gate_name = gate_print_ibm[typeid(*g)];
		stringstream var_name1, var_name2;
		var_name1 << "q[" << qid1 << "]";
		var_name2 << "q[" << qid2 << "]";
		out_file << "qc." << gate_name << "(" << var_name1.str() << "," << var_name2.str() << ")\n";
	}
	if(M->machine_name == "ibmqx5" || M->machine_name == "ibmqx4" || M->machine_name == "ibmq_16_melbourne"){
		gate_name = gate_print_ibm[typeid(*g)];
		out_file << gate_name << " ";
		stringstream var_name1, var_name2;
		var_name1 << "q[" << qid1 << "]";
		var_name2 << "q[" << qid2 << "]";
		out_file << var_name1.str() << "," << var_name2.str() << ";" << endl;
	}else if(M->machine_name == "agave" || M->machine_name == "Aspen1" || M->machine_name == "Aspen3"){
		if(M->machine_name == "Aspen1"){
			qid1 = aspen1_map[qid1];
			qid2 = aspen1_map[qid2];
		}
		if(M->machine_name == "Aspen3"){
			qid1 = aspen3_map[qid1];
			qid2 = aspen3_map[qid2];
		}
		gate_name = gate_print_rigetti[typeid(*g)];
		out_file << "p +=" << gate_name << "(" << qid1 << "," << qid2 << ")\n";
	}else if(M->machine_name == "tion"){
		if (typeid(*g) == typeid(CNOT)) {
			out_file << "CN" << qid1 + 1 << qid2 + 1;
			if (!is_last_gate){
				out_file << tion_sep;
			}
		} else {
			gate_name = gate_print_tion[typeid(*g)];
			out_file << gate_name;
			int pi_div_factor = floor(PI / g->angle);
			assert(pi_div_factor == 4 || pi_div_factor == -4);
			out_file << qid1 + 1 << qid2 + 1;
			if (pi_div_factor >= 0) {
				out_file << "+";
			} else {
				out_file << "-";
			}
			if (!is_last_gate)
				out_file << tion_sep;
		}
	}
}

void Targetter::init_gate_print_maps(){
	gate_print_ibm[typeid(CNOT)] = "cx";
	gate_print_ibm[typeid(H)] = "h";
	gate_print_ibm[typeid(X)] = "x";
	gate_print_ibm[typeid(Y)] = "y";
	gate_print_ibm[typeid(Z)] = "z";
	gate_print_ibm[typeid(T)] = "t";
	gate_print_ibm[typeid(Tdag)] = "tdg";
	gate_print_ibm[typeid(S)] = "s";
	gate_print_ibm[typeid(Sdag)] = "sdg";
	gate_print_ibm[typeid(MeasZ)] = "measure";
	gate_print_ibm[typeid(U1)] = "u1";
	gate_print_ibm[typeid(U2)] = "u2";
	gate_print_ibm[typeid(U3)] = "u3";


	gate_print_rigetti[typeid(CNOT)] = "CNOT";
	gate_print_rigetti[typeid(CZ)] = "CZ";
	gate_print_rigetti[typeid(H)] = "H";
	gate_print_rigetti[typeid(X)] = "X";
	gate_print_rigetti[typeid(Y)] = "Y";
	gate_print_rigetti[typeid(Z)] = "Z";
	gate_print_rigetti[typeid(T)] = "T";
	gate_print_rigetti[typeid(Tdag)] = "RZ(-np.pi/4)";
	gate_print_rigetti[typeid(S)] = "S";
	gate_print_rigetti[typeid(Sdag)] = "RZ(-np.pi/2)";
	gate_print_rigetti[typeid(MeasZ)] = "MEASURE";

	gate_print_tion[typeid(XX)] = "XX";
	gate_print_tion[typeid(RX)] = "RX";
	gate_print_tion[typeid(RY)] = "RY";
	gate_print_tion[typeid(RZ)] = "RZ";

	aspen1_map[0] = 0;
	aspen1_map[1] = 1;
	aspen1_map[2] = 2;
	aspen1_map[3] = 3;
	aspen1_map[4] = 4;
	aspen1_map[5] = 5;
	aspen1_map[6] = 6;
	aspen1_map[7] = 7;

	aspen1_map[8] = 10;
	aspen1_map[9] = 11;
	aspen1_map[10] = 12;
	aspen1_map[11] = 13;
	aspen1_map[12] = 14;
	aspen1_map[13] = 15;
	aspen1_map[14] = 16;
	aspen1_map[15] = 17;

	aspen3_map[0] = 0;
	aspen3_map[1] = 1;
	aspen3_map[2] = 2;
	aspen3_map[3] = 3;
	aspen3_map[4] = 4;
	aspen3_map[5] = 5;
	aspen3_map[6] = 6;

	aspen3_map[7] = 10;
	aspen3_map[8] = 11;
	aspen3_map[9] = 12;
	aspen3_map[10] = 13;
	aspen3_map[11] = 14;
	aspen3_map[12] = 15;
	aspen3_map[13] = 16;

 }
