/*
 * VCluster_semantic_unit_test.hpp
 *
 *  Created on: Feb 8, 2016
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_
#define OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_

#include "Grid/grid_util_test.hpp"
#include "data_type/aggregate.hpp"

//! Example structure
struct Aexample
{
	//! Example size_t
	size_t a;

	//! Example float
	float b;

	//! Example double
	double c;
};


BOOST_AUTO_TEST_SUITE( VCluster_semantic_test )

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessUnitID() == 0 && i == 0)
			std::cout << "Semantic gather test start" << std::endl;

		if (vcl.getProcessingUnits() >= 32)
			return;

		//! [Gather the data on master]

		openfpm::vector<size_t> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{v1.get(i) = 5;}

		openfpm::vector<size_t> v2;

		vcl.SGather(v1,v2,(i%vcl.getProcessingUnits()));

		//! [Gather the data on master]

		if (vcl.getProcessUnitID() == (i%vcl.getProcessingUnits()))
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

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_2)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		//! [Gather the data on master complex]

		openfpm::vector<size_t> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{v1.get(i) = 5;}

		openfpm::vector<openfpm::vector<size_t>> v2;

		vcl.SGather(v1,v2,0);

		//! [Gather the data on master complex]

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n);

			bool is_five = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				for (size_t j = 0 ; j < v2.get(i).size() ; j++)
					is_five &= (v2.get(i).get(j) == 5);
			}
			BOOST_REQUIRE_EQUAL(is_five,true);

		}

		openfpm::vector<openfpm::vector<size_t>> v3;

		vcl.SGather(v1,v3,1);

		if (vcl.getProcessUnitID() == 1)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v3.size(),n-1);

			bool is_five = true;
			for (size_t i = 0 ; i < v3.size() ; i++)
			{
				for (size_t j = 0 ; j < v3.get(i).size() ; j++)
					is_five &= (v3.get(i).get(j) == 5);
			}
			BOOST_REQUIRE_EQUAL(is_five,true);

		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_3)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> > v1;

		openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> v1_int;
		aggregate<float, openfpm::vector<size_t>, Point_test<float>> aggr;
		openfpm::vector<size_t> v1_int2;

		v1_int2.add((size_t)7);
		v1_int2.add((size_t)7);

		aggr.template get<0>() = 7;
		aggr.template get<1>() = v1_int2;
		Point_test<float> p;
		p.fill();
		aggr.template get<2>() = p;

		v1_int.add(aggr);
		v1_int.add(aggr);
		v1_int.add(aggr);

		v1.add(v1_int);
		v1.add(v1_int);
		v1.add(v1_int);
		v1.add(v1_int);

		openfpm::vector<openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> > v2;

		vcl.SGather(v1,v2,0);

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();

			BOOST_REQUIRE_EQUAL(v2.size(),v1.size()*n);

			bool is_seven = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				for (size_t j = 0 ; j < v2.get(i).size() ; j++)
				{
					is_seven &= (v2.get(i).template get<0>(j) == 7);

					for (size_t k = 0; k < v2.get(i).template get<1>(j).size(); k++)
						is_seven &= (v2.get(i).template get<1>(j).get(k) == 7);

					Point_test<float> p = v2.get(i).template get<2>(j);

					BOOST_REQUIRE(p.template get<0>() == 1);
					BOOST_REQUIRE(p.template get<1>() == 2);
					BOOST_REQUIRE(p.template get<2>() == 3);
					BOOST_REQUIRE(p.template get<3>() == 4);

					for (size_t l = 0 ; l < 3 ; l++)
						p.template get<4>()[l] = 5;

					for (size_t m = 0 ; m < 3 ; m++)
					{
						for (size_t n = 0 ; n < 3 ; n++)
						{
							p.template get<5>()[m][n] = 6;
						}
					}
				}
			}
			BOOST_REQUIRE_EQUAL(is_seven,true);
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_4)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		size_t sz[] = {16,16};

		grid_cpu<2,Point_test<float>> g1(sz);
		g1.setMemory();
		fill_grid<2>(g1);

		openfpm::vector<grid_cpu<2,Point_test<float>>> v2;

		vcl.SGather(g1,v2,0);

		typedef Point_test<float> p;

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n);

			bool match = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				auto it = v2.get(i).getIterator();

				while (it.isNext())
				{
					grid_key_dx<2> key = it.get();

					match &= (v2.get(i).template get<p::x>(key) == g1.template get<p::x>(key));
					match &= (v2.get(i).template get<p::y>(key) == g1.template get<p::y>(key));
					match &= (v2.get(i).template get<p::z>(key) == g1.template get<p::z>(key));
					match &= (v2.get(i).template get<p::s>(key) == g1.template get<p::s>(key));

					match &= (v2.get(i).template get<p::v>(key)[0] == g1.template get<p::v>(key)[0]);
					match &= (v2.get(i).template get<p::v>(key)[1] == g1.template get<p::v>(key)[1]);
					match &= (v2.get(i).template get<p::v>(key)[2] == g1.template get<p::v>(key)[2]);

					match &= (v2.get(i).template get<p::t>(key)[0][0] == g1.template get<p::t>(key)[0][0]);
					match &= (v2.get(i).template get<p::t>(key)[0][1] == g1.template get<p::t>(key)[0][1]);
					match &= (v2.get(i).template get<p::t>(key)[0][2] == g1.template get<p::t>(key)[0][2]);
					match &= (v2.get(i).template get<p::t>(key)[1][0] == g1.template get<p::t>(key)[1][0]);
					match &= (v2.get(i).template get<p::t>(key)[1][1] == g1.template get<p::t>(key)[1][1]);
					match &= (v2.get(i).template get<p::t>(key)[1][2] == g1.template get<p::t>(key)[1][2]);
					match &= (v2.get(i).template get<p::t>(key)[2][0] == g1.template get<p::t>(key)[2][0]);
					match &= (v2.get(i).template get<p::t>(key)[2][1] == g1.template get<p::t>(key)[2][1]);
					match &= (v2.get(i).template get<p::t>(key)[2][2] == g1.template get<p::t>(key)[2][2]);

					++it;
				}

			}
			BOOST_REQUIRE_EQUAL(match,true);
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_5)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		size_t sz[] = {16,16};
		grid_cpu<2,Point_test<float>> g1(sz);
		g1.setMemory();
		fill_grid<2>(g1);
		openfpm::vector<grid_cpu<2,Point_test<float>>> v1;

		v1.add(g1);
		v1.add(g1);
		v1.add(g1);

		openfpm::vector<grid_cpu<2,Point_test<float>>> v2;

		vcl.SGather(v1,v2,1);

		typedef Point_test<float> p;

		if (vcl.getProcessUnitID() == 1)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),v1.size()*n);

			bool match = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				auto it = v2.get(i).getIterator();

				while (it.isNext())
				{
					grid_key_dx<2> key = it.get();

					match &= (v2.get(i).template get<p::x>(key) == g1.template get<p::x>(key));
					match &= (v2.get(i).template get<p::y>(key) == g1.template get<p::y>(key));
					match &= (v2.get(i).template get<p::z>(key) == g1.template get<p::z>(key));
					match &= (v2.get(i).template get<p::s>(key) == g1.template get<p::s>(key));

					match &= (v2.get(i).template get<p::v>(key)[0] == g1.template get<p::v>(key)[0]);
					match &= (v2.get(i).template get<p::v>(key)[1] == g1.template get<p::v>(key)[1]);
					match &= (v2.get(i).template get<p::v>(key)[2] == g1.template get<p::v>(key)[2]);

					match &= (v2.get(i).template get<p::t>(key)[0][0] == g1.template get<p::t>(key)[0][0]);
					match &= (v2.get(i).template get<p::t>(key)[0][1] == g1.template get<p::t>(key)[0][1]);
					match &= (v2.get(i).template get<p::t>(key)[0][2] == g1.template get<p::t>(key)[0][2]);
					match &= (v2.get(i).template get<p::t>(key)[1][0] == g1.template get<p::t>(key)[1][0]);
					match &= (v2.get(i).template get<p::t>(key)[1][1] == g1.template get<p::t>(key)[1][1]);
					match &= (v2.get(i).template get<p::t>(key)[1][2] == g1.template get<p::t>(key)[1][2]);
					match &= (v2.get(i).template get<p::t>(key)[2][0] == g1.template get<p::t>(key)[2][0]);
					match &= (v2.get(i).template get<p::t>(key)[2][1] == g1.template get<p::t>(key)[2][1]);
					match &= (v2.get(i).template get<p::t>(key)[2][2] == g1.template get<p::t>(key)[2][2]);

					++it;
				}

			}
			BOOST_REQUIRE_EQUAL(match,true);
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_6)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<openfpm::vector<openfpm::vector<size_t>>> v1;
		openfpm::vector<openfpm::vector<size_t>> v1_int;
		openfpm::vector<size_t> v1_int2;

		v1_int2.add((size_t)7);
		v1_int2.add((size_t)7);

		v1_int.add(v1_int2);
		v1_int.add(v1_int2);
		v1_int.add(v1_int2);

		v1.add(v1_int);
		v1.add(v1_int);
		v1.add(v1_int);
		v1.add(v1_int);

		openfpm::vector<openfpm::vector<openfpm::vector<size_t>>> v2;

		vcl.SGather(v1,v2,0);

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();

			BOOST_REQUIRE_EQUAL(v2.size(),v1.size()*n);

			bool is_seven = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				for (size_t j = 0 ; j < v2.get(i).size() ; j++)
				{
					for (size_t k = 0 ; k < v2.get(i).get(j).size() ; k++)
						is_seven &= (v2.get(i).get(j).get(k) == 7);
				}
			}
			BOOST_REQUIRE_EQUAL(is_seven,true);
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_7)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<Point_test<float>> v1;

		Point_test<float> p1;
		p1.fill();

		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{v1.get(i) = p1;}

		openfpm::vector<openfpm::vector<Point_test<float>>> v2;

		vcl.SGather(v1,v2,0);

		typedef Point_test<float> p;

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n);

			bool match = true;

			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				for (size_t j = 0 ; j < v2.get(i).size() ; j++)
				{
					Point_test<float> p2 = v2.get(i).get(j);
					//BOOST_REQUIRE(p2 == p1);

					match &= (p2.template get<p::x>() == p1.template get<p::x>());
					match &= (p2.template get<p::y>() == p1.template get<p::y>());
					match &= (p2.template get<p::z>() == p1.template get<p::z>());
					match &= (p2.template get<p::s>() == p1.template get<p::s>());

					match &= (p2.template get<p::v>()[0] == p1.template get<p::v>()[0]);
					match &= (p2.template get<p::v>()[1] == p1.template get<p::v>()[1]);
					match &= (p2.template get<p::v>()[2] == p1.template get<p::v>()[2]);

					match &= (p2.template get<p::t>()[0][0] == p1.template get<p::t>()[0][0]);
					match &= (p2.template get<p::t>()[0][1] == p1.template get<p::t>()[0][1]);
					match &= (p2.template get<p::t>()[0][2] == p1.template get<p::t>()[0][2]);
					match &= (p2.template get<p::t>()[1][0] == p1.template get<p::t>()[1][0]);
					match &= (p2.template get<p::t>()[1][1] == p1.template get<p::t>()[1][1]);
					match &= (p2.template get<p::t>()[1][2] == p1.template get<p::t>()[1][2]);
					match &= (p2.template get<p::t>()[2][0] == p1.template get<p::t>()[2][0]);
					match &= (p2.template get<p::t>()[2][1] == p1.template get<p::t>()[2][1]);
					match &= (p2.template get<p::t>()[2][2] == p1.template get<p::t>()[2][2]);
				}
			}
			BOOST_REQUIRE_EQUAL(match,true);
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_gather_8)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<Box<3,size_t>> v1;

		Box<3,size_t> bx;
		bx.setLow(0, 1);
		bx.setLow(1, 2);
		bx.setLow(2, 3);
		bx.setHigh(0, 4);
		bx.setHigh(1, 5);
		bx.setHigh(2, 6);


		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
			v1.get(i) = bx;

		openfpm::vector<openfpm::vector<Box<3,size_t>>> v2;

		vcl.SGather(v1,v2,0);

		if (vcl.getProcessUnitID() == 0)
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n);

			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				for (size_t j = 0 ; j < v2.get(i).size() ; j++)
				{
					Box<3,size_t> b2 = v2.get(i).get(j);
					BOOST_REQUIRE(bx == b2);
				}
			}
		}
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_struct_gather)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<Aexample> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
		{
			v1.get(i).a = 5;
			v1.get(i).b = 10.0;
			v1.get(i).c = 11.0;
		}

		openfpm::vector<Aexample> v2;

		vcl.SGather(v1,v2,(i%vcl.getProcessingUnits()));

		if (vcl.getProcessUnitID() == (i%vcl.getProcessingUnits()))
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
		if (vcl.getProcessUnitID() == 0 && i == 99)
			std::cout << "Semantic gather test stop" << std::endl;
	}
}

#define SSCATTER_MAX 7

BOOST_AUTO_TEST_CASE (Vcluster_semantic_scatter)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_elements = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		openfpm::vector<size_t> v1;
		v1.resize(n_elements);

		for(size_t i = 0 ; i < n_elements ; i++)
		{v1.get(i) = 5;}

		//! [Scatter the data from master]

		openfpm::vector<size_t> v2;

		openfpm::vector<size_t> prc;
		openfpm::vector<size_t> sz;

		// Scatter pattern
		for (size_t i = 0 ; i < vcl.getProcessingUnits() ; i++)
		{
			sz.add(i % SSCATTER_MAX);
			prc.add(i);
		}

		vcl.SScatter(v1,v2,prc,sz,(i%vcl.getProcessingUnits()));

		//! [Scatter the data from master]

		BOOST_REQUIRE_EQUAL(v2.size(),vcl.getProcessUnitID() % SSCATTER_MAX);

		bool is_five = true;
		for (size_t i = 0 ; i < v2.size() ; i++)
			is_five &= (v2.get(i) == 5);

		BOOST_REQUIRE_EQUAL(is_five,true);
	}
}


BOOST_AUTO_TEST_CASE (Vcluster_semantic_struct_scatter)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_elements = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		openfpm::vector<size_t> v1;
		v1.resize(n_elements);

		for(size_t i = 0 ; i < n_elements ; i++)
			v1.get(i) = 5;

		openfpm::vector<size_t> v2;

		openfpm::vector<size_t> prc;
		openfpm::vector<size_t> sz;

		// Scatter pattern
		for (size_t i = 0 ; i < vcl.getProcessingUnits() ; i++)
		{
			sz.add(i % SSCATTER_MAX);
			prc.add(i);
		}

		vcl.SScatter(v1,v2,prc,sz,(i%vcl.getProcessingUnits()));

		if (vcl.getProcessUnitID() == (i%vcl.getProcessingUnits()))
		{
			BOOST_REQUIRE_EQUAL(v2.size(),vcl.getProcessUnitID() % SSCATTER_MAX);

			bool is_five = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
				is_five &= (v2.get(i) == 5);

			BOOST_REQUIRE_EQUAL(is_five,true);
		}
	}
}



BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_all_unknown)
{
	openfpm::vector<size_t> prc_recv2;
	openfpm::vector<size_t> prc_recv3;

	openfpm::vector<size_t> sz_recv2;
	openfpm::vector<size_t> sz_recv3;

	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessUnitID() == 0 && i == 0)
			std::cout << "Semantic sendrecv test start" << std::endl;


		if (vcl.getProcessingUnits() >= 32)
		{return;}

		prc_recv2.clear();
		prc_recv3.clear();
		openfpm::vector<size_t> prc_send;
		sz_recv2.clear();
		sz_recv3.clear();

		//! [dsde with complex objects1]

		// A vector of vector we want to send each internal vector to one specified processor
		openfpm::vector<openfpm::vector<size_t>> v1;

		// We use this empty vector to receive data
		openfpm::vector<size_t> v2;

		// We use this empty vector to receive data
		openfpm::vector<openfpm::vector<size_t>> v3;

		// in this case each processor will send a message of different size to all the other processor
		// but can also be a subset of processors
		v1.resize(vcl.getProcessingUnits());

		// We fill the send buffer with some sense-less data
		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			// each vector is filled with a different message size
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
				v1.get(i).add(j);

			// generate the sending list (in this case the sendinf list is all the other processor)
			// but in general can be some of them and totally random
			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		// Send and receive from the other processor v2 container the received data
		// Because in this case v2 is an openfpm::vector<size_t>, all the received
		// vector are concatenated one over the other. For example if the processor receive 3 openfpm::vector<size_t>
		// each having 3,4,5 elements. v2 will be a vector of 12 elements
		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		// Send and receive from the other processors v2 contain the received data
		// Because in this case v2 is an openfpm::vector<openfpm::vector<size_t>>, all the vector from
		// each processor will be collected. For example if the processor receive 3 openfpm::vector<size_t>
		// each having 3,4,5 elements. v2 will be a vector of vector of 3 elements (openfpm::vector) and
		// each element will be respectivly 3,4,5 elements
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		//! [dsde with complex objects1]

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		size_t nc_check = (vcl.getProcessingUnits()-1) / SSCATTER_MAX;
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc_check);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				match &= v2.get(s+j) == j;
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size() ; j++)
			{
				match &= v3.get(i).get(j) == j;
			}
		}

		BOOST_REQUIRE_EQUAL(match,true);
	}
}


BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_receive_size_known)
{
	openfpm::vector<size_t> prc_recv2;
	openfpm::vector<size_t> prc_recv3;

	openfpm::vector<size_t> sz_recv2;
	openfpm::vector<size_t> sz_recv3;

	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessUnitID() == 0 && i == 0)
		{std::cout << "Semantic sendrecv test start" << std::endl;}


		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<size_t> prc_send;

		openfpm::vector<openfpm::vector<size_t>> v1;
		openfpm::vector<size_t> v2;
		openfpm::vector<openfpm::vector<size_t>> v3;

		v1.resize(vcl.getProcessingUnits());

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
				v1.get(i).add(j);

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		// We receive to fill prc_recv2 and sz_recv2
		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		// We reset v2 and we receive again saying that the processors are known and we know the elements
		v2.clear();
		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2,RECEIVE_KNOWN | KNOWN_ELEMENT_OR_BYTE);
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		size_t nc_check = (vcl.getProcessingUnits()-1) / SSCATTER_MAX;
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc_check);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				match &= v2.get(s+j) == j;
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size() ; j++)
			{
				match &= v3.get(i).get(j) == j;
			}
		}

		BOOST_REQUIRE_EQUAL(match,true);
	}
}



BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_receive_known)
{
	openfpm::vector<size_t> prc_recv2;
	openfpm::vector<size_t> prc_recv3;

	openfpm::vector<size_t> sz_recv2;
	openfpm::vector<size_t> sz_recv3;

	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessUnitID() == 0 && i == 0)
		{std::cout << "Semantic sendrecv test start" << std::endl;}


		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<size_t> prc_send;

		openfpm::vector<openfpm::vector<size_t>> v1;
		openfpm::vector<size_t> v2;
		openfpm::vector<openfpm::vector<size_t>> v3;

		v1.resize(vcl.getProcessingUnits());

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
				v1.get(i).add(j);

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		// Receive to fill prc_recv2
		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		// Reset v2 and sz_recv2

		v2.clear();
		sz_recv2.clear();

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2,RECEIVE_KNOWN);
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		size_t nc_check = (vcl.getProcessingUnits()-1) / SSCATTER_MAX;
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc_check);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				match &= v2.get(s+j) == j;
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size() ; j++)
			{
				match &= v3.get(i).get(j) == j;
			}
		}

		BOOST_REQUIRE_EQUAL(match,true);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_struct_sendrecv)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
		{return;}

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;
		openfpm::vector<openfpm::vector<Box<3,size_t>>> v1;
		openfpm::vector<Box<3,size_t>> v2;
		openfpm::vector<openfpm::vector<Box<3,size_t>>> v3;

		v1.resize(vcl.getProcessingUnits());

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
			{
				Box<3,size_t> b({j,j,j},{j,j,j});
				v1.get(i).add(b);
			}

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		size_t nc_check = (vcl.getProcessingUnits()-1) / SSCATTER_MAX;
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc_check);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				Box<3,size_t> b({j,j,j},{j,j,j});
				Box<3,size_t> bt = v2.get(s+j);
				match &= bt == b;
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size() ; j++)
			{
				Box<3,size_t> b({j,j,j},{j,j,j});
				Box<3,size_t> bt = v3.get(i).get(j);
				match &= bt == b;
			}
		}

		BOOST_REQUIRE_EQUAL(match,true);
	}

	// Send and receive 0 and check

	{
		Vcluster & vcl = create_vcluster();

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<openfpm::vector<Box<3,size_t>>> v1;
		openfpm::vector<Box<3,size_t>> v2;

		v1.resize(vcl.getProcessingUnits());


		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		BOOST_REQUIRE_EQUAL(v2.size(),0ul);
		BOOST_REQUIRE_EQUAL(prc_recv2.size(),0ul);
		BOOST_REQUIRE_EQUAL(sz_recv2.size(),0ul);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_2)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;

		openfpm::vector<openfpm::vector<aggregate<openfpm::vector<size_t>>> > v1;
		openfpm::vector<aggregate<openfpm::vector<size_t>>> v2;
		openfpm::vector<openfpm::vector<aggregate<openfpm::vector<size_t>>> > v3;

		openfpm::vector<aggregate<openfpm::vector<size_t>>> v1_int;
		aggregate<openfpm::vector<size_t>> aggr;
		openfpm::vector<size_t> v1_int2;

		v1_int2.add(7);
		v1_int2.add(7);
		v1_int2.add(7);

		aggr.template get<0>() = v1_int2;

		v1_int.add(aggr);
		v1_int.add(aggr);
		v1_int.add(aggr);

		v1.resize(vcl.getProcessingUnits());

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
			{
				v1.get(i).add(aggr);
			}

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);

		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits());

		bool match = true;
		bool is_seven = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				for (size_t k = 0; k < v2.get(s+j).template get<0>().size(); k++)
					is_seven &= (v2.get(s+j).template get<0>().get(k) == 7);
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size(); j++)
			{
				for (size_t k = 0; k < v3.get(i).template get<0>(j).size(); k++)
					is_seven &= (v3.get(i).template get<0>(j).get(k) == 7);
			}
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_3)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;

		openfpm::vector<openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> > v1;
		openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> v2;
		openfpm::vector<openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> > v3;

		openfpm::vector<aggregate<float, openfpm::vector<size_t>, Point_test<float>>> v1_int;
		aggregate<float, openfpm::vector<size_t>, Point_test<float>> aggr;
		openfpm::vector<size_t> v1_int2;

		v1_int2.add((size_t)7);
		v1_int2.add((size_t)7);

		aggr.template get<0>() = 7;
		aggr.template get<1>() = v1_int2;

		typedef Point_test<float> p;
		p p1;
		p1.fill();
		aggr.template get<2>() = p1;

		v1_int.add(aggr);
		v1_int.add(aggr);
		v1_int.add(aggr);

		v1.resize(vcl.getProcessingUnits());

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
			{
				v1.get(i).add(aggr);
			}

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);

		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits());

		bool match = true;
		bool is_seven = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				is_seven &= (v2.get(s+j).template get<0>() == 7);

				for (size_t k = 0; k < v2.get(s+j).template get<1>().size(); k++)
					is_seven &= (v2.get(s+j).template get<1>().get(k) == 7);

				Point_test<float> p2 = v2.get(s+j).template get<2>();

				match &= (p2.template get<p::x>() == p1.template get<p::x>());
				match &= (p2.template get<p::y>() == p1.template get<p::y>());
				match &= (p2.template get<p::z>() == p1.template get<p::z>());
				match &= (p2.template get<p::s>() == p1.template get<p::s>());

				match &= (p2.template get<p::v>()[0] == p1.template get<p::v>()[0]);
				match &= (p2.template get<p::v>()[1] == p1.template get<p::v>()[1]);
				match &= (p2.template get<p::v>()[2] == p1.template get<p::v>()[2]);

				match &= (p2.template get<p::t>()[0][0] == p1.template get<p::t>()[0][0]);
				match &= (p2.template get<p::t>()[0][1] == p1.template get<p::t>()[0][1]);
				match &= (p2.template get<p::t>()[0][2] == p1.template get<p::t>()[0][2]);
				match &= (p2.template get<p::t>()[1][0] == p1.template get<p::t>()[1][0]);
				match &= (p2.template get<p::t>()[1][1] == p1.template get<p::t>()[1][1]);
				match &= (p2.template get<p::t>()[1][2] == p1.template get<p::t>()[1][2]);
				match &= (p2.template get<p::t>()[2][0] == p1.template get<p::t>()[2][0]);
				match &= (p2.template get<p::t>()[2][1] == p1.template get<p::t>()[2][1]);
				match &= (p2.template get<p::t>()[2][2] == p1.template get<p::t>()[2][2]);
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size(); j++)
			{
				is_seven &= (v3.get(i).get(j).template get<0>() == 7);

				for (size_t k = 0; k < v3.get(i).get(j).template get<1>().size(); k++)
					is_seven &= (v3.get(i).get(j).template get<1>().get(k) == 7);

				Point_test<float> p2 = v3.get(i).get(j).template get<2>();

				match &= (p2.template get<p::x>() == p1.template get<p::x>());
				match &= (p2.template get<p::y>() == p1.template get<p::y>());
				match &= (p2.template get<p::z>() == p1.template get<p::z>());
				match &= (p2.template get<p::s>() == p1.template get<p::s>());

				match &= (p2.template get<p::v>()[0] == p1.template get<p::v>()[0]);
				match &= (p2.template get<p::v>()[1] == p1.template get<p::v>()[1]);
				match &= (p2.template get<p::v>()[2] == p1.template get<p::v>()[2]);

				match &= (p2.template get<p::t>()[0][0] == p1.template get<p::t>()[0][0]);
				match &= (p2.template get<p::t>()[0][1] == p1.template get<p::t>()[0][1]);
				match &= (p2.template get<p::t>()[0][2] == p1.template get<p::t>()[0][2]);
				match &= (p2.template get<p::t>()[1][0] == p1.template get<p::t>()[1][0]);
				match &= (p2.template get<p::t>()[1][1] == p1.template get<p::t>()[1][1]);
				match &= (p2.template get<p::t>()[1][2] == p1.template get<p::t>()[1][2]);
				match &= (p2.template get<p::t>()[2][0] == p1.template get<p::t>()[2][0]);
				match &= (p2.template get<p::t>()[2][1] == p1.template get<p::t>()[2][1]);
				match &= (p2.template get<p::t>()[2][2] == p1.template get<p::t>()[2][2]);
			}
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_4)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;
		openfpm::vector<openfpm::vector<aggregate<float,Point_test<float>>> > v1;
		openfpm::vector<aggregate<float,Point_test<float>> > v2;
		openfpm::vector<openfpm::vector<aggregate<float,Point_test<float>>> > v3;

		v1.resize(vcl.getProcessingUnits());

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		//Prepare an aggregate
		aggregate<float, Point_test<float> > aggr;

		typedef Point_test<float> p;

		p p1;
		p1.fill();

		aggr.template get<0>() = 7;
		aggr.template get<1>() = p1;

		//Fill v1 with aggregates
		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
			{
				v1.get(i).add(aggr);
			}

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		size_t nc_check = (vcl.getProcessingUnits()-1) / SSCATTER_MAX;
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc_check);
		bool match = true;
		bool is_seven = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				is_seven &= (v2.get(s+j).template get<0>() == 7);

				Point_test<float> p2 = v2.get(s+j).template get<1>();

				match &= (p2.template get<p::x>() == p1.template get<p::x>());
				match &= (p2.template get<p::y>() == p1.template get<p::y>());
				match &= (p2.template get<p::z>() == p1.template get<p::z>());
				match &= (p2.template get<p::s>() == p1.template get<p::s>());

				match &= (p2.template get<p::v>()[0] == p1.template get<p::v>()[0]);
				match &= (p2.template get<p::v>()[1] == p1.template get<p::v>()[1]);
				match &= (p2.template get<p::v>()[2] == p1.template get<p::v>()[2]);

				match &= (p2.template get<p::t>()[0][0] == p1.template get<p::t>()[0][0]);
				match &= (p2.template get<p::t>()[0][1] == p1.template get<p::t>()[0][1]);
				match &= (p2.template get<p::t>()[0][2] == p1.template get<p::t>()[0][2]);
				match &= (p2.template get<p::t>()[1][0] == p1.template get<p::t>()[1][0]);
				match &= (p2.template get<p::t>()[1][1] == p1.template get<p::t>()[1][1]);
				match &= (p2.template get<p::t>()[1][2] == p1.template get<p::t>()[1][2]);
				match &= (p2.template get<p::t>()[2][0] == p1.template get<p::t>()[2][0]);
				match &= (p2.template get<p::t>()[2][1] == p1.template get<p::t>()[2][1]);
				match &= (p2.template get<p::t>()[2][2] == p1.template get<p::t>()[2][2]);
			}
			s += sz_recv2.get(i);
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size() ; j++)
			{
				is_seven &= (v3.get(i).get(j).template get<0>() == 7);

				Point_test<float> p2 = v3.get(i).get(j).template get<1>();

				match &= (p2.template get<p::x>() == p1.template get<p::x>());
				match &= (p2.template get<p::y>() == p1.template get<p::y>());
				match &= (p2.template get<p::z>() == p1.template get<p::z>());
				match &= (p2.template get<p::s>() == p1.template get<p::s>());

				match &= (p2.template get<p::v>()[0] == p1.template get<p::v>()[0]);
				match &= (p2.template get<p::v>()[1] == p1.template get<p::v>()[1]);
				match &= (p2.template get<p::v>()[2] == p1.template get<p::v>()[2]);

				match &= (p2.template get<p::t>()[0][0] == p1.template get<p::t>()[0][0]);
				match &= (p2.template get<p::t>()[0][1] == p1.template get<p::t>()[0][1]);
				match &= (p2.template get<p::t>()[0][2] == p1.template get<p::t>()[0][2]);
				match &= (p2.template get<p::t>()[1][0] == p1.template get<p::t>()[1][0]);
				match &= (p2.template get<p::t>()[1][1] == p1.template get<p::t>()[1][1]);
				match &= (p2.template get<p::t>()[1][2] == p1.template get<p::t>()[1][2]);
				match &= (p2.template get<p::t>()[2][0] == p1.template get<p::t>()[2][0]);
				match &= (p2.template get<p::t>()[2][1] == p1.template get<p::t>()[2][1]);
				match &= (p2.template get<p::t>()[2][2] == p1.template get<p::t>()[2][2]);
			}
		}

		BOOST_REQUIRE_EQUAL(is_seven,true);
		BOOST_REQUIRE_EQUAL(match,true);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_5)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;

		size_t sz[] = {16,16};

		grid_cpu<2,Point_test<float>> g1(sz);
		g1.setMemory();
		fill_grid<2>(g1);

		aggregate<grid_cpu<2,Point_test<float>>> aggr;
		aggr.template get<0>() = g1;


		openfpm::vector<openfpm::vector<aggregate<grid_cpu<2,Point_test<float>>>> > v1;
		openfpm::vector<aggregate<grid_cpu<2,Point_test<float>>> > v2;
		openfpm::vector<openfpm::vector<aggregate<grid_cpu<2,Point_test<float>>>> > v3;

		v1.resize(vcl.getProcessingUnits());

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			for (size_t j = 0 ; j < i % SSCATTER_MAX ; j++)
			{
				v1.get(i).add(aggr);
			}

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		size_t nc = vcl.getProcessingUnits() / SSCATTER_MAX;
		size_t nr = vcl.getProcessingUnits() - nc * SSCATTER_MAX;
		nr = ((nr-1) * nr) / 2;

		size_t n_ele = nc * SSCATTER_MAX * (SSCATTER_MAX - 1) / 2 + nr;

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);

		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits());

		bool match = true;
		size_t s = 0;
		typedef Point_test<float> p;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i); j++)
			{
				grid_cpu<2,Point_test<float>> g2 = v2.get(s+j).template get<0>();

				auto it = g2.getIterator();

				while (it.isNext())
				{
					grid_key_dx<2> key = it.get();

					match &= (g2.template get<p::x>(key) == g1.template get<p::x>(key));
					match &= (g2.template get<p::y>(key) == g1.template get<p::y>(key));
					match &= (g2.template get<p::z>(key) == g1.template get<p::z>(key));
					match &= (g2.template get<p::s>(key) == g1.template get<p::s>(key));

					match &= (g2.template get<p::v>(key)[0] == g1.template get<p::v>(key)[0]);
					match &= (g2.template get<p::v>(key)[1] == g1.template get<p::v>(key)[1]);
					match &= (g2.template get<p::v>(key)[2] == g1.template get<p::v>(key)[2]);

					match &= (g2.template get<p::t>(key)[0][0] == g1.template get<p::t>(key)[0][0]);
					match &= (g2.template get<p::t>(key)[0][1] == g1.template get<p::t>(key)[0][1]);
					match &= (g2.template get<p::t>(key)[0][2] == g1.template get<p::t>(key)[0][2]);
					match &= (g2.template get<p::t>(key)[1][0] == g1.template get<p::t>(key)[1][0]);
					match &= (g2.template get<p::t>(key)[1][1] == g1.template get<p::t>(key)[1][1]);
					match &= (g2.template get<p::t>(key)[1][2] == g1.template get<p::t>(key)[1][2]);
					match &= (g2.template get<p::t>(key)[2][0] == g1.template get<p::t>(key)[2][0]);
					match &= (g2.template get<p::t>(key)[2][1] == g1.template get<p::t>(key)[2][1]);
					match &= (g2.template get<p::t>(key)[2][2] == g1.template get<p::t>(key)[2][2]);

					++it;
				}
			}
			s += sz_recv2.get(i);
		}
		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size(); j++)
			{
				grid_cpu<2,Point_test<float>> g2 = v3.get(i).get(j).template get<0>();

				auto it = g2.getIterator();

				while (it.isNext())
				{
					grid_key_dx<2> key = it.get();

					match &= (g2.template get<p::x>(key) == g1.template get<p::x>(key));
					match &= (g2.template get<p::y>(key) == g1.template get<p::y>(key));
					match &= (g2.template get<p::z>(key) == g1.template get<p::z>(key));
					match &= (g2.template get<p::s>(key) == g1.template get<p::s>(key));

					match &= (g2.template get<p::v>(key)[0] == g1.template get<p::v>(key)[0]);
					match &= (g2.template get<p::v>(key)[1] == g1.template get<p::v>(key)[1]);
					match &= (g2.template get<p::v>(key)[2] == g1.template get<p::v>(key)[2]);

					match &= (g2.template get<p::t>(key)[0][0] == g1.template get<p::t>(key)[0][0]);
					match &= (g2.template get<p::t>(key)[0][1] == g1.template get<p::t>(key)[0][1]);
					match &= (g2.template get<p::t>(key)[0][2] == g1.template get<p::t>(key)[0][2]);
					match &= (g2.template get<p::t>(key)[1][0] == g1.template get<p::t>(key)[1][0]);
					match &= (g2.template get<p::t>(key)[1][1] == g1.template get<p::t>(key)[1][1]);
					match &= (g2.template get<p::t>(key)[1][2] == g1.template get<p::t>(key)[1][2]);
					match &= (g2.template get<p::t>(key)[2][0] == g1.template get<p::t>(key)[2][0]);
					match &= (g2.template get<p::t>(key)[2][1] == g1.template get<p::t>(key)[2][1]);
					match &= (g2.template get<p::t>(key)[2][2] == g1.template get<p::t>(key)[2][2]);

					++it;
				}
			}
		}
		BOOST_REQUIRE_EQUAL(match,true);
	}
}

BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv_6)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> prc_recv2;
		openfpm::vector<size_t> prc_recv3;
		openfpm::vector<size_t> prc_send;
		openfpm::vector<size_t> sz_recv2;
		openfpm::vector<size_t> sz_recv3;

		size_t sz[] = {8,10};

		grid_cpu<2,Point_test<float>> g1(sz);
		g1.setMemory();
		fill_grid<2>(g1);

		openfpm::vector<grid_cpu<2,Point_test<float>>> v1;
		openfpm::vector<grid_cpu<2,Point_test<float>>> v3;

		v1.resize(vcl.getProcessingUnits());

		for(size_t i = 0 ; i < v1.size() ; i++)
		{
			v1.get(i) = g1;

			prc_send.add((i + vcl.getProcessUnitID()) % vcl.getProcessingUnits());
		}

		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits());

		bool match = true;
		typedef Point_test<float> p;

		for (size_t i = 0 ; i < v3.size() ; i++)
		{
			for (size_t j = 0 ; j < v3.get(i).size(); j++)
			{
				grid_cpu<2,Point_test<float>> g2 = v3.get(i);

				auto it = g2.getIterator();

				while (it.isNext())
				{
					grid_key_dx<2> key = it.get();

					match &= (g2.template get<p::x>(key) == g1.template get<p::x>(key));
					match &= (g2.template get<p::y>(key) == g1.template get<p::y>(key));
					match &= (g2.template get<p::z>(key) == g1.template get<p::z>(key));
					match &= (g2.template get<p::s>(key) == g1.template get<p::s>(key));

					match &= (g2.template get<p::v>(key)[0] == g1.template get<p::v>(key)[0]);
					match &= (g2.template get<p::v>(key)[1] == g1.template get<p::v>(key)[1]);
					match &= (g2.template get<p::v>(key)[2] == g1.template get<p::v>(key)[2]);

					match &= (g2.template get<p::t>(key)[0][0] == g1.template get<p::t>(key)[0][0]);
					match &= (g2.template get<p::t>(key)[0][1] == g1.template get<p::t>(key)[0][1]);
					match &= (g2.template get<p::t>(key)[0][2] == g1.template get<p::t>(key)[0][2]);
					match &= (g2.template get<p::t>(key)[1][0] == g1.template get<p::t>(key)[1][0]);
					match &= (g2.template get<p::t>(key)[1][1] == g1.template get<p::t>(key)[1][1]);
					match &= (g2.template get<p::t>(key)[1][2] == g1.template get<p::t>(key)[1][2]);
					match &= (g2.template get<p::t>(key)[2][0] == g1.template get<p::t>(key)[2][0]);
					match &= (g2.template get<p::t>(key)[2][1] == g1.template get<p::t>(key)[2][1]);
					match &= (g2.template get<p::t>(key)[2][2] == g1.template get<p::t>(key)[2][2]);

					++it;
				}
			}
		}
		BOOST_REQUIRE_EQUAL(match,true);

		if (vcl.getProcessUnitID() == 0 && i == 99)
			std::cout << "Semantic sendrecv test start" << std::endl;
	}
}

/*BOOST_AUTO_TEST_CASE (Vcluster_semantic_bench_all_all)
{
                Vcluster & vcl = create_vcluster();

                if (vcl.getProcessingUnits() >= 32)
                        return;

                openfpm::vector<size_t> prc_recv2;
                openfpm::vector<size_t> prc_recv3;
                openfpm::vector<size_t> prc_send;
                openfpm::vector<size_t> sz_recv2;
                openfpm::vector<size_t> sz_recv3;
                openfpm::vector<openfpm::vector<Box<3,size_t>>> v1;
                openfpm::vector<Box<3,size_t>> v2;
                openfpm::vector<openfpm::vector<Box<3,size_t>>> v3;

                v1.resize(vcl.getProcessingUnits());

                for(size_t i = 0 ; i < v1.size() ; i++)
                {
                        for (size_t j = 0 ; j < 1000000 ; j++)
                        {
                                Box<3,size_t> b({j,j,j},{j,j,j});
                                v1.get(i).add(b);
                        }

                        prc_send.add(i);
                }

                timer comm_time;
                comm_time.start();

                vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);

                comm_time.stop();
                std::cout << "Communication time " << comm_time.getwct() << std::endl;

                std::cout << "Total sent: " << tot_sent << "    Tot recv: "  << tot_recv << std::endl;

                std::cout << "END" << std::endl;
}*/


BOOST_AUTO_TEST_SUITE_END()

#endif /* OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_ */
