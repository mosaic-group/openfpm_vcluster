/*
 * VCluster_unit_test_util_cuda.cuh
 *
 *  Created on: Dec 21, 2018
 *      Author: i-bird
 */

#ifndef VCLUSTER_UNIT_TEST_UTIL_CUDA_CUH_
#define VCLUSTER_UNIT_TEST_UTIL_CUDA_CUH_

#include "VCluster/VCluster.hpp"
#include <boost/test/unit_test.hpp>

template<typename T,typename Memory, template <typename> class layout_base>  void test_single_all_broadcast_primitives(Vcluster<> & vcl)
{
	//! [bcast numbers]

	openfpm::vector<T,Memory,typename layout_base<T>::type,layout_base> bdata;

	if (vcl.getProcessUnitID() == 0)
	{
		bdata.add(0);
		bdata.add(1);
		bdata.add(2);
		bdata.add(3);
		bdata.add(4);
		bdata.add(5);
		bdata.add(6);
	}
	else
	{
		bdata.resize(7);
	}

	vcl.Bcast(bdata,0);
	vcl.execute();

	for (size_t i = 0 ; i < bdata.size() ; i++)
	{BOOST_REQUIRE_EQUAL(i,(size_t)bdata.get(i));}

	//! [bcast numbers]

}

template<typename T,typename Memory, template <typename> class layout_base>  void test_single_all_broadcast_complex(Vcluster<> & vcl)
{
	//! [bcast numbers]

	openfpm::vector<T,Memory,typename layout_base<T>::type,layout_base> bdata;

	if (vcl.getProcessUnitID() == 0)
	{
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 0;
		bdata.template get<1>(bdata.size()-1) = 1000;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 1;
		bdata.template get<1>(bdata.size()-1) = 1001;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 2;
		bdata.template get<1>(bdata.size()-1) = 1002;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 3;
		bdata.template get<1>(bdata.size()-1) = 1003;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 4;
		bdata.template get<1>(bdata.size()-1) = 1004;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 5;
		bdata.template get<1>(bdata.size()-1) = 1005;
		bdata.add();
		bdata.template get<0>(bdata.size()-1) = 6;
		bdata.template get<1>(bdata.size()-1) = 1006;
	}
	else
	{
		bdata.resize(7);
	}

	vcl.Bcast(bdata,0);
	vcl.execute();

	for (size_t i = 0 ; i < bdata.size() ; i++)
	{
		BOOST_REQUIRE_EQUAL(i,(size_t)bdata.template get<0>(i));
		BOOST_REQUIRE_EQUAL(i+1000,(size_t)bdata.template get<1>(i));
	}

	//! [bcast numbers]

}


#endif /* VCLUSTER_UNIT_TEST_UTIL_CUDA_CUH_ */
