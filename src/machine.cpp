/*
 * machine.cpp
 *
 *  Created on: Sep 6, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "machine.hpp"

std::vector<int>* Machine::compute_shortest_sequence(int source, int dest,
		std::vector<vertex_descriptor> &parents) {
	std::vector<vertex_descriptor> path;
	vertex_descriptor current = dest;
	while (current != source) {
		path.push_back(current);
		current = parents[current];
	}
	path.push_back(source);
	std::vector<int> *req_path = new std::vector<int>;
	std::vector<vertex_descriptor>::reverse_iterator it;
	for (it = path.rbegin(); it != path.rend(); ++it) {
		req_path->push_back(*it);
	}
	return req_path;
}

vector< vector<int>* >* Machine::compute_swap_paths_one_vertex(int q) {
	std::vector<vertex_descriptor> parents(boost::num_vertices(gswap_log));
	std::vector<float> distances(boost::num_vertices(gswap_log));
	boost::dijkstra_shortest_paths(gswap_log, q,
			boost::predecessor_map(&parents[0]).distance_map(&distances[0]));
	vector < vector <int>* > *req_vector = new vector < vector <int>* >;

	boost::graph_traits < DirectedGraph >::vertex_iterator vertexIterator, vend;
	vector<float> *reliab_vector1 = new vector<float>;
	vector<float> *reliab_vector2 = new vector<float>;
	for (boost::tie(vertexIterator, vend) = boost::vertices(gswap_log); vertexIterator != vend; ++vertexIterator)
	{
		float d = distances[*vertexIterator];
		if(d != 0){
			reliab_vector1->push_back(d);
			reliab_vector2->push_back(log2act(d));
		}else{
			reliab_vector1->push_back(-1); //invalid case
			reliab_vector2->push_back(-1); //invalid case
		}
	}
	swap_log_reliab.push_back(reliab_vector1);
	t_reliab.push_back(reliab_vector2);

	for(int i=0; i<qubits.size();i++){
		req_vector->push_back(compute_shortest_sequence(q, qubits[i]->id, parents));
	}
	return req_vector;
}

void Machine::compute_swap_paths(){
	setup_topology();
	for(int i=0; i<qubits.size();i++){
		swap_path.push_back(compute_swap_paths_one_vertex(qubits[i]->id));
	}

	//adjustment for the solver: adjust values in t_reliab for adjacent cx gates
	int i;
	int u, v;
	float cx_cost = 0, rev_cx_cost = 0;
	for (i = 0; i < edge_u.size(); i++) {
		u = edge_u[i];
		v = edge_v[i];
		cx_cost = edge_reliab[i];
		if (machine_name != "ibmqx5" && machine_name != "ibmqx4" && machine_name != "ibmq_16_melbourne") {
			rev_cx_cost = edge_reliab[i];
		} else {
			rev_cx_cost = edge_reliab[i] * s_reliab[u] * s_reliab[v]
					* s_reliab[u] * s_reliab[v];
		}
		(*t_reliab[u])[v] = cx_cost;
		(*t_reliab[v])[u] = rev_cx_cost;
	}
}

void Machine::delete_swap_paths(){
	for(int i=0; i<qubits.size(); i++){
		vector< vector<int>* >* p1 = swap_path[i];
		for(int j=0; j<qubits.size(); j++){
			vector<int> *p2 = (*p1)[j];
			delete p2;
		}
		delete p1;
		delete t_reliab[i];
	}
}

void Machine::print_swap_paths(){
	for(int i=0; i<qubits.size(); i++){
		vector< vector<int>* >* p1 = swap_path[i];
		for(int j=0; j<qubits.size(); j++){
			cout << "(" << i << ", " << j << ") reliab: " << (*t_reliab[i])[j] << " path: ";
			vector<int> *p2 = (*p1)[j];
			for(int k=0; k<p2->size(); k++){
				cout << (*p2)[k] << " ";
			}
			cout << "\n";
		}
	}
}

int Machine::determine_entry_point(int q1, int q2){
	assert(q1 != q2);
	//iterate through neighbors of q2, determine neighbor with best swap+cx cost
	int i;
	vector <int> neighbors;
	for(i=0; i<qubits.size(); i++){
		if(is_edge(i, q2)){
			neighbors.push_back(i);
		}
	}
	vector <float> total_cost;
	int entry;
	for(i=0; i<neighbors.size(); i++){
		entry = neighbors[i];
		float swap_cost;
		float cx_cost;
		if(entry == q1){ //i.e. if q1 itself is one of the neighbors
			swap_cost = 0;
			cx_cost = cx_log_reliab[make_pair(q1,q2)];
		}else{
			swap_cost = (*swap_log_reliab[q1])[entry];
			cx_cost = cx_log_reliab[make_pair(entry,q2)];
		}
		total_cost.push_back(swap_cost+cx_cost);
	}
	float min_val = total_cost[0];
	int min_id = 0;
	for (i = 0; i < neighbors.size(); i++) {
		if(total_cost[i] < min_val){
			min_val = total_cost[i];
			min_id = i;
		}
	}
	cout << "q1 " << q1 << " q2 " << q2 << " "<< exp(-min_val) << "\n";
	return neighbors[min_id];
}
void Machine::fill_swap_info(int q1, int q2, SwapInfo *s){
	assert(q1 != q2);
	s->c = q1;
	s->t = q2;
	s->te = determine_entry_point(q1, q2);
	s->ce = determine_entry_point(q2, q1);
	assert(is_edge(s->te, s->t));
	assert(is_edge(s->c, s->ce));

	if(s->c == s->te) s->c2te_swap_reliab = 0;
	else s->c2te_swap_reliab = (*swap_log_reliab[s->c])[s->te];

	if(s->t == s->ce) s->t2ce_swap_reliab = 0;
	else s->t2ce_swap_reliab = (*swap_log_reliab[s->t])[s->ce];

	s->c2ce_cx_reliab = cx_log_reliab[make_pair(s->c, s->ce)];
	s->te2t_cx_reliab = cx_log_reliab[make_pair(s->te, s->t)];

	s->c2te_path = (*swap_path[s->c])[s->te];
	s->t2ce_path = (*swap_path[s->t])[s->ce];
/*
	cout << "C:" << q1 << " T:" << q2 << " CE:" << s->ce << " TE:" << s->te << endl;
	cout << "C->TE: ";
	for(int i=0; i < (s->c2te_path)->size(); i++){
		cout << (*(s->c2te_path))[i] << " ";
	}
	cout << endl;
	cout << "T->CE: ";
	for(int i=0; i < (s->t2ce_path)->size(); i++){
		cout << (*(s->t2ce_path))[i] << " ";
	}
	cout << endl;
*/
}

void Machine::test_swap_info(int i, int j){
	SwapInfo *sij = (*(swap_info[i]))[j];
	if(i==j) return;
	assert(sij->c == sij->c2te_path->at(0));
	assert(sij->te == sij->c2te_path->at(sij->c2te_path->size()-1));
	assert(sij->t == sij->t2ce_path->at(0));
	assert(sij->ce == sij->t2ce_path->at(sij->t2ce_path->size()-1));
}

void Machine::compute_swap_info(){
	int i,j;
	for(i=0; i<qubits.size(); i++){
		vector <SwapInfo*> *pi = new vector<SwapInfo*>;
		for(j=0; j<qubits.size(); j++){
			SwapInfo *sij = new SwapInfo;
			if(i != j){// invalid case
				fill_swap_info(i, j, sij);
			}else{
				sij->c = -1;
				sij->t = -1;
			}
			pi->push_back(sij);
		}
		swap_info.push_back(pi);
	}
	for(i=0; i<qubits.size(); i++){
		for(j=0; j<qubits.size(); j++){
			if(i != j){
				test_swap_info(i, j);
			}
		}
	}



}
void Machine::setup_topology() {
	int i;
	int u, v;
	float cx_cost = 0, rev_cx_cost = 0, swap_cost = 0;
	for (i = 0; i < edge_u.size(); i++) {
		u = edge_u[i];
		v = edge_v[i];
		cx_cost = edge_reliab[i];
		if (machine_name != "ibmqx5" && machine_name != "ibmqx4" && machine_name != "ibmq_16_melbourne") {
			rev_cx_cost = edge_reliab[i];
		} else {
			rev_cx_cost = edge_reliab[i] * s_reliab[u] * s_reliab[v]
					* s_reliab[u] * s_reliab[v];
		}
		swap_cost = cx_cost * cx_cost * rev_cx_cost;
		boost::add_edge(u, v, -log(swap_cost), gswap_log);
		boost::add_edge(v, u, -log(swap_cost), gswap_log);
		cx_log_reliab[make_pair(u,v)] = -log(cx_cost);
		cx_log_reliab[make_pair(v,u)] = -log(rev_cx_cost);
		if(machine_name == "ibmqx5" || machine_name == "ibmqx4" || machine_name == "ibmq_16_melbourne"){
			hw_cnot_directions.push_back(make_pair(u,v));
		}else{
			hw_cnot_directions.push_back(make_pair(u,v));
			hw_cnot_directions.push_back(make_pair(v,u));
		}
	}
}

float global_scale_value(float value){
	float err = 1.0-value;
	err = err/ErrorScaleFactor;
	return (1.0-err);
	//return 0.99;
}

void Machine::read_s_reliability(string fname){
  ifstream fileIn;
  fileIn.open(fname.c_str());
  int q;
  fileIn >> q;
  assert(nQ == q);
  int i;
  int pos;
  float val;
  for(i=0; i<nQ; i++){
    fileIn >> pos >> val;
    assert(pos == i);
    s_reliab.push_back(global_scale_value(val));
  }
}
void Machine::read_m_reliability(string fname){
  ifstream fileIn;
  fileIn.open(fname.c_str());
  int q;
  fileIn >> q;
  assert(nQ == q);
  int i;
  int pos;
  float val;
  for(i=0; i<nQ; i++){
    fileIn >> pos >> val;
    assert(pos == i);
    m_reliab.push_back(global_scale_value(val));
  }
}
void Machine::read_t_reliability(string fname){
  ifstream fileIn;
  fileIn.open(fname.c_str());
  int pairs;
  fileIn >> pairs;
  int i;
  int idx1, idx2;
  float val;

  for(i=0; i<pairs; i++){
	fileIn >> idx1 >> idx2 >> val;
	edge_u.push_back(idx1);
	edge_v.push_back(idx2);
	edge_reliab.push_back(global_scale_value(val));
  }
}
void Machine::read_reliability(string basefname){
  string s, m, t;
  s = basefname + "_S.rlb";
  m = basefname + "_M.rlb";
  t = basefname + "_T.rlb";
  read_s_reliability(s);
  read_m_reliability(m);
  read_t_reliability(t);
  cout << "Read reliability\n";
}

int Machine::is_edge(int q1, int q2) {
	std::pair<DirectedGraph::edge_descriptor, bool> edgePair = boost::edge(q1,
			q2, gswap_log);
	if (edgePair.second == true) {
		return 1;
	} else {
		return 0;
	}
	return 0;
}

void Machine::read_partition_data(string fname){
	ifstream fileIn;
	fileIn.open(fname.c_str());
	int part;

	for (unsigned i = 0; i < qubits.size(); i++) {
		fileIn >> part;
		qubits[i]->partition = part;
	}
}

void Machine::print_partition_data(){
	for (unsigned i = 0; i < qubits.size(); i++) {
		cout << "Qubit " << qubits[i]->id << " " << qubits[i]->partition << "\n";
	}
}
#if 0
//deleted code
	std::cout << "distances and parents:" << std::endl;
	boost::graph_traits < DirectedGraph >::vertex_iterator vertexIterator, vend;
	for (boost::tie(vertexIterator, vend) = boost::vertices(gswap_log); vertexIterator != vend; ++vertexIterator)
	{
		std::cout << "distance(" << *vertexIterator << ") = " << distances[*vertexIterator] << ", ";
		std::cout << "parent(" << *vertexIterator << ") = " << parents[*vertexIterator] << std::endl;
	}
	std::cout << std::endl;



#endif
