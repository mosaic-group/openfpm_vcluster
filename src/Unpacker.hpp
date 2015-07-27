/*
 * Unpacker.hpp
 *
 *  Created on: Jul 17, 2015
 *      Author: i-bird
 */

#ifndef SRC_UNPACKER_HPP_
#define SRC_UNPACKER_HPP_

#include "util/object_util.hpp"
#include "Grid/util.hpp"
#include "Vector/util.hpp"
#include "memory/ExtPreAlloc.hpp"
#include "util/util_debug.hpp"
#include "Pack_selector.hpp"
#include "Pack_stat.hpp"
#include "memory/PtrMemory.hpp"

/*! \brief Unpacker class
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 * \tparam Implementation of the unpacker (the Pack_selector choose the correct one)
 *
 */
template<typename T, typename Mem, int pack_type=Pack_selector<T>::value >
class Unpacker
{
public:

	/*! \brief Error, no implementation
	 *
	 */
	static void unpack(ExtPreAlloc<Mem> , T & obj)
	{
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " packing for the type " << demangle(typeid(T).name()) << " is not implemented\n";
	}
};

/*! \brief Unpacker for primitives
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_PRIMITIVE>
{
public:


	/*! \brief It unpack C++ primitives
	 *
	 * \param ext preallocated memory from where to unpack the object
	 * \param obj object where to unpack
	 *
	 */
	static void unpack(ExtPreAlloc<Mem> & ext, T & obj,Pack_stat & ps)
	{
		T * ptr = static_cast<T *>(ext.getPointer(ps.reqPack()));
		obj = *ptr;

		ps.incReq();
	}
};


/*! \brief Unpacker for objects with no possibility to check for internal pointers
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_OBJECTS_WITH_WARNING_POINTERS>
{
public:

	/*! \brief unpack object
	 *
	 * \param ext preallocated memory from where to unpack the object
	 * \param obj object where to unpack
	 *
	 */
	static void unpack(ExtPreAlloc<Mem> & ext, T & obj, Pack_stat & ps)
	{
#ifdef DEBUG
		std::cerr << "Warning: " << __FILE__ << ":" << __LINE__ << " impossible to check the type " << demangle(typeid(T).name()) << " please consider to add a static method \"void noPointers()\" \n" ;
#endif
		memcpy(&obj,(T *)ext.getPointer(ps.reqPack()),sizeof(T));

		ps.incReq();
	}
};

/*! \brief Unpacker class for objects
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_OBJECTS_WITH_POINTER_CHECK>
{
public:

	/*! \brief It unpack any object checking that the object does not have pointers inside
	 *
	 * \param ext preallocated memory from where to unpack the object
	 * \param obj object where to unpack
	 *
	 */
	static void unpack(ExtPreAlloc<Mem> & ext, T & obj, Pack_stat & ps)
	{
#ifdef DEBUG
		if (obj.noPointers() == false)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " has pointers inside, sending pointers values has no sense\n";
#endif
		memcpy(&obj,(T *)ext.getPointer(ps.reqPack()),sizeof(T));

		ps.incReq();
	}
};

/*! \brief Unpacker for vectors
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_VECTOR>
{
public:

	/*! \brief unpack a vector
	 *
	 * \warning the properties should match the packed properties, and the obj must have the same size of the packed vector, consider to pack
	 *          this information if you cannot infer-it
	 *
	 * \param ext preallocated memory from where to unpack the vector
	 * \param obj object where to unpack
	 *
	 */
	template<unsigned int ... prp> void static unpack(ExtPreAlloc<Mem> & mem, T & obj, Pack_stat & ps)
	{
		// if no properties should be unpacked return
		if (sizeof...(prp) == 0)
			return;

		size_t id = 0;

		// Sending property object
		typedef object<typename object_creator<typename T::value_type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,PtrMemory,openfpm::grow_policy_identity> stype;

		// Create an object over the preallocated memory (No allocation is produced)
		PtrMemory & ptr = *(new PtrMemory(mem.getPointer(ps.reqPack()),stype::calculateMem(obj.size(),0)));

		stype src;
		src.setMemory(ptr);
		src.resize(obj.size());
		auto obj_it = obj.getIterator();

		while (obj_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<1,typename T::value_type,typename T::memory_conf > encap_dst;
			// destination object type
			typedef encapc<1,prp_object,typename stype::memory_conf > encap_src;

			// Copy only the selected properties
			object_s_di<encap_src,encap_dst,ENCAP,prp...>(src.get(id),obj.get(obj_it.get()));

			++id;
			++obj_it;
		}

		ps.incReq();
	}
};

/*! \brief Unpacker for grids
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_GRID>
{
	/*! \brief unpack the grid given an iterator
	 *
	 * \tparam it type of iterator
	 * \tparam prp of the grid object to unpack
	 *
	 */
	template <typename it, typename stype, unsigned int ... prp> static void unpack_with_iterator(ExtPreAlloc<Mem> & mem, it & sub_it, T & obj, stype & src, Pack_stat & ps)
	{
		size_t id = 0;

		// Sending property object
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;

		// unpacking the information
		while (sub_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<T::dims,typename T::type,typename T::memory_conf > encap_dst;
			// destination object type
			typedef encapc<1,prp_object,typename grid_cpu<T::dims,prp_object>::memory_conf > encap_src;

			// Copy only the selected properties
			object_s_di<encap_src,encap_dst,ENCAP,prp...>(src.get(id),obj.get_o(sub_it.get()));

			++id;
			++sub_it;
		}
	}

public:

	/*! \brief unpack the grid object
	 *
	 * \tparam prp properties to unpack
	 *
	 * \param ext preallocated memory from where to unpack the grid
	 * \param obj object where to unpack
	 *
	 */
	template<unsigned int ... prp> static void unpack(ExtPreAlloc<Mem> & mem, T & obj, Pack_stat & ps)
	{
		// object that store the information in mem
		typedef object<typename object_creator<typename T::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,PtrMemory,openfpm::grow_policy_identity> stype;

		// Create an object over the preallocated memory (No allocation is produced)
		PtrMemory & ptr = *(new PtrMemory(mem.getPointer(ps.reqPack()),stype::calculateMem(obj.size(),0)));

		// Create an object over a pointer (No allocation is produced)
		stype src;
		src.setMemory(mem);
		src.resize(obj.size());

		auto it = obj.getIterator();

		unpack_with_iterator<decltype(it),stype,prp...>(mem,it,obj,src,ps);

		ps.incReq();
	}

	/*! \brief unpack the sub-grid object
	 *
	 * \tparam prp properties to unpack
	 *
	 * \param mem preallocated memory from where to unpack the object
	 * \param sub sub-grid iterator
	 * \param obj object where to unpack
	 *
	 */
	template<unsigned int ... prp> static void unpack(ExtPreAlloc<Mem> & mem, grid_key_dx_iterator_sub<T::dims> & sub_it, T & obj, Pack_stat & ps)
	{
		// object that store the information in mem
		typedef object<typename object_creator<typename T::type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,PtrMemory,openfpm::grow_policy_identity> stype;

		// Create an object over the preallocated memory (No allocation is produced)
		PtrMemory & ptr = *(new PtrMemory(mem.getPointer(ps.reqPack()),stype::calculateMem(obj.size(),0)));

		// Create an object of the packed information over a pointer (No allocation is produced)
		stype src;
		src.setMemory(ptr);
		src.resize(sub_it.getVolume());

		unpack_with_iterator<grid_key_dx_iterator_sub<T::dims>,stype,prp...>(mem,sub_it,obj,src,ps);

		ps.incReq();
	}
};

/*! \brief Unpacker for encapsulated objects
 *
 * \tparam T object type to unpack
 * \tparam Mem Memory origin HeapMemory CudaMemory ...
 *
 */
template<typename T, typename Mem>
class Unpacker<T,Mem,PACKER_ENCAP_OBJECTS>
{
public:

	/*! \brief
	 *
	 *
	 */
	void pack(ExtPreAlloc<Mem> & mem, T & eobj)
	{
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


#endif /* SRC_UNPACKER_HPP_ */
