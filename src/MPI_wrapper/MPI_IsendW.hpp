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
	static inline void send(size_t proc , size_t tag ,void * buf, size_t sz, MPI_Request & req)
	{
		MPI_Isend(buf, sz,MPI_BYTE, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief General send for a vector of any type
 *
 * \tparam any type
 *
 */

template<typename T, typename ly, typename Mem, typename gr> class MPI_IsendW
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<T,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size() * sizeof(T),MPI_BYTE, proc, tag , MPI_COMM_WORLD,&req);
	}
};


/*! \brief specialization for vector of integer
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<int,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<int,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_INT, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for unsigned integer
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<unsigned int,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned int,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for short
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<short,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<short,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_SHORT, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for short
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<unsigned short,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned short,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_SHORT, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for char
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<char,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<char,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_CHAR, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for char
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<unsigned char,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<unsigned char,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_CHAR, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for size_t
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<size_t,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<size_t,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_UNSIGNED_LONG, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for size_t
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<long int,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<long int,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_LONG, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for float
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<float,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<float,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_FLOAT, proc, tag , MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for double
 *
 */
template<typename ly, typename Mem, typename gr> class MPI_IsendW<double,ly,Mem,gr>
{
public:
	static inline void send(size_t proc , size_t tag ,openfpm::vector<double,ly,Mem,gr> & v, MPI_Request & req)
	{
		MPI_Isend(v.getPointer(), v.size(),MPI_DOUBLE, proc, tag , MPI_COMM_WORLD,&req);
	}
};

#endif
