/*
 * qubit.hpp
 *
 *  Created on: Sep 2, 2018
 *      Author: prakash
 */

#ifndef QUBIT_HPP_
#define QUBIT_HPP_

class Qubit{
public:
	int id;
	int partition;
	Qubit(int qid){
		id = qid;
		partition = 0;
	}
};

class HwQubit: public Qubit{
public:
	HwQubit(int qid) : Qubit(qid){
	}
	~HwQubit();
};

class ProgQubit: public Qubit{
public:
	ProgQubit(int qid): Qubit(qid){
	}
	~ProgQubit();
};

#endif /* QUBIT_HPP_ */
