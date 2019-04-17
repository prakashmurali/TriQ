/*
 * backtrack.cpp
 *
 *  Created on: Sep 10, 2018
 *      Author: prakash
 */

#include "backtrack.hpp"
#include "headers.hpp"
#include "machine.hpp"


//vector<BacktrackNode*> Backtrack::all_nodes;
int Backtrack::is_leaf(BacktrackNode *n){
	if(n->gate_idx == (gate_order.size()-1)) return 1;
	else return 0;
}

void Backtrack::print_perm(map<Qubit*, int> *qmap){
	for(int i=0; i<C->qubits.size(); i++){
		cout << "Q" << C->qubits[i]->id << " : " << (*qmap)[C->qubits[i]] << endl;
	}
}

void change_perm(map<Qubit*, int> *qmap, map<int, Qubit*> *hw_map, vector<int> *spath){
	int i;
	int s1, s2;
	for(i=0; i<spath->size()-1; i++){
		s1 = spath->at(i);
		s2 = spath->at(i+1);
		//swap hw qubits
		Qubit *tmp = hw_map->at(s1);
		(*hw_map)[s1] = hw_map->at(s2);
		(*hw_map)[s2] = tmp;
		//adjust entries in qmap
		Qubit *p1 = hw_map->at(s1);
		Qubit *p2 = hw_map->at(s2);
		if(p1 != 0){
			(*qmap)[p1] = s1;
		}
		if(p2 != 0){
			(*qmap)[p2] = s2;
		}
	}
}

BacktrackNode* Backtrack::create_child(BacktrackNode *parent, swap_decision option){
	BacktrackNode *c = new BacktrackNode;
	c->parent = parent;
	c->reliab = parent->reliab;
	c->gate_idx = parent->gate_idx + 1;
	c->decision = option;
	c->qubit_map = parent->qubit_map;
	c->hw_map = parent->hw_map;
	all_nodes.push_back(c);
	Gate *g = gate_order[c->gate_idx];
	float gate_reliab;

	int hwq1 = parent->qubit_map[g->vars[0]];
	int hwq2 = parent->qubit_map[g->vars[1]];

	SwapInfo *s = (*(M->swap_info[hwq1]))[hwq2];
	c->s = s;

	assert(s->c == hwq1);
	assert(s->t == hwq2);

	float c2te_swap_reliab = s->c2te_swap_reliab;
	float te2t_cx_reliab = s->te2t_cx_reliab;
	float t2ce_swap_reliab = s->t2ce_swap_reliab;
	float c2ce_cx_reliab = s->c2ce_cx_reliab;
	if(option == CtrlRestore){
		gate_reliab = c2te_swap_reliab + te2t_cx_reliab + c2te_swap_reliab;
	}else if(option == TargRestore){
		gate_reliab = t2ce_swap_reliab + c2ce_cx_reliab + t2ce_swap_reliab;
	}else if(option == CtrlNotRestore){
		gate_reliab = c2te_swap_reliab + te2t_cx_reliab;
		change_perm(&(c->qubit_map), &(c->hw_map), s->c2te_path);
	}else if(option == TargNotRestore){
		gate_reliab = t2ce_swap_reliab + c2ce_cx_reliab;
		change_perm(&(c->qubit_map), &(c->hw_map), s->t2ce_path);
	}

	c->reliab = c->reliab + gate_reliab;
	return c;
}

void print_int_vector(vector<int> *v, string name){
	cout << name << endl;
	for(int i=0; i<v->size(); i++){
		cout << v->at(i) << " ";
	}
	cout << endl;
}
void Backtrack::print_trace(BacktrackNode *n, BacktrackNode *root){
	BacktrackNode *parent = n;

	while (n != root) {
		Gate *g = gate_order[n->gate_idx];
		cout << "N:" << n->gate_idx << "D:" << n->decision << "\t";
		g->print();
		print_perm(&(n->qubit_map));
		if(n->decision == 2){
			print_int_vector(n->s->c2te_path, "C2TE");
		}
		if(n->decision == 3){
			print_int_vector(n->s->t2ce_path, "T2CE");
		}
		n = n->parent;
		cout << "----\n";
	}
	print_perm(&(root->qubit_map));
}

map<Gate*, BacktrackSolution*> Backtrack::get_solution(int apply_first_gate_check){
	BacktrackNode *n = best_leaf;
	map<Gate*, BacktrackSolution*> sol;
	while (n != root){
		Gate *g = gate_order[n->gate_idx];
		sol[g] = new BacktrackSolution;
		BacktrackSolution *s = sol[g];
		s->decision = n->decision;
		s->post_perm = n->qubit_map;
		s->pre_perm = n->parent->qubit_map;
		s->sinfo = n->s;
		//s->bnode = n;
		n = n->parent;
	}
	if (apply_first_gate_check) {
		if (sol[gate_order[0]]->decision == CtrlNotRestore
				|| sol[gate_order[0]]->decision == TargNotRestore) {
			BacktrackSolution *s = sol[gate_order[0]];
			s->decision = CtrlRestore;
			s->pre_perm = s->post_perm;
			root->qubit_map = s->post_perm;
			for (int i = 0; i < C->qubits.size(); i++) {
				Qubit *qi = C->qubits[i];
				int hw_qubit = root->qubit_map[qi];
				root->hw_map[hw_qubit] = qi;
			}

			int hwq1 = root->qubit_map[gate_order[0]->vars[0]];
			int hwq2 = root->qubit_map[gate_order[0]->vars[1]];
			s->sinfo = (*(M->swap_info[hwq1]))[hwq2];
			*orig_mapping = root->qubit_map;
		}
	}
	return sol;
}

int Backtrack::check_if_backtracking_required(BacktrackNode *n){
	if(M->machine_name == "tion") return 0;
	int gidx = n->gate_idx;
	assert(!is_leaf(n));
	Gate *g = gate_order[gidx+1]; //g is the next gate
	Qubit *q1 = g->vars[0];
	Qubit *q2 = g->vars[1];
	auto qmap = n->qubit_map;
	if(n != root){
		qmap = n->parent->qubit_map;
	}
	int hw1 = qmap[q1];
	int hw2 = qmap[q2];
	if(M->is_edge(hw1, hw2)){
		return 0;
	}else{
		return 1;
	}
}

#if 0
void Backtrack::solve(BacktrackNode *n){
	if(is_leaf(n)) cout << "Saw a leaf" << n->gate_idx << "\n";
	if(n == root || !is_leaf(n)){
		int i;
		if (check_if_backtracking_required(n)) {
			n->child.push_back(create_child(n, CtrlRestore));
			n->child.push_back(create_child(n, TargRestore));
			n->child.push_back(create_child(n, CtrlNotRestore));
			n->child.push_back(create_child(n, TargNotRestore));
			for (i = 0; i < 4; i++) {
				if (n->child[i]->reliab < best_reliab)
					solve(n->child[i]);
			}
		} else {
			n->child.push_back(create_child(n, CtrlRestore));
			if (n->child[0]->reliab < best_reliab){
				cout << "Issuing solve for a child\n";
				solve(n->child[0]);
				cout << "Returned from only child\n";
			}
		}

		if(n == root){
			cout << "Best reliab:" << exp(-best_reliab) << endl;
			print_trace(best_leaf, root);
		}
	}else{
		if(n->reliab < best_reliab){
			cout << "Updated best leaf" << n->gate_idx << " " << n->decision << "\n";
			cout << exp(-n->reliab) << " " << exp(-best_reliab);
			best_reliab = n->reliab;
			best_leaf = n;
		}
		return;
	}
}
#endif

void Backtrack::solve(BacktrackNode *n, int consider_measurements){
	if(!is_leaf(n)){
		if(CompilerAlgorithm == CompileOpt && check_if_backtracking_required(n)){
			n->child.push_back(create_child(n, CtrlRestore));
			n->child.push_back(create_child(n, TargRestore));
			n->child.push_back(create_child(n, CtrlNotRestore));
			n->child.push_back(create_child(n, TargNotRestore));
			for (int i = 0; i < 4; i++) {
				if (n->child[i]->reliab < best_reliab)
					solve(n->child[i], consider_measurements);
			}
		}else{
			if(CompilerAlgorithm == CompileOpt){
				n->child.push_back(create_child(n, CtrlRestore));
			}else if(CompilerAlgorithm == CompileDijkstra){
				n->child.push_back(create_child(n, CtrlNotRestore));
			}else if(CompilerAlgorithm == CompileRevSwaps){
				n->child.push_back(create_child(n, CtrlRestore));
			}else;

			if (n->child[0]->reliab < best_reliab){
				solve(n->child[0], consider_measurements);
			}
		}

		if(n == root){
			cout << "Best reliab:" << exp(-best_reliab) << endl;
			//print_trace(best_leaf, root);
		}


	} else {
		Gate *g = gate_order[n->gate_idx];
		//Assume all the program qubits are measured at the leaf
		if (consider_measurements) {
			float measure_reliab = 0;
			for (auto qmap_entry : n->qubit_map) {
				measure_reliab = measure_reliab
						- log(M->m_reliab[qmap_entry.second]);
			}
			n->reliab = n->reliab + measure_reliab;
		}
		if (n->reliab < best_reliab) {
			best_reliab = n->reliab;
			best_leaf = n;
		}
		return;
	}
}

void Backtrack::init(std::map<Qubit*, int> *initial_map, vector<Gate*> *top_order){
	int i;
	for(i=0; i<top_order->size(); i++){
		Gate *Gi = (*top_order)[i];
		if(typeid(*Gi) == typeid(CNOT)){
			gate_order.push_back(Gi);
		}
	}
	best_reliab = 1000;
	root = new BacktrackNode;
	root->parent = 0;
	root->gate_idx = -1;
	root->reliab = 0;
	root->qubit_map = *initial_map;
	orig_mapping = initial_map;

	for(int i=0; i<M->qubits.size(); i++){
		root->hw_map[M->qubits[i]->id] = 0;
	}

	for(int i=0; i<C->qubits.size(); i++){
		Qubit *qi = C->qubits[i];
		int hw_qubit = (*initial_map)[qi];
		root->hw_map[hw_qubit] = qi;
	}
}

map<Gate*, BacktrackSolution*> BacktrackFiniteLookahead::solve(){
	for(unsigned int i=0; i<input_gate_order.size(); i++){
		Gate *Gi = input_gate_order[i];
		if(typeid(*Gi) == typeid(CNOT)){
			cx_order.push_back(Gi);
		}
	}
	int num_chunks = cx_order.size()/chunk_size;
	if(cx_order.size() % chunk_size > 0){
		num_chunks += 1;
	}

	unsigned idx=0;
	double est_reliab = 1.0;
	vector<map<Gate*, BacktrackSolution*> > solution_dump;
	for(int i=0; i<num_chunks; i++){
		vector<Gate*> chunk;
		for(int j=0; j<chunk_size; j++){
			if(idx == cx_order.size()) break;
			chunk.push_back(cx_order[idx]);
			idx++;
		}
		Backtrack B(C, M);
		B.init(&qubit_mapping, &chunk);
		if(i != num_chunks-1){
			B.solve(B.root, 0);
		}else{
			B.solve(B.root, 1);
		}
		map<Gate*, BacktrackSolution*> bsol = B.get_solution(0);
		solution_dump.push_back(bsol);
		for(auto item : bsol){
			Gate *Gi = item.first;
			BacktrackSolution *Bi = item.second;
			final_solution[Gi] = Bi;
		}
		//adjust qubit mapping using last item's post perm
		qubit_mapping = final_solution[chunk.back()]->post_perm;
		double chunk_reliab = exp(-B.best_reliab);
		est_reliab *= chunk_reliab;
		B.cleanup();
	}
	for(auto item : qubit_mapping){
		int hw_qid = item.second;
		est_reliab *= M->m_reliab[hw_qid];
	}
	cout << "EST_RELIAB:" << est_reliab << endl;

	return final_solution;
}
