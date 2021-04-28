/*
 * MPI_AllGather.hpp
 *
 *  Created on: Oct 26, 2015
 *      Author: i-bird
 */

#ifndef OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IALLGATHER_HPP_
#define OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IALLGATHER_HPP_


/*! \brief Set of wrapping classing for MPI_Irecv
 *
 * The purpose of these classes is to correctly choose the right call based on the type we want to receive
 *
 */

/*! \brief General gather for general buffers
 *
 * \param proc processor from which to receive
 * \param tag
 * \param buf buffer where to store the data
 * \param sz size to receive
 * \param req MPI request
 *
 */

class MPI_IAllGatherWB
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_BYTE, rbuf, sz_r, MPI_BYTE, MPI_COMM_WORLD,&req));
	}
};

/*! \brief General recv for vector of
 *
 * \tparam any type
 *
 */

template<typename T> class MPI_IAllGatherW
{
public:

	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sizeof(T) * sz_s,MPI_BYTE, rbuf, sz_r * sizeof(T), MPI_BYTE, MPI_COMM_WORLD,&req));
	}
};


/*! \brief specialization for vector of integer
 *
 */
template<> class MPI_IAllGatherW<int>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_INT, rbuf, sz_r, MPI_INT, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of unsigned integer
 *
 */
template<> class MPI_IAllGatherW<unsigned int>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_UNSIGNED, rbuf, sz_r, MPI_UNSIGNED, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of short
 *
 */
template<> class MPI_IAllGatherW<short>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_SHORT, rbuf, sz_r, MPI_SHORT, MPI_COMM_WORLD,&req));
	}
};


/*! \brief specialization for vector of short
 *
 */
template<> class MPI_IAllGatherW<unsigned short>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_UNSIGNED_SHORT, rbuf, sz_r, MPI_UNSIGNED_SHORT, MPI_COMM_WORLD,&req));
	}
};


/*! \brief specialization for vector of char
 *
 */
template<> class MPI_IAllGatherW<char>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_CHAR, rbuf, sz_r, MPI_CHAR, MPI_COMM_WORLD,&req));
	}
};


/*! \brief specialization for vector of unsigned char
 *
 */
template<> class MPI_IAllGatherW<unsigned char>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_UNSIGNED_CHAR, rbuf, sz_r, MPI_UNSIGNED_CHAR, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of size_t
 *
 */
template<> class MPI_IAllGatherW<size_t>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_UINT64_T, rbuf, sz_r, MPI_UINT64_T, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of long int
 *
 */
template<> class MPI_IAllGatherW<long int>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_LONG, rbuf, sz_r, MPI_LONG, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of float
 *
 */
template<> class MPI_IAllGatherW<float>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_FLOAT, rbuf, sz_r, MPI_FLOAT, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for vector of double
 *
 */
template<> class MPI_IAllGatherW<double>
{
public:
	static inline void gather(void * sbuf, size_t sz_s ,void * rbuf, size_t sz_r, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallgather(sbuf,sz_s,MPI_DOUBLE, rbuf, sz_r, MPI_DOUBLE, MPI_COMM_WORLD,&req));
	}
};


#endif /* OPENFPM_VCLUSTER_SRC_MPI_WRAPPER_MPI_IALLGATHER_HPP_ */
