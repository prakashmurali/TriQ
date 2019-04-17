/*
 * machine.hpp
 *
 *  Created on: Sep 3, 2018
 *      Author: prakash
 */

#ifndef MACHINE_HPP_
#define MACHINE_HPP_

#include "headers.hpp"
#include "qubit.hpp"
#include "gate.hpp"

typedef boost::property<boost::edge_weight_t, float> EdgeWeightProperty;
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
		boost::no_property, EdgeWeightProperty> DirectedGraph;
typedef boost::graph_traits<DirectedGraph>::edge_iterator edge_iterator;
typedef boost::property_map<DirectedGraph, boost::edge_weight_t>::type EdgeWeightMap;
typedef boost::graph_traits<DirectedGraph>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<DirectedGraph>::edge_descriptor edge_descriptor;
typedef std::pair<int, int> Edge;

//SwapInfo objects are created for a every pair of hardware qubits
struct SwapInfo{
	int c;
	int t;
	int ce;
	int te;
	float c2te_swap_reliab;
	float te2t_cx_reliab;
	float t2ce_swap_reliab;
	float c2ce_cx_reliab;
	vector<int> *c2te_path;
	vector<int> *t2ce_path;
};

class Machine{
public:
	int nQ;
	std::vector<Qubit*> qubits;
	string data_file;
	string machine_name;
	string partition_file;
	int num_partitions;
	Machine(string name, int aerNQ=0, string aer_data_file="", string aer_partition_file="", int aer_num_partitions=0){
		machine_name = name;
		if(name == "ibmqx5"){
			nQ = 16;
			data_file = "config/ibmqx5";
		}else if(name == "ibmqx4"){
			nQ = 5;
			data_file = "config/ibmqx4";
		}else if(name == "ibmq_16_melbourne"){
			nQ = 14;
			data_file = "config/ibmq_16_melbourne";
		}else if(name == "tion"){
			nQ = 5;
			data_file = "config/tion";
		}else if(name == "agave"){
			nQ = 4;
			data_file = "config/agave";
		}else if(name == "Aspen1"){
			nQ = 16;
			data_file = "config/Aspen1";
		}
		else if(name == "Aspen3"){
			nQ = 14;
			data_file = "config/Aspen3";
		}
		else if(name == "RM4x8"){
			nQ = 32;
			data_file = "config/RM4x8";
		}else if(name == "RM8x8"){
			nQ = 64;
			data_file = "config/RM8x8";
		}else if(name == "RM8x9"){
			nQ = 72;
			data_file = "config/RM8x9";
		}else if(name == "RM8x16"){
			nQ = 128;
			data_file = "config/RM8x16";
		}else if(name == "dummy"){
			nQ = 6;
			data_file = "config/dummy";
		}
		else{
			assert(name == "Aer");
			nQ = aerNQ;
			data_file = aer_data_file;
		}
		for (int i = 0; i < nQ; i++) {
			HwQubit *hq = new HwQubit(i);
			qubits.push_back(hq);
		}
		read_reliability(data_file);
		compute_swap_paths();
		print_swap_paths();
		compute_swap_info();
	}

	~Machine(){
		delete_swap_paths();
	}

	/**
	 * Data structures with actual reliabilities in range [0,1]
	 */
	vector<float> s_reliab; //single qubit gate: Qx1 array
	vector<float> m_reliab; //measurement operation: Qx1 array
	vector<int> edge_u;
	vector<int> edge_v;
	vector<float> edge_reliab;
	vector< vector<float>* > t_reliab; // two qubit gate: QxQ array pairwise swap/cx reliabilities depending on adjacency

	/**
	 * Data structures with -log reliabilities
	 */
	map< pair<int,int>,float > cx_log_reliab;
 	vector< vector<float>* > swap_log_reliab; // two qubit gate: QxQ array: pairwise swap reliabilities
	vector< vector< vector<int>* >* > swap_path; //two qubit gate pairwise swap paths
	vector< vector<SwapInfo*>* > swap_info; //info req. for backtracking
    DirectedGraph gswap_log;


    vector< pair<int, int> > hw_cnot_directions;

	void read_s_reliability(string fname);
	void read_m_reliability(string fname);
	void read_t_reliability(string fname);
	void read_reliability(string basefname);
	void read_partition_data(string fname);
	int is_edge(int q1, int q2);

private:
	void setup_topology();
	vector< vector<int>* >* compute_swap_paths_one_vertex(int q);
	std::vector<int>* compute_shortest_sequence(int source, int dest,
			std::vector<vertex_descriptor> &parents);
	void compute_swap_paths();
	void delete_swap_paths();
	void print_swap_paths();
	void print_partition_data();
	void compute_swap_info();
	void fill_swap_info(int q1, int q2, SwapInfo *s);
	int determine_entry_point(int q1, int q2);
	void test_swap_info(int i, int j);

	float log2act(float val){
		return exp(-val);
	}
};



#endif /* MACHINE_HPP_ */
