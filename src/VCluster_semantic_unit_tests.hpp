/*
 * VCluster_semantic_unit_test.hpp
 *
 *  Created on: Feb 8, 2016
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_
#define OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_

struct A
{
	size_t a;
	float b;
	double c;
};


BOOST_AUTO_TEST_SUITE( VCluster_semantic_test )

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather)
{
	{
		Vcluster & vcl = *global_v_cluster;

		if (vcl.getProcessingUnits() >= 32)
			return;

		//! [Gather the data on master]

		openfpm::vector<size_t> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
			v1.get(i) = 5;

		openfpm::vector<size_t> v2;

		vcl.SGather(v1,v2,0);

		//! [Gather the data on master]

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n*(n-1)/2);

			bool is_five = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
				is_five &= (v2.get(i) == 5);

			BOOST_REQUIRE_EQUAL(is_five,true);
		}
	}
	{
		Vcluster & vcl = *global_v_cluster;

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
			v1.get(i) = 5;

		openfpm::vector<size_t> v2;

		vcl.SGather(v1,v2,1);

		if (vcl.getProcessUnitID() == 1)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n*(n-1)/2);

			bool is_five = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
				is_five &= (v2.get(i) == 5);

			BOOST_REQUIRE_EQUAL(is_five,true);
		}
	}
}


BOOST_AUTO_TEST_CASE (Vcluster_semantic_struct_gather)
{
	{
		Vcluster & vcl = *global_v_cluster;

		if (vcl.getProcessingUnits() >= 32)
			return;

		//! [Gather the data on master complex]

		openfpm::vector<A> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{
			v1.get(i).a = 5;
			v1.get(i).b = 10.0;
			v1.get(i).c = 11.0;
		}

		openfpm::vector<A> v2;

		vcl.SGather(v1,v2,0);

		//! [Gather the data on master complex]

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n*(n-1)/2);

			bool is_correct = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				is_correct &= (v2.get(i).a == 5);
				is_correct &= (v2.get(i).b == 10.0);
				is_correct &= (v2.get(i).c == 11.0);
			}

			BOOST_REQUIRE_EQUAL(is_correct,true);
		}
	}
	{
		Vcluster & vcl = *global_v_cluster;

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<A> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{
			v1.get(i).a = 5;
			v1.get(i).b = 10.0;
			v1.get(i).c = 11.0;
		}

		openfpm::vector<A> v2;

		vcl.SGather(v1,v2,1);

		if (vcl.getProcessUnitID() == 1)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n*(n-1)/2);

			bool is_correct = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				is_correct &= (v2.get(i).a == 5);
				is_correct &= (v2.get(i).b == 10.0);
				is_correct &= (v2.get(i).c == 11.0);
			}

			BOOST_REQUIRE_EQUAL(is_correct,true);
		}
	}
}


BOOST_AUTO_TEST_SUITE_END()

#endif /* OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_ */
