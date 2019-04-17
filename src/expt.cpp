/*
 * expt.cpp
 *
 *  Created on: Sep 2, 2018
 *      Author: prakash
 */
#include "headers.hpp"
#include "circuit.hpp"
#include "mapper.hpp"
#include "machine.hpp"
#include "pattern.hpp"
#include "backtrack.hpp"
#include "targetter.hpp"
#include "optimize_1q.hpp"

int CompilerAlgorithm;
int ErrorScaleFactor=1;

int MyGetTime(){
	struct timespec ts_time_now;
	clock_gettime(CLOCK_MONOTONIC, &ts_time_now);
	return(ts_time_now.tv_sec);
}


void compile_and_print_for_machine(string progName, string outName, string machineName, int algorithm,
								int aerNQ=0, string aer_data_file="", string aer_partition_file="", int aer_num_partitions=0){
	Circuit C;
	C.load_from_file(progName);
	int t1 = MyGetTime();
	CompilerAlgorithm = algorithm;
	if(CompilerAlgorithm == CompileOpt){
		cout << "Running CompileOpt\n";
	}else if(CompilerAlgorithm == CompileDijkstra){
		cout << "Running CompileDijkstra\n";
	}else if(CompilerAlgorithm == CompileRevSwaps){
		cout << "Running CompileRevSwaps\n";
	}else assert(0);

	Machine M(machineName, aerNQ, aer_data_file, aer_partition_file, aer_num_partitions);
	Mapper pMapper(&M, &C);
	pMapper.set_config(MapSum, VarUnique);
	//pMapper.dummy_mapper();
	pMapper.config.approx_factor = 1.001;
	pMapper.map_with_z3();
	//pMapper.print_stats();
	vector<Gate*>* torder;
	torder = C.topological_ordering();
	C.enforce_topological_ordering(torder);

	map<Gate*, BacktrackSolution*> bsol;
	int t2 = MyGetTime();

	int cx_count = 0;
	for(auto gi : C.gates){
		if(gi->nvars == 2) cx_count++;
	}
	if (cx_count < 20) {
		Backtrack B(&C, &M);
		B.init(&pMapper.qubit_map, torder);
		B.solve(B.root, 1);
		if (CompilerAlgorithm == CompileOpt) {
			bsol = B.get_solution(1);
		} else {
			bsol = B.get_solution(0);
		}
	} else {
		BacktrackFiniteLookahead B(&C, &M, *torder, pMapper.qubit_map, 10);
		bsol = B.solve();
	}
	C.add_scheduling_dependency(torder, bsol);
	vector<Gate*>* torder_new = C.topological_ordering(); //this ordering incorportes new scheduling edges
	Targetter Tgen(&M, &C, &pMapper.qubit_map, torder_new, bsol);
	int t3 = MyGetTime();
	cout << "PROG " << progName << " MACHINE " << machineName << " MAPPER_TIME " << t2-t1 << " BACKTRACK_TIME " << t3-t2 << endl;

	Circuit *C_trans=0;
	Circuit *C_1q_opt=0;

	if(M.machine_name == "tion"){
		C_trans = Tgen.map_to_trapped_ion();
		OptimizeSingleQubitOps sq_opt(C_trans);
		C_1q_opt = sq_opt.test_optimize();
		Tgen.print_code(C_1q_opt, outName);
		//Tgen.print_code(C_trans, outName);
	}else{
		C_trans = Tgen.map_and_insert_swap_operations(); //We require a mapping to insert swaps
		OptimizeSingleQubitOps sq_opt(C_trans);
		C_1q_opt = sq_opt.test_optimize();
		Tgen.print_code(C_1q_opt, outName);
		//Tgen.print_code(C_trans, outName);
	}

	if(C_trans) delete C_trans;
	if(C_1q_opt) delete C_1q_opt;
	delete torder;
	cout << "Done!\n";
}

int main(int argc, char **argv){
	//compile_and_print_for_machine("programs/T1.in", "output/test.qasm", "ibmqx5", 0);
	compile_and_print_for_machine(argv[1], argv[2], argv[3], atoi(argv[4]));
	return 0;
}
