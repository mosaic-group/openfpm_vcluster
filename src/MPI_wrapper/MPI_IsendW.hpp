#ifndef MPI_ISEND_HPP
#define MPI_ISEND_HPP


#include <mpi.h>

/*! \brief Set of wrapping classing for MPI_Isend
 *
 * The purpose of these classes is to correctly choose the right call based on the type we want to send
 *
 */

/*! \brief General send for a buffer
 *
 */
class MPI_IsendWB
{
public:
	static inline void send(size_t proc , size_t tag ,const void * buf, size_t sz, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(buf, sz,MPI_BYTE, proc, tag , ext_comm,&req);
	}
};

/*! \brief General send for a vector of any type
 *
 * \tparam any type
 *
 */

template<typename T, typename Mem, typename gr> class MPI_IsendW
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<T,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc, tag , ext_comm,&req);
	}
};

/*! \brief General send for a vector of any type
 *
 * \tparam any type
 *
 */

template<typename T>
class MPI_IsendW<T,HeapMemory,int>
{
public:
    static inline void send(size_t proc , size_t tag ,openfpm::vector_ofp<T> & v, MPI_Request & req, MPI_Comm ext_comm)
    {
        MPI_Isend(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc, tag , ext_comm,&req);
    }
};

/*! \brief specialization for vector of integer
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<int,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<int,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_INT, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for unsigned integer
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<unsigned int,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned int,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for short
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<short,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<short,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_SHORT, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for short
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<unsigned short,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned short,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_SHORT, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for char
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<char,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<char,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_CHAR, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for char
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<unsigned char,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned char,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_CHAR, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for size_t
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<size_t,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<size_t,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_LONG, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for size_t
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<long int,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<long int,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_LONG, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for float
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<float,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<float,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_FLOAT, proc, tag , ext_comm,&req);
	}
};

/*! \brief specialization for double
 *
 */
template<typename Mem, typename gr> class MPI_IsendW<double,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<double,Mem,gr> & v, MPI_Request & req, MPI_Comm ext_comm)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_DOUBLE, proc, tag , ext_comm,&req);
	}
};

#endif
