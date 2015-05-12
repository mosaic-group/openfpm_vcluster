/*
 * VObject.hpp
 *
 *  Created on: Feb 5, 2015
 *      Author: i-bird
 */

#ifndef VOBJECT_HPP_
#define VOBJECT_HPP_

/*! \brief VObject
 *
 * Any object produced by the Virtual cluster (MUST) inherit this class
 *
 */

class VObject
{
public:

	// Check if this Object is an array
	virtual bool isArray() = 0;

	// destroy the object
	virtual void destroy() = 0;

	// get the size of the memory needed to pack the object
	virtual size_t packObjectSize() = 0;

	// pack the object
	virtual size_t packObject(void *) = 0;

	// get the size of the memory needed to pack the object in the array
	virtual size_t packObjectInArraySize(size_t i) = 0;

	// pack the object in the array (the message produced can be used to move one)
	// object from one processor to another
	virtual size_t packObjectInArray(size_t i, void * p) = 0;

	// destroy an element from the array
	virtual void destroy(size_t n) = 0;
};


#endif /* VOBJECT_HPP_ */
