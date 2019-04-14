/*
 * headers.hpp
 *
 *  Created on: Sep 2, 2018
 *      Author: prakash
 */

#ifndef HEADERS_HPP_
#define HEADERS_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <stack>
#include <set>
#include <algorithm>
#include <iterator>
#include <typeinfo>
#include <utility>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <iomanip>
#include <cmath>
#include <limits>
#include <time.h>

/* Library Dependencies: Boost, Z3 */
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graphviz.hpp>
#include "z3++.h"

using namespace std;
const double PI = 3.141592653589793238;


enum CompileAlgorithm{
	CompileOpt, CompileDijkstra, CompileRevSwaps
};

extern int CompilerAlgorithm;
extern int ErrorScaleFactor;

#endif /* HEADERS_HPP_ */
