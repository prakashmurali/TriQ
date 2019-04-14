/*
 * quaternion.hpp
 *
 *  Created on: Oct 29, 2018
 *      Author: prakash
 */

#ifndef QUATERNION_HPP_
#define QUATERNION_HPP_

class Quaternion{
public:
	double d[4];
	double e[3]; //euler angles
	double mat[3][3];

	Quaternion(double w, double x, double y, double z){
		d[0] = w;
		d[1] = x;
		d[2] = y;
		d[3] = z;
	}

	void normalize();
	static Quaternion *make_quaternion_from_euler(double angle1, double angle2, double angle3, string order);
	static Quaternion *make_quaternion_from_axis(double angle, const char& axis);
	static Quaternion *multiply(Quaternion *q1, Quaternion *q2);
	void to_zyz();
	void to_matrix();
};



#endif /* QUATERNION_HPP_ */
