/*
 * Vcluster_object_array.hpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Pietro Incardona
 */

#ifndef VCLUSTER_OBJECT_ARRAY_HPP_
#define VCLUSTER_OBJECT_ARRAY_HPP_

#include <vector>
#include "VObject.hpp"

/*! \brief Encapsulate any object created by the Virtual cluster machine
 *
 * \tparam original object
 *
 */

template<typename T>
class Vcluster_object_array : public VObject
{
	std::vector<T> objects;

public:

	/*! \brief Constructor of object array
	 *
	 */
	Vcluster_object_array()
	{

	}

	/*! \brief Return the size of the objects array
	 *
	 * \return the size of the array
	 *
	 */
	size_t size()
	{
		return objects.size();
	}

	/*! \brief Return the element i
	 *
	 * \return a reference to the object i
	 *
	 */

	T & get(unsigned int i)
	{
		return objects[i];
	}

	/*! \brief Return the element i
	 *
	 * \return a reference to the object i
	 *
	 */
	const T & get(unsigned int i) const
	{
		return objects[i];
	}

	/*! \brief Check if this Object is an array
	 *
	 * \return true, it is an array
	 *
	 */
	bool isArray()
	{
		return true;
	}

	/*! \brief Destroy the object
	 *
	 */
	virtual void destroy()
	{
		// Destroy the objects
		objects.clear();
	}

	/*! \brief Get the size of the memory needed to pack the object
	 *
	 * \return the size of the message to pack the object
	 *
	 */
	size_t packObjectSize()
	{
		size_t message = 0;

		// Destroy each objects
		for (size_t i = 0 ; i < objects.size() ; i++)
		{
			message += objects[i].packObjectSize();
		}

		return message;
	}


	/*! \brief Get the size of the memory needed to pack the object
	 *
	 * \param Memory where to write the packed object
	 *
	 * \return the size of the message to pack the object
	 *
	 */
	size_t packObject(void * mem)
	{
		// Pointer is zero
		size_t ptr = 0;
		unsigned char * m = (unsigned char *)mem;

		// pack each object
		for (size_t i = 0 ; i < objects.size() ; i++)
		{
			ptr += objects[i].packObject(&m[ptr]);
		}

#ifdef DEBUG
		if (ptr != packObjectSize())
		{
			std::cerr << "Error " << __FILE__ << " " << __LINE__ << " the pack object size does not match the message" << "\n";
		}
#endif

		return ptr;
	}

	/*! \brief Calculate the size to pack an object in the array
	 *
	 * \param array object index
	 *
	 */
	size_t packObjectInArraySize(size_t i)
	{
		return objects[i].packObjectSize();
	}

	/*! \brief pack the object in the array (the message produced can be used to move one)
	 * object from one processor to another
	 *
	 * \param i index of the object to pack
	 * \param p Memory of the packed object message
	 *
	 */
	size_t packObjectInArray(size_t i, void * p)
	{
		return objects[i].packObject(p);
	}

	/*! \brief Destroy an object from the array
	 *
	 * \param i object to destroy
	 *
	 */
	void destroy(size_t i)
	{
		objects.erase(objects.begin() + i);
	}

	/*! \brief Return the object j in the array
	 *
	 * \param j element j
	 *
	 */
	T & operator[](size_t j)
	{
		return objects[j];
	}

	/*! \brief Resize the array
	 *
	 * \param size
	 *
	 */
	void resize(size_t n)
	{
		objects.resize(n);
	}
};


#endif /* VCLUSTER_OBJECT_HPP_ */
