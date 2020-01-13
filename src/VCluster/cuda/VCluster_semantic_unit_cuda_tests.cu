
#include <hip/hip_runtime.h>
#include "config.h"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "VCluster/VCluster.hpp"
#include "VCluster/cuda/VCluster_semantic_unit_tests_funcs.hpp"



BOOST_AUTO_TEST_SUITE( VCluster_cuda_tests )

BOOST_AUTO_TEST_CASE( Vcluster_semantic_ssend_recv_layout_switch )
{
	test_ssend_recv_layout_switch<HeapMemory>(0);
}

BOOST_AUTO_TEST_CASE( Vcluster_semantic_gpu_direct )
{
	test_ssend_recv_layout_switch<CudaMemory>(MPI_GPU_DIRECT);
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_layout_inte_gather)
{
	test_different_layouts<CudaMemory,memory_traits_inte>();
	test_different_layouts<CudaMemory,memory_traits_lin>();
}

BOOST_AUTO_TEST_SUITE_END()
