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

/*! \brief Packing class
 *
 * This class is going to pack objects
 *
 * \tparam T base type
 * \tparam Mem Memory type
 * \tparam pack_type type of packing
 *
 */
template<typename T, typename Mem, int pack_type>
class Packer
{
public:

	/*! \brief Error, no implementation
	 *
	 */
	void pack(ExtPreAlloc<Mem> , T & obj)
	{
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " packing for the type " << demangle(typeid(T).name()) << " is not implemented\n";
	}

	/*! \brief Error, no implementation
	 *
	 */
	size_t packRequest(std::vector<size_t> & req)
	{
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " packing for the type " << demangle(typeid(T).name()) << " is not implemented\n";
		return 0;
	}
};

/*! \brief Packer class for primitives
 *
 * \tparam T base type
 * \tparam Mem Memory type
 * \tparam pack_type type of packing
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_PRIMITIVE>
{
public:

	/*! \brief
	 *
	 * It pack any C++ primitives
	 *
	 */
	void pack(ExtPreAlloc<Mem> & ext, T & obj)
	{
		ext.allocate(sizeof(T));
		*(T *)ext.getPointer() = obj;
	}

	/*! \brief
	 *
	 *
	 */
	size_t packRequest(std::vector<size_t> & req)
	{
		return req.push_back(sizeof(T));
	}
};


/*! \brief Packer class for objects
 *
 * \tparam T Type of Memory
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_OBJECTS>
{
public:

	/*! \brief
	 *
	 *
	 */
	void pack(ExtPreAlloc<Mem> & ext, T & obj)
	{
		ext.allocate(sizeof(T));
		memcpy((T *)ext.getPointer(),&obj,sizeof(T));
	}

	/*! \brief
	 *
	 *
	 */
	size_t packRequest(std::vector<size_t> & req)
	{
		return req.push_back(sizeof(T));
	}
};

/*! \brief Packer class for vectors
 *
 * \tparam T Type of Memory
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_VECTOR>
{
public:

	/*! \brief pack a vector of objects selecting the properties to pack
	 *
	 *
	 */
	template<unsigned int ... prp> void pack(ExtPreAlloc<Mem> & mem, T & obj)
	{

	}

	/*! \brief Insert an allocation request into the vector
	 *
	 * \param vector of request
	 *
	 */
	template<unsigned int ... prp> void packRequest(T & obj, std::vector<size_t> & v)
	{
		// Calculate the required memory for packing
		size_t alloc_ele = openfpm::vector<prp>::calculateMem(obj.size(),0);

		v.push_back(alloc_ele);
	}
};

/*! \brief Packer class for grids
 *
 * \tparam T Type of Memory
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_GRID>
{
	/*! \brief Pack the object into the memory given an iterator
	 *
	 * \tparam it Type of iterator
	 * \tparam dim Dimensionality of the grid
	 * \tparam properties to pack
	 *
	 */
	template <typename it,unsigned int dim, unsigned int ... prp> static void pack_with_iterator(ExtPreAlloc<Mem> & mem, it & sub_it, T & obj)
	{
		// Sending property object
		typedef object<typename object_creator<typename T::type,prp...>::type> prp_object;

		// Packing the information
		while (sub_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<dim,T,typename grid_cpu<dim,T>::memory_conf > encap_src;
			// destination object type
			typedef encapc<1,prp_object,typename grid_cpu<dim,prp_object>::memory_conf > encap_dst;

			// Copy only the selected properties
			object_si_d<encap_src,encap_dst,ENCAP,prp...>(obj.get_o(sub_it.get()),dest.get(sub_it.getLoc()));


			++sub_it;
		}
	}

public:

	/*! \brief Pack the object into the memory given an iterator
	 *
	 * \tparam dim Dimensionality of the grid
	 * \tparam prp properties to pack
	 *
	 * \param mem Memory type
	 * \param obj object to pack
	 *
	 */
	template<unsigned int dim, unsigned int ... prp> void pack(ExtPreAlloc<Mem> & mem, T & obj)
	{
		// Calculate the required memory for packing
		size_t alloc_ele = openfpm::vector<prp>::calculateMem(obj.size(),0);

		// Allocate memory
		mem.allocate(alloc_ele);

		// Sending property object
		typedef object<typename object_creator<typename T::type,prp...>::type> prp_object;

		// Create an object over the preallocated memory (No allocation is produced)
		openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>> dest;
		dest.setMemory(mem);
		dest.resize(obj.size());

		auto it = obj.getIterator();

		pack_with_iterator(mem,it,obj);
	}

	/*! \brief Pack the object into the memory given an iterator
	 *
	 * \tparam dim Dimensionality of the grid
	 * \tparam prp properties to pack
	 *
	 * \param mem Memory type
	 * \param sub_it sub grid iterator ( or the element in the grid to pack )
	 * \param obj object to pack
	 *
	 */
	template<unsigned int dim, unsigned int ... prp> void pack(ExtPreAlloc<Mem> & mem, grid_key_dx_iterator_sub<dim> & sub_it, T & obj)
	{
		// Calculate the required memory for packing
		size_t alloc_ele = openfpm::vector<prp>::calculateMem(sub_it.getVolume(),0);

		// Allocate memory
		mem.allocate(alloc_ele);

		// Sending property object
		typedef object<typename object_creator<typename T::type,prp...>::type> prp_object;

		// Create an object over the preallocated memory (No allocation is produced)
		openfpm::vector<prp_object,openfpm::device_cpu<prp_object>,ExtPreAlloc<Mem>> dest;
		dest.setMemory(mem);
		dest.resize(sub_it.getVolume());

		pack_with_iterator(mem,sub_it,obj);
	}

	/*! \brief Insert an allocation request into the vector
	 *
	 * \param vector of request
	 *
	 */
	template<unsigned int dim, unsigned int ... prp> void packRequest(T & obj, std::vector<size_t> & v)
	{
		// Calculate the required memory for packing
		size_t alloc_ele = openfpm::vector<prp>::calculateMem(obj.size(),0);

		v.push_back(alloc_ele);
	}
};

/*! \brief Packer class for encapsulated objects
 *
 * \tparam T Type of Memory
 *
 */
template<typename T, typename Mem>
class Packer<T,Mem,PACKER_ENCAP_OBJECT>
{
public:

	/*! \brief
	 *
	 *
	 */
	void pack(ExtPreAlloc<Mem> , T & obj)
	{

	}

	/*! \brief
	 *
	 *
	 */
	size_t packRequest()
	{
		return sizeof(T);
	}
};

#endif /* SRC_PACKER_HPP_ */
