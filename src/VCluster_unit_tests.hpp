/*
 * VCluster_unit_tests.hpp
 *
 *  Created on: May 9, 2015
 *      Author: Pietro incardona
 */

#ifndef VCLUSTER_UNIT_TESTS_HPP_
#define VCLUSTER_UNIT_TESTS_HPP_

#include "VCluster.hpp"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
#include "timer.hpp"
#include <random>
#include "VCluster_unit_test_util.hpp"
#include "Point_test.hpp"

BOOST_AUTO_TEST_SUITE( VCluster_test )

BOOST_AUTO_TEST_CASE (Vcluster_robustness)
{
	Vcluster & vcl = *global_v_cluster;

	vcl.execute();
}

BOOST_AUTO_TEST_CASE( VCluster_use_reductions)
{
	Vcluster & vcl = *global_v_cluster;

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
		vcl.reduce(c);
	if ( vcl.getProcessingUnits() < 256 )
		vcl.reduce(uc);
	if ( vcl.getProcessingUnits() < 32768 )
		vcl.reduce(s);
	if ( vcl.getProcessingUnits() < 65536 )
		vcl.reduce(us);
	if ( vcl.getProcessingUnits() < 2147483648 )
		vcl.reduce(i);
	if ( vcl.getProcessingUnits() < 4294967296 )
		vcl.reduce(ui);
	vcl.reduce(li);
	vcl.reduce(uli);
	vcl.reduce(f);
	vcl.reduce(d);

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

	if ( vcl.getProcessingUnits() < 128 )
	{BOOST_REQUIRE_EQUAL(c_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 256 )
	{BOOST_REQUIRE_EQUAL(uc_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 32768 )
	{BOOST_REQUIRE_EQUAL(s_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 65536 )
	{BOOST_REQUIRE_EQUAL(us_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 2147483648 )
	{BOOST_REQUIRE_EQUAL(i_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 4294967296 )
	{BOOST_REQUIRE_EQUAL(ui_max,vcl.getProcessingUnits()-1);}

	BOOST_REQUIRE_EQUAL(li_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(uli_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(f_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(d_max,vcl.getProcessingUnits()-1);
}

#define N_V_ELEMENTS 16

#include "Vector/vector_test_util.hpp"

BOOST_AUTO_TEST_CASE(VCluster_send_recv)
{
	Vcluster & vcl = *global_v_cluster;

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

BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv)
{
	std::cout << "VCluster unit test start" << "\n";

	totp_check = true;
	test<NBX>();
	totp_check = false;
	test<PCX>();

	std::cout << "VCluster unit test stop" << "\n";
}

BOOST_AUTO_TEST_SUITE_END()


#endif /* VCLUSTER_UNIT_TESTS_HPP_ */
