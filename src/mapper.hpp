/*
 * mapper.hpp
 *
 *  Created on: Sep 3, 2018
 *      Author: prakash
 */

#ifndef MAPPER_HPP_
#define MAPPER_HPP_

#include "headers.hpp"
#include "qubit.hpp"
#include "circuit.hpp"
#include "machine.hpp"

using namespace z3;

enum SolverObjective{
	MapMin, MapSum
};

enum SolverVarType{
	VarMultiple, VarUnique
};

class SolverConfig{
public:
	int is_sum_objective;
	int is_var_unique;
	float thresh;
	float approx_factor;
	SolverConfig(){
		is_sum_objective = 1;
		is_var_unique = 1;
		thresh = 0.9;
		approx_factor = 1.01;
	}
};

class SolverOut{
public:
	std::map<Qubit*, int> current_map;
	int success;

	SolverOut(){
	}
};

class MapperStats{
  public:
    int z3_arith_conflicts;
    int z3_conflicts;
    int z3_arith_add_rows;
    int z3_mk_clause;
    int z3_added_eqs;
    int z3_binary_propagations;
    int z3_restarts;
    int z3_decisions;
};


class Mapper{
public:
	std::map<Qubit*, int> qubit_map;
	Machine *M;
	Circuit *C;
	map<int, z3::expr*> hw_loc;
	map<int, z3::expr*> hw_reliab;
	z3::context *ctx;
	z3::solver *opt;
	SolverConfig config;
	MapperStats mapper_stats;

	map<pair<int, int>, z3::expr* > all_cx_pairs;

	z3::expr reliab(Gate *g);
	z3::expr loc(Qubit *q){
		return *hw_loc[q->id];
	}
	Mapper(Machine *machine, Circuit *circuit){
		M = machine;
		C = circuit;
		ctx = 0;
		opt = 0;
		assert(C->qubits.size() <= M->qubits.size());
		mapper_stats.z3_arith_conflicts = 0;
		mapper_stats.z3_conflicts = 0;
		mapper_stats.z3_arith_add_rows = 0;
		mapper_stats.z3_mk_clause = 0;
		mapper_stats.z3_added_eqs = 0;
		mapper_stats.z3_binary_propagations = 0;
		mapper_stats.z3_restarts = 0;
		mapper_stats.z3_decisions = 0;
	}

	~Mapper(){
	}

	void dummy_mapper();
	SolverOut* map_with_z3();
	SolverOut* solve_one_instance(float t);
	int log_reliab(float val);
	void dummy_mapper_perm(int j);
	void set_config(int obj_type, int var_type);
	void accumulate_stats(stats S);
	void print_stats();
private:
	void create_solver_vars();
	void generate_constraints();
	void solve_optimization(SolverOut &sp);
	void cleanup_solver();
};

#endif /* MAPPER_HPP_ */
