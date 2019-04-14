/*
 * mapper.cpp
 *
 *  Created on: Sep 3, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "mapper.hpp"
#include "circuit.hpp"

using namespace z3;

pair<int, int> get_cx_pair(Gate *g){
	assert(g->nvars == 2);
	int qid1, qid2;
	if(g->vars[0]->id <= g->vars[1]->id){
		qid1 = g->vars[0]->id;
		qid2 = g->vars[1]->id;
	}else{
		qid2 = g->vars[0]->id;
		qid1 = g->vars[1]->id;
	}
	return make_pair(qid1, qid2);
}

void Mapper::set_config(int obj_type, int var_type){
	config.is_sum_objective = obj_type;
	config.is_var_unique = var_type;
}

void Mapper::dummy_mapper(){
	for(int i=0; i<C->qubits.size(); i++){
		qubit_map[C->qubits[i]] = i;
	}
}

void Mapper::dummy_mapper_perm(int j){
	int qubits[16];
	for(int i=0; i<C->qubits.size(); i++){
		qubits[i] = i;
	}
	int c=0;
	do {
		if (c == j) {
			for (int i = 0; i < C->qubits.size(); i++) {
				qubit_map[C->qubits[i]] = qubits[i];
				C->qubits[i]->id = qubits[i];
			}
			break;
		}
		c++;
	} while ( std::next_permutation(qubits,qubits+C->qubits.size()) );
}

int Mapper::log_reliab(float val) {
	assert(val > 0 && val <= 1.0);
	if(config.is_sum_objective == MapSum){
		return int(1000 * log(val));
	}else if(config.is_sum_objective == MapMin){
		return int(1000 * val);
	}else assert(0);
}

z3::expr Mapper::reliab(Gate *g){
	if(g->nvars == 2){
		if(config.is_var_unique == VarMultiple){
			return *hw_reliab[g->id];
		}else if(config.is_var_unique == VarUnique){
			pair<int, int> my_pair = get_cx_pair(g);
			return *(all_cx_pairs[my_pair]);
		}
	}else{
		return *hw_reliab[g->id];
	}
}

void Mapper::create_solver_vars(){
	ctx = new z3::context;
	opt = new z3::solver(*ctx);
	all_cx_pairs.clear();

	int i;
	for (i = 0; i < C->qubits.size(); i++) {
		stringstream ss;
		ss << "q";
		ss << C->qubits[i]->id;
		hw_loc[C->qubits[i]->id] = new z3::expr(ctx->int_const(ss.str().c_str()));
	}
	for (i = 0; i < C->gates.size(); i++) {
		stringstream ss;
		ss << "g";
		ss << C->gates[i]->id;
		ss << "_reliab";
		hw_reliab[C->gates[i]->id] = new expr(ctx->int_const(ss.str().c_str()));

		if (config.is_var_unique == VarUnique && (C->gates[i]->nvars == 2)) {
			pair<int, int> my_pair = get_cx_pair(C->gates[i]);
			if (all_cx_pairs.find(my_pair) == all_cx_pairs.end()) {
				stringstream scx;
				scx << "cx" << my_pair.first << "_" << my_pair.second << "_reliab";
				all_cx_pairs[my_pair] = new z3::expr(ctx->int_const(scx.str().c_str()));
			}
		}
	}
}

void Mapper::generate_constraints() {
	//mapping constraints
	int qcnt = C->qubits.size();
	int gcnt = C->gates.size();

	for (int i = 0; i < qcnt; i++) {
		Qubit *Qi = C->qubits[i];
		expr cons = (loc(Qi) >= 0) && (loc(Qi) < M->nQ);
		opt->add(cons);
	}
	for (int i = 0; i < qcnt; i++) {
		Qubit *Qi = C->qubits[i];
		for (int j = i + 1; j < qcnt; j++) {
			Qubit *Qj = C->qubits[j];
			expr cons = (loc(Qi) != loc(Qj));
			opt->add(cons);
		}
	}

	map< pair<int, int>, int> cx_cnt;
	for(auto pairs : all_cx_pairs){
		cx_cnt[pairs.first] = 0;
	}

	//reliability constraints
	for (int i = 0; i < gcnt; i++) {
		Gate *Gi = C->gates[i];
		if (typeid(*Gi) == typeid(CNOT)) {
			Qubit *ctrl = Gi->vars[0];
			Qubit *targ = Gi->vars[1];
			if(config.is_var_unique == VarUnique){
				if(cx_cnt[get_cx_pair(Gi)] > 0){
					cx_cnt[get_cx_pair(Gi)]++;
					continue;
					//This is to avoid adding redundant reliability-set constraints for the same CNOT qubit pair
				}else{
					cx_cnt[get_cx_pair(Gi)]++;
				}
			}
			for (int p1 = 0; p1 < M->nQ; p1++) {
				for (int p2 = 0; p2 < M->nQ; p2++) {
					if (p1 == p2)
						continue;
					expr is_loc = ((loc(ctrl) == p1) && (loc(targ) == p2));
					if ((*(M->t_reliab[p1]))[p2] < 0) {
						assert(0);
						//opt->add(!is_loc);
					} else {
						//Constraints to specify the reliability based on mapping
						int r = log_reliab((*(M->t_reliab[p1]))[p2]);
						expr cons = implies(is_loc, reliab(Gi) == r);
						opt->add(cons);
					}
				}
			}
		}
	}


	for (int i = 0; i < gcnt; i++) {
		Gate *Gi = C->gates[i];
		if (typeid(*Gi) == typeid(MeasZ)) {
			Qubit *Qi = Gi->vars[0];
			for (int p1 = 0; p1 < M->nQ; p1++) {
				expr is_loc = (loc(Qi) == p1);
				int r = log_reliab(M->m_reliab[p1]);
				expr cons = implies(is_loc, reliab(Gi) == r);
				opt->add(cons);
			}

		}
	}

	//sum objective
	if (config.is_sum_objective == MapSum) {
		expr_vector opr_reliab(*ctx);
		int cnt = 0;
		for (int i = 0; i < gcnt; i++) {
			Gate *Gi = C->gates[i];
			if (Gi->vars.size() == 2 || typeid(*Gi) == typeid(MeasZ)) {
				opr_reliab.push_back(reliab(Gi));
				cnt++;
			}
		}
		expr s = sum(opr_reliab);
		opt->add(s >= (log_reliab(config.thresh) * cnt));
		expr sum_test = s >= (log_reliab(config.thresh) * cnt);
		//cout << sum_test << endl;
	} else if(config.is_sum_objective == MapMin){
		//individual objectives

		for(auto k1 : all_cx_pairs){
			for(auto k2 : all_cx_pairs){
				if(k1 == k2) continue;
				int cx1 = cx_cnt[k1.first];
				int cx2 = cx_cnt[k2.first];
				if (cx1 != 0 && cx2 != 0) {
					if (cx1 >= 2 * cx2) {
						opt->add(*(k1.second) >= *(k2.second));
						//cout << "Adding freq. constraint:" << "(" << k1.first.first << " " << k1.first.second << "),(" << k2.first.first << " " << k2.first.second << ")" << "\n";
					}else
						;
				}
			}
		}
		for(auto pairs : all_cx_pairs){
			cx_cnt[pairs.first] = 0;
		}
		for (int i = 0; i < gcnt; i++) {
			Gate *Gi = C->gates[i];
			if (Gi->vars.size() == 2 || typeid(*Gi) == typeid(MeasZ)) {
				if(typeid(*Gi) == typeid(MeasZ)){
					opt->add(reliab(Gi) >= log_reliab(config.thresh));
					expr test = reliab(Gi) >= log_reliab(config.thresh);
				}else{
					if(cx_cnt[get_cx_pair(Gi)] == 0){
						opt->add(reliab(Gi) >= log_reliab(config.thresh));
						cx_cnt[get_cx_pair(Gi)] = 1;
						expr test = reliab(Gi) >= log_reliab(config.thresh);
					}
				}
			}
		}
	} else {
		assert(0);
	}
}

void Mapper::solve_optimization(SolverOut &sp) {
	if (opt->check() == sat) {
		model m = opt->get_model();
		sp.success = 1;
		for (unsigned int i = 0; i < C->qubits.size(); i++) {
			sp.current_map[C->qubits[i]] = m.eval(loc(C->qubits[i])) .get_numeral_int();
		}
	} else {
		sp.success = 0;
	}
	stats solve_stats = opt->statistics();
	accumulate_stats(solve_stats);
}

void Mapper::cleanup_solver(){
	if(opt != 0){
		delete opt;
	}
	if(ctx != 0){
		delete ctx;
	}
}

SolverOut* Mapper::map_with_z3() {
	float low, mid, high;
	float approx;
	low = 0;
	high = 1;
	SolverOut *sol = 0;
	while (1) {
		mid = (low + high) / 2;
		SolverOut *out = solve_one_instance(mid);
		if (out->success == 1) {
			low = mid;
			if (sol != 0)
				delete sol;
			sol = out;
		} else {
			high = mid;
			delete out;
		}
		if (high / low <= config.approx_factor)
			break;
	}

	int i=0;
	cout << "Qubit Mapping\n";
	for(i=0; i<C->qubits.size(); i++){
		qubit_map[C->qubits[i]] = sol->current_map[C->qubits[i]];
		cout << "(" << C->qubits[i]->id << "," << qubit_map[C->qubits[i]] << ")" << endl;
	}
	return sol;
}

SolverOut* Mapper::solve_one_instance(float t){
	cout << "Solving with thresh=" << t << endl;
	config.thresh = t;
	SolverOut *sp = new SolverOut;
	create_solver_vars();
	generate_constraints();
	solve_optimization(*sp);
	cleanup_solver();
	return sp;
}



void Mapper::accumulate_stats(stats S){
  int i;
  for(unsigned i=0; i<S.size()-1; i++){
    string k = S.key(i);
    int val;
    if(S.is_uint(i))
      val = S.uint_value(i);
    else
      continue;
    if(k == "decisions") mapper_stats.z3_decisions += val;
    if(k == "arith conflicts") mapper_stats.z3_arith_conflicts += val;
    if(k == "conflicts") mapper_stats.z3_conflicts += val;
    if(k == "arith add rows") mapper_stats.z3_arith_add_rows += val;
    if(k == "mk clause") mapper_stats.z3_mk_clause += val;
    if(k == "added eqs") mapper_stats.z3_added_eqs += val;
    if(k == "binary propagations") mapper_stats.z3_binary_propagations += val;
    if(k == "restarts") mapper_stats.z3_restarts += val;

  }
}

void Mapper::print_stats(){
  cout << "Stats:\n";
  cout << "arith_conflicts " << mapper_stats.z3_arith_conflicts << endl;
  cout << "arith_add_rows " << mapper_stats.z3_arith_add_rows << endl;
  cout << "conflicts " <<  mapper_stats.z3_conflicts << endl;
  cout << "decisions " << mapper_stats.z3_decisions << endl;
  cout << "mk_clause " <<  mapper_stats.z3_mk_clause << endl;
  cout << "added_eqs " << mapper_stats.z3_added_eqs << endl;
  cout << "binary_propagations " << mapper_stats.z3_binary_propagations << endl;
  cout << "restarts " << mapper_stats.z3_restarts << endl;
}

