#include "config.h"
#define BOOST_TEST_MODULE "C++ test module for OpenFPM_vcluster project"
#include <boost/test/included/unit_test.hpp>
#include "VCluster.hpp"

/*struct MPIFixture {
    MPIFixture() { MPI_Init(&boost::unit_test::framework::master_test_suite().argc,&boost::unit_test::framework::master_test_suite().argv);}
    ~MPIFixture() { MPI_Finalize(); }
};

BOOST_GLOBAL_FIXTURE(MPIFixture);*/

#include "unit_test_init_cleanup.hpp"
#include "VCluster_unit_tests.hpp"
