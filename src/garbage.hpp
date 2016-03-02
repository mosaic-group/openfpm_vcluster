/*
 * garbage.hpp
 *
 *  Created on: Dec 14, 2015
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_GARBAGE_HPP_
#define OPENFPM_VCLUSTER_SRC_GARBAGE_HPP_


//	openfpm::vector<size_t> NN_proc;

	// Distributed processor graph
//	MPI_Comm proc_comm_graph;

	/*! \brief
	 *
	 * Set the near processor of this processors
	 *
	 */


/*	void setLocality(openfpm::vector<size_t> NN_proc)
	{
		// Number of sources in the graph, and sources processors
		size_t sources = NN_proc.size();
		openfpm::vector<int> src_proc;

		// number of destination in the graph
		size_t dest = NN_proc.size();
		openfpm::vector<int> dest_proc;

		// insert in sources and out sources
		for (size_t i = 0; i < NN_proc.size() ; i++)
		{
			src_proc.add(NN_proc.get(i));
			dest_proc.add(NN_proc.get(i));
			// Copy the vector
			this->NN_proc.get(i) = NN_proc.get(i);
		}

		MPI_Dist_graph_create_adjacent(MPI_COMM_WORLD,sources,&src_proc.get(0),(const int *)MPI_UNWEIGHTED,dest,&dest_proc.get(0),(const int *)MPI_UNWEIGHTED,MPI_INFO_NULL,true,&proc_comm_graph);
	}*/


#endif /* OPENFPM_VCLUSTER_SRC_GARBAGE_HPP_ */
