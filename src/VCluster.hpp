#ifndef VCLUSTER
#define VCLUSTER

#include "config.h"
#include <mpi.h>
#include "MPI_wrapper/MPI_util.hpp"
#include "VCluster_object.hpp"
#include "Vector/map_vector.hpp"
#include "MPI_wrapper/MPI_IallreduceW.hpp"
#include "MPI_wrapper/MPI_IrecvW.hpp"
#include "MPI_wrapper/MPI_IsendW.hpp"
#include "MPI_wrapper/MPI_IAllGather.hpp"
#include <exception>
#include "Vector/map_vector.hpp"
#ifdef DEBUG
#include "util/check_no_pointers.hpp"
#include "util/util_debug.hpp"
#endif
#include "util/Vcluster_log.hpp"

#define MSG_LENGTH 1024
#define MSG_SEND_RECV 1025
#define SEND_SPARSE 4096
#define NONE 1
#define NEED_ALL_SIZE 2

#define SERIVCE_MESSAGE_TAG 16384
#define SEND_RECV_BASE 8192
#define GATHER_BASE 24576

// number of vcluster instances
extern size_t n_vcluster;
// Global MPI initialization
extern bool global_mpi_init;

///////////////////// Post functions /////////////

template<typename T> void assign(T * ptr1, T * ptr2)
{
	*ptr1 = *ptr2;
};

//////////////////////////////////////////////////

// temporal buffer for reductions
union red
{
	char c;
	unsigned char uc;
	short s;
	unsigned short us;
	int i;
	unsigned int ui;
	float f;
	double d;
};

/*! \brief Virtual Cluster exception
 *
 * This a a class that signal an exception on MPI_WaitAll
 *
 */

class exec_exception: public std::exception
{
  virtual const char* what() const throw()
  {
    return "One or more request has failed or reported an error";
  }
};

/*! \brief This class virtualize the cluster of PC as a set of processes that communicate
 *
 * At the moment it is an MPI-like interface, with a more type aware, and simple, interface.
 *  It also give some more complex communication functionalities like **Dynamic Sparse Data Exchange**
 *
 * Actually VCluster expose a Computation driven parallelism (MPI-like), with a plan of extending to
 * communication driven parallelism
 *
 * * In computation driven parallelism, the program compute than communicate to the other processors
 *
 * * In a communication driven parallelism, (Charm++ or HPX), the program receive messages, this receiving
 *   messages trigger computation
 *
 * ### An example of sending and receive plain buffers
 * \snippet VCluster_unit_test_util.hpp Send and receive plain buffer data
 * ### An example of sending vectors of primitives with (T=float,double,lont int,...)
 * \snippet VCluster_unit_test_util.hpp Sending and receiving primitives
 * ### An example of sending vectors of complexes object
 * \snippet VCluster_unit_test_util.hpp Send and receive vectors of complex
 * ### An example of gathering numbers from all processors
 * \snippet VCluster_unit_test_util.hpp allGather numbers
 *
 */

class Vcluster
{
	Vcluster_log log;

	//! NBX has a potential pitfall that must be addressed,
	//! NBX Send all the messages and probe for incoming messages,
	//! if there is an incoming message it receive it producing
	//! an acknowledge notification on the sending processor.
	//! When all the sends has been acknowledged, the processor call the MPI_Ibarrier
	//! when all the processors call MPI_Ibarrier all send has been received.
	//! While the processors are waiting for the MPI_Ibarrier to complete, all processors
	//! are still probing for incoming message, Unfortunately some processor
	//! can quit the MPI_Ibarrier before others and this mean that some
	//! processor can exit the probing status before others, these processors can in theory
	//! start new communications while the other processor are still in probing status producing
	//! a wrong send/recv association to
	//! resolve this problem an incremental NBX_cnt is used as message TAG to distinguish that the
	//! messages come from other send or subsequent NBX procedures
	size_t NBX_cnt;

	// temporal vector used for meta-communication
	// ( or meta-data before the real communication )
	openfpm::vector<size_t> proc_com;

	// vector that contain the scatter map (it is basically an array of one)
	openfpm::vector<int> map_scatter;

	// vector of MPI requests
	openfpm::vector<MPI_Request> req;

	// vector of MPI status
	openfpm::vector<MPI_Status> stat;

	// vector of functions to execute after all the request has been performed
	std::vector<int> post_exe;

	// Object array


	// Single objects

	// number of processes
	int size;
	// actual rank
	int rank;

	// number of processing unit per process
	int numPE = 1;

	/* This buffer is a temporal buffer for reductions
	 *
	 * MPI_Iallreduce does not accept recv and send buffer to be the same
	 * r is used to overcome this problem (is given as second parameter)
	 * after the execution the data is copied back
	 *
	 */
	std::vector<red> r;

	// vector of pointers of send buffers
	openfpm::vector<void *> ptr_send;

	// vector of the size of send buffers
	openfpm::vector<size_t> sz_send;

	// sending map
	openfpm::vector<size_t> map;

	// Receive buffers
	openfpm::vector<HeapMemory> recv_buf;

	// barrier request
	MPI_Request bar_req;
	// barrier status
	MPI_Status bar_stat;

public:

	// Finalize the MPI program
	~Vcluster()
	{
#ifdef SE_CLASS2
		check_delete(this);
#endif
		n_vcluster--;

		// if there are no other vcluster instances finalize
		if (n_vcluster == 0)
		{
			int already_finalised;

			MPI_Finalized(&already_finalised);
			if (!already_finalised)
			{
				if (MPI_Finalize() != 0)
				{
					std::cerr << __FILE__ << ":" << __LINE__  << " MPI_Finalize FAILED \n";
				}
			}
		}
	}

	/*! \brief Virtual cluster constructor
	 *
	 * \param argc pointer to arguments counts passed to the program
	 * \param argv pointer to arguments vector passed to the program
	 *
	 */
	Vcluster(int *argc, char ***argv)
	:NBX_cnt(0)
	{
#ifdef SE_CLASS2
		check_new(this,8,VCLUSTER_EVENT,PRJ_VCLUSTER);
#endif

		n_vcluster++;

		int already_initialised;
		MPI_Initialized(&already_initialised);

		// Check if MPI is already initialized
		if (!already_initialised)
		{

			MPI_Init(argc,argv);
		}

		// Get the total number of process
		// and the rank of this process

		MPI_Comm_size(MPI_COMM_WORLD, &size);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#ifdef SE_CLASS2
			process_v_cl = rank;
#endif

		// create and fill map scatter with one
		map_scatter.resize(size);

		for (size_t i = 0 ; i < map_scatter.size() ; i++)
		{
			map_scatter.get(i) = 1;
		}

		// open the log file
		log.openLog(rank);

		// Initialize bar_req
		bar_req = MPI_Request();
		bar_stat = MPI_Status();
	}

#ifdef DEBUG

	/*! \brief Check for wrong types
	 *
	 * In general we do not know if a type T make sense to be sent or not, but if it has pointer
	 * inside it does not. This function check if the basic type T has a method called noPointers,
	 * This function in general notify if T has internally pointers. If T has pointer an error
	 * is printed, is T does not have the method a WARNING is printed
	 *
	 * \tparam T type to check
	 *
	 */
	template<typename T> void checkType()
	{
		// if T is a primitive like int, long int, float, double, ... make sense
		// (pointers, l-references and r-references are not fundamentals)
		if (std::is_fundamental<T>::value == true)
			return;

		// if it is a pointer make no sense
		if (std::is_pointer<T>::value == true)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";

		// if it is an l-value reference make no send
		if (std::is_lvalue_reference<T>::value == true)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";

		// if it is an r-value reference make no send
		if (std::is_rvalue_reference<T>::value == true)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";

		// ... if not, check that T has a method called noPointers
		switch (check_no_pointers<T>::value())
		{
			case PNP::UNKNOWN:
			{
				std::cerr << "Warning: " << __FILE__ << ":" << __LINE__ << " impossible to check the type " << demangle(typeid(T).name()) << " please consider to add a static method \"static bool noPointers()\" \n" ;
				break;
			}
			case PNP::POINTERS:
			{
				std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " has pointers inside, sending pointers values has no sense\n";
				break;
			}
			default:
			{

			}
		}
	}

#endif

	/*! \brief Get the MPI_Communicator (or processor group) this VCluster is using
	 *
	 * \return MPI comunicator
	 *
	 */
	MPI_Comm getMPIComm()
	{
		return MPI_COMM_WORLD;
	}

	//! Get the total number of processing units
	size_t getProcessingUnits()
	{
		return size*numPE;
	}

	//! Get the process unit id
	size_t getProcessUnitID()
	{
		return rank;
	}

	/*! \brief Sum the numbers across all processors and get the result
	 *
	 * \param num to reduce, input and output
	 *
	 */

	template<typename T> void sum(T & num)
	{
#ifdef DEBUG
		checkType<T>();
#endif

		// reduce over MPI

		// Create one request
		req.add();

		// reduce
		MPI_IallreduceW<T>::reduce(num,MPI_SUM,req.last());
	}

	/*! \brief Get the maximum number across all processors (or reduction with infinity norm)
	 *
	 * \param num to reduce
	 *
	 */
	template<typename T> void max(T & num)
	{
#ifdef DEBUG
		checkType<T>();
#endif
		// reduce over MPI

		// Create one request
		req.add();

		// reduce
		MPI_IallreduceW<T>::reduce(num,MPI_MAX,req.last());
	}


	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function, NBX is more performant than PCX with more processors (1000+)
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 vector of 100 integers to processor 1
	 * * 1 vector of 50 integers to processor 6
	 * * 1 vector of 48 integers to processor 7
	 * * 1 vector of 70 integers to processor 8
	 *
	 * \param prc list of processors you should communicate with [1,1,6,7,8]
	 *
	 * \param v vector containing the data to send [v=vector<vector<int>>, v.size()=4, T=vector<int>], T at the moment
	 *          is only tested for vectors of 0 or more generic elements (without pointers)
	 *
	 * \param msg_alloc This is a call-back with the purpose to allocate space
	 *        for the incoming messages and give back a valid pointer, supposing that this call-back has been triggered by
	 *        the processor of id 5 that want to communicate with me a message of size 100 byte the call-back will have
	 *        the following 6 parameters
	 *        in the call-back in order:
	 *        * message size required to receive the message (100)
	 *        * total message size to receive from all the processors (NBX does not provide this information)
	 *        * the total number of processor want to communicate with you (NBX does not provide this information)
	 *        * processor id (5)
	 *        * ri request id (it is an id that goes from 0 to total_p, and is incremented
	 *           every time message_alloc is called)
	 *        * void pointer, parameter for additional data to pass to the call-back
	 *
	 * \param opt options, only NONE supported
	 *
	 */
	template<typename T> void sendrecvMultipleMessagesNBX(openfpm::vector< size_t > & prc, openfpm::vector< T > & data, void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,void *), void * ptr_arg, long int opt=NONE)
	{
#ifdef DEBUG
		checkType<typename T::value_type>();
#endif
		// resize the pointer list
		ptr_send.resize(prc.size());
		sz_send.resize(prc.size());

		for (size_t i = 0 ; i < prc.size() ; i++)
		{
			ptr_send.get(i) = data.get(i).getPointer();
			sz_send.get(i) = data.get(i).size() * sizeof(typename T::value_type);
		}

		sendrecvMultipleMessagesNBX(prc.size(),(size_t *)sz_send.getPointer(),(size_t *)prc.getPointer(),(void **)ptr_send.getPointer(),msg_alloc,ptr_arg,opt);
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It sends multiple messages to a set of processors and receives
	 * multiple messages from another set of processors, all the processor must call this
	 * function, NBX is more performant than PCX with more processors (1000+)
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 vector of 100 integers to processor 1
	 * * 1 vector of 50 integers to processor 6
	 * * 1 vector of 48 integers to processor 7
	 * * 1 vector of 70 integers to processor 8
	 *
	 * \param prc list of processors you should communicate with [1,1,6,7,8]
	 *
	 * \param v vector containing the data to send [v=vector<vector<int>>, v.size()=4, T=vector<int>], T at the moment
	 *          is only tested for vectors of 0 or more generic elements (without pointers)
	 *
	 * \param msg_alloc This is a call-back with the purpose to allocate space
	 *        for the incoming messages and give back a valid pointer, supposing that this call-back has been triggered by
	 *        the processor of id 5 that want to communicate with me a message of size 100 byte the call-back will have
	 *        the following 6 parameters
	 *        in the call-back in order:
	 *        * message size required to receive the message [100]
	 *        * total message size to receive from all the processors (NBX does not provide this information)
	 *        * the total number of processor want to communicate with you (NBX does not provide this information)
	 *        * processor id [5]
	 *        * ri request id (it is an id that goes from 0 to total_p, and is incremented
	 *           every time message_alloc is called)
	 *        * void pointer, parameter for additional data to pass to the call-back
	 *
	 * \param opt options, only NONE supported
	 *
	 */
	template<typename T> void sendrecvMultipleMessagesPCX(openfpm::vector< size_t > & prc, openfpm::vector< T > & data, void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,void *), void * ptr_arg, long int opt=NONE)
	{
#ifdef DEBUG
		checkType<typename T::value_type>();
#endif
		// resize map with the number of processors
		map.resize(size);

		// reset the sending buffer
		map.fill(0);

		// create sending map
		for (size_t i = 0 ; i < prc.size() ; i++)
		{map.get(prc.get(i)) = 1;}

		// resize the pointer list
		ptr_send.resize(prc.size());
		sz_send.resize(prc.size());

		for (size_t i = 0 ; i < prc.size() ; i++)
		{
			ptr_send.get(i) = data.get(i).getPointer();
			sz_send.get(i) = data.get(i).size() * sizeof(typename T::value_type);
		}

		sendrecvMultipleMessagesPCX(prc.size(),(size_t *)map.getPointer(),(size_t *)sz_send.getPointer(),(size_t *)prc.getPointer(),(void **)ptr_send.getPointer(),msg_alloc,ptr_arg,opt);
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function, NBX is more performant than PCX with more processors (1000+)
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 messages of size 100 byte to processor 1
	 * * 1 message of size 50 byte to processor 6
	 * * 1 message of size 48 byte to processor 7
	 * * 1 message of size 70 byte to processor 8
	 *
	 * \param n_send number of send for this processor [4]
	 *
	 * \param prc list of processor with which it should communicate
	 *        [1,1,6,7,8]
	 *
	 * \param sz the array contain the size of the message for each processor
	 *        (zeros must not be presents) [100,100,50,48,70]
	 *
	 * \param ptr array that contain the pointers to the message to send
	 *
	 * \param msg_alloc This is a call-back with the purpose of allocate space
	 *        for the incoming message and give back a valid pointer, supposing that this call-back has been triggered by
	 *        the processor of id 5 that want to communicate with me a message of size 100 byte the call-back will have
	 *        the following 6 parameters
	 *        in the call-back are in order:
	 *        * message size required to receive the message [100]
	 *        * total message size to receive from all the processors (NBX does not provide this information)
	 *        * the total number of processor want to communicate with you (NBX does not provide this information)
	 *        * processor id [5]
	 *        * ri request id (it is an id that goes from 0 to total_p, and is incremented
	 *           every time message_alloc is called)
	 *        * void pointer, parameter for additional data to pass to the call-back
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBX(size_t n_send , size_t sz[], size_t prc[] , void * ptr[], void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,void *), void * ptr_arg, long int opt = NONE)
	{
		if (stat.size() != 0 || req.size() != 0)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " this function must be called when no other requests are in progress \n";


		stat.clear();
		req.clear();
		// Do MPI_Issend

		for (size_t i = 0 ; i < n_send ; i++)
		{
			if (sz[i] != 0)
			{
				req.add();

#ifdef SE_CLASS2
				check_valid(ptr[i],sz[i]);
#endif

				MPI_SAFE_CALL(MPI_Issend(ptr[i], sz[i], MPI_BYTE, prc[i], SEND_SPARSE + NBX_cnt, MPI_COMM_WORLD,&req.last()));
				log.logSend(prc[i]);
			}
		}

		size_t rid = 0;
		int flag = false;

		bool reached_bar_req = false;

		log.start(10);

		// Wait that all the send are acknowledge
		do
		{

			// flag that notify that this processor reach the barrier
			// Barrier request

			MPI_Status stat_t;
			int stat = false;
			MPI_SAFE_CALL(MPI_Iprobe(MPI_ANY_SOURCE,SEND_SPARSE + NBX_cnt,MPI_COMM_WORLD,&stat,&stat_t));

			// If I have an incoming message and is related to this NBX communication
			if (stat == true)
			{
				// Get the message size
				int msize;
				MPI_SAFE_CALL(MPI_Get_count(&stat_t,MPI_BYTE,&msize));

				// Get the pointer to receive the message
				void * ptr = msg_alloc(msize,0,0,stat_t.MPI_SOURCE,rid,ptr_arg);

				// Log the receiving request
				log.logRecv(stat_t);

				rid++;

				// Check the pointer
#ifdef SE_CLASS2
				check_valid(ptr,msize);
#endif
				MPI_SAFE_CALL(MPI_Recv(ptr,msize,MPI_BYTE,stat_t.MPI_SOURCE,SEND_SPARSE+NBX_cnt,MPI_COMM_WORLD,&stat_t));

#ifdef SE_CLASS2
				check_valid(ptr,msize);
#endif
			}

			// Check the status of all the MPI_issend and call the barrier if finished

			if (reached_bar_req == false)
			{
				int flag = false;
				if (req.size() != 0)
				{MPI_SAFE_CALL(MPI_Testall(req.size(),&req.get(0),&flag,MPI_STATUSES_IGNORE));}
				else
					flag = true;

				// If all send has been completed
				if (flag == true)
				{MPI_SAFE_CALL(MPI_Ibarrier(MPI_COMM_WORLD,&bar_req));reached_bar_req = true;}
			}

			// Check if all processor reached the async barrier
			if (reached_bar_req)
			{MPI_SAFE_CALL(MPI_Test(&bar_req,&flag,&bar_stat))};

			// produce a report if communication get stuck
			log.NBXreport(NBX_cnt,req,reached_bar_req,bar_stat);

		} while (flag == false);

		// Remove the executed request

		req.clear();
		stat.clear();
		log.clear();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % 1024;
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function, NBX is more performant than PCX with more processors (1000+)
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 messages of size 100 byte to processor 1
	 * * 1 message of size 50 byte to processor 6
	 * * 1 message of size 48 byte to processor 7
	 * * 1 message of size 70 byte to processor 8
	 *
	 * \param n_send number of send for this processor [4]
	 *
	 * \param prc list of processor with which it should communicate
	 *        [1,1,6,7,8]
	 *
	 * \param map array containing an array of the number of messages for
	 *        each processor the colling processor want to communicate
	 *        [0 2 0 0 0 0 1 1 1]
	 *
	 * \param sz the array contain the size of the message for each processor
	 *        (zeros must not be presents) [100,100,50,48,70]
	 *
	 * \param ptr array that contain the pointers to the message to send
	 *
	 * \param msg_alloc This is a call-back with the purpose of allocate space
	 *        for the incoming message and give back a valid pointer, supposing that this call-back has been triggered by
	 *        the processor of id 5 that want to communicate with me a message of size 100 byte the call-back will have
	 *        the following 6 parameters
	 *        in the call-back are in order:
	 *        * message size required to receive the message [100]
	 *        * total message size to receive from all the processors
	 *        * the total number of processor want to communicate with the calling processor
	 *        * processor id [5]
	 *        * ri request id (it is an id that goes from 0 to total_p, and is incremented
	 *           every time message_alloc is called)
	 *        * void pointer, parameter for additional data to pass to the call-back
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesPCX(size_t n_send, size_t * map, size_t sz[], size_t prc[] , void * ptr[], void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,void *), void * ptr_arg, long int opt)
	{
		if (stat.size() != 0 || req.size() != 0)
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " this function must be called when no other requests are in progress \n";

		stat.clear();
		req.clear();

		req.add();
		stat.add();

		proc_com.resize(1);

		MPI_SAFE_CALL(MPI_Ireduce_scatter(map,&proc_com.get(0),&map_scatter.get(0),MPI_UNSIGNED_LONG,MPI_SUM,MPI_COMM_WORLD,&req.last()));
		MPI_SAFE_CALL(MPI_Waitall(req.size(),&req.get(0),&stat.get(0)));

		// Remove the executed request

		req.clear();
		stat.clear();

		// Allocate the temporal buffer to store the message size for each processor

		size_t n_proc_com = proc_com.get(0);
		proc_com.resize(n_proc_com * 2);

		// queue n_proc_com MPI_Irecv with source ANY_SOURCE to get
		// the message length from each processor and
		//  send the message length to each processor

		for (size_t i = 0 ; i < n_proc_com ; i++)
		{
			req.add();
			MPI_SAFE_CALL(MPI_Irecv(&proc_com.get(i),1,MPI_UNSIGNED_LONG,MPI_ANY_SOURCE,MSG_LENGTH,MPI_COMM_WORLD,&req.last()));
		}

		for (size_t i = 0 ; i < n_send ; i++)
		{
			req.add();
			MPI_SAFE_CALL(MPI_Isend(&sz[i],1,MPI_UNSIGNED_LONG,prc[i],MSG_LENGTH,MPI_COMM_WORLD,&req.last()));
		}

		stat.resize(req.size());
		if (req.size() != 0) {MPI_SAFE_CALL(MPI_Waitall(req.size(),&req.get(0),&stat.get(0)));}

		// Use proc_com to get the processor id that try to communicate

		for (size_t i = 0 ; i < n_proc_com ; i++)
		{
			proc_com.get(n_proc_com+i) = stat.get(i).MPI_SOURCE;
		}

		// Remove the executed request

		req.clear();
		stat.clear();

		// Calculate the total size of the message

		size_t total_msg = 0;

		for (size_t i = 0 ; i < n_proc_com ; i++)
		{
			total_msg += proc_com.get(i);
		}

		// Receive the message

		for (size_t i = 0 ; i < n_proc_com ; i++)
		{
			void * ptr = msg_alloc(proc_com.get(i),total_msg,n_proc_com,proc_com.get(n_proc_com+i),i,ptr_arg);

			req.add();

#ifdef SE_CLASS2
				check_valid(ptr,proc_com.get(i));
#endif

			MPI_SAFE_CALL(MPI_Irecv(ptr,proc_com.get(i),MPI_BYTE,proc_com.get(i+n_proc_com),MSG_SEND_RECV,MPI_COMM_WORLD,&req.last()));
		}

		// Send all the messages this processor must send

		for (size_t i = 0 ; i < n_send ; i++)
		{
			req.add();

#ifdef SE_CLASS2
			check_valid(ptr[i],sz[i]);
#endif

			MPI_SAFE_CALL(MPI_Isend(ptr[i],sz[i],MPI_BYTE,prc[i],MSG_SEND_RECV,MPI_COMM_WORLD,&req.last()));
		}

		stat.resize(req.size());
		if (req.size() != 0) {MPI_SAFE_CALL(MPI_Waitall(req.size(),&req.get(0),&stat.get(0)));}

		// Remove the executed request
		req.clear();
		stat.clear();
	}

	/*! \brief Send data to a processor
	 *
	 * \warning In order to avoid deadlock every send must be coupled with a recv
	 *          in case you want to send data without knowledge from the other side
	 *          consider to use sendRecvMultipleMessages
	 *
	 * \warning operation is asynchronous execute must be called to ensure they are executed
	 *
	 * \see sendRecvMultipleMessages
	 *
	 * \param proc processor id
	 * \param tag id
	 * \param mem buffer with the data to send
	 * \param sz size
	 *
	 * \return true if succeed false otherwise
	 *
	 */
	bool send(size_t proc, size_t tag, const void * mem, size_t sz)
	{
		// send over MPI

		// Create one request
		req.add();

		// send
		MPI_IsendWB::send(proc,SEND_RECV_BASE + tag,mem,sz,req.last());

		return true;
	}


	/*! \brief Send data to a processor
	 *
	 * \warning In order to avoid deadlock every send must be coupled with a recv
	 *          in case you want to send data without knowledge from the other side
	 *          consider to use sendRecvMultipleMessages
	 *
	 * \warning operation is asynchronous execute must be called to ensure they are executed
	 *
	 * \see sendRecvMultipleMessages
	 *
	 * \param proc processor id
	 * \param tag id
	 * \param v buffer to send
	 *
	 * \return true if succeed false otherwise
	 *
	 */
	template<typename T, typename Mem, typename gr> bool send(size_t proc, size_t tag, openfpm::vector<T,Mem,gr> & v)
	{
#ifdef DEBUG
		checkType<T>();
#endif

		// send over MPI

		// Create one request
		req.add();

		// send
		MPI_IsendW<T,Mem,gr>::send(proc,SEND_RECV_BASE + tag,v,req.last());

		return true;
	}

	/*! \brief Recv data from a processor
	 *
	 * \warning In order to avoid deadlock every recv must be coupled with a send
	 *          in case you want to send data without knowledge from the other side
	 *          consider to use sendrecvMultipleMessagesPCX or sendrecvMultipleMessagesNBX
	 *
	 * \warning operation is asynchronous execute must be called to ensure they are executed
	 *
	 * \see sendrecvMultipleMessagesPCX sendrecvMultipleMessagesNBX
	 *
	 * \param proc processor id
	 * \param tag id
	 * \param v buffer to send
	 * \param sz size of the buffer
	 *
	 * \return true if succeed false otherwise
	 *
	 */
	bool recv(size_t proc, size_t tag, void * v, size_t sz)
	{
		// recv over MPI

		// Create one request
		req.add();

		// receive
		MPI_IrecvWB::recv(proc,SEND_RECV_BASE + tag,v,sz,req.last());

		return true;
	}

    /*! \brief Recv data from a processor
     *
     * \warning In order to avoid deadlock every recv must be coupled with a send
     *          in case you want to send data without knowledge from the other side
     *          consider to use sendrecvMultipleMessagesPCX sendrecvMultipleMessagesNBX
     *
     * \warning operation is asynchronous execute must be called to ensure they are executed
     *
     * \see sendrecvMultipleMessagesPCX sendrecvMultipleMessagesNBX
     *
     * \param proc processor id
     * \param tag id
     * \param v vector to send
     *
     * \return true if succeed false otherwise
     *
     */
    template<typename T, typename Mem, typename gr> bool recv(size_t proc, size_t tag, openfpm::vector<T,Mem,gr> & v)
    {
#ifdef DEBUG
            checkType<T>();
#endif

            // recv over MPI

            // Create one request
            req.add();

            // receive
            MPI_IrecvW<T>::recv(proc,SEND_RECV_BASE + tag,v,req.last());

            return true;
    }

	/*! \brief Gather the data from all processors
	 *
	 * send a primitive data T receive the same primitive T from all the other processors
	 *
	 * \warning operation is asynchronous execute must be called to ensure they are executed
	 *
	 * \param v vector to receive (automaticaly resized)
	 * \param send data to send
	 *
	 * \return true if succeed false otherwise
	 *
	 */
	template<typename T, typename Mem, typename gr> bool allGather(T & send, openfpm::vector<T,Mem,gr> & v)
	{
#ifdef DEBUG
		checkType<T>();
#endif

		// Create one request
		req.add();

		// Number of processors
		v.resize(getProcessingUnits());

		// gather
		MPI_IAllGatherW<T>::gather(&send,1,v.getPointer(),1,req.last());

		return true;
	}

	/*! \brief Execute all the requests
	 *
	 */
	void execute()
	{
		int err = 0;

		// if req == 0 return
		if (req.size() == 0)
			return;

		// Wait for all the requests
		stat.resize(req.size());
		err = MPI_Waitall(req.size(),&req.get(0),&stat.get(0));

		// MPI error get the message and abort MPI
		if (err != MPI_SUCCESS)
			MPI_Abort(MPI_COMM_WORLD,1);

		// Remove executed request and status
		req.clear();
		stat.clear();
	}


	/////////////////////// Semantic communication ///////////////////////
	#include "VCluster_semantic.ipp"
};

// Function to initialize the global VCluster //

extern Vcluster * global_v_cluster;


/*! \brief Initialize a global instance of Runtime Virtual Cluster Machine
 *
 * Initialize a global instance of Runtime Virtual Cluster Machine
 *
 */

static inline void init_global_v_cluster(int *argc, char ***argv)
{
	if (global_v_cluster == NULL)
		global_v_cluster = new Vcluster(argc,argv);
}

static inline void delete_global_v_cluster()
{
	delete global_v_cluster;
}

#endif

