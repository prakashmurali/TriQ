/*
 * circuit.cpp
 *
 *  Created on: Sep 3, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "circuit.hpp"
#include "qubit.hpp"
#include "optimize_1q.hpp"

void Circuit::load_from_file(string fname) {
	ifstream fileIn;
	cout << "Reading " << fname << endl;
	fileIn.open(fname.c_str());
	int nQ, nG;
	fileIn >> nQ >> nG;
	int i;

	ProgQubit *pQ = 0;
	for (i = 0; i < nQ; i++){
		pQ = new ProgQubit(i);
		qubits.push_back(pQ);
	}
	vector< vector<int>* > gatePred;
	for (i = 0; i < nG; i++) {
		int gate_id;
		string gate_type;
		fileIn >> gate_id >> gate_type;
		Gate *pG = Gate::create_new_gate(gate_type);
		pG->id = gate_id;
		int nvars_check;
		fileIn >> nvars_check;
		assert(nvars_check == pG->nvars);

		int tmp;
		for (int j = 0; j < pG->nvars; j++) {
			fileIn >> tmp;
			pG->vars.push_back(qubits[tmp]);
		}

		vector <int>* my_pred = new vector <int>;
		int npred;
		fileIn >> npred;
		for (int j = 0; j < npred; j++) {
			fileIn >> tmp;
			my_pred->push_back(tmp);
		}
		gatePred.push_back(my_pred);
		gates.push_back(pG);
	}
	for(i=0; i<nG; i++){
		Gate *pG = gates[i];
		for(unsigned int j=0; j<(*gatePred[i]).size(); j++){
			int dep_id = (*gatePred[i])[j];
			Gate *pDep = gates[dep_id];
			pG->pred.push_back(pDep);
			pDep->succ.push_back(pG);
		}
		delete gatePred[i];
	}
	fileIn.close();
}

void Circuit::_add_dependency_for_gate(Gate *g, BacktrackSolution *bs){
	SwapInfo *sinfo = bs->sinfo;
	vector<int> *swap_seq;
	if(bs->decision == CtrlRestore || bs->decision == CtrlNotRestore){
		swap_seq = sinfo->c2te_path;
	}else{
		swap_seq = sinfo->t2ce_path;
	}
	if (swap_seq->size() > 1) {
		auto dep = can_overlap_set(g);
		set<Gate*> delete_set;
		//auto dep = require_dependency_edge(g, gate_order);
		vector<Gate*> dep_list;
		if (!dep.empty()) {
			for(auto d : dep) dep_list.push_back(d);
			for (unsigned int i = 0; i < dep_list.size() - 1; i++) {
				auto desc_set = descendants(dep_list[i]);
				for (unsigned int j = i + 1; j < dep_list.size(); j++) {
					if (desc_set.find(dep_list[j]) != desc_set.end()) {
						delete_set.insert(dep_list[j]);
					}
				}
			}
			for(auto d : dep){
				if(delete_set.find(d) == delete_set.end()){
					//insert dependency between g and d
					//Note: this should by default avoid cyclic dependencies between parallel cnots because of topological ordering of the gates
					/*
					g->succ.push_back(d);
					d->pred.push_back(g);
					cout << "added dep " << g->id << " to " << d->id << endl;
					sched_depends.push_back(make_pair(g, d));
					*/
					g->pred.push_back(d);
					d->succ.push_back(g);
					cout << "added dep " << d->id << " to " << g->id << endl;
					sched_depends.push_back(make_pair(d, g));

				}
			}
		}
	}

}

void Circuit::add_scheduling_dependency(vector<Gate*> *gate_order, map<Gate*, BacktrackSolution*> bsol){
	for(auto g: *gate_order){
		if(g->vars.size() == 1){
			continue;
		}else if(g->vars.size() == 2){
			BacktrackSolution *bs = bsol[g];
			_add_dependency_for_gate(g, bs);
		}
	}
}

struct Vertex{
	string name;
};

void Circuit::print_dot_file(string fname){

	typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
			Vertex, boost::no_property> CircuitGraph;

	typedef boost::graph_traits<CircuitGraph>::vertex_descriptor vertex_descriptor;
	CircuitGraph cgraph;

	map<Gate*, vertex_descriptor> V;
	for (auto g : gates) {
		Vertex *testv = new Vertex;
		stringstream vname;
		vname << g->id << " ";
		vname << g->printStr << " ";
		for(auto qb : g->vars){
			vname << qb->id << " ";
		}
		testv->name = vname.str();
		V[g] = boost::add_vertex(*testv, cgraph);
	}

	for (auto g : gates) {
		for (auto s : g->succ) {
			boost::add_edge(V[g], V[s], cgraph);
		}
		//for(auto p : g->pred){
		//	boost::add_edge(V[g], V[p], cgraph);
		//}
	}
	ofstream outFile;
	outFile.open(fname.c_str());
	write_graphviz(outFile, cgraph,
			make_label_writer(boost::get(&Vertex::name, cgraph)));
}

int Circuit::check_use_qubit(Gate *g, Qubit *q) {
	if (find(g->vars.begin(), g->vars.end(), q) != g->vars.end()) {
		return 1;
	} else {
		return 0;
	}
}

void Circuit::replace_with_chain(vector <Gate*> in_gates, vector <Gate*> out_gates){
	Gate *front, *back;
	/*cout << "replacing:\n";
	for(auto in_g : in_gates) in_g->print();
	cout << "with:\n";
	for(auto out_g : out_gates) out_g->print();
	cout << "\n";*/

	if(!out_gates.empty()){
		front = out_gates[0];
		back = out_gates[out_gates.size()-1];
	}

	//list of predecessors
	std::set <Gate*> pred_gates;
	for(auto g : in_gates){
		for(auto p : g->pred){
			pred_gates.insert(p);
		}
	}
	//remove in_gates from the set of predecessors
	for(auto g : in_gates){
		pred_gates.erase(g);
	}

	//remove in_gates from the predecessors' succ list and add front
	for(auto p : pred_gates){
		for(auto g : in_gates){
			p->succ.erase(std::remove(p->succ.begin(), p->succ.end(), g), p->succ.end());
		}
		if(out_gates.size()){
			p->succ.push_back(front);
			front->pred.push_back(p);
		}
	}

	std::set <Gate*> succ_gates;
	for(auto g : in_gates){
		for(auto s : g->succ){
			succ_gates.insert(s);
		}
	}

	//remove in_gates from the set of successors
	for(auto g : in_gates){
		succ_gates.erase(g);
	}

	for(auto p : pred_gates){
		succ_gates.erase(p);
	}

	//remove in_gates from successors' pred list and add back
	for(auto s : succ_gates){
		for(auto g : in_gates){
			s->pred.erase(std::remove(s->pred.begin(), s->pred.end(), g), s->pred.end());
		}
		if (!out_gates.empty()) {
			s->pred.push_back(back);
			back->succ.push_back(s);
		}
	}

	if (!out_gates.empty()) {
		//interlink the out gates
		for (unsigned int i = 0; i < out_gates.size()-1; i++) {
			Gate *g1 = out_gates[i];
			Gate *g2 = out_gates[i + 1];
			g1->succ.push_back(g2);
			g2->pred.push_back(g1);
		}
		for (auto g : out_gates) {
			gates.push_back(g);
		}
	}

	for(auto g : in_gates){
		gates.erase(std::remove(gates.begin(), gates.end(), g), gates.end());
	}
	//for(auto out_g : out_gates) out_g->print();


}

void Circuit::optimize_1q_gate_sequence() {
	for(auto q : qubits){
		cout << "Run extraction:" << q->id << endl;
		auto torder = topological_ordering();
		vector <Gate*> run_of_1q_gates;
		for (auto g : *torder) {
			if (check_use_qubit(g, q)) {
				if(typeid(*g) != typeid(CNOT)){
					run_of_1q_gates.push_back(g);
				} else{
					if(run_of_1q_gates.size() > 0){
						cout << "Gate run: ";
						for(auto it : run_of_1q_gates){
							it->print();
						}
						Optimize1QGates opt(run_of_1q_gates);
						opt.optimize();
						vector <Gate*> new_gates = opt.get_optimized_gates();
						for(auto ngate : new_gates){
							ngate->vars.push_back(q);
						}
						//replace_with_chain(run_of_1q_gates, new_gates);
					}
					run_of_1q_gates.clear();
				}
			}
		}
	}

}

vector<Gate*> *Circuit::topological_ordering(){
	std::queue<Gate*> q;
	std::vector<Gate*> *ans = new std::vector<Gate*>;
	std::map<Gate*, int> in_degree;
	for(auto Gi : gates){
		in_degree[Gi] = Gi->pred.size();
		if(in_degree[Gi] == 0){
			q.push(Gi);
		}
	}
	while(!q.empty()){
		Gate *front = q.front();
		q.pop();
		ans->push_back(front);
		for(auto s : front->succ){
			in_degree[s]--;
			if(in_degree[s] == 0) q.push(s);
		}
	}
	for(auto g : gates){
		if(in_degree[g] != 0){
			cout << "Warning\n";
			g->print();
		}
		//assert(in_degree[g] == 0);
	}
	return ans;
}

set<Gate*> Circuit::descendants(Gate *g){
	map <int, int> is_visited;
	for(auto g : gates){
		is_visited[g->id] = -1;
	}
	std::queue<Gate*> bfs_queue;
	std::set<Gate*> ans;
	bfs_queue.push(g);
	is_visited[g->id] = 1;

	while(!bfs_queue.empty()){
		Gate *f = bfs_queue.front();
		bfs_queue.pop();
		if(f != g){
			ans.insert(f);
		}
		for(auto s : f->succ){
			if(is_visited[s->id] != 1){
				bfs_queue.push(s);
				is_visited[s->id] = 1;
			}
		}
	}
	return ans;
}

set<Gate*> Circuit::ancestors(Gate *g){
	map <int, int> is_visited;
	for(auto g : gates){
		is_visited[g->id] = -1;
	}
	std::queue<Gate*> bfs_queue;
	std::set<Gate*> ans;
	bfs_queue.push(g);
	is_visited[g->id] = 1;
	while(!bfs_queue.empty()){
		Gate *f = bfs_queue.front();
		bfs_queue.pop();
		if(f != g){
			ans.insert(f);
		}
		for(auto p : f->pred){
			if(is_visited[p->id] != 1){
				bfs_queue.push(p);
				is_visited[p->id] = 1;
			}
		}
	}
	return ans;
}

int Circuit::is_descendant(Gate *g1, Gate *g2){
	if(g1 == g2) return 0;
	set<Gate*> B = descendants(g1);
	for(auto it : B){
		if(it == g2){
			return 1;
		}
	}
	return 0;
}

set<Gate*> Circuit::can_overlap_set(Gate *g){
	set<Gate*> A = ancestors(g);
	set<Gate*> B = descendants(g);
	B.insert(g);
	set<Gate*> all_gates;
	for(auto g : gates){
		all_gates.insert(g);
	}

	for(set<Gate*>::iterator it = A.begin(); it != A.end(); it++){
		all_gates.erase(*it);
	}
	for(set<Gate*>::iterator it = B.begin(); it != B.end(); it++){
		all_gates.erase(*it);
	}
#if 0
	//set<Gate*> transitive_deps;
	for(auto it1 = all_gates.begin(); it1 != all_gates.end(); it1++){
		for(auto it2 = all_gates.begin(); it2 != all_gates.end(); it2++){
			int is_d = is_descendant(*it1, *it2);
			if(is_d){
				all_gates.erase(*it2);
				//transitive_deps.insert(*it2);
			}
		}
	}
	/*
	for(set<Gate*>::iterator it = transitive_deps.begin(); it != transitive_deps.end(); it++){
			all_gates.erase(*it);
	}
	*/

	for(auto it1 = all_gates.begin(); it1 != all_gates.end(); it1++){
		for(auto it2 = all_gates.begin(); it2 != all_gates.end(); it2++){
			int is_d = is_descendant(*it1, *it2);
			assert(is_d == 0);
		}
	}
#endif
	return all_gates;
}

//extra dependencies should be added only in the topological order, otherwise there will be cyclic dependencies
set<Gate*> Circuit::require_dependency_edge(Gate *g, vector<Gate*>* gate_order){
	set<Gate*> can_overlap = can_overlap_set(g);
	set<Gate*> req_overlap;
	//todo pick out hte right gates heres
	vector<Gate*>::iterator it;
	it = find(gate_order->begin(), gate_order->end(), g);
	if(it == gate_order->end()){
		assert(0);
	}
	for(; it!=gate_order->end(); it++){
		if(can_overlap.find(*it) != can_overlap.end()){
			req_overlap.insert(*it);
		}
	}
	return req_overlap;
}

void Circuit::enforce_topological_ordering(vector<Gate*>* gate_order){
	for(auto g : gates){
		auto dep = require_dependency_edge(g, gate_order);
		for(auto d : dep){
			//cout << g->id << " " << d->id << "needs dependency" << endl;
			if(g->nvars == 1 && d->nvars == 1){
				continue;
			}
			g->succ.push_back(d);
			d->pred.push_back(g);
		}
	}
#if 0
	for (auto q : qubits) {
		vector<Gate*> my_gates;
		for (auto g : *gate_order) {
			if (check_use_qubit(g, q)) {
				my_gates.push_back(g);
			}
		}
		for(int i=0; i<my_gates.size()-1; i++){
			Gate *c = my_gates[i];
			Gate *n = my_gates[i+1];
			if(find(c->succ.begin(), c->succ.end(), n) == c->succ.end()){
				c->succ.push_back(n);
			}

			if(find(n->pred.begin(), n->pred.end(), c) == n->pred.end()){
				n->pred.push_back(c);
			}
		}
	}
#endif

}


void Circuit::duplicate_circuit(Circuit *C, map<Gate*, Gate*> *old2new){
	C->qubits = qubits; //same set of qubits
	C->gates.clear();

	for(auto g_old : gates){
		Gate *g_new = Gate::create_new_gate(g_old->printStr);
		g_new->vars = g_old->vars;
		C->gates.push_back(g_new);
		(*old2new)[g_old] = g_new;
	}
	for(auto g_old : gates){
		Gate *g_new = (*old2new)[g_old];
		for(unsigned int j=0; j<g_old->pred.size(); j++){
			g_new->pred.push_back((*old2new)[g_old->pred[j]]);
		}
		for (unsigned int j = 0; j < g_old->succ.size(); j++) {
			g_new->succ.push_back((*old2new)[g_old->succ[j]]);
		}
	}
	for(auto p : sched_depends){
		Gate *old_g1 = p.first;
		Gate *old_g2 = p.second;
		Gate *new_g1 = (*old2new)[old_g1];
		Gate *new_g2 = (*old2new)[old_g2];
		C->sched_depends.push_back(make_pair(new_g1, new_g2));
	}
}
/**
 * Re-write circuit using mapping and machine qubits
 */
Circuit* Circuit::rewrite_with_mapping(std::map<Qubit*, int> *qubit_map, Machine *M){
	Circuit *C = new Circuit();
	assert(this->qubits.size() <= M->qubits.size());
	C->qubits = M->qubits; // can use any hardware qubit

	map<Gate*, Gate*> old2new;

	for(unsigned int i=0; i<this->gates.size(); i++){
		Gate *g_old = this->gates[i];
		Gate *g_new = Gate::create_new_gate(g_old->printStr); //TODO
		//g_new->id = g_old->id;
		C->gates.push_back(g_new);
		old2new[g_old] = g_new;
	}

	for(unsigned int i=0; i<this->gates.size(); i++){
		Gate *g_old = this->gates[i];
		Gate *g_new = C->gates[i];
		for(int j=0; j<g_old->nvars; j++){
			int qid_new = (*qubit_map)[g_old->vars[j]];
			g_new->vars.push_back(M->qubits[qid_new]);
		}
	}

	for(unsigned int i=0; i<this->gates.size(); i++){
		Gate *g_old = this->gates[i];
		Gate *g_new = C->gates[i];
		for(unsigned int j=0; j<g_old->pred.size(); j++){
			g_new->pred.push_back(old2new[g_old->pred[j]]);
		}
		for (unsigned int j = 0; j < g_old->succ.size(); j++) {
			g_new->succ.push_back(old2new[g_old->succ[j]]);
		}
	}
	return C;
}


void Circuit::print_sched_depends(){
	cout << "Scheduling dependencies:\n";
	if(sched_depends.empty()){
		cout << "No dependencies\n";
		return;
	}
	for(auto p : sched_depends){
		p.first->print();
		cout << "-> ";
		p.second->print();
	}
	cout << endl;
}
void Circuit::print_gates(){
	cout << "Gates" << endl;
	for(auto g : gates){
		g->print();
	}
}


Circuit::~Circuit(){
	for(auto g : gates){
		delete g;
	}
}
