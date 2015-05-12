/*
 * Vcluster_object.hpp
 *
 *  Created on: Feb 4, 2015
 *      Author: i-bird
 */

#ifndef VCLUSTER_OBJECT_HPP_
#define VCLUSTER_OBJECT_HPP_

/*! \brief Encapsulate any object created by the Virtual cluster machine
 *
 * \tparam original object
 *
 */

template<typename T>
class Vcluster_object : public T
{
};


#endif /* VCLUSTER_OBJECT_HPP_ */
