#include "VCluster.hpp"

Vcluster * global_v_cluster_private = NULL;

// number of vcluster instances
size_t n_vcluster = 0;
bool ofp_initialized = false;

size_t tot_sent = 0;
size_t tot_recv = 0;
