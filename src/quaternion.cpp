/*
 * quaternion.cpp
 *
 *  Created on: Oct 29, 2018
 *      Author: prakash
 */

#include "headers.hpp"
#include "quaternion.hpp"

/**
 * This implementation was adapted from the methods in Qiskit
 * qiskit/quantum_info/operators/quaternion.py
 */

void Quaternion::normalize(){
	double norm=0;
	norm = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2] + d[3]*d[3]);
	d[0] = d[0]/norm;
	d[1] = d[1]/norm;
	d[2] = d[2]/norm;
	d[3] = d[3]/norm;
}

Quaternion *Quaternion::make_quaternion_from_axis(double angle, const char& axis){
	Quaternion *q = new Quaternion(0,0,0,0);
	if(axis == 'x'){
		q->d[1] = 1;
	}else if(axis == 'y'){
		q->d[2] = 1;
	}else if(axis == 'z'){
		q->d[3] = 1;
	}else{
		assert(0);
	}
	for(int i=0; i<4; i++){
		q->d[i] = q->d[i]*sin(angle/2.0);
	}
	q->d[0] = cos(angle/2.0);
	return q;
}

Quaternion *Quaternion::multiply(Quaternion *q1, Quaternion *q2){
	Quaternion *q = new Quaternion(0,0,0,0);
	double *a = q1->d;
	double *b = q2->d;
	q->d[0] = b[0] * a[0] - b[1] * a[1] - b[2] * a[2] - b[3] * a[3];
	q->d[1] = b[0] * a[1] + b[1] * a[0] - b[2] * a[3] + b[3] * a[2];
	q->d[2] = b[0] * a[2] + b[1] * a[3] + b[2] * a[0] - b[3] * a[1];
	q->d[3] = b[0] * a[3] - b[1] * a[2] + b[2] * a[1] + b[3] * a[0];
	q->normalize();
	return q;
}

Quaternion *Quaternion::make_quaternion_from_euler(double angle1, double angle2, double angle3, string order){
	Quaternion *Q1 = Quaternion::make_quaternion_from_axis(angle1, order[0]);
	Quaternion *Q2 = Quaternion::make_quaternion_from_axis(angle2, order[1]);
	Quaternion *Q3 = Quaternion::make_quaternion_from_axis(angle3, order[2]);
	Quaternion *q = Quaternion::multiply(Quaternion::multiply(Q1, Q2), Q3);
	q->normalize();
	return q;
}

void Quaternion::to_matrix(){
	normalize();
	double w = d[0];
	double x = d[1];
	double y = d[2];
	double z = d[3];
	mat[0][0] = 1 - 2 * y * y - 2 * z * z;
	mat[0][1] = 2 * x * y - 2 * z * w;
	mat[0][2] = 2 * x * z + 2 * y * w;

	mat[1][0] = 2 * x * y + 2 * z * w;
	mat[1][1] = 1 - 2 * x * x - 2 * z * z;
	mat[1][2] = 2 * y * z - 2 * x * w;

	mat[2][0] = 2 * x * z - 2 * y * w;
	mat[2][1] = 2 * y * z + 2 * x * w;
	mat[2][2] = 1 - 2 * x * x - 2 * y * y;
}

void Quaternion::to_zyz(){
	to_matrix();
	e[0] = 0;
	e[1] = 0;
	e[2] = 0;
	if (mat[2][2] < 1) {
		if (mat[2][2] > -1) {
			e[0] = atan2(mat[1][2], mat[0][2]);
			e[1] = acos(mat[2][2]);
			e[2] = atan2(mat[2][1], -mat[2][0]);
		} else {
			e[0] = -atan2(mat[1][0], mat[1][1]);
			e[1] = PI;
		}
	}else{
		e[0] = atan2(mat[1][0], mat[1][1]);
	}
}
