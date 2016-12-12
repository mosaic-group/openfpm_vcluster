/*
 * VCluster_meta_function.hpp
 *
 *  Created on: Dec 8, 2016
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_VCLUSTER_VCLUSTER_META_FUNCTION_HPP_
#define OPENFPM_VCLUSTER_SRC_VCLUSTER_VCLUSTER_META_FUNCTION_HPP_

#include "memory/BHeapMemory.hpp"
#include "Packer_Unpacker/has_max_prop.hpp"

template<bool result, typename T, typename S>
struct unpack_selector_with_prp
{
	template<typename op, int ... prp> static void call_unpack(S & recv, openfpm::vector<BHeapMemory> & recv_buf, openfpm::vector<size_t> * sz, openfpm::vector<size_t> * sz_byte, op & op_param)
	{
		if (sz_byte != NULL)
			sz_byte->resize(recv_buf.size());

		for (size_t i = 0 ; i < recv_buf.size() ; i++)
		{
			T unp;

			ExtPreAlloc<HeapMemory> & mem = *(new ExtPreAlloc<HeapMemory>(recv_buf.get(i).size(),recv_buf.get(i)));
			mem.incRef();

			Unpack_stat ps;

			Unpacker<T,HeapMemory>::template unpack<>(mem, unp, ps);

			size_t recv_size_old = recv.size();
			// Merge the information

			op_param.template execute<true,T,decltype(recv),decltype(unp),prp...>(recv,unp,i);

			size_t recv_size_new = recv.size();

			if (sz_byte != NULL)
				sz_byte->get(i) = recv_buf.get(i).size();
			if (sz != NULL)
				sz->get(i) = recv_size_new - recv_size_old;

			mem.decRef();
			delete &mem;
		}
	}
};

//
template<typename T, typename S>
struct unpack_selector_with_prp<true,T,S>
{
	template<typename op, unsigned int ... prp> static void call_unpack(S & recv, openfpm::vector<BHeapMemory> & recv_buf, openfpm::vector<size_t> * sz, openfpm::vector<size_t> * sz_byte, op & op_param)
	{
		if (sz_byte != NULL)
			sz_byte->resize(recv_buf.size());

		for (size_t i = 0 ; i < recv_buf.size() ; i++)
		{
			// calculate the number of received elements
			size_t n_ele = recv_buf.get(i).size() / sizeof(typename T::value_type);

			// add the received particles to the vector
			PtrMemory * ptr1 = new PtrMemory(recv_buf.get(i).getPointer(),recv_buf.get(i).size());

			// create vector representation to a piece of memory already allocated
			openfpm::vector<typename T::value_type,PtrMemory,typename memory_traits_lin<typename T::value_type>::type, memory_traits_lin,openfpm::grow_policy_identity> v2;

			v2.setMemory(*ptr1);

			// resize with the number of elements
			v2.resize(n_ele);

			// Merge the information

			size_t recv_size_old = recv.size();

			op_param.template execute<false,T,decltype(recv),decltype(v2),prp...>(recv,v2,i);

			size_t recv_size_new = recv.size();

			if (sz_byte != NULL)
				sz_byte->get(i) = recv_buf.get(i).size();
			if (sz != NULL)
				sz->get(i) = recv_size_new - recv_size_old;
		}
	}
};


template<typename T>
struct call_serialize_variadic {};

template<int ... prp>
struct call_serialize_variadic<index_tuple<prp...>>
{
	template<typename T> inline static void call_pr(T & send, size_t & tot_size)
	{
		Packer<T,HeapMemory>::template packRequest<prp...>(send,tot_size);
	}

	template<typename T> inline static void call_pack(ExtPreAlloc<HeapMemory> & mem, T & send, Pack_stat & sts)
	{
		Packer<T,HeapMemory>::template pack<prp...>(mem,send,sts);
	}

	template<typename op, typename T, typename S> inline static void call_unpack(S & recv, openfpm::vector<BHeapMemory> & recv_buf, openfpm::vector<size_t> * sz, openfpm::vector<size_t> * sz_byte, op & op_param)
	{
		const bool result = has_pack_gen<typename T::value_type>::value == false && is_vector<T>::value == true;

		unpack_selector_with_prp<result, T, S>::template call_unpack<op,prp...>(recv, recv_buf, sz, sz_byte, op_param);
	}
};

//! There is max_prop inside
template<bool cond, typename op, typename T, typename S, unsigned int ... prp>
struct pack_unpack_cond_with_prp
{
	static void packingRequest(T & send, size_t & tot_size, openfpm::vector<size_t> & sz)
	{
		typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;
		if (has_pack_gen<typename T::value_type>::value == false && is_vector<T>::value == true)
		//if (has_pack<typename T::value_type>::type::value == false && has_pack_agg<typename T::value_type>::result::value == false && is_vector<T>::value == true)
		{
			sz.add(send.size()*sizeof(typename T::value_type));
		}
		else
		{
			call_serialize_variadic<ind_prop_to_pack>::call_pr(send,tot_size);

			sz.add(tot_size);
		}
	}

	static void packing(ExtPreAlloc<HeapMemory> & mem, T & send, Pack_stat & sts, openfpm::vector<const void *> & send_buf)
	{
		typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;
		if (has_pack_gen<typename T::value_type>::value == false && is_vector<T>::value == true)
		//if (has_pack<typename T::value_type>::type::value == false && has_pack_agg<typename T::value_type>::result::value == false && is_vector<T>::value == true)
		{
			//std::cout << demangle(typeid(T).name()) << std::endl;
			send_buf.add(send.getPointer());
		}
		else
		{
			send_buf.add(mem.getPointerEnd());
			call_serialize_variadic<ind_prop_to_pack>::call_pack(mem,send,sts);
		}
	}

	static void unpacking(S & recv, openfpm::vector<BHeapMemory> & recv_buf, openfpm::vector<size_t> * sz, openfpm::vector<size_t> * sz_byte, op & op_param)
	{
		typedef index_tuple<prp...> ind_prop_to_pack;
		call_serialize_variadic<ind_prop_to_pack>::template call_unpack<op,T,S>(recv, recv_buf, sz, sz_byte, op_param);
	}
};


/////////////////////////////

//! Helper class to add data without serialization
template<bool sr>
struct op_ssend_recv_add_sr
{
	//! Add data
	template<typename T, typename D, typename S, int ... prp> static void execute(D & recv,S & v2, size_t i)
	{
		// Merge the information
		recv.template add_prp<typename T::value_type,PtrMemory,openfpm::grow_policy_identity,openfpm::vect_isel<typename T::value_type>::value,prp...>(v2);
	}
};

//! Helper class to add data with serialization
template<>
struct op_ssend_recv_add_sr<true>
{
	//! Add data
	template<typename T, typename D, typename S, int ... prp> static void execute(D & recv,S & v2, size_t i)
	{
		// Merge the information
		recv.template add_prp<typename T::value_type,HeapMemory,openfpm::grow_policy_double,openfpm::vect_isel<typename T::value_type>::value, prp...>(v2);
	}
};

//! Helper class to add data
template<typename op>
struct op_ssend_recv_add
{
	//! Add data
	template<bool sr, typename T, typename D, typename S, int ... prp> static void execute(D & recv,S & v2, size_t i)
	{
		// Merge the information
		op_ssend_recv_add_sr<sr>::template execute<T,D,S,prp...>(recv,v2,i);
	}
};

//! Helper class to merge data without serialization
template<bool sr,template<typename,typename> class op>
struct op_ssend_recv_merge_impl
{
	//! Merge the
	template<typename T, typename D, typename S, int ... prp> inline static void execute(D & recv,S & v2,size_t i,openfpm::vector<openfpm::vector<aggregate<size_t,size_t>>> & opart)
	{
		// Merge the information
		recv.template merge_prp_v<op,typename T::value_type, PtrMemory, openfpm::grow_policy_identity, prp...>(v2,opart.get(i));
	}
};

//! Helper class to merge data with serialization
template<template<typename,typename> class op>
struct op_ssend_recv_merge_impl<true,op>
{
	//! merge the data
	template<typename T, typename D, typename S, int ... prp> inline static void execute(D & recv,S & v2,size_t i,openfpm::vector<openfpm::vector<aggregate<size_t,size_t>>> & opart)
	{
		// Merge the information
		recv.template merge_prp_v<op,typename T::value_type, HeapMemory, openfpm::grow_policy_double, prp...>(v2,opart.get(i));
	}
};

//! Helper class to merge data
template<template<typename,typename> class op>
struct op_ssend_recv_merge
{
	//! For each processor contain the list of the particles with which I must merge the information
	openfpm::vector<openfpm::vector<aggregate<size_t,size_t>>> & opart;

	//! constructor
	op_ssend_recv_merge(openfpm::vector<openfpm::vector<aggregate<size_t,size_t>>> & opart)
	:opart(opart)
	{}

	//! execute the merge
	template<bool sr, typename T, typename D, typename S, int ... prp> void execute(D & recv,S & v2,size_t i)
	{
		op_ssend_recv_merge_impl<sr,op>::template execute<T,D,S,prp...>(recv,v2,i,opart);
	}
};

//! Helper class to merge data without serialization
template<bool sr>
struct op_ssend_gg_recv_merge_impl
{
	//! Merge the
	template<typename T, typename D, typename S, int ... prp> inline static void execute(D & recv,S & v2,size_t i,size_t & start)
	{
		// Merge the information
		recv.template merge_prp_v<replace_,typename T::value_type, PtrMemory, openfpm::grow_policy_identity, prp...>(v2,start);

		start += v2.size();
	}
};

//! Helper class to merge data with serialization
template<>
struct op_ssend_gg_recv_merge_impl<true>
{
	//! merge the data
	template<typename T, typename D, typename S, int ... prp> inline static void execute(D & recv,S & v2,size_t i,size_t & start)
	{
		// Merge the information
		recv.template merge_prp_v<replace_,typename T::value_type, HeapMemory, openfpm::grow_policy_double, prp...>(v2,start);

		// from
		start += v2.size();
	}
};

//! Helper class to merge data
struct op_ssend_gg_recv_merge
{
	//! starting marker
	size_t start;

	//! constructor
	op_ssend_gg_recv_merge(size_t start)
	:start(start)
	{}

	//! execute the merge
	template<bool sr, typename T, typename D, typename S, int ... prp> void execute(D & recv,S & v2,size_t i)
	{
		op_ssend_gg_recv_merge_impl<sr>::template execute<T,D,S,prp...>(recv,v2,i,start);
	}
};

//////////////////////////////////////////////////



#endif /* OPENFPM_VCLUSTER_SRC_VCLUSTER_VCLUSTER_META_FUNCTION_HPP_ */
