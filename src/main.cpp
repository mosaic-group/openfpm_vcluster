#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#ifndef NO_INIT_AND_MAIN

// initialization function:
bool init_unit_test()
{
  return true;
}

// entry point:
int main(int argc, char* argv[])
{
	return boost::unit_test::unit_test_main( &init_unit_test, argc, argv );
}

#include "unit_test_init_cleanup.hpp"

#endif

#include "config.h"
#include "VCluster/VCluster.hpp"

