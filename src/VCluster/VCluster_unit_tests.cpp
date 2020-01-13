/*
 * VCluster_unit_tests.hpp
 *
 *  Created on: May 9, 2015
 *      Author: Pietro incardona
 */

#include "config.h"

#include <sstream>
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "timer.hpp"
#include <random>
#include "VCluster_unit_test_util.hpp"
#include "Point_test.hpp"
#include "VCluster_base.hpp"
#include "Vector/vector_test_util.hpp"
#include "VCluster/VCluster.hpp"
#include "VCluster/cuda/VCluster_unit_test_util_cuda.cuh"

BOOST_AUTO_TEST_SUITE( VCluster_test )

BOOST_AUTO_TEST_CASE (Vcluster_robustness)
{
	Vcluster<> & vcl = create_vcluster();

	vcl.execute();
}

BOOST_AUTO_TEST_CASE( VCluster_use_reductions)
{

	//! [max min sum]

	Vcluster<> & vcl = create_vcluster();

	unsigned char uc = 1;
	char c = 1;
	short s = 1;
	unsigned short us = 1;
	int i = 1;
	unsigned int ui = 1;
	long int li = 1;
	unsigned long int uli = 1;
	float f = 1;
	double d = 1;

	unsigned char uc_max = vcl.getProcessUnitID();
	char c_max = vcl.getProcessUnitID();
	short s_max = vcl.getProcessUnitID();
	unsigned short us_max = vcl.getProcessUnitID();
	int i_max = vcl.getProcessUnitID();
	unsigned int ui_max = vcl.getProcessUnitID();
	long int li_max = vcl.getProcessUnitID();
	unsigned long int uli_max = vcl.getProcessUnitID();
	float f_max = vcl.getProcessUnitID();
	double d_max = vcl.getProcessUnitID();

	// Sum reductions
	if ( vcl.getProcessingUnits() < 128 )
		vcl.sum(c);
	if ( vcl.getProcessingUnits() < 256 )
		vcl.sum(uc);
	if ( vcl.getProcessingUnits() < 32768 )
		vcl.sum(s);
	if ( vcl.getProcessingUnits() < 65536 )
		vcl.sum(us);
	if ( vcl.getProcessingUnits() < 2147483648 )
		vcl.sum(i);
	if ( vcl.getProcessingUnits() < 4294967296 )
		vcl.sum(ui);
	vcl.sum(li);
	vcl.sum(uli);
	vcl.sum(f);
	vcl.sum(d);

	// Max reduction
	if ( vcl.getProcessingUnits() < 128 )
		vcl.max(c_max);
	if ( vcl.getProcessingUnits() < 256 )
		vcl.max(uc_max);
	if ( vcl.getProcessingUnits() < 32768 )
		vcl.max(s_max);
	if ( vcl.getProcessingUnits() < 65536 )
		vcl.max(us_max);
	if ( vcl.getProcessingUnits() < 2147483648 )
		vcl.max(i_max);
	if ( vcl.getProcessingUnits() < 4294967296 )
		vcl.max(ui_max);
	vcl.max(li_max);
	vcl.max(uli_max);
	vcl.max(f_max);
	vcl.max(d_max);
	vcl.execute();

	//! [max min sum]

	if ( vcl.getProcessingUnits() < 128 )
	{BOOST_REQUIRE_EQUAL(c_max,(char)vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 256 )
	{BOOST_REQUIRE_EQUAL(uc_max,(unsigned char)vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 32768 )
	{BOOST_REQUIRE_EQUAL(s_max,(short int) vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 65536 )
	{BOOST_REQUIRE_EQUAL(us_max,(unsigned short)vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 2147483648 )
	{BOOST_REQUIRE_EQUAL(i_max,(int)vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 4294967296 )
	{BOOST_REQUIRE_EQUAL(ui_max,(unsigned int)vcl.getProcessingUnits()-1);}

	BOOST_REQUIRE_EQUAL(li_max,(long int)vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(uli_max,(unsigned long int)vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(f_max,(float)vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(d_max,(double)vcl.getProcessingUnits()-1);
}

#define N_V_ELEMENTS 16

BOOST_AUTO_TEST_CASE(VCluster_send_recv)
{
	Vcluster<> & vcl = create_vcluster();

	test_send_recv_complex(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<unsigned char>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<char>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<short>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<unsigned short>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<int>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<unsigned int>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<long int>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<unsigned long int>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<float>(N_V_ELEMENTS,vcl);
	test_send_recv_primitives<double>(N_V_ELEMENTS,vcl);
}

BOOST_AUTO_TEST_CASE(VCluster_allgather)
{
	Vcluster<> & vcl = create_vcluster();

	if (vcl.getProcessingUnits() < 256)
		test_single_all_gather_primitives<unsigned char>(vcl);

	if (vcl.getProcessingUnits() < 128)
		test_single_all_gather_primitives<char>(vcl);

	test_single_all_gather_primitives<short>(vcl);
	test_single_all_gather_primitives<unsigned short>(vcl);
	test_single_all_gather_primitives<int>(vcl);
	test_single_all_gather_primitives<unsigned int>(vcl);
	test_single_all_gather_primitives<long int>(vcl);
	test_single_all_gather_primitives<unsigned long int>(vcl);
	test_single_all_gather_primitives<float>(vcl);
	test_single_all_gather_primitives<double>(vcl);
}

struct brt_test
{
	double a;
	double b;
};

BOOST_AUTO_TEST_CASE(VCluster_bcast_test)
{
	Vcluster<> & vcl = create_vcluster();

	std::cout << "Broadcast test " << std::endl;

	test_single_all_broadcast_primitives<unsigned char,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<char,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<short,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<unsigned short,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<int,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<unsigned int,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<long int,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<unsigned long int,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<float,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_primitives<double,HeapMemory,memory_traits_lin>(vcl);
}

BOOST_AUTO_TEST_CASE(VCluster_bcast_complex_test)
{
	Vcluster<> & vcl = create_vcluster();

	std::cout << "Broadcast complex test " << std::endl;

	test_single_all_broadcast_complex<aggregate<int,int>,HeapMemory,memory_traits_lin>(vcl);
	test_single_all_broadcast_complex<aggregate<int,int>,HeapMemory,memory_traits_inte>(vcl);
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_short_unkn)
{
	std::cout << "VCluster unit test start sendrecv short unknown" << "\n";

	totp_check = false;
	test_short<NBX>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv short unknown" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_short_unkn_async)
{
	std::cout << "VCluster unit test start sendrecv short unknown async" << "\n";

	totp_check = false;
	test_short<NBX_ASYNC>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv short unknown async" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_short_unkn_async_multiple)
{
	std::cout << "VCluster unit test start sendrecv short unknown async multiple" << "\n";

	totp_check = false;
	test_short_multiple<NBX_ASYNC>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv short unknown async multiple" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_rand_unkn)
{
	std::cout << "VCluster unit test start sendrecv random unknown" << "\n";

	totp_check = false;
	test_random<NBX>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv random unknown" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_rand_unkn_async)
{
	std::cout << "VCluster unit test start sendrecv random unknown async" << "\n";

	totp_check = false;
	test_random<NBX_ASYNC>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv random unknown async" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_rand_unkn_async_multiple)
{
	std::cout << "VCluster unit test start sendrecv random unknown async" << "\n";

	totp_check = false;
	test_random_multiple<NBX_ASYNC>(RECEIVE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv random unknown async" << "\n";
}


BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_someempty)
{
	std::cout << "VCluster unit test start sendrecv some empty" << "\n";

	totp_check = false;
	test_no_send_some_peer<NBX>();

	std::cout << "VCluster unit test stop sendrecv some empty" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_short_prc_known)
{
	std::cout << "VCluster unit test start sendrecv short known prc" << "\n";

	totp_check = false;
	test_short<NBX>(RECEIVE_SIZE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv short known prc" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_short_prc_known_multiple)
{
	std::cout << "VCluster unit test start sendrecv short known prc" << "\n";

	totp_check = false;
	test_short_multiple<NBX>(RECEIVE_SIZE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv short known prc" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_random_prc_known)
{
	std::cout << "VCluster unit test start sendrecv random known prc" << "\n";

	totp_check = false;
	test_random<NBX>(RECEIVE_SIZE_UNKNOWN);

	std::cout << "VCluster unit test stop sendrecv random known prc" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known )
{
	std::cout << "VCluster unit test start known" << "\n";

	test_known<NBX>(0);

	std::cout << "VCluster unit test stop known" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known_async )
{
	std::cout << "VCluster unit test start known" << "\n";

	test_known<NBX_ASYNC>(0);

	std::cout << "VCluster unit test stop known" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known_async_multiple )
{
	std::cout << "VCluster unit test start known" << "\n";

	test_known_multiple<NBX_ASYNC>(0);

	std::cout << "VCluster unit test stop known" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known_prc )
{
	std::cout << "VCluster unit test start known prc" << "\n";

	test_known<NBX>(KNOWN_PRC);

	std::cout << "VCluster unit test stop known prc" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known_prc_async )
{
	std::cout << "VCluster unit test start known prc" << "\n";

	test_known<NBX_ASYNC>(KNOWN_PRC);

	std::cout << "VCluster unit test stop known prc" << "\n";
}

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv_known_prc_async_multiple )
{
	std::cout << "VCluster unit test start known prc" << "\n";

	test_known_multiple<NBX_ASYNC>(KNOWN_PRC);

	std::cout << "VCluster unit test stop known prc" << "\n";
}

BOOST_AUTO_TEST_SUITE_END()

