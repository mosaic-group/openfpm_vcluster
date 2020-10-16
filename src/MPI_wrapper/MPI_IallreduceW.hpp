#ifndef MPI_IALLREDUCEW_HPP
#define MPI_IALLREDUCEW_HPP

#include <mpi.h>

/*! \brief Set of wrapping classing for MPI_Iallreduce
 *
 * The purpose of these classes is to correctly choose the right call based on the type we want to reduce
 *
 */


/*! \brief General reduction
 *
 * \tparam any type
 *
 */

template<typename T> class MPI_IallreduceW
{
public:
	static inline void reduce(T & buf,MPI_Op op, MPI_Request & req)
	{
#ifndef DISABLE_ALL_RTTI
		std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " cannot recognize " << typeid(T).name() << "\n";
#endif
	}
};


/*! \brief specialization for integer
 *
 */
template<> class MPI_IallreduceW<int>
{
public:
	static inline void reduce(int & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_INT, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for unsigned integer
 *
 */
template<> class MPI_IallreduceW<unsigned int>
{
public:
	static inline void reduce(unsigned int & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_UNSIGNED, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IallreduceW<short>
{
public:
	static inline void reduce(short & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_SHORT, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for short
 *
 */
template<> class MPI_IallreduceW<unsigned short>
{
public:
	static inline void reduce(unsigned short & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_UNSIGNED_SHORT, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IallreduceW<char>
{
public:
	static inline void reduce(char & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_CHAR, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for char
 *
 */
template<> class MPI_IallreduceW<unsigned char>
{
public:
	static inline void reduce(unsigned char & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_UNSIGNED_CHAR, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IallreduceW<size_t>
{
public:
	static inline void reduce(size_t & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_UNSIGNED_LONG, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for size_t
 *
 */
template<> class MPI_IallreduceW<long int>
{
public:
	static inline void reduce(long int & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_LONG, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for float
 *
 */
template<> class MPI_IallreduceW<float>
{
public:
	static inline void reduce(float & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_FLOAT, op, MPI_COMM_WORLD,&req));
	}
};

/*! \brief specialization for double
 *
 */
template<> class MPI_IallreduceW<double>
{
public:
	static inline void reduce(double & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_SAFE_CALL(MPI_Iallreduce(MPI_IN_PLACE, &buf, 1,MPI_DOUBLE, op, MPI_COMM_WORLD,&req));
	}
};

////////////////// Specialization for vectors ///////////////

/*! \brief specialization for vector integer
 *
 */
template<> class MPI_IallreduceW<openfpm::vector<int>>
{
public:
	static inline void reduce(openfpm::vector<int> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_INT, op, MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for vector short
 *
 */
template<> class MPI_IallreduceW<openfpm::vector<short>>
{
public:
	static inline void reduce(openfpm::vector<short> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_SHORT, op, MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for vector char
 *
 */
template<> class MPI_IallreduceW<openfpm::vector<char>>
{
public:
	static inline void reduce(openfpm::vector<char> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_CHAR, op, MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for vector size_t
 *
 */
template<> class MPI_IallreduceW<openfpm::vector<size_t>>
{
public:
	static inline void reduce(openfpm::vector<size_t> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_UNSIGNED_LONG, op, MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for vector float
 *
 */
template<> class MPI_IallreduceW<openfpm::vector<float>>
{
public:
	static inline void reduce(openfpm::vector<float> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_FLOAT, op, MPI_COMM_WORLD,&req);
	}
};

/*! \brief specialization for vector double
 *
 */

template<> class MPI_IallreduceW<openfpm::vector<double>>
{
public:
	static inline void reduce(openfpm::vector<double> & buf,MPI_Op op, MPI_Request & req)
	{
		MPI_Iallreduce(MPI_IN_PLACE, &buf.get(0), buf.size(),MPI_DOUBLE, op, MPI_COMM_WORLD,&req);
	}
};

#endif
