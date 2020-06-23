#ifndef MPI_IRECV_HPP
#define MPI_IRECV_HPP


#include <mpi.h>

/*! \brief Set of wrapping classing for MPI_Irecv
 *
 * The purpose of these classes is to correctly choose the right call based on the type we want to receive
 *
 */
class MPI_IrecvWB
{
public:

	/*! \brief General recv for general buffer
	 *
	 * \param proc processor from which to receive
	 * \param tag
	 * \param buf buffer where to store the data
	 * \param sz size to receive
	 * \param req MPI request
	 *
	 */
	static inline void recv(size_t proc , size_t tag ,void * buf, size_t sz, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(buf,sz,MPI_BYTE, proc, tag , ext_comm,&req));
	}
};

/*! \brief General recv for vector of
 *
 * \tparam any type
 *
 */

template<typename T> class MPI_IrecvW
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<T> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc, tag , ext_comm,&req));
	}

    static inline void recv(size_t proc , size_t tag ,openfpm::vector_ofp<T> & v, MPI_Request & req, MPI_Comm ext_comm)
    {
        MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc, tag , ext_comm,&req));
    }
};


/*! \brief specialization for vector of integer
 *
 */
template<> class MPI_IrecvW<int>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_INT, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for unsigned integer
 *
 */
template<> class MPI_IrecvW<unsigned int>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<unsigned int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_UNSIGNED, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IrecvW<short>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<short> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_SHORT, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IrecvW<unsigned short>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<unsigned short> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_UNSIGNED_SHORT, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IrecvW<char>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<char> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_CHAR, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IrecvW<unsigned char>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<unsigned char> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_UNSIGNED_CHAR, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IrecvW<size_t>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<size_t> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_UNSIGNED_LONG, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IrecvW<long int>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<long int> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_LONG, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for float
 *
 */
template<> class MPI_IrecvW<float>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<float> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_FLOAT, proc, tag , ext_comm,&req));
	}
};

/*! \brief specialization for double
 *
 */
template<> class MPI_IrecvW<double>
{
public:
	static inline void recv(size_t proc , size_t tag ,openfpm::vector<double> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_SAFE_CALL(MPI_Irecv(v.getPointer(), v.size(),MPI_DOUBLE, proc, tag , ext_comm,&req));
	}
};

#endif

