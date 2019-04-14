/*
 * optimize_1q.cpp
 *
 *  Created on: Sep 28, 2018
 *      Author: prakash
 */

#include "circuit.hpp"
#include "optimize_1q.hpp"
#include "quaternion.hpp"

/**
 * This implementation is adapted from the corresponding pass in Qiskit
 * qiskit/transpiler/passes/optimize_1q_gates.py
 */

int is_multiple_of_2pi(double angle){
	double thresh = 0.000001;
	if(fabs(angle) <= thresh) return 1;

	double tmp = angle;
	while(1){
		tmp = tmp - 2*PI;
		if(fabs(tmp) <= thresh){
			return 1;
		}
		if(tmp < 0){
			return 0;
		}
	}
}

void print_rot_sequence(vector <URotate*> rot_seq){
	for(auto g : rot_seq){
		g->print();
	}
}

URotate* Optimize1QGates::create_U1_gate(double lambda){
	URotate *g = new URotate();
	g->type = 1;
	g->theta = 0;
	g->phi = 0;
	g->lambda = lambda;
}

URotate* Optimize1QGates::create_U2_gate(double phi, double lambda){
	URotate *g = new URotate();
	g->type = 2;
	g->theta = PI/2.0;
	g->phi = phi;
	g->lambda = lambda;
}

URotate* Optimize1QGates::create_U3_gate(double theta, double phi, double lambda){
	URotate *g = new URotate();
	g->type = 3;
	g->theta = theta;
	g->phi = phi;
	g->lambda = lambda;
}

void Optimize1QGates::decompose(){
	//cout << "optimizing...\n";
	for(int i = in_gates.size()-1; i >= 0; i--){
		Gate *g = in_gates[i];
		if(typeid(*g) == typeid(X)){
			rot_seq.push_back(create_U3_gate(PI, 0, PI));
		}else if(typeid(*g) == typeid(Y)){
			rot_seq.push_back(create_U3_gate(PI, PI/2.0, PI/2.0));
		}else if(typeid(*g) == typeid(Z)){
			rot_seq.push_back(create_U1_gate(PI));
		}else if(typeid(*g) == typeid(H)){
			rot_seq.push_back(create_U2_gate(0, PI));
		}else if(typeid(*g) == typeid(S)){
			rot_seq.push_back(create_U1_gate(PI/2));
		}else if(typeid(*g) == typeid(Sdag)){
			rot_seq.push_back(create_U1_gate(-PI/2));
		}else if(typeid(*g) == typeid(T)){
			rot_seq.push_back(create_U1_gate(PI/4));
		}else if(typeid(*g) == typeid(Tdag)){
			rot_seq.push_back(create_U1_gate(-PI/4));
		}else if(typeid(*g) == typeid(RX)){
			rot_seq.push_back( create_U3_gate(g->angle, -PI/2.0, PI/2.0));
		}else if(typeid(*g) == typeid(RY)){
			rot_seq.push_back( create_U3_gate(g->angle, 0, 0) );
		}else if(typeid(*g) == typeid(RZ)){
			rot_seq.push_back(create_U1_gate(g->angle));
		}else{
			assert(0);
		}
	}
	//print_rot_sequence(rot_seq);
}

void Optimize1QGates::yzy_to_zyz(double xi, double theta1, double theta2, double& thetap, double& phip, double& lambdap){
	Quaternion* Q = Quaternion::make_quaternion_from_euler(theta1, xi, theta2, "yzy");
	Q->to_zyz();
	thetap = Q->e[1];
	phip = Q->e[0];
	lambdap = Q->e[2];
}

void Optimize1QGates::merge_u3_gates(URotate* g1, URotate* g2){
	double thetap, phip, lambdap;
	yzy_to_zyz(g1->lambda + g2->phi, g1->theta, g2->theta, thetap, phip, lambdap);

	g2->theta = thetap;
	g2->phi = g1->phi + phip;
	g2->lambda = g2->lambda + lambdap;
}

void Optimize1QGates::optimize(){
	cout << setprecision(16);
	int prev_size=rot_seq.size();
	while(1){
		if(rot_seq.size() == 1) break;
		int m = rot_seq.size()-1;
		set <URotate*> marked_for_delete;
		for(int i=0; i<m; i++){
			URotate *g1 = rot_seq[i];
			URotate *g2 = rot_seq[i+1];
			if(marked_for_delete.find(g1) != marked_for_delete.end()) continue;

			//U1, U1
			if(g1->type == 1 && g2->type == 1){
				g2->type = 1;
				g2->theta = 0;
				g2->phi = 0;
				g2->lambda += g1->lambda;
				marked_for_delete.insert(g1);
			}
			//U1, U2
			else if(g1->type == 1 && g2->type == 2){
				g2->type = 2;
				g2->theta = PI/2;
				g2->phi += g1->lambda;
				marked_for_delete.insert(g1);
			}
			//U2, U1
			else if(g1->type == 2 && g2->type == 1){
				g2->type = 2;
				g2->theta = PI/2;
				g2->phi = g1->phi;
				g2->lambda += g1->lambda;
				marked_for_delete.insert(g1);
			}
			//U1, U3
			else if(g1->type == 1 && g2->type == 3){
				g2->phi += g1->lambda;
				marked_for_delete.insert(g1);
			}
			//U3, U1
			else if(g1->type == 3 && g2->type == 1){
				g2->type = 3;
				g2->theta = g1->theta;
				g2->phi = g1->phi;
				g2->lambda += g1->lambda;
				marked_for_delete.insert(g1);
			}
			//U2, U2
			else if(g1->type == 2 && g2->type == 2){
				g2->type = 3;
				g2->theta = PI - g1->lambda - g2->phi;
				g2->phi = g1->phi + PI/2.0;
				g2->lambda = g2->lambda + PI/2.0;
				marked_for_delete.insert(g1);
			}
			//U2, U3 or U3, U2 or U3, U3
			else{
				int case1, case2, case3;
				case1 = (g1->type == 2 && g2->type == 3);
				case2 = (g1->type == 3 && g2->type == 2);
				case3 = (g1->type == 3 && g2->type == 3);
				assert(case1 || case2 || case3);

				if(g1->type == 2) g1->theta = PI/2.0;
				if(g2->type == 2) g2->theta = PI/2.0;
				g1->type = 3;
				g2->type = 3;
				//cout << "merging u3s\n";
				//g1->print();
				//g2->print();
				merge_u3_gates(g1, g2);
				//cout << "\t"; g2->print();
				marked_for_delete.insert(g1);
			}

			if(g2->type != 1 && is_multiple_of_2pi(g2->theta)){
				g2->type = 1;
				g2->lambda = g2->theta + g2->phi + g2->lambda;
				g2->theta = 0;
				g2->phi = 0;
			}
			if(g2->type == 3){
				int f1 = is_multiple_of_2pi(g2->theta - (PI/2));
				int f2 = is_multiple_of_2pi(g2->theta + (PI/2));
				if(f1){
					g2->type = 2;
					double new_theta, new_phi, new_lambda;
					new_theta = PI/2;
					new_phi = g2->phi;
					new_lambda = g2->lambda + g2->theta - PI/2;
					g2->theta = new_theta;
					g2->phi = new_phi;
					g2->lambda = new_lambda;
				}
				if(f2){
					g2->type = 2;
					double new_theta, new_phi, new_lambda;
					new_theta = PI / 2;
					new_phi = g2->phi + PI;
					new_lambda = g2->lambda + g2->theta - PI / 2;
					g2->theta = new_theta;
					g2->phi = new_phi;
					g2->lambda = new_lambda;
				}
			}
			if(g2->type == 1){
				if(is_multiple_of_2pi(g2->lambda)){
					g2->lambda = 0;
					g2->theta = 0;
					g2->phi = 0;
				}
			}
		}
		for(auto urot : marked_for_delete){
			rot_seq.erase(std::remove(rot_seq.begin(), rot_seq.end(), urot), rot_seq.end());
		}
		if(rot_seq.size() == prev_size){
			break;
		}else{
			prev_size = rot_seq.size();
		}
	}
	//cout << "after optimize\n";
	//print_rot_sequence(rot_seq);
}

vector<Gate*> Optimize1QGates::get_optimized_gates(){
	vector<Gate*> optimized_gates;
	for(int i=rot_seq.size()-1; i>=0; i--){
		URotate *u = rot_seq[i];
		Gate *g;
		if(u->type == 1){
			g = Gate::create_new_gate("U1");
		}else if(u->type == 2){
			g = Gate::create_new_gate("U2");
		}else if(u->type == 3){
			g = Gate::create_new_gate("U3");
		}else assert(0);
		g->theta = u->theta;
		g->phi = u->phi;
		g->lambda = u->lambda;
		optimized_gates.push_back(g);
	}
	return optimized_gates;
}


void OptimizeSingleQubitOps::optimize(){
	for(auto it : runs){
		//cout << "Procecssing run with head " << it.first->id << endl;
		Qubit *q = it.first->vars[0];
		vector<Gate*> my_run = it.second;
		Optimize1QGates opt(my_run);
		opt.optimize();
		vector <Gate*> new_gates = opt.get_optimized_gates();
		for(auto ngate : new_gates){
				ngate->vars.push_back(q);
		}
		C->replace_with_chain(my_run, new_gates);
	}
}

void OptimizeSingleQubitOps::extract_runs(){
	//init: every gate in its own run
	for (auto g : C->gates) {
		if (g->nvars == 1 && typeid(*g) != typeid(MeasZ)) {
			vector<Gate*> g_set;
			g_set.push_back(g);
			runs[g] = g_set;
		}
	}
	vector<Gate*> top_order = *(C->topological_ordering());
	for(auto it = top_order.rbegin(); it != top_order.rend(); it++){
		Gate *g = *it;
		if(g->nvars != 1 || typeid(*g) == typeid(MeasZ)) continue;
		for(auto p : g->pred){
			if(p->nvars != 1 || typeid(*p) == typeid(MeasZ)) continue;
			if(p->vars[0] == g->vars[0] && g->pred.size() == 1) {
				//p is my parent
				for(auto t : runs[g]){
					runs[p].push_back(t);
				}
				auto run_it = runs.find(g);
				runs.erase(run_it);
			}
		}
	}
	/*
	for(auto it : runs){
		cout << it.first->id << endl;
		for(auto g : it.second){
			g->print();
		}
		cout << endl;
	}*/
}

vector<Gate*> OptimizeSingleQubitOps::get_optimized_layer(vector <Gate*> gate_list){
	set <Qubit*> qlist;
	for(auto g : gate_list){
		assert(g->nvars == 1);
		qlist.insert(g->vars[0]);
	}
	vector<Gate*> return_list;
	for(auto q : qlist){
		vector<Gate*> q_gate_list;
		for(auto g : gate_list){
			if(g->vars[0] == q){
				q_gate_list.push_back(g);
			}
		}
		Optimize1QGates opt(q_gate_list);
		opt.optimize();
		vector <Gate*> new_gates = opt.get_optimized_gates();
		for(auto ngate : new_gates){
				ngate->vars.push_back(q);
				return_list.push_back(ngate);
		}
	}
	return return_list;
}

Circuit *OptimizeSingleQubitOps::test_optimize(){
	vector<Gate*> top_order = *(C->topological_ordering());
	vector<Gate*> *current_run = new vector<Gate*>;
	int dirty = 0;
	vector<Gate*> final_gate_list;
	for(auto g : top_order){
		if(g->nvars == 2){
			//for(auto pg : *current_run){
			//	pg->print();
			//}
			//cout << "\n";
			auto new_list = get_optimized_layer(*current_run);
			for (auto pg : new_list) {
				//pg->print();
				final_gate_list.push_back(pg);
			}
			//cout << "\n";
			Gate *new_gate = Gate::create_new_gate(g->printStr);
			new_gate->nvars = 2;
			new_gate->vars.push_back(g->vars[0]);
			new_gate->vars.push_back(g->vars[1]);
			if(typeid(*new_gate) == typeid(XX)){
				new_gate->angle = g->angle;
			}
			final_gate_list.push_back(new_gate);
			delete current_run;
			current_run = new vector<Gate*>;
			dirty = 0;
		}else{
			if(typeid(*g) != typeid(MeasZ)){
				current_run->push_back(g);
				dirty = 1;
			}
		}
	}
	if(dirty){
		auto new_list = get_optimized_layer(*current_run);
		for (auto pg : new_list) {
			final_gate_list.push_back(pg);
		}
		delete current_run;
	}
	for(unsigned int i=0; i < final_gate_list.size()-1; i++){
		Gate *cur, *nxt;
		cur = final_gate_list[i];
		nxt = final_gate_list[i+1];
		cur->succ.push_back(nxt);
		nxt->pred.push_back(cur);
	}
	Circuit *newC = new Circuit;
	newC->qubits = C->qubits;
	newC->gates = final_gate_list;
	return newC;
}
