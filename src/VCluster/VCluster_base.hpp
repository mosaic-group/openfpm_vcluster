#ifndef VCLUSTER_BASE_HPP_
#define VCLUSTER_BASE_HPP_


#include "util/cuda_util.hpp"
#ifdef OPENMPI
#include <mpi.h>
#include <mpi-ext.h>
#else
#include <mpi.h>
#endif
#include "MPI_wrapper/MPI_util.hpp"
#include "Vector/map_vector.hpp"
#include "MPI_wrapper/MPI_IallreduceW.hpp"
#include "MPI_wrapper/MPI_IrecvW.hpp"
#include "MPI_wrapper/MPI_IsendW.hpp"
#include "MPI_wrapper/MPI_IAllGather.hpp"
#include "MPI_wrapper/MPI_IBcastW.hpp"
#include <exception>
#include "Vector/map_vector.hpp"
#ifdef DEBUG
#include "util/check_no_pointers.hpp"
#include "util/util_debug.hpp"
#endif
#include "util/Vcluster_log.hpp"
#include "memory/BHeapMemory.hpp"
#include "Packer_Unpacker/has_max_prop.hpp"
#include "data_type/aggregate.hpp"
#include "util/ofp_context.hpp"

#ifdef HAVE_PETSC
#include <petscvec.h>
#endif

extern double time_spent;

enum NBX_Type
{
	NBX_UNACTIVE,
	NBX_UNKNOWN,
	NBX_KNOWN,
	NBX_KNOWN_PRC
};

constexpr int MSG_LENGTH = 1024;
constexpr int MSG_SEND_RECV = 1025;
constexpr int SEND_SPARSE = 8192;
constexpr int NONE = 1;
constexpr int NEED_ALL_SIZE = 2;

constexpr int SERIVCE_MESSAGE_TAG = 16384;
constexpr int SEND_RECV_BASE = 4096;
constexpr int GATHER_BASE = 24576;

constexpr int RECEIVE_KNOWN = 4;
constexpr int KNOWN_ELEMENT_OR_BYTE = 8;
constexpr int MPI_GPU_DIRECT = 16;

constexpr int NQUEUE = 4;

// number of vcluster instances
extern size_t n_vcluster;
// Global MPI initialization
extern bool global_mpi_init;
// initialization flag
extern bool ofp_initialized;
extern size_t tot_sent;
extern size_t tot_recv;

///////////////////// Post functions /////////////

extern size_t NBX_cnt;

template<typename T> void assign(T * ptr1, T * ptr2)
{
	*ptr1 = *ptr2;
};


//! temporal buffer for reductions
union red
{
	//! char
	char c;
	//! unsigned char
	unsigned char uc;
	//! signed
	short s;
	//! unsigned short
	unsigned short us;
	//! integer
	int i;
	//! unsigned integer
	unsigned int ui;
	//! float
	float f;
	//! double
	double d;
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
template<typename InternalMemory>
class Vcluster_base
{
	//! log file
	Vcluster_log log;

	//! temporal vector used for meta-communication
	//! ( or meta-data before the real communication )
	openfpm::vector<size_t> proc_com;

	//! vector that contain the scatter map (it is basically an array of one)
	openfpm::vector<int> map_scatter;

	//! vector of MPI requests
	openfpm::vector<MPI_Request> req;

	//! vector of MPI status
	openfpm::vector<MPI_Status> stat;

	//! vector of functions to execute after all the request has been performed
	std::vector<int> post_exe;

	//! standard context for gpu (if cuda is detected otherwise is unused)
	gpu::ofp_context_t* gpuContext;

	// Single objects

	//! number of processes
	int m_size;
	//! actual rank
	int m_rank;

	//! number of processing unit per process
	int numPE = 1;

	//////////////// NBX calls status variables ///////////////

	NBX_Type NBX_active[NQUEUE];

	//! request id
	size_t rid[NQUEUE];

	//! NBX comunication on queue (-1 mean 0, 0 mean 1, 1 mean 2, .... )
	int NBX_prc_qcnt = -1;

	//! Is the barrier request reached
	bool NBX_prc_reached_bar_req[NQUEUE];

	////// Status variables for NBX send with known/unknown processors

	int NBX_prc_cnt_base = 0;
	size_t NBX_prc_n_send[NQUEUE];
	size_t * NBX_prc_prc[NQUEUE];
	void ** NBX_prc_ptr[NQUEUE];
	size_t * NBX_prc_sz[NQUEUE];
	size_t NBX_prc_n_recv[NQUEUE];
	void * (* NBX_prc_msg_alloc[NQUEUE])(size_t,size_t,size_t,size_t,size_t,size_t,void *);
	size_t * NBX_prc_prc_recv[NQUEUE];
	void * NBX_prc_ptr_arg[NQUEUE];

	///////////////////////////////////////////////////////////

	/*! This buffer is a temporal buffer for reductions
	 *
	 * MPI_Iallreduce does not accept recv and send buffer to be the same
	 * r is used to overcome this problem (is given as second parameter)
	 * after the execution the data is copied back
	 *
	 */
	std::vector<red> r;

	//! vector of pointers of send buffers
	openfpm::vector<void *> ptr_send[NQUEUE];

	//! vector of the size of send buffers
	openfpm::vector<size_t> sz_send[NQUEUE];

	//! barrier request
	MPI_Request bar_req;

	//! barrier status
	MPI_Status bar_stat;

	//! disable operator=
	Vcluster_base & operator=(const Vcluster_base &)	{return *this;};

	//! rank within the node
	int shmrank;

	//! NBX_cycle
	int nbx_cycle;

	//! disable copy constructor
	Vcluster_base(const Vcluster_base &)
	{};

	void queue_all_sends(size_t n_send , size_t sz[],
            			 size_t prc[], void * ptr[])
	{
		if (stat.size() != 0 || (req.size() != 0 && NBX_prc_qcnt == 0))
		{std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " this function must be called when no other requests are in progress. Please remember that if you use function like max(),sum(),send(),recv() check that you did not miss to call the function execute() \n";}


		if (NBX_prc_qcnt == 0)
		{
			stat.clear();
			req.clear();
			// Do MPI_Issend
		}

		for (size_t i = 0 ; i < n_send ; i++)
		{
			if (sz[i] != 0)
			{
				req.add();

#ifdef SE_CLASS2
				check_valid(ptr[i],sz[i]);
#endif

				tot_sent += sz[i];

//				std::cout << "TAG: " << SEND_SPARSE + (NBX_cnt + NBX_prc_qcnt)*131072 + i << "   " << NBX_cnt << "   "  << NBX_prc_qcnt << "  " << " rank: " << rank() << "   " << NBX_prc_cnt_base << "  nbx_cycle: " << nbx_cycle << std::endl;

				if (sz[i] > 2147483647)
				{MPI_SAFE_CALL(MPI_Issend(ptr[i], (sz[i] >> 3) + 1 , MPI_DOUBLE, prc[i], SEND_SPARSE + (NBX_cnt + NBX_prc_qcnt)*131072 + i, MPI_COMM_WORLD,&req.last()));}
				else
				{MPI_SAFE_CALL(MPI_Issend(ptr[i], sz[i], MPI_BYTE, prc[i], SEND_SPARSE + (NBX_cnt + NBX_prc_qcnt)*131072 + i, MPI_COMM_WORLD,&req.last()));}
				log.logSend(prc[i]);
			}
		}
	}

protected:

	//! Receive buffers
	openfpm::vector_fr<BMemory<InternalMemory>> recv_buf[NQUEUE];

	//! tags receiving
	openfpm::vector<size_t> tags[NQUEUE];

public:

	// Finalize the MPI program
	~Vcluster_base()
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

		delete gpuContext;
	}

	/*! \brief Virtual cluster constructor
	 *
	 * \param argc pointer to arguments counts passed to the program
	 * \param argv pointer to arguments vector passed to the program
	 *
	 */
	Vcluster_base(int *argc, char ***argv)
	{
		// reset NBX_Active

		for (unsigned int i = 0 ; i < NQUEUE ; i++)
		{
			NBX_active[i] = NBX_Type::NBX_UNACTIVE;
			rid[i] = 0;
		}

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

		// We try to get the local processors rank

		MPI_Comm shmcomm;
		MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0,
		                    MPI_INFO_NULL, &shmcomm);

		MPI_Comm_rank(shmcomm, &shmrank);
		MPI_Comm_free(&shmcomm);

		// Get the total number of process
		// and the rank of this process

		MPI_Comm_size(MPI_COMM_WORLD, &m_size);
		MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);

#ifdef SE_CLASS2
			process_v_cl = m_rank;
#endif

		// create and fill map scatter with one
		map_scatter.resize(m_size);

		for (size_t i = 0 ; i < map_scatter.size() ; i++)
		{
			map_scatter.get(i) = 1;
		}

		// open the log file
		log.openLog(m_rank);

		// Initialize bar_req
		bar_req = MPI_Request();
		bar_stat = MPI_Status();

#ifdef EXTERNAL_SET_GPU
                int dev;
                cudaGetDevice(&dev);
                gpuContext = new gpu::ofp_context_t(gpu::gpu_context_opt::no_print_props,dev);
#else
                gpuContext = new gpu::ofp_context_t(gpu::gpu_context_opt::no_print_props,shmrank);
#endif


#if defined(PRINT_RANK_TO_GPU) && defined(CUDA_GPU)

                char node_name[MPI_MAX_PROCESSOR_NAME];
                int len;

                MPI_Get_processor_name(node_name,&len);

                std::cout << "Rank: " << m_rank << " on host: " << node_name << " work on GPU: " << gpuContext->getDevice() << "/" << gpuContext->getNDevice() << std::endl;
#endif

                int flag;
                void *tag_ub_v;
                int tag_ub;

                MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &tag_ub_v, &flag);
                tag_ub = *(int*)tag_ub_v;

                if (flag == true)
                {
                	nbx_cycle = (tag_ub - SEND_SPARSE - 131072 - NQUEUE*131072) / 131072;

                	if (nbx_cycle < NQUEUE*2)
                	{std::cerr << __FILE__ << ":" << __LINE__ << " Error MPI_TAG_UB is too small for OpenFPM" << std::endl;}
                }
                else
                {nbx_cycle = 2048;}
	}

#ifdef SE_CLASS1

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
		{return;}

		// if it is a pointer make no sense
		if (std::is_pointer<T>::value == true)
		{std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";}

		// if it is an l-value reference make no send
		if (std::is_lvalue_reference<T>::value == true)
		{std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";}

		// if it is an r-value reference make no send
		if (std::is_rvalue_reference<T>::value == true)
		{std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " the type " << demangle(typeid(T).name()) << " is a pointer, sending pointers values has no sense\n";}

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

	/*! \brief If nvidia cuda is activated return a gpu context
	 *
	 * \param iw ignore warning
	 *
	 */
	gpu::ofp_context_t& getgpuContext(bool iw = true)
	{
		if (gpuContext == NULL && iw == true)
		{
			std::cout << __FILE__ << ":" << __LINE__ << " Warning: it seem that a gpu context is not initialized."
					                                    "Either a compatible working cuda device has not been found, either openfpm_init has been called in a file that not compiled with NVCC" << std::endl;
		}

		return *gpuContext;
	}

	/*! \brief Get the MPI_Communicator (or processor group) this VCluster is using
	 *
	 * \return MPI comunicator
	 *
	 */
	MPI_Comm getMPIComm()
	{
		return MPI_COMM_WORLD;
	}

	/*! \brief Get the total number of processors
	 *
	 * \return the total number of processors
	 *
	 */
	size_t getProcessingUnits()
	{
		return m_size*numPE;
	}

	/*! \brief Get the total number of processors
	 *
	 * It is the same as getProcessingUnits()
	 *
	 * \see getProcessingUnits()
	 *
	 * \return the total number of processors
	 *
	 */
	size_t size()
	{
		return this->m_size*numPE;
	}

	void print_stats()
	{
#ifdef VCLUSTER_PERF_REPORT
		std::cout << "-- REPORT COMMUNICATIONS -- " << std::endl;

		std::cout << "Processor " << this->rank() << " sent: " << tot_sent << std::endl;
		std::cout << "Processor " << this->rank() << " received: " << tot_recv << std::endl;

		std::cout << "Processor " << this->rank() << " time spent: " << time_spent << std::endl;
		std::cout << "Processor " << this->rank() << " Bandwidth: S:"  << (double)tot_sent / time_spent * 1e-9 << "GB/s  R:" << (double)tot_recv / time_spent * 1e-9 << "GB/s" <<  std::endl;
#else

		std::cout << "Error to activate performance stats on VCluster enable VCLUSTER_PERF_REPORT" << std::endl;

#endif
	}

	void clear_stats()
	{
#ifdef VCLUSTER_PERF_REPORT

		tot_sent = 0;
		tot_recv = 0;

		time_spent = 0;
#else

		std::cout << "Error to activate performance stats on VCluster enable VCLUSTER_PERF_REPORT" << std::endl;

#endif
	}

	/*! \brief Get the process unit id
	 *
	 * \return the process ID (rank in MPI)
	 *
	 */
	size_t getProcessUnitID()
	{
		return m_rank;
	}

	/*! \brief Get the process unit id
	 *
	 * It is the same as getProcessUnitID()
	 *
	 * \see getProcessUnitID()
	 *
	 * \return the process ID
	 *
	 */
	size_t rank()
	{
		return m_rank;
	}


	/*! \brief Sum the numbers across all processors and get the result
	 *
	 * \param num to reduce, input and output
	 *
	 */

	template<typename T> void sum(T & num)
	{
#ifdef SE_CLASS1
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
#ifdef SE_CLASS1
		checkType<T>();
#endif
		// reduce over MPI

		// Create one request
		req.add();

		// reduce
		MPI_IallreduceW<T>::reduce(num,MPI_MAX,req.last());
	}

	/*! \brief Get the minimum number across all processors (or reduction with insinity norm)
	 *
	 * \param num to reduce
	 *
	 */

	template<typename T> void min(T & num)
	{
#ifdef SE_CLASS1
		checkType<T>();
#endif
		// reduce over MPI

		// Create one request
		req.add();

		// reduce
		MPI_IallreduceW<T>::reduce(num,MPI_MIN,req.last());
	}

	/*! \brief In case of Asynchonous communications like sendrecvMultipleMessagesNBXAsync this function
	 * progress the communication
	 *
	 *
	 */
	void progressCommunication()
	{
		MPI_Status stat_t;
		int stat = false;
		MPI_SAFE_CALL(MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&stat,&stat_t));

		// If I have an incoming message and is related to this NBX communication
		if (stat == true)
		{
			unsigned int i = (stat_t.MPI_TAG - SEND_SPARSE) / 131072 - NBX_prc_cnt_base;

			if (i >= NQUEUE || NBX_active[i] == NBX_Type::NBX_UNACTIVE || NBX_active[i] == NBX_Type::NBX_KNOWN  || NBX_active[i] == NBX_Type::NBX_KNOWN_PRC)
			{return;}

			int msize_;
			long int msize;
			bool big_data = true;			

			// Get the message tag and size
			
			MPI_SAFE_CALL(MPI_Get_count(&stat_t,MPI_DOUBLE,&msize_));
			if (msize_ == MPI_UNDEFINED)
			{
				big_data = false;
				MPI_SAFE_CALL(MPI_Get_count(&stat_t,MPI_BYTE,&msize_));
				msize = msize_;
			}
			else
			{
				msize = ((size_t)msize_) << 3;
			}

			// Ok we check if the TAG come from one of our send TAG
			if (stat_t.MPI_TAG >= (int)(SEND_SPARSE + NBX_prc_cnt_base*131072) && stat_t.MPI_TAG < (int)(SEND_SPARSE + (NBX_prc_cnt_base + NBX_prc_qcnt + 1)*131072))
			{
				// Get the pointer to receive the message
				void * ptr = this->NBX_prc_msg_alloc[i](msize,0,0,stat_t.MPI_SOURCE,rid[i],stat_t.MPI_TAG,this->NBX_prc_ptr_arg[i]);

				// Log the receiving request
				log.logRecv(stat_t);

				rid[i]++;

				// Check the pointer
#ifdef SE_CLASS2
				check_valid(ptr,msize);
#endif
				tot_recv += msize;
				#ifdef VCLUSTER_GARBAGE_INJECTOR
					#if defined (__NVCC__) && !defined(CUDA_ON_CPU)
					cudaPointerAttributes cpa;
					auto error = cudaPointerGetAttributes(&cpa,ptr);
					if (error == cudaSuccess)
					{
						if(cpa.type == cudaMemoryTypeDevice)
						{cudaMemset(ptr,0xFF,msize);}
						else
						{memset(ptr,0xFF,msize);}
					}
					#else
					memset(ptr,0xFF,msize);
					#endif
				#endif
				if (big_data == true)
				{
//					std::cout << "RECEVING BIG MESSAGE " << msize_ << "   "  << msize << std::endl;
					MPI_SAFE_CALL(MPI_Recv(ptr,msize >> 3,MPI_DOUBLE,stat_t.MPI_SOURCE,stat_t.MPI_TAG,MPI_COMM_WORLD,&stat_t));
				}
				else
				{
					MPI_SAFE_CALL(MPI_Recv(ptr,msize,MPI_BYTE,stat_t.MPI_SOURCE,stat_t.MPI_TAG,MPI_COMM_WORLD,&stat_t));
				}
#ifdef SE_CLASS2
				check_valid(ptr,msize);
#endif
			}
		}

		// Check the status of all the MPI_issend and call the barrier if finished

		for (unsigned int i = 0 ; i < NQUEUE ; i++)
		{
			if (i >= NQUEUE || NBX_active[i] == NBX_Type::NBX_UNACTIVE || NBX_active[i] == NBX_Type::NBX_KNOWN || NBX_active[i] == NBX_Type::NBX_KNOWN_PRC)
			{continue;}

			if (NBX_prc_reached_bar_req[i] == false)
			{
				int flag = false;
				if (req.size() != 0)
				{MPI_SAFE_CALL(MPI_Testall(req.size(),&req.get(0),&flag,MPI_STATUSES_IGNORE));}
				else
				{flag = true;}

				// If all send has been completed
				if (flag == true)
				{MPI_SAFE_CALL(MPI_Ibarrier(MPI_COMM_WORLD,&bar_req));NBX_prc_reached_bar_req[i] = true;}
			}
		}
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive.
	 *
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 messages of size 100 byte to processor 1
	 * * 1 message of size 50 byte to processor 6
	 * * 1 message of size 48 byte to processor 7
	 * * 1 message of size 70 byte to processor 8
	 *
	 *
	 * \param prc list of processor with which it should communicate
	 *        [1,1,6,7,8]
	 *
	 * \param data data to send for each processors in contain a pointer to some type T
	 *        this type T must have a method size() that return the size of the data-structure
	 *
	 * \param prc_recv processor that receive data
	 *
	 * \param recv_sz for each processor indicate the size of the data received
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	template<typename T> void sendrecvMultipleMessagesNBX(openfpm::vector< size_t > & prc,
			                                              openfpm::vector< T > & data,
														  openfpm::vector< size_t > & prc_recv,
														  openfpm::vector< size_t > & recv_sz ,
														  void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
														  void * ptr_arg,
														  long int opt=NONE)
	{
		#ifdef VCLUSTER_PERF_REPORT
		timer nbx_timer;
		nbx_timer.start();

		#endif

		// Allocate the buffers

		for (size_t i = 0 ; i < prc.size() ; i++)
		{send(prc.get(i),SEND_SPARSE + NBX_cnt*131072,data.get(i).getPointer(),data.get(i).size());}

		for (size_t i = 0 ; i < prc_recv.size() ; i++)
		{
			void * ptr_recv = msg_alloc(recv_sz.get(i),0,0,prc_recv.get(i),i,SEND_SPARSE + NBX_cnt*131072,ptr_arg);

			recv(prc_recv.get(i),SEND_SPARSE + NBX_cnt*131072,ptr_recv,recv_sz.get(i));
		}

		execute();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % nbx_cycle;

		#ifdef VCLUSTER_PERF_REPORT
		nbx_timer.stop();
		time_spent += nbx_timer.getwct();
		#endif
	}

	/*! \brief Send and receive multiple messages asynchronous version
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive.
	 *
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 messages of size 100 byte to processor 1
	 * * 1 message of size 50 byte to processor 6
	 * * 1 message of size 48 byte to processor 7
	 * * 1 message of size 70 byte to processor 8
	 *
	 *
	 * \param prc list of processor with which it should communicate
	 *        [1,1,6,7,8]
	 *
	 * \param data data to send for each processors in contain a pointer to some type T
	 *        this type T must have a method size() that return the size of the data-structure
	 *
	 * \param prc_recv processor that receive data
	 *
	 * \param recv_sz for each processor indicate the size of the data received
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	template<typename T> void sendrecvMultipleMessagesNBXAsync(openfpm::vector< size_t > & prc,
			                                              openfpm::vector< T > & data,
														  openfpm::vector< size_t > & prc_recv,
														  openfpm::vector< size_t > & recv_sz ,
														  void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
														  void * ptr_arg,
														  long int opt=NONE)
	{
		NBX_prc_qcnt++;
		if (NBX_prc_qcnt >= NQUEUE)
		{
			std::cout << __FILE__ << ":" << __LINE__ << " error you can queue at most " << NQUEUE << " asychronous communication functions " << std::endl;
			return;
		}

		// Allocate the buffers

		for (size_t i = 0 ; i < prc.size() ; i++)
		{send(prc.get(i),SEND_SPARSE + NBX_cnt*131072,data.get(i).getPointer(),data.get(i).size());}

		for (size_t i = 0 ; i < prc_recv.size() ; i++)
		{
			void * ptr_recv = msg_alloc(recv_sz.get(i),0,0,prc_recv.get(i),i,SEND_SPARSE + NBX_cnt*131072,ptr_arg);

			recv(prc_recv.get(i),SEND_SPARSE + NBX_cnt*131072,ptr_recv,recv_sz.get(i));
		}

		NBX_active[NBX_prc_qcnt] = NBX_Type::NBX_KNOWN;
		if (NBX_prc_qcnt == 0)
		{NBX_prc_cnt_base = NBX_cnt;}
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 vector of 100 integers to processor 1
	 * * 1 vector of 50 integers to processor 6
	 * * 1 vector of 48 integers to processor 7
	 * * 1 vector of 70 integers to processor 8
	 *
	 * \param prc list of processors you should communicate with [1,1,6,7,8]
	 *
	 * \param data vector containing the data to send [v=vector<vector<int>>, v.size()=4, T=vector<int>], T at the moment
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, only NONE supported
	 *
	 */
	template<typename T>
	void sendrecvMultipleMessagesNBX(openfpm::vector< size_t > & prc,
									 openfpm::vector< T > & data,
									 void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
									 void * ptr_arg, long int opt=NONE)
	{
#ifdef SE_CLASS1
		checkType<typename T::value_type>();
#endif

		// resize the pointer list
		ptr_send[NBX_prc_qcnt+1].resize(prc.size());
		sz_send[NBX_prc_qcnt+1].resize(prc.size());

		for (size_t i = 0 ; i < prc.size() ; i++)
		{
			ptr_send[NBX_prc_qcnt+1].get(i) = data.get(i).getPointer();
			sz_send[NBX_prc_qcnt+1].get(i) = data.get(i).size() * sizeof(typename T::value_type);
		}

		sendrecvMultipleMessagesNBX(prc.size(),(size_t *)sz_send[NBX_prc_qcnt+1].getPointer(),(size_t *)prc.getPointer(),(void **)ptr_send[NBX_prc_qcnt+1].getPointer(),msg_alloc,ptr_arg,opt);
	}

	/*! \brief Send and receive multiple messages asynchronous version
	 *
	 * This is the Asynchronous version of Send and receive NBX. This call return immediately, use
	 * sendrecvMultipleMessagesNBXWait to synchronize. Optionally you can use the function progress_communication
	 * to move on the communication
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function
	 *
	 * suppose the following situation the calling processor want to communicate
	 * * 2 vector of 100 integers to processor 1
	 * * 1 vector of 50 integers to processor 6
	 * * 1 vector of 48 integers to processor 7
	 * * 1 vector of 70 integers to processor 8
	 *
	 * \param prc list of processors you should communicate with [1,1,6,7,8]
	 *
	 * \param data vector containing the data to send [v=vector<vector<int>>, v.size()=4, T=vector<int>], T at the moment
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, only NONE supported
	 *
	 */
	template<typename T>
	void sendrecvMultipleMessagesNBXAsync(openfpm::vector< size_t > & prc,
									 openfpm::vector< T > & data,
									 void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
									 void * ptr_arg, long int opt=NONE)
	{
#ifdef SE_CLASS1
		checkType<typename T::value_type>();
#endif
		// resize the pointer list
		ptr_send[NBX_prc_qcnt+1].resize(prc.size());
		sz_send[NBX_prc_qcnt+1].resize(prc.size());

		for (size_t i = 0 ; i < prc.size() ; i++)
		{
			ptr_send[NBX_prc_qcnt+1].get(i) = data.get(i).getPointer();
			sz_send[NBX_prc_qcnt+1].get(i) = data.get(i).size() * sizeof(typename T::value_type);
		}

		sendrecvMultipleMessagesNBXAsync(prc.size(),(size_t *)sz_send[NBX_prc_qcnt+1].getPointer(),(size_t *)prc.getPointer(),(void **)ptr_send[NBX_prc_qcnt+1].getPointer(),msg_alloc,ptr_arg,opt);
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive.
	 *
	 * \warning this function only work with one send for each processor
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBX(size_t n_send , size_t sz[],
			                         size_t prc[] , void * ptr[],
			                         size_t n_recv, size_t prc_recv[] ,
			                         size_t sz_recv[] ,void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t, size_t,void *),
			                         void * ptr_arg, long int opt=NONE)
	{
		#ifdef VCLUSTER_PERF_REPORT
		timer nbx_timer;
		nbx_timer.start();

		#endif

		// Allocate the buffers

		for (size_t i = 0 ; i < n_send ; i++)
		{send(prc[i],SEND_SPARSE + NBX_cnt*131072,ptr[i],sz[i]);}

		for (size_t i = 0 ; i < n_recv ; i++)
		{
			void * ptr_recv = msg_alloc(sz_recv[i],0,0,prc_recv[i],i,SEND_SPARSE + NBX_cnt*131072,ptr_arg);

			recv(prc_recv[i],SEND_SPARSE + NBX_cnt*131072,ptr_recv,sz_recv[i]);
		}

		execute();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % nbx_cycle;

		#ifdef VCLUSTER_PERF_REPORT
		nbx_timer.stop();
		time_spent += nbx_timer.getwct();
		#endif
	}

	/*! \brief Send and receive multiple messages asynchronous version
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive.
	 *
	 * \warning this function only work with one send for each processor
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBXAsync(size_t n_send , size_t sz[],
			                         size_t prc[] , void * ptr[],
			                         size_t n_recv, size_t prc_recv[] ,
			                         size_t sz_recv[] ,void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t, size_t,void *),
			                         void * ptr_arg, long int opt=NONE)
	{
		NBX_prc_qcnt++;
		if (NBX_prc_qcnt >= NQUEUE)
		{
			std::cout << __FILE__ << ":" << __LINE__ << " error you can queue at most " << NQUEUE << " asychronous communication functions " << std::endl;
			return;
		}

		// Allocate the buffers

		for (size_t i = 0 ; i < n_send ; i++)
		{send(prc[i],SEND_SPARSE + NBX_cnt*131072,ptr[i],sz[i]);}

		for (size_t i = 0 ; i < n_recv ; i++)
		{
			void * ptr_recv = msg_alloc(sz_recv[i],0,0,prc_recv[i],i,SEND_SPARSE + NBX_cnt*131072,ptr_arg);

			recv(prc_recv[i],SEND_SPARSE + NBX_cnt*131072,ptr_recv,sz_recv[i]);
		}

		NBX_active[NBX_prc_qcnt] = NBX_Type::NBX_KNOWN;
		if (NBX_prc_qcnt == 0)
		{NBX_prc_cnt_base = NBX_cnt;}
	}

	openfpm::vector<size_t> sz_recv_tmp;

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive, but does not know the size.
	 *
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBX(size_t n_send , size_t sz[], size_t prc[] ,
									 void * ptr[], size_t n_recv, size_t prc_recv[] ,
									 void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
									 void * ptr_arg, long int opt=NONE)
	{
		#ifdef VCLUSTER_PERF_REPORT
		timer nbx_timer;
		nbx_timer.start();
		#endif

		sz_recv_tmp.resize(n_recv);

		// First we understand the receive size for each processor

		for (size_t i = 0 ; i < n_send ; i++)
		{send(prc[i],SEND_SPARSE + NBX_cnt*131072,&sz[i],sizeof(size_t));}

		for (size_t i = 0 ; i < n_recv ; i++)
		{recv(prc_recv[i],SEND_SPARSE + NBX_cnt*131072,&sz_recv_tmp.get(i),sizeof(size_t));}

		execute();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % nbx_cycle;

		// Allocate the buffers

		for (size_t i = 0 ; i < n_send ; i++)
		{send(prc[i],SEND_SPARSE + NBX_cnt*131072,ptr[i],sz[i]);}

		for (size_t i = 0 ; i < n_recv ; i++)
		{
			void * ptr_recv = msg_alloc(sz_recv_tmp.get(i),0,0,prc_recv[i],i,0,ptr_arg);

			recv(prc_recv[i],SEND_SPARSE + NBX_cnt*131072,ptr_recv,sz_recv_tmp.get(i));
		}

		execute();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % nbx_cycle;

		#ifdef VCLUSTER_PERF_REPORT
		nbx_timer.stop();
		time_spent += nbx_timer.getwct();
		#endif
	}

	/*! \brief Send and receive multiple messages asynchronous version
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function. In this particular case the receiver know from which processor is going
	 * to receive, but does not know the size.
	 *
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBXAsync(size_t n_send , size_t sz[], size_t prc[] ,
									 void * ptr[], size_t n_recv, size_t prc_recv[] ,
									 void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
									 void * ptr_arg, long int opt=NONE)
	{
		NBX_prc_qcnt++;
		if (NBX_prc_qcnt >= NQUEUE)
		{
			std::cout << __FILE__ << ":" << __LINE__ << " error you can queue at most " << NQUEUE << " asychronous communication functions " << std::endl;
			return;
		}

		sz_recv_tmp.resize(n_recv);

		// First we understand the receive size for each processor

		for (size_t i = 0 ; i < n_send ; i++)
		{send(prc[i],SEND_SPARSE + NBX_cnt*131072,&sz[i],sizeof(size_t));}

		for (size_t i = 0 ; i < n_recv ; i++)
		{recv(prc_recv[i],SEND_SPARSE + NBX_cnt*131072,&sz_recv_tmp.get(i),sizeof(size_t));}

		////// Save all the status variables

		NBX_prc_n_send[NBX_prc_qcnt] = n_send;
		NBX_prc_prc[NBX_prc_qcnt] = prc;
		NBX_prc_ptr[NBX_prc_qcnt] = ptr;
		NBX_prc_sz[NBX_prc_qcnt] = sz;
		NBX_prc_n_recv[NBX_prc_qcnt] = n_recv;
		NBX_prc_prc_recv[NBX_prc_qcnt] = prc_recv;
		NBX_prc_msg_alloc[NBX_prc_qcnt] = msg_alloc;
		NBX_prc_ptr_arg[NBX_prc_qcnt] = ptr_arg;

		NBX_active[NBX_prc_qcnt] = NBX_Type::NBX_KNOWN_PRC;
		if (NBX_prc_qcnt == 0)
		{NBX_prc_cnt_base = NBX_cnt;}
	}

	/*! \brief Send and receive multiple messages
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBX(size_t n_send , size_t sz[],
			                         size_t prc[] , void * ptr[],
			                         void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
			                         void * ptr_arg, long int opt = NONE)
	{
		#ifdef VCLUSTER_PERF_REPORT
		timer nbx_timer;
		nbx_timer.start();

		#endif

		NBX_prc_qcnt++;
		if (NBX_prc_qcnt != 0)
		{
			std::cout << __FILE__ << ":" << __LINE__ << " error there are some asynchronous call running you have to complete them before go back to synchronous" << std::endl;
			return;
		}

		queue_all_sends(n_send,sz,prc,ptr);

		this->NBX_prc_ptr_arg[NBX_prc_qcnt] = ptr_arg;
		this->NBX_prc_msg_alloc[NBX_prc_qcnt] = msg_alloc;

		rid[NBX_prc_qcnt] = 0;
		int flag = false;

		NBX_prc_reached_bar_req[NBX_prc_qcnt] = false;
		NBX_active[NBX_prc_qcnt] = NBX_Type::NBX_UNKNOWN;
		NBX_prc_cnt_base = NBX_cnt;

		log.start(10);

		// Wait that all the send are acknowledge
		do
		{
			progressCommunication();

			// Check if all processor reached the async barrier
			if (NBX_prc_reached_bar_req[NBX_prc_qcnt])
			{MPI_SAFE_CALL(MPI_Test(&bar_req,&flag,&bar_stat))};

			// produce a report if communication get stuck
			log.NBXreport(NBX_cnt,req,NBX_prc_reached_bar_req[NBX_prc_qcnt],bar_stat);

		} while (flag == false);

		// Remove the executed request

		req.clear();
		stat.clear();
		log.clear();

		// Circular counter
		NBX_cnt = (NBX_cnt + 1) % nbx_cycle;
		NBX_prc_qcnt = -1;

		#ifdef VCLUSTER_PERF_REPORT
		nbx_timer.stop();
		time_spent += nbx_timer.getwct();
		#endif
	}

	/*! \brief Send and receive multiple messages Asynchronous version
	 *
	 * This is the Asynchronous version of Send and receive NBX. This call return immediately, use
	 * sendrecvMultipleMessagesNBXWait to synchronize. Optionally you can use the function progress_communication
	 * to move on the communication
	 *
	 * It send multiple messages to a set of processors the and receive
	 * multiple messages from another set of processors, all the processor must call this
	 * function
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
	 * \param ptr_arg data passed to the call-back function specified
	 *
	 * \param opt options, NONE (ignored in this moment)
	 *
	 */
	void sendrecvMultipleMessagesNBXAsync(size_t n_send , size_t sz[],
			                         size_t prc[] , void * ptr[],
			                         void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,size_t,void *),
			                         void * ptr_arg, long int opt = NONE)
	{
		NBX_prc_qcnt++;
		queue_all_sends(n_send,sz,prc,ptr);

		this->NBX_prc_ptr_arg[NBX_prc_qcnt] = ptr_arg;
		this->NBX_prc_msg_alloc[NBX_prc_qcnt] = msg_alloc;

		rid[NBX_prc_qcnt] = 0;

		NBX_prc_reached_bar_req[NBX_prc_qcnt] = false;
		NBX_active[NBX_prc_qcnt] = NBX_Type::NBX_UNKNOWN;

		log.start(10);
		if (NBX_prc_qcnt == 0)
		{NBX_prc_cnt_base = NBX_cnt;}

		return;
	}

	/*! \brief Send and receive multiple messages wait NBX communication to complete
	 *
	 *
	 */
	void sendrecvMultipleMessagesNBXWait()
	{
		for (unsigned int j = 0 ; j < NQUEUE ; j++)
		{
			if (NBX_active[j] == NBX_Type::NBX_UNACTIVE)
			{continue;}

			if (NBX_active[j] == NBX_Type::NBX_KNOWN_PRC)
			{
				execute();

				// Circular counter
				NBX_cnt = (NBX_cnt + 1) % nbx_cycle;

				// Allocate the buffers

				for (size_t i = 0 ; i < NBX_prc_n_send[j] ; i++)
				{send(NBX_prc_prc[j][i],SEND_SPARSE + NBX_cnt*131072,NBX_prc_ptr[j][i],NBX_prc_sz[j][i]);}

				for (size_t i = 0 ; i < NBX_prc_n_recv[j] ; i++)
				{
					void * ptr_recv = NBX_prc_msg_alloc[j](sz_recv_tmp.get(i),0,0,NBX_prc_prc_recv[j][i],i,0,this->NBX_prc_ptr_arg[j]);

					recv(NBX_prc_prc_recv[j][i],SEND_SPARSE + NBX_cnt*131072,ptr_recv,sz_recv_tmp.get(i));
				}

				NBX_active[j] = NBX_Type::NBX_KNOWN;
			}

			if (NBX_active[j] == NBX_Type::NBX_KNOWN)
			{
				execute();

				// Circular counter
				NBX_cnt = (NBX_cnt + 1) % nbx_cycle;
				NBX_active[j] = NBX_Type::NBX_UNACTIVE;

				continue;
			}

			int flag = false;

			// Wait that all the send are acknowledge
			do
			{
				progressCommunication();

				// Check if all processor reached the async barrier
				if (NBX_prc_reached_bar_req[j])
				{MPI_SAFE_CALL(MPI_Test(&bar_req,&flag,&bar_stat))};

				// produce a report if communication get stuck
				log.NBXreport(NBX_cnt,req,NBX_prc_reached_bar_req[j],bar_stat);

			} while (flag == false);

			// Remove the executed request

			req.clear();
			stat.clear();
			log.clear();

			// Circular counter
			NBX_cnt = (NBX_cnt + 1) % nbx_cycle;
			NBX_active[j] = NBX_Type::NBX_UNACTIVE;

		}

		NBX_prc_qcnt = -1;
		return;
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
	template<typename T, typename Mem, template<typename> class gr> bool send(size_t proc, size_t tag, openfpm::vector<T,Mem,gr> & v)
	{
#ifdef SE_CLASS1
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
	 *          consider to use or sendrecvMultipleMessagesNBX
	 *
	 * \warning operation is asynchronous execute must be called to ensure they are executed
	 *
	 * \see sendrecvMultipleMessagesNBX
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
     *          consider to use sendrecvMultipleMessagesNBX
     *
     * \warning operation is asynchronous execute must be called to ensure they are executed
     *
     * \see sendrecvMultipleMessagesNBX
     *
     * \param proc processor id
     * \param tag id
     * \param v vector to send
     *
     * \return true if succeed false otherwise
     *
     */
    template<typename T, typename Mem, template<typename> class gr> bool recv(size_t proc, size_t tag, openfpm::vector<T,Mem,gr> & v)
    {
#ifdef SE_CLASS1
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
	template<typename T, typename Mem, template<typename> class gr> bool allGather(T & send, openfpm::vector<T,Mem,gr> & v)
	{
#ifdef SE_CLASS1
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

	/*! \brief Broadcast the data to all processors
	 *
	 * broadcast a vector of primitives.
	 *
	 * \warning operation is asynchronous execute must be called to ensure the operation is executed
	 *
	 * \warning the non-root processor must resize the vector to the exact receive size. This mean the
	 *          each processor must known a priory the receiving size
	 *
	 * \param v vector to send in the case of the root processor and vector where to receive in the case of
	 *          non-root
	 * \param root processor (who broadcast)
	 *
	 * \return true if succeed false otherwise
	 *
	 */
	template<typename T, typename Mem, template<typename> class layout_base >
	bool Bcast(openfpm::vector<T,Mem,layout_base> & v, size_t root)
	{
#ifdef SE_CLASS1
		checkType<T>();
#endif

		b_cast_helper<openfpm::vect_isel<T>::value == STD_VECTOR || is_layout_mlin<layout_base<T>>::value >::bcast_(req,v,root);

		return true;
	}

	/*! \brief Execute all the requests
	 *
	 */
	void execute()
	{
		// if req == 0 return
		if (req.size() == 0)
			return;

		// Wait for all the requests
		stat.resize(req.size());

		MPI_SAFE_CALL(MPI_Waitall(req.size(),&req.get(0),&stat.get(0)));

		// Remove executed request and status
		req.clear();
		stat.clear();
	}

	/*! \brief Release the buffer used for communication
	 *
	 *
	 */
	void clear()
	{
		for (size_t i = 0 ; i < NQUEUE ; i++)
		{recv_buf[i].clear();}
	}
};




#endif

