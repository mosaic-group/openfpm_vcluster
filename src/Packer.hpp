/*
 * Packer_cls.hpp
 *
 *  Created on: Jul 15, 2015
 *      Author: i-bird
 */

#ifndef SRC_PACKER_HPP_
#define SRC_PACKER_HPP_

#include "util/object_util.hpp"
#include "Grid/util.hpp"
#include "Vector/util.hpp"
#include "memory/ExtPreAlloc.hpp"
#include "util/util_debug.hpp"
#include "Pack_stat.hpp"
#include "Pack_selector.hpp"

/*! \brief Packing class
 *
 * This class pack objects primitives vectors and grids, the general usage is to create a vector of
 * packing request (std::vector<size_t>) that contain the size of the required space needed to pack
 * the information. Calculate the total size, allocating it on HeapMemory (for example), Create an
 * ExtPreAlloc memory object giving the preallocated memory to it and finally Pack all the objects
 * subsequently
 *
 * In order to unpack the information the Unpacker class can be used
 *
 * \see Unpacker
 *
 * \snippet Packer_unit_tests.hpp Pack into a message primitives objects vectors and grids
 *
 * \tparam T object type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 * \tparam Implementation of the packer (the Pack_selector choose the correct one)
 *
 */
template<typename T, typename Mem, int pack_type=Pack_selector<T>::value >
class Packer
{
public:

	/*! \brief Error, no implementation
	 *
	 */
	static void pack(ExtPreAlloc<Mem> , const T & obj)
	{
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " packing for the type " << demangle(typeid(T).name()) << " is not implemented\n";
	}

	/*! \brief Error, no implementation
	 *
	 */
	static size_t packRequest(std::vector<size_t> & req)
	{
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " packing for the type " << demangle(typeid(T).name()) << " is not implemented\n";
		return 0;
	}
};

/*! \brief Packer for primitives
 *
 * \tparam T object type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_PRIMITIVE>
{
public:

	/*! \brief It pack any C++ primitives
	 *
	 * \param ext preallocated memory where to pack the object
	 * \param obj object to pack
	 * \param sts pack-stat info
	 *
	 */
	inline static void pack(ExtPreAlloc<Mem> & ext, const T & obj, Pack_stat & sts)
	{
#ifdef DEBUG
		if (ext.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
#endif

		ext.allocate(sizeof(T));
		*(T *)ext.getPointer() = obj;

		// update statistic
		sts.incReq();
	}

	/*! \brief It add a request to pack a C++ primitive
	 *
	 * \param req requests vector
	 *
	 */
	static void packRequest(std::vector<size_t> & req)
	{
		req.push_back(sizeof(T));
	}
};


/*! \brief Packer for objects, with impossibility to check for internal pointers
 *
 * \tparam T object type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_OBJECTS_WITH_WARNING_POINTERS>
{
public:

	/*! \brief It pack an object
	 *
	 * \param ext preallocated memory where to pack the objects
	 * \param obj object to pack
	 * \param sts pack-stat info
	 *
	 */
	static void pack(ExtPreAlloc<Mem> & ext, T & obj, Pack_stat & sts)
	{
#ifdef DEBUG
		if (ext.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";

		std::cerr << "Warning: " << __FILE__ << ":" << __LINE__ << " impossible to check the type " << demangle(typeid(T).name()) << " please consider to add a static method \"void noPointers()\" \n" ;
#endif
		ext.allocate(sizeof(T));
		memcpy((T *)ext.getPointer(),&obj,sizeof(T));

		// update statistic
		sts.incReq();
	}

	/*! \brief it add a request to pack an object
	 *
	 * \param req requests vector
	 *
	 */
	static void packRequest(std::vector<size_t> & req)
	{
		req.push_back(sizeof(T));
	}
};

/*! \brief Packer class for objects
 *
 * \tparam T object type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_OBJECTS_WITH_POINTER_CHECK>
{
public:

	/*! \brief It pack any object checking that the object does not have pointers inside
	 *
	 * \param ext preallocated memory where to pack the objects
	 * \param obj object to pack
	 * \param sts pack-stat info
	 *
	 */
	static void pack(ExtPreAlloc<Mem> & ext, T & obj, Pack_stat & sts)
	{
#ifdef DEBUG
		if (ext.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";

		if (obj.noPointers() == false)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " has pointers inside, sending pointers values has no sense\n";
#endif
		ext.allocate(sizeof(T));
		memcpy((T *)ext.getPointer(),&obj,sizeof(T));

		// Update statistic
		sts.incReq();
	}

	/*! \brief it add a request to pack an object
	 *
	 * \param req requests vector
	 *
	 */
	static void packRequest(std::vector<size_t> & req)
	{
		req.push_back(sizeof(T));
	}
};

/*! \brief Packer class for vectors
 *
 * \tparam T vector type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_VECTOR>
{
public:

	/*! \brief pack a vector selecting the properties to pack
	 *
	 * \param mem preallocated memory where to pack the vector
	 * \param obj object to pack
	 * \param sts pack-stat info
	 *
	 */
	template<int ... prp> static void pack(ExtPreAlloc<Mem> & mem, T & obj, Pack_stat & sts)
	{
#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
#endif

		// if no properties should be packed return
		if (sizeof...(prp) == 0)
			return;

		// Sending property object
		typedef object<typename object_creator<typename T::value_type::type,prp...>::type> prp_object;

		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>,openfpm::grow_policy_identity> dtype;

		// Create an object over the preallocated memory (No allocation is produced)
		dtype dest;
		dest.setMemory(mem);
		dest.resize(obj.size());
		auto obj_it = obj.getIterator();

		while (obj_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<1,typename T::value_type,typename T::memory_conf > encap_src;
			// destination object type
			typedef encapc<1,prp_object,typename dtype::memory_conf > encap_dst;

			// Copy only the selected properties
			object_si_d<encap_src,encap_dst,ENCAP,prp...>(obj.get(obj_it.get()),dest.get(obj_it.get()));

			++obj_it;
		}

		// Update statistic
		sts.incReq();

	}

	/*! \brief Insert an allocation request into the vector
	 *
	 * \param obj vector object to pack
	 * \param requests vector
	 *
	 */
	template<int ... prp> static void packRequest(T & obj, std::vector<size_t> & v)
	{
		typedef object<typename object_creator<typename T::value_type::type,prp...>::type> prp_object;

		// Calculate the required memory for packing
		size_t alloc_ele = openfpm::vector<prp_object>::calculateMem(obj.size(),0);

		v.push_back(alloc_ele);
	}
};

/*! \brief Packer for grids and sub-grids
 *
 * \tparam T grid type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_GRID>
{
	/*! \brief Pack an N-dimensional grid into a vector like structure B given an iterator of the grid
	 *
	 * \tparam it type of iterator of the grid-structure
	 * \tparam dtype type of the structure B
	 * \tparam dim Dimensionality of the grid
	 * \tparam properties to pack
	 *
	 * \param it Grid iterator
	 * \param obj object to pack
	 * \param dest where to pack
	 *
	 */
	template <typename it, typename dtype, int ... prp> static void pack_with_iterator(it & sub_it, T & obj, dtype & dest)
	{
		// Sending property object
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;

		size_t id = 0;

		// Packing the information
		while (sub_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<T::dims,typename T::type,typename T::memory_conf > encap_src;
			// destination object type
			typedef encapc<1,prp_object,typename dtype::memory_conf > encap_dst;

			// Copy only the selected properties
			object_si_d<encap_src,encap_dst,ENCAP,prp...>(obj.get_o(sub_it.get()),dest.get(id));

			++id;
			++sub_it;
		}
	}

public:

	/*! \brief Pack the object into the memory given an iterator
	 *
	 * \tparam dim Dimensionality of the grid
	 * \tparam prp properties to pack
	 *
	 * \param mem preallocated memory where to pack the objects
	 * \param obj object to pack
	 * \param sts pack statistic
	 *
	 */
	template<int ... prp> static void pack(ExtPreAlloc<Mem> & mem, T & obj, Pack_stat & sts)
	{
#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
#endif

		// Sending property object and vector
		typedef object<typename object_creator<typename T::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>> dtype;

		// Calculate the required memory for packing
		size_t alloc_ele = dtype::calculateMem(obj.size(),0);

		// Create an object over the preallocated memory (No allocation is produced)
		dtype dest;
		dest.setMemory(mem);
		dest.resize(obj.size());

		auto it = obj.getIterator();

		pack_with_iterator<decltype(it),dtype,prp...>(mem,it,obj,dest);

		// Update statistic
		sts.incReq();
	}

	/*! \brief Pack the object into the memory given an iterator
	 *
	 * \tparam prp properties to pack
	 *
	 * \param mem preallocated memory where to pack the objects
	 * \param sub_it sub grid iterator ( or the elements in the grid to pack )
	 * \param obj object to pack
	 * \param sts pack statistic
	 *
	 */
	template<int ... prp> static void pack(ExtPreAlloc<Mem> & mem, T & obj, grid_key_dx_iterator_sub<T::dims> & sub_it, Pack_stat & sts)
	{
#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
#endif

		// Sending property object
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>,openfpm::grow_policy_identity> dtype;

		// Create an object over the preallocated memory (No allocation is produced)
		dtype dest;
		dest.setMemory(mem);
		dest.resize(sub_it.getVolume());

		pack_with_iterator<grid_key_dx_iterator_sub<T::dims>,dtype,prp...>(sub_it,obj,dest);

		// Update statistic
		sts.incReq();
	}

	/*! \brief Insert an allocation request
	 *
	 * \param vector of requests
	 *
	 */
	template<int ... prp> static void packRequest(T & obj, std::vector<size_t> & v)
	{
		// Sending property object
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>,openfpm::grow_policy_identity> dtype;

		// Calculate the required memory for packing
		size_t alloc_ele = dtype::calculateMem(obj.size(),0);

		v.push_back(alloc_ele);
	}

	/*! \brief Insert an allocation request
	 *
	 * \tparam prp set of properties to pack
	 *
	 * \param obj grid to pack
	 * \param sub sub-grid iterator
	 * \param vector of requests
	 *
	 */
	template<int ... prp> static void packRequest(T & obj, grid_key_dx_iterator_sub<T::dims> & sub, std::vector<size_t> & v)
	{
		// Sending property object
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>,openfpm::grow_policy_identity> dtype;

		// Calculate the required memory for packing
		size_t alloc_ele = dtype::calculateMem(sub.getVolume(),0);

		v.push_back(alloc_ele);
	}
};

/*! \brief Packer class for encapsulated objects
 *
 * \tparam T object type to pack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_ENCAP_OBJECTS>
{
public:

	/*! \brief
	 *
	 *
	 */
	void pack(ExtPreAlloc<Mem> & mem, T & eobj)
	{
#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
#endif

		// Create an object out of the encapsulated object and copy
		typename T::type obj = eobj;

		memcpy(mem.getPointer(),&obj,sizeof(T::type));
	}

	/*! \brief
	 *
	 *
	 */
	void packRequest(std::vector<size_t> & v)
	{
		v.push_back(sizeof(T::type));
	}
};

#endif /* SRC_PACKER_HPP_ */
