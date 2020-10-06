/*
 * py_runner.hpp
 *
 *  Created on: Sep 10, 2020
 *      Author: i-bird
 */

#ifndef PY_RUNNER_HPP_
#define PY_RUNNER_HPP_

#include "Vector/map_vector.hpp"

struct execute_request
{
	//! Indicate the request has been executed
	bool executed;

	//! code to run
	std::string code;

	//! standard output
	std::string out;

	//! standard error
	std::string err;

	//! payload
	openfpm::vector<unsigned char> data;
};

struct structs_list
{
	//! name of the string
	std::string name;

	//! dimensionality of the structure
	int dim;

	//! sizes
	openfpm::vector<int> sizes;

	//! Type of the grid
	std::string type;
};

void init_python_server(int argc, char **argv, int rank);
void finalize_py_server();

void step_done();
void do_breakpoint();
void add_structure(const structs_list & sa);

#endif /* PY_RUNNER_HPP_ */
