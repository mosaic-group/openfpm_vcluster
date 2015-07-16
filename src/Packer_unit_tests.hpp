/*
 * Packer_unit_tests.hpp
 *
 *  Created on: Jul 15, 2015
 *      Author: Pietro Incardona
 */

#ifndef SRC_PACKER_UNIT_TESTS_HPP_
#define SRC_PACKER_UNIT_TESTS_HPP_

BOOST_AUTO_TEST_SUITE( Packer )

#include "Pack_selector.hpp"

BOOST_AUTO_TEST_CASE ( Packer_test )
{
	//! [Pack selector usage]

	int val = Pack_selector<unsigned char>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<char>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<short>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<unsigned short>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<int>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<unsigned int>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<long int>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<unsigned long int>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<float>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);
	val = Pack_selector<double>::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_PRIMITIVE);

	val = Pack_selector< openfpm::vector<Point_test<float>> >::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_VECTOR);
	val = Pack_selector< grid_cpu<3,Point_test<float>> >::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_GRID);
//	val = Pack_selector< encapc<3,Point_test<float>, memory_traits_lin<Point_test<float>> > >::value;
//	BOOST_REQUIRE_EQUAL(val,PACKER_ENCAP_OBJECTS);

	struct test_s
	{
		float a;
		float b;

		bool noPointers() {return true;}
	};

	val = Pack_selector< test_s >::value;
	BOOST_REQUIRE_EQUAL(val,PACKER_OBJECTS_WITH_POINTER_CHECK);

	//! [Pack selector usage]

	//! [Pack into a message primitives objects vector and grids]

	unsigned char uc = 1;
	char c = 2;
	short s = 3;
	unsigned short us = 4;
	int i = 5;
	unsigned int ui = 6;
	long int li = 7;
	unsigned long int uli = 8;
	float f = 9;
	double d = 10;

	std::vector<size_t> pap_prp;

/*	Packer<unsigned char>::packRequest(pap_prp);
	Packer<char>::packRequest();
	Packer<short>*/


	// try to pack all the primitives

	//! [Pack into a message primitives objects vector and grids]
}

BOOST_AUTO_TEST_SUITE_END()



#endif /* SRC_PACKER_UNIT_TESTS_HPP_ */
