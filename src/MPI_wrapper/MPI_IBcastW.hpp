/*
 * MPI_IBcastW.hpp
 *
 *  Created on: Apr 8, 2017
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IBCASTW_HPP_
#define OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IBCASTW_HPP_



#include <mpi.h>



/*! \brief Set of wrapping classing for MPI_Irecv
 *
 * The purpose of these classes is to correctly choose the right call based on the type we want to receive
 *
 */

/*! \brief General recv for general buffer
 *
 * \param proc processor from which to receive
 * \param tag
 * \param buf buffer where to store the data
 * \param sz size to receive
 * \param req MPI request
 *
 */

class MPI_IBcastWB
{
public:
	static inline void bcast(size_t proc ,void * buf, size_t sz, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(buf,sz,MPI_BYTE, proc , ext_comm,&req));
	}
};

/*! \brief General recv for vector of
 *
 * \tparam any type
 *
 */

template<typename T> class MPI_IBcastW
{
public:
	template<typename Memory> static inline void bcast(size_t proc ,openfpm::vector<T,Memory> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc , ext_comm,&req));
	}
};


/*! \brief specialization for vector of integer
 *
 */
template<> class MPI_IBcastW<int>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_INT, proc , ext_comm,&req));
	}
};

/*! \brief specialization for unsigned integer
 *
 */
template<> class MPI_IBcastW<unsigned int>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<unsigned int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_UNSIGNED, proc , ext_comm,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IBcastW<short>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<short> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_SHORT, proc , ext_comm,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IBcastW<unsigned short>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<unsigned short> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_UNSIGNED_SHORT, proc , ext_comm,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IBcastW<char>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<char> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_CHAR, proc , ext_comm,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IBcastW<unsigned char>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<unsigned char> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_UNSIGNED_CHAR, proc , ext_comm,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IBcastW<size_t>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<size_t> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_UNSIGNED_LONG, proc , ext_comm,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IBcastW<long int>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<long int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_LONG, proc , ext_comm,&req));
	}
};

/*! \brief specialization for float
 *
 */
template<> class MPI_IBcastW<float>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<float> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_FLOAT, proc , ext_comm,&req));
	}
};

/*! \brief specialization for double
 *
 */
template<> class MPI_IBcastW<double>
{
public:
	static inline void bcast(size_t proc ,openfpm::vector<double> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Ibcast(v.getPointer(), v.size(),MPI_DOUBLE, proc , ext_comm,&req));
	}
};


/*! \brief this class is a functor for "for_each" algorithm
 *
 * This class is a functor for "for_each" algorithm. For each
 * element of the boost::vector the operator() is called.
 * Is mainly used to process broadcast request for each buffer
 *
 */
template<typename vect>
struct bcast_inte_impl
{
	//! vector to broadcast
	vect & send;

	//! vector of requests
	openfpm::vector<MPI_Request> & req;

	//! root processor
	size_t root;

	//! MPI communicator
	MPI_Comm ext_comm;

	/*! \brief constructor
	 *
	 * \param v set of pointer buffers to set
	 *
	 */
	inline bcast_inte_impl(vect & send,
			       openfpm::vector<MPI_Request> & req,
			       size_t root,
			       MPI_Comm ext_comm)
	:send(send),req(req),root(root),ext_comm(ext_comm)
	{};

	//! It call the copy function for each property
	template<typename T>
	inline void operator()(T& t)
	{
		typedef typename boost::mpl::at<typename vect::value_type::type,T>::type send_type;

		// Create one request
		req.add();

		// gather
		MPI_IBcastWB::bcast(root,&send.template get<T::value>(0),send.size()*sizeof(send_type),req.last(),ext_comm);
	}
};

template<bool is_lin_or_inte>
struct b_cast_helper
{
	 template<typename T, typename Mem, typename lt_type, template<typename> class layout_base >
	 static void bcast_(openfpm::vector<MPI_Request> & req,
			            openfpm::vector<T,Mem,lt_type,layout_base> & v,
			            size_t root,
				    MPI_Comm ext_comm)
	{
		// Create one request
		req.add();

		// gather
		MPI_IBcastW<T>::bcast(root,v,req.last(),ext_comm);
	}
};

template<>
struct b_cast_helper<false>
{
	 template<typename T, typename Mem, typename lt_type, template<typename> class layout_base >
	 static void bcast_(openfpm::vector<MPI_Request> & req,
			            openfpm::vector<T,Mem,lt_type,layout_base> & v,
			            size_t root,
				    MPI_Comm ext_comm)
	{
		 bcast_inte_impl<openfpm::vector<T,Mem,lt_type,layout_base>> bc(v,req,root,ext_comm);

		 boost::mpl::for_each_ref<boost::mpl::range_c<int,0,T::max_prop>>(bc);
	}
};

#endif /* OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IBCASTW_HPP_ */
