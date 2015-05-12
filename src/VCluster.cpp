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

// Deallocator object, it deallocate the global_v_cluster at the end of the program
class init_glob_v_cluster
{
public:
	~init_glob_v_cluster()
	{
		delete global_v_cluster;
	};
};

// Deallocate at the end
init_glob_v_cluster v_cls;

bool global_mpi_initialization = false;
