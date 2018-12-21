/*
 * VCluster_semantic_unit_tests_funcs.hpp
 *
 *  Created on: Aug 18, 2018
 *      Author: i-bird
 */

#ifndef VCLUSTER_SEMANTIC_UNIT_TESTS_FUNCS_HPP_
#define VCLUSTER_SEMANTIC_UNIT_TESTS_FUNCS_HPP_

#include "VCluster/VCluster.hpp"

template<typename Memory, template<typename> class layout_base>
void test_different_layouts()
{
	for (size_t i = 0 ; i < 100 ; i++)
	{
		Vcluster<> & vcl = create_vcluster();

		if (vcl.getProcessingUnits() >= 32)
			return;

		openfpm::vector<aggregate<int,float,size_t>,Memory,typename layout_base<aggregate<int,float,size_t>>::type,layout_base> v1;
		v1.resize(vcl.getProcessUnitID());

		for(size_t j = 0 ; j < vcl.getProcessUnitID() ; j++)
		{
			v1.template get<0>(j) = 5;
			v1.template get<1>(j) = 10.0+1000.0;
			v1.template get<2>(j) = 11.0+100000;
		}

		openfpm::vector<aggregate<int,float,size_t>,Memory,typename layout_base<aggregate<int,float,size_t>>::type,layout_base> v2;

		vcl.SGather<decltype(v1),decltype(v2),layout_base>(v1,v2,(i%vcl.getProcessingUnits()));

		if (vcl.getProcessUnitID() == (i%vcl.getProcessingUnits()))
		{
			size_t n = vcl.getProcessingUnits();
			BOOST_REQUIRE_EQUAL(v2.size(),n*(n-1)/2);

			bool is_correct = true;
			for (size_t i = 0 ; i < v2.size() ; i++)
			{
				is_correct &= (v2.template get<0>(i) == 5);
				is_correct &= (v2.template get<1>(i) == 10.0+1000.0);
				is_correct &= (v2.template get<2>(i) == 11.0+100000.0);
			}

			BOOST_REQUIRE_EQUAL(is_correct,true);
		}
		if (vcl.getProcessUnitID() == 0 && i == 99)
			std::cout << "Semantic gather test stop" << std::endl;
	}
}

template<typename Memory>
void test_ssend_recv_layout_switch(size_t opt)
{
	auto & v_cl = create_vcluster<Memory>();

	if (v_cl.size() > 10)	{return;}

	openfpm::vector<openfpm::vector_gpu_single<aggregate<float,float[3]>>> vd;
	openfpm::vector_gpu<aggregate<float,float[3]>> collect;
	openfpm::vector_gpu<aggregate<float,float[3]>> collect2;
	openfpm::vector<size_t> prc_send;
    openfpm::vector<size_t> prc_recv;
    openfpm::vector<size_t> sz_recv;

	vd.resize(v_cl.size());

	for (size_t i = 0 ; i < vd.size() ; i++)
	{
		vd.get(i).resize(100);

		for (size_t j = 0 ; j < vd.get(i).size() ; j++)
		{
			vd.get(i).template get<0>(j) = 10000*i + v_cl.rank()*100 + j;

			vd.get(i).template get<1>(j)[0] = 400000 + 10000*i + v_cl.rank()*100 + j;
			vd.get(i).template get<1>(j)[1] = 400000 + 10000*i + v_cl.rank()*100 + j;
			vd.get(i).template get<1>(j)[2] = 400000 + 10000*i + v_cl.rank()*100 + j;
		}

		prc_send.add(i);

		if (opt & MPI_GPU_DIRECT)
		{
			vd.get(i).template hostToDevice<0,1>();

			// Reset host

			for (size_t j = 0 ; j < vd.get(i).size() ; j++)
			{
				vd.get(i).template get<0>(j) = 0.0;

				vd.get(i).template get<1>(j)[0] = 0.0;
				vd.get(i).template get<1>(j)[1] = 0.0;
				vd.get(i).template get<1>(j)[2] = 0.0;
			}
		}
	}

	v_cl.template SSendRecv<openfpm::vector_gpu_single<aggregate<float,float[3]>>,decltype(collect),memory_traits_inte>
	(vd,collect,prc_send, prc_recv,sz_recv,opt);

	v_cl.template SSendRecvP<openfpm::vector_gpu_single<aggregate<float,float[3]>>,decltype(collect),memory_traits_inte,0,1>
	(vd,collect2,prc_send, prc_recv,sz_recv,opt);

	// collect must have 100 * v_cl.size()

	BOOST_REQUIRE_EQUAL(collect.size(),100*v_cl.size());
	BOOST_REQUIRE_EQUAL(collect2.size(),100*v_cl.size());

	// we reset the host collected data if data must be on device

	if (opt & MPI_GPU_DIRECT)
	{
		for (size_t j = 0 ; j < collect.size() ; j++)
		{
			collect.template get<0>(j) = 0.0;

			collect.template get<1>(j)[0] = 0.0;
			collect.template get<1>(j)[1] = 0.0;
			collect.template get<1>(j)[2] = 0.0;

			collect2.template get<0>(j) = 0.0;

			collect2.template get<1>(j)[0] = 0.0;
			collect2.template get<1>(j)[1] = 0.0;
			collect2.template get<1>(j)[2] = 0.0;
		}
	}

	// from device to host

	if (opt & MPI_GPU_DIRECT)
	{
		collect.template deviceToHost<0,1>();
		collect2.template deviceToHost<0,1>();
	}

	// now we check what we received

	// check what we received

	bool match = true;
	for (size_t i = 0 ; i < v_cl.size() ; i++)
	{
		for (size_t j = 0 ; j < 100 ; j++)
		{
			match &= collect.template get<0>(i*100 +j) == v_cl.rank()*10000 + i*100 + j;

			match &= collect.template get<1>(i*100 +j)[0] == 400000 + v_cl.rank()*10000 + i*100 + j;
			match &= collect.template get<1>(i*100 +j)[1] == 400000 + v_cl.rank()*10000 + i*100 + j;
			match &= collect.template get<1>(i*100 +j)[2] == 400000 + v_cl.rank()*10000 + i*100 + j;

			match &= collect2.template get<0>(i*100 +j) == v_cl.rank()*10000 + i*100 + j;

			match &= collect2.template get<1>(i*100 +j)[0] == 400000 + v_cl.rank()*10000 + i*100 + j;
			match &= collect2.template get<1>(i*100 +j)[1] == 400000 + v_cl.rank()*10000 + i*100 + j;
			match &= collect2.template get<1>(i*100 +j)[2] == 400000 + v_cl.rank()*10000 + i*100 + j;
		}

		if (match == false){break;}
	}

	BOOST_REQUIRE_EQUAL(match,true);
}

#endif /* VCLUSTER_SEMANTIC_UNIT_TESTS_FUNCS_HPP_ */
