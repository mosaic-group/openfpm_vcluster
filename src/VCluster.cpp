#include "VCluster.hpp"

Vcluster * global_v_cluster = NULL;

/*! \brief Initialize a global instance of Runtime Virtual Cluster Machine
 *
 * Initialize a global instance of Runtime Virtual Cluster Machine
 *
 */

void init_global_v_cluster(int *argc, char ***argv)
{
	if (global_v_cluster == NULL)
		global_v_cluster = new Vcluster(argc,argv);
}

// Global MPI initialization
bool global_mpi_init = false;

// number of vcluster instances
size_t n_vcluster = 0;
