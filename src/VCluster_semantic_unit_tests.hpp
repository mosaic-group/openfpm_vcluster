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
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<size_t> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t i = 0 ; i < vcl.getProcessUnitID() ; i++)
			v1.get(i) = 5;

		openfpm::vector<size_t> v2;

		vcl.SGather(v1,v2,(i%vcl.getProcessingUnits()));

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


BOOST_AUTO_TEST_CASE (Vcluster_semantic_struct_gather)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

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
	}
}

#define SSCATTER_MAX 7

BOOST_AUTO_TEST_CASE (Vcluster_semantic_scatter)
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

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
			return;

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



BOOST_AUTO_TEST_CASE (Vcluster_semantic_sendrecv)
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

		vcl.SSendRecv(v1,v2,prc_send,prc_recv2,sz_recv2);
		vcl.SSendRecv(v1,v3,prc_send,prc_recv3,sz_recv3);

		BOOST_REQUIRE_EQUAL(v2.size(),n_ele);
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i) % SSCATTER_MAX ; j++)
			{
				match &= v2.get(s+j) == j;
			}
			s += sz_recv2.get(i) % SSCATTER_MAX;
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < sz_recv3.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv3.get(i) % SSCATTER_MAX ; j++)
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
		BOOST_REQUIRE_EQUAL(v3.size(),vcl.getProcessingUnits()-1-nc);

		bool match = true;
		size_t s = 0;

		for (size_t i = 0 ; i < sz_recv2.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv2.get(i) % SSCATTER_MAX ; j++)
			{
				Box<3,size_t> b({j,j,j},{j,j,j});
				Box<3,size_t> bt = v2.get(s+j);
				match &= bt == b;
			}
			s += sz_recv2.get(i) % SSCATTER_MAX;
		}

		BOOST_REQUIRE_EQUAL(match,true);

		for (size_t i = 0 ; i < sz_recv3.size() ; i++)
		{
			for (size_t j = 0 ; j < sz_recv3.get(i) % SSCATTER_MAX ; j++)
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

BOOST_AUTO_TEST_SUITE_END()

#endif /* OPENFPM_VCLUSTER_SRC_VCLUSTER_SEMANTIC_UNIT_TESTS_HPP_ */
