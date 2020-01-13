#include "config.h"

#include <sstream>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "timer.hpp"
#include <random>
#include "Point_test.hpp"
#include "VCluster/VCluster_base.hpp"
#include "VCluster/VCluster.hpp"
#include "VCluster/cuda/VCluster_unit_test_util_cuda.cuh"

BOOST_AUTO_TEST_SUITE( VCluster_test_cuda )

BOOST_AUTO_TEST_CASE(VCluster_bcast_complex_test)
{
	Vcluster<> & vcl = create_vcluster();

	std::cout << "Broadcast complex test CUDA" << std::endl;

	test_single_all_broadcast_complex<aggregate<int,int>,CudaMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_complex<aggregate<int,int>,CudaMemory,memory_traits_inte>(vcl);
}

BOOST_AUTO_TEST_SUITE_END()
