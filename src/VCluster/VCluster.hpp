/*
 * Vcluster.hpp
 *
 *  Created on: Feb 8, 2016
 *      Author: Pietro Incardona
 */

#ifndef VCLUSTER_HPP
#define VCLUSTER_HPP

#include <signal.h>

#include "VCluster_base.hpp"
#include "VCluster_meta_function.hpp"
#include "util/math_util_complex.hpp"

#ifdef CUDA_GPU
extern CudaMemory mem_tmp;

#ifndef MAX_NUMER_OF_PROPERTIES
#define MAX_NUMER_OF_PROPERTIES 20
#endif

extern CudaMemory exp_tmp;
extern CudaMemory exp_tmp2[MAX_NUMER_OF_PROPERTIES];

extern CudaMemory rem_tmp;
extern CudaMemory rem_tmp2[MAX_NUMER_OF_PROPERTIES];

#endif

void bt_sighandler(int sig, siginfo_t * info, void * ctx);

/*! \brief Implementation of VCluster class
 *
 * This class implement communication functions. Like summation, minimum and maximum across
 * processors, or Dynamic Sparse Data Exchange (DSDE)
 *
 * ## Vcluster Min max sum
 * \snippet VCluster_unit_tests.hpp max min sum
 *
 * ## Vcluster all gather
 * \snippet VCluster_unit_test_util.hpp allGather numbers
 *
 * ## Dynamic sparse data exchange with complex objects
 * \snippet VCluster_semantic_unit_tests.hpp dsde with complex objects1
 *
 * ## Dynamic sparse data exchange with buffers
 * \snippet VCluster_unit_test_util.hpp dsde
 * \snippet VCluster_unit_test_util.hpp message alloc
 *
 */
template<typename InternalMemory = HeapMemory>
class Vcluster: public Vcluster_base<InternalMemory>
{
	// Internal memory
	ExtPreAlloc<HeapMemory> * mem[NQUEUE];

	// Buffer that store the received bytes
	openfpm::vector<size_t> sz_recv_byte[NQUEUE];

	// The sending buffer used by semantic calls
	openfpm::vector<const void *> send_buf;
	openfpm::vector<size_t> send_sz_byte;
	openfpm::vector<size_t> prc_send_;

	unsigned int NBX_prc_scnt = 0;
	unsigned int NBX_prc_pcnt = 0;

	///////////////////////

	// Internal Heap memory
	HeapMemory * pmem[NQUEUE];

	/*! \brief Base info
	 *
	 * \param recv_buf receive buffers
	 * \param prc processors involved
	 * \param size of the received data
	 *
	 */
	template<typename Memory>
	struct base_info
	{
		//! Receive buffer
		openfpm::vector_fr<BMemory<Memory>> * recv_buf;
		//! receiving processor list
		openfpm::vector<size_t> * prc;
		//! size of each message
		openfpm::vector<size_t> * sz;
		//! tags
		openfpm::vector<size_t> * tags;

		//! options
		size_t opt;

		//! default constructor
		base_info()
		{}

		//! constructor
		base_info(openfpm::vector_fr<BMemory<Memory>> * recv_buf, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, openfpm::vector<size_t> & tags,size_t opt)
		:recv_buf(recv_buf),prc(&prc),sz(&sz),tags(&tags),opt(opt)
		{}

		void set(openfpm::vector_fr<BMemory<Memory>> * recv_buf, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, openfpm::vector<size_t> & tags,size_t opt)
		{
			this->recv_buf = recv_buf;
			this->prc = &prc;
			this->sz = &sz;
			this->tags = &tags;
			this->opt = opt;
		}
	};

	// Internal temporaty buffer
	base_info<InternalMemory> NBX_prc_bi[NQUEUE];

	typedef Vcluster_base<InternalMemory> self_base;

	template<typename T>
	struct index_gen {};

	//! Process the receive buffer using the specified properties (meta-function)
	template<int ... prp>
	struct index_gen<index_tuple<prp...>>
	{
		//! Process the receive buffer
		template<typename op,
		         typename T,
				 typename S,
				 template <typename> class layout_base = memory_traits_lin>
		inline static void process_recv(Vcluster & vcl, S & recv, openfpm::vector<size_t> * sz_recv,
				                        openfpm::vector<size_t> * sz_recv_byte, op & op_param,size_t opt)
		{
			if (opt == MPI_GPU_DIRECT && !std::is_same<InternalMemory,CudaMemory>::value)
			{
				// In order to have this option activated InternalMemory must be  CudaMemory

				std::cout << __FILE__ << ":" << __LINE__ << " error: in order to have MPI_GPU_DIRECT VCluster must use CudaMemory internally, the most probable" <<
						                                    " cause of this problem is that you are using MPI_GPU_DIRECT option with a non-GPU data-structure" << std::endl;
			}

			vcl.process_receive_buffer_with_prp<op,T,S,layout_base,prp...>(recv,sz_recv,sz_recv_byte,op_param,opt);
		}
	};

	/*! \brief Prepare the send buffer and send the message to other processors
	 *
	 * \tparam op Operation to execute in merging the receiving data
	 * \tparam T sending object
	 * \tparam S receiving object
	 *
	 * \note T and S must not be the same object but a S.operation(T) must be defined. There the flexibility
	 * of the operation is defined by op
	 *
	 * \param send sending buffer
	 * \param recv receiving object
	 * \param prc_send each object T in the vector send is sent to one processor specified in this list.
	 *                 This mean that prc_send.size() == send.size()
	 * \param prc_recv list of processor from where we receive (output), in case of RECEIVE_KNOWN muts be filled
	 * \param sz_recv size of each receiving message (output), in case of RECEICE_KNOWN must be filled
	 * \param opt Options using RECEIVE_KNOWN enable patters with less latencies, in case of RECEIVE_KNOWN
	 *
	 */
	template<typename op, typename T, typename S, template <typename> class layout_base>
	void prepare_send_buffer(openfpm::vector<T> & send,
			                 S & recv,
							 openfpm::vector<size_t> & prc_send,
						     openfpm::vector<size_t> & prc_recv,
							 openfpm::vector<size_t> & sz_recv,
							 size_t opt)
	{
		sz_recv_byte[NBX_prc_scnt].resize(sz_recv.size());

		// Reset the receive buffer
		reset_recv_buf();

	#ifdef SE_CLASS1

		if (send.size() != prc_send.size())
			std::cerr << __FILE__ << ":" << __LINE__ << " Error, the number of processor involved \"prc.size()\" must match the number of sending buffers \"send.size()\" " << std::endl;

	#endif

		// Prepare the sending buffer
		send_buf.resize(0);
		send_sz_byte.resize(0);
		prc_send_.resize(0);

		size_t tot_size = 0;

		for (size_t i = 0; i < send.size() ; i++)
		{
			size_t req = 0;

			//Pack requesting
			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op, T, S, layout_base>::packingRequest(send.get(i), req, send_sz_byte);
			tot_size += req;
		}

		pack_unpack_cond_with_prp_inte_lin<T>::construct_prc(prc_send,prc_send_);

		//////// A question can raise on why we use HeapMemory instead of more generally InternalMemory for pmem
		////////
		//////// First we have consider that this HeapMemory is used to pack complex objects like a vector/container
		//////// of objects where the object contain pointers (is not a POD object).
		//////// In case the object is a POD pmem it is defined but never used. On a general base we can easily change
		//////// the code to use the general InternalMemory instead of HeapMemory, so that if we have a container defined
		//////// on cuda memory, we can serialize on Cuda directly. Unfortunately this concept crash on the requirement
		//////// that you need kernels/device code able to serialize containers of non POD object like a vector of vector
		//////// or more complex stuff. At the moment this is not the case, and probably unlikely to happen, most probably
		//////// code like this is CPU only. So it does not sound practical go beyond HeapMemory and impose container with
		//////// accelerated serializers for non-POD objects. (Relaxing the constrain saying in case
		//////// accelerated serializers for non-POD objects are not implemented create a stub that print error messages, still
		//////// does not sound very practical, at least not for now because of lack of cases)
		//////// Also to note that because pmem is used only in complex serialization, this
		//////// does not effect GPU RDMA in case of the containers of primitives with ready device pointer to send and when the
		//////// MPI_GPU_DIRECT option is used.
		////////
		//////// Another point to notice is that if we have kernels able to serialize containers of non-POD object
		//////// or complex containers on accelerator we can use the approach of grid_dist_id in which semantic Send and Receive
		//////// are not used. Serialization is operated inside the grid_dist_id structure, and the serialized buffer
		//////// are sent using low level sends primitives. Same concept for the de-serialization, and so this function is
		//////// not even called. grid_dist_id require some flexibility that the semantic send and receive are not able to give.
		////////
		//////// And so ... here is also another trade-off, at the moment there is not much will to potentially complicate
		//////// even more the semantic send and receive. They already have to handle a lot of cases. if you need more flexibility
		//////// go one step below use the serialization functions of the data-structures directly and use low level send
		//////// and receive to send these buffers. Semantic send and receive are not for you.
		////////
		////////

		pmem[NBX_prc_scnt] = new HeapMemory;

		mem[NBX_prc_scnt] = new ExtPreAlloc<HeapMemory>(tot_size,*pmem[NBX_prc_scnt]);
		mem[NBX_prc_scnt]->incRef();

		for (size_t i = 0; i < send.size() ; i++)
		{
			//Packing

			Pack_stat sts;

			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value, op, T, S, layout_base>::packing(*mem[NBX_prc_scnt], send.get(i), sts, send_buf,opt);
		}

		// receive information
		NBX_prc_bi[NBX_prc_scnt].set(&this->recv_buf[NBX_prc_scnt],prc_recv,sz_recv_byte[NBX_prc_scnt],this->tags[NBX_prc_scnt],opt);

		// Send and recv multiple messages
		if (opt & RECEIVE_KNOWN)
		{
			// We we are passing the number of element but not the byte, calculate the byte
			if (opt & KNOWN_ELEMENT_OR_BYTE)
			{
				// We know the number of element convert to byte (ONLY if it is possible)
				if (has_pack_gen<typename T::value_type>::value == false && is_vector<T>::value == true)
				{
					for (size_t i = 0 ; i < sz_recv.size() ; i++)
					{sz_recv_byte[NBX_prc_scnt].get(i) = sz_recv.get(i) * sizeof(typename T::value_type);}
				}
				else
				{
#ifndef DISABLE_ALL_RTTI
					std::cout << __FILE__ << ":" << __LINE__ << " Error " << demangle(typeid(T).name()) << " the type does not work with the option or NO_CHANGE_ELEMENTS" << std::endl;
#endif
				}

				self_base::sendrecvMultipleMessagesNBXAsync(prc_send.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send.getPointer(),(void **)send_buf.getPointer(),
											prc_recv.size(),(size_t *)prc_recv.getPointer(),(size_t *)sz_recv_byte[NBX_prc_scnt].getPointer(),msg_alloc_known,(void *)&NBX_prc_bi);
			}
			else
			{
				self_base::sendrecvMultipleMessagesNBXAsync(prc_send.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send.getPointer(),(void **)send_buf.getPointer(),
											prc_recv.size(),(size_t *)prc_recv.getPointer(),msg_alloc_known,(void *)&NBX_prc_bi);
				sz_recv_byte[NBX_prc_scnt] = self_base::sz_recv_tmp;
			}
		}
		else
		{
			self_base::tags[NBX_prc_scnt].clear();
			prc_recv.clear();
			self_base::sendrecvMultipleMessagesNBXAsync(prc_send_.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send_.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&NBX_prc_bi[NBX_prc_scnt]);
		}
	}


	/*! \brief Reset the receive buffer
	 *
	 *
	 */
	void reset_recv_buf()
	{
		for (size_t i = 0 ; i < self_base::recv_buf[NBX_prc_scnt].size() ; i++)
		{self_base::recv_buf[NBX_prc_scnt].get(i).resize(0);}

		self_base::recv_buf[NBX_prc_scnt].resize(0);
	}

	/*! \brief Call-back to allocate buffer to receive data
	 *
	 * \param msg_i size required to receive the message from i
	 * \param total_msg total size to receive from all the processors
	 * \param total_p the total number of processor that want to communicate with you
	 * \param i processor id
	 * \param ri request id (it is an id that goes from 0 to total_p, and is unique
	 *           every time message_alloc is called)
	 * \param ptr a pointer to the vector_dist structure
	 *
	 * \return the pointer where to store the message for the processor i
	 *
	 */
	static void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, size_t tag, void * ptr)
	{
		base_info<InternalMemory> & rinfo = *(base_info<InternalMemory> *)ptr;

		if (rinfo.recv_buf == NULL)
		{
			std::cerr << __FILE__ << ":" << __LINE__ << " Internal error this processor is not suppose to receive\n";
			return NULL;
		}

		rinfo.recv_buf->resize(ri+1);

		rinfo.recv_buf->get(ri).resize(msg_i);

		// Receive info
		rinfo.prc->add(i);
		rinfo.sz->add(msg_i);
		rinfo.tags->add(tag);

		// return the pointer

		// If we have GPU direct activated use directly the cuda buffer
		if (rinfo.opt & MPI_GPU_DIRECT)
		{
#if defined(MPIX_CUDA_AWARE_SUPPORT) && MPIX_CUDA_AWARE_SUPPORT
			return rinfo.recv_buf->last().getDevicePointer();
#else
			return rinfo.recv_buf->last().getPointer();
#endif
		}

		return rinfo.recv_buf->last().getPointer();
	}


	/*! \brief Call-back to allocate buffer to receive data
	 *
	 * \param msg_i size required to receive the message from i
	 * \param total_msg total size to receive from all the processors
	 * \param total_p the total number of processor that want to communicate with you
	 * \param i processor id
	 * \param ri request id (it is an id that goes from 0 to total_p, and is unique
	 *           every time message_alloc is called)
	 * \param ptr a pointer to the vector_dist structure
	 *
	 * \return the pointer where to store the message for the processor i
	 *
	 */
	static void * msg_alloc_known(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, size_t tag, void * ptr)
	{
		base_info<InternalMemory> & rinfo = *(base_info<InternalMemory> *)ptr;

		if (rinfo.recv_buf == NULL)
		{
			std::cerr << __FILE__ << ":" << __LINE__ << " Internal error this processor is not suppose to receive\n";
			return NULL;
		}

		rinfo.recv_buf->resize(ri+1);

		rinfo.recv_buf->get(ri).resize(msg_i);

		// return the pointer
		return rinfo.recv_buf->last().getPointer();
	}
	
	/*! \brief Process the receive buffer
	 *
	 * \tparam op operation to do in merging the received data
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties to receive
	 *
	 * \param recv receive object
	 * \param sz vector that store how many element has been added per processors on S
	 * \param sz_byte byte received on a per processor base
	 * \param op_param operation to do in merging the received information with recv
	 *
	 */
	template<typename op, typename T, typename S, template <typename> class layout_base ,unsigned int ... prp >
	void process_receive_buffer_with_prp(S & recv,
			                             openfpm::vector<size_t> * sz,
										 openfpm::vector<size_t> * sz_byte,
										 op & op_param,
										 size_t opt)
	{
		if (sz != NULL)
		{sz->resize(self_base::recv_buf[NBX_prc_pcnt].size());}

		pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op, T, S, layout_base, prp... >::unpacking(recv, self_base::recv_buf[NBX_prc_pcnt], sz, sz_byte, op_param,opt);
	}

	public:

	/*! \brief Constructor
	 *
	 * \param argc main number of arguments
	 * \param argv main set of arguments
	 *
	 */
	Vcluster(int *argc, char ***argv)
	:Vcluster_base<InternalMemory>(argc,argv)
	{
	}

	/*! \brief Semantic Gather, gather the data from all processors into one node
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * Gather(T,S,root,op=add);
	 *
	 * "Gather" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add(T).
	 *
	 * ### Example send a vector of structures, and merge all together in one vector
	 * \snippet VCluster_semantic_unit_tests.hpp Gather the data on master
	 *
	 * ### Example send a vector of structures, and merge all together in one vector
	 * \snippet VCluster_semantic_unit_tests.hpp Gather the data on master complex
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param root witch node should collect the information
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base=memory_traits_lin> bool SGather(T & send, S & recv,size_t root)
	{
		openfpm::vector<size_t> prc;
		openfpm::vector<size_t> sz;

		return SGather<T,S,layout_base>(send,recv,prc,sz,root);
	}

	//! metafunction
	template<size_t index, size_t N> struct MetaFuncOrd {
	   enum { value = index };
	};

	/*! \brief Semantic Gather, gather the data from all processors into one node
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * Gather(T,S,root,op=add);
	 *
	 * "Gather" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add(T).
	 *
	 * ### Example send a vector of structures, and merge all together in one vector
	 * \snippet VCluster_semantic_unit_tests.hpp Gather the data on master
	 *
	 * ### Example send a vector of structures, and merge all together in one vector
	 * \snippet VCluster_semantic_unit_tests.hpp Gather the data on master complex
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param root witch node should collect the information
	 * \param prc processors from witch we received the information
	 * \param sz size of the received information for each processor
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T,
	         typename S,
			 template <typename> class layout_base = memory_traits_lin>
	bool SGather(T & send,
			     S & recv,
				 openfpm::vector<size_t> & prc,
				 openfpm::vector<size_t> & sz,
				 size_t root)
	{
#ifdef SE_CLASS1
		if (&send == (T *)&recv)
		{std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " using SGather in general the sending object and the receiving object must be different" << std::endl;}
#endif

		// Reset the receive buffer
		reset_recv_buf();

		// If we are on master collect the information
		if (self_base::getProcessUnitID() == root)
		{
			// send buffer (master does not send anything) so send req and send_buf
			// remain buffer with size 0
			openfpm::vector<size_t> send_req;

			self_base::tags[NBX_prc_scnt].clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf[NBX_prc_scnt],prc,sz,this->tags[NBX_prc_scnt],0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&bi);

			// we generate the list of the properties to unpack
			typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

			// operation object
			op_ssend_recv_add<void> opa;

			// Reorder the buffer
			reorder_buffer(prc,self_base::tags[NBX_prc_scnt],sz);

			index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,&sz,NULL,opa,0);

			recv.add(send);
			prc.add(root);
			sz.add(send.size());
		}
		else
		{
			// send buffer (master does not send anything) so send req and send_buf
			// remain buffer with size 0
			openfpm::vector<size_t> send_prc;
			openfpm::vector<size_t> send_prc_;
			send_prc.add(root);

			openfpm::vector<size_t> sz;

			openfpm::vector<const void *> send_buf;
				
			//Pack requesting

			size_t tot_size = 0;

			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op_ssend_recv_add<void>, T, S, layout_base>::packingRequest(send, tot_size, sz);

			HeapMemory pmem;

			ExtPreAlloc<HeapMemory> & mem = *(new ExtPreAlloc<HeapMemory>(tot_size,pmem));
			mem.incRef();

			//Packing

			Pack_stat sts;
			
			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op_ssend_recv_add<void>, T, S, layout_base>::packing(mem, send, sts, send_buf);

			pack_unpack_cond_with_prp_inte_lin<T>::construct_prc(send_prc,send_prc_);

			self_base::tags[NBX_prc_scnt].clear();

			// receive information
			base_info<InternalMemory> bi(NULL,prc,sz,self_base::tags[NBX_prc_scnt],0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(send_prc_.size(),(size_t *)sz.getPointer(),(size_t *)send_prc_.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi,NONE);

			mem.decRef();
			delete &mem;
		}
		
		return true;
	}

	/*! \brief Just a call to mpi_barrier
	 *
	 *
	 */
	void barrier()
	{
		MPI_Barrier(MPI_COMM_WORLD);
	}

	/*! \brief Semantic Scatter, scatter the data from one processor to the other node
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * Scatter(T,S,...,op=add);
	 *
	 * "Scatter" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc processor involved in the scatter
	 * \param sz size of each chunks
	 * \param root which processor should scatter the information
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base=memory_traits_lin>
	bool SScatter(T & send, S & recv, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, size_t root)
	{
		// Reset the receive buffer
		reset_recv_buf();

		// If we are on master scatter the information
		if (self_base::getProcessUnitID() == root)
		{
			// Prepare the sending buffer
			openfpm::vector<const void *> send_buf;


			openfpm::vector<size_t> sz_byte;
			sz_byte.resize(sz.size());

			size_t ptr = 0;

			for (size_t i = 0; i < sz.size() ; i++)
			{
				send_buf.add((char *)send.getPointer() + sizeof(typename T::value_type)*ptr );
				sz_byte.get(i) = sz.get(i) * sizeof(typename T::value_type);
				ptr += sz.get(i);
			}

			self_base::tags[NBX_prc_scnt].clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf[NBX_prc_scnt],prc,sz,this->tags[NBX_prc_scnt],0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(prc.size(),(size_t *)sz_byte.getPointer(),(size_t *)prc.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi);

			// we generate the list of the properties to pack
			typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

			// operation object
			op_ssend_recv_add<void> opa;

			index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,NULL,NULL,opa,0);
		}
		else
		{
			// The non-root receive
			openfpm::vector<size_t> send_req;

			self_base::tags[NBX_prc_scnt].clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf[NBX_prc_scnt],prc,sz,this->tags[NBX_prc_scnt],0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&bi);

			// we generate the list of the properties to pack
			typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

			// operation object
			op_ssend_recv_add<void> opa;

			index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,NULL,NULL,opa,0);
		}

		return true;
	}
	
	/*! \brief reorder the receiving buffer
	 *
	 * \param prc list of the receiving processors
	 * \param sz_recv list of size of the receiving messages (in byte)
	 *
	 */
	void reorder_buffer(openfpm::vector<size_t> & prc, const openfpm::vector<size_t> & tags, openfpm::vector<size_t> & sz_recv)
	{

		struct recv_buff_reorder
		{
			//! processor
			size_t proc;

			size_t tag;

			//! position in the receive list
			size_t pos;

			//! default constructor
			recv_buff_reorder()
			:proc(0),tag(0),pos(0)
			{};

			//! needed to reorder
			bool operator<(const recv_buff_reorder & rd) const
			{
				if (proc == rd.proc)
				{return tag < rd.tag;}

				return (proc < rd.proc);
			}
		};

		openfpm::vector<recv_buff_reorder> rcv;

		rcv.resize(self_base::recv_buf[NBX_prc_pcnt].size());

		for (size_t i = 0 ; i < rcv.size() ; i++)
		{
			rcv.get(i).proc = prc.get(i);
			if (i < tags.size())
			{rcv.get(i).tag = tags.get(i);}
			else
			{rcv.get(i).tag = (unsigned int)-1;}
			rcv.get(i).pos = i;
		}

		// we sort based on processor
		rcv.sort();

		openfpm::vector_fr<BMemory<InternalMemory>> recv_ord;
		recv_ord.resize(rcv.size());

		openfpm::vector<size_t> prc_ord;
		prc_ord.resize(rcv.size());

		openfpm::vector<size_t> sz_recv_ord;
		sz_recv_ord.resize(rcv.size());

		// Now we reorder rcv
		for (size_t i = 0 ; i < rcv.size() ; i++)
		{
			recv_ord.get(i).swap(self_base::recv_buf[NBX_prc_pcnt].get(rcv.get(i).pos));
			prc_ord.get(i) = rcv.get(i).proc;
			sz_recv_ord.get(i) = sz_recv.get(rcv.get(i).pos);
		}

		// move rcv into recv
		// Now we swap back to recv_buf in an ordered way
		for (size_t i = 0 ; i < rcv.size() ; i++)
		{
			self_base::recv_buf[NBX_prc_pcnt].get(i).swap(recv_ord.get(i));
		}

		prc.swap(prc_ord);
		sz_recv.swap(sz_recv_ord);

		// reorder prc_recv and recv_sz
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * Recv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp dsde with complex objects1
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv list of the receiving processors
	 * \param sz_recv number of elements added
	 * \param opt options
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T,
	         typename S,
			 template <typename> class layout_base = memory_traits_lin>
	bool SSendRecv(openfpm::vector<T> & send,
			       S & recv,
				   openfpm::vector<size_t> & prc_send,
				   openfpm::vector<size_t> & prc_recv,
				   openfpm::vector<size_t> & sz_recv,
				   size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_scnt],sz_recv_byte[NBX_prc_scnt]);

		mem[NBX_prc_scnt]->decRef();
		delete mem[NBX_prc_scnt];
		delete pmem[NBX_prc_scnt];

		// we generate the list of the properties to pack
		typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

		op_ssend_recv_add<void> opa;

		index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,&sz_recv,NULL,opa,opt);

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors
	 *  asynchronous version
	 *
	 * \see progressCommunication to progress communications SSendRecvWait for synchronizing
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * Recv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp dsde with complex objects1
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv list of the receiving processors
	 * \param sz_recv number of elements added
	 * \param opt options
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T,
	         typename S,
			 template <typename> class layout_base = memory_traits_lin>
	bool SSendRecvAsync(openfpm::vector<T> & send,
			       S & recv,
				   openfpm::vector<size_t> & prc_send,
				   openfpm::vector<size_t> & prc_recv,
				   openfpm::vector<size_t> & sz_recv,
				   size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		NBX_prc_scnt++;

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors (with properties)
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv processors from which we received
	 * \param sz_recv number of elements added per processor
	 * \param sz_recv_byte message received from each processor in byte
	 *
	 * \return true if the function completed successful
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvP(openfpm::vector<T> & send,
			                                                      S & recv,
																  openfpm::vector<size_t> & prc_send,
																  openfpm::vector<size_t> & prc_recv,
																  openfpm::vector<size_t> & sz_recv,
																  openfpm::vector<size_t> & sz_recv_byte_out,
																  size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_scnt],sz_recv_byte[NBX_prc_scnt]);

		mem[NBX_prc_scnt]->decRef();
		delete mem[NBX_prc_scnt];
		delete pmem[NBX_prc_scnt];

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,&sz_recv_byte_out,opa,opt);

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors (with properties)
	 *         asynchronous version
	 *
	 * \see progressCommunication to progress communications SSendRecvWait for synchronizing
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv processors from which we received
	 * \param sz_recv number of elements added per processor
	 * \param sz_recv_byte message received from each processor in byte
	 *
	 * \return true if the function completed successful
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvPAsync(openfpm::vector<T> & send,
			                                                      S & recv,
																  openfpm::vector<size_t> & prc_send,
																  openfpm::vector<size_t> & prc_recv,
																  openfpm::vector<size_t> & sz_recv,
																  openfpm::vector<size_t> & sz_recv_byte_out,
																  size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		NBX_prc_scnt++;

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors (with properties)
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv list of the processors from which we receive
	 * \param sz_recv number of elements added per processors
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvP(openfpm::vector<T> & send,
			        S & recv,
					openfpm::vector<size_t> & prc_send,
			    	openfpm::vector<size_t> & prc_recv,
					openfpm::vector<size_t> & sz_recv,
					size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_scnt],sz_recv_byte[NBX_prc_scnt]);

		mem[NBX_prc_scnt]->decRef();
		delete mem[NBX_prc_scnt];
		delete pmem[NBX_prc_scnt];

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,NULL,opa,opt);

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors (with properties)
	 *         asynchronous version
	 *
	 * \see progressCommunication to progress communications SSendRecvWait for synchronizing
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param prc_recv list of the processors from which we receive
	 * \param sz_recv number of elements added per processors
	 *
	 * \return true if the function completed succefully
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvPAsync(openfpm::vector<T> & send,
			        S & recv,
					openfpm::vector<size_t> & prc_send,
			    	openfpm::vector<size_t> & prc_recv,
					openfpm::vector<size_t> & sz_recv,
					size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		NBX_prc_scnt++;

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam op type of operation
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param op_param operation object (operation to do im merging the information)
	 * \param recv_sz size of each receiving buffer. This parameters are output
	 *        with RECEIVE_KNOWN you must feed this parameter
	 * \param prc_recv from which processor we receive messages
	 *        with RECEIVE_KNOWN you must feed this parameter
	 * \param opt options default is NONE, another is RECEIVE_KNOWN. In this case each
	 *        processor is assumed to know from which processor receive, and the size of
	 *        the message. in such case prc_recv and sz_recv are not anymore parameters
	 *        but must be input.
	 *
	 *
	 * \return true if the function completed successful
	 *
	 */
	template<typename op,
	         typename T,
			 typename S,
			 template <typename> class layout_base,
			 int ... prp>
	bool SSendRecvP_op(openfpm::vector<T> & send,
			           S & recv,
					   openfpm::vector<size_t> & prc_send,
					   op & op_param,
					   openfpm::vector<size_t> & prc_recv,
					   openfpm::vector<size_t> & recv_sz,
				 	   size_t opt = NONE)
	{
		prepare_send_buffer<op,T,S,layout_base>(send,recv,prc_send,prc_recv,recv_sz,opt);

		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_scnt],sz_recv_byte[NBX_prc_scnt]);

		mem[NBX_prc_scnt]->decRef();
		delete mem[NBX_prc_scnt];
		delete pmem[NBX_prc_scnt];

		// process the received information
		process_receive_buffer_with_prp<op,T,S,layout_base,prp...>(recv,NULL,NULL,op_param,opt);

		return true;
	}

	/*! \brief Semantic Send and receive, send the data to processors and receive from the other processors asynchronous version
	 *
	 * \see progressCommunication to incrementally progress the communication  SSendRecvP_opWait to synchronize
	 *
	 * Semantic communication differ from the normal one. They in general
	 * follow the following model.
	 *
	 * SSendRecv(T,S,...,op=add);
	 *
	 * "SendRecv" indicate the communication pattern, or how the information flow
	 * T is the object to send, S is the object that will receive the data.
	 * In order to work S must implement the interface S.add<prp...>(T).
	 *
	 * ### Example scatter a vector of structures, to other processors
	 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
	 *
	 * \tparam op type of operation
	 * \tparam T type of sending object
	 * \tparam S type of receiving object
	 * \tparam prp properties for merging
	 *
	 * \param send Object to send
	 * \param recv Object to receive
	 * \param prc_send destination processors
	 * \param op_param operation object (operation to do im merging the information)
	 * \param recv_sz size of each receiving buffer. This parameters are output
	 *        with RECEIVE_KNOWN you must feed this parameter
	 * \param prc_recv from which processor we receive messages
	 *        with RECEIVE_KNOWN you must feed this parameter
	 * \param opt options default is NONE, another is RECEIVE_KNOWN. In this case each
	 *        processor is assumed to know from which processor receive, and the size of
	 *        the message. in such case prc_recv and sz_recv are not anymore parameters
	 *        but must be input.
	 *
	 *
	 * \return true if the function completed successful
	 *
	 */
	template<typename op,
	         typename T,
			 typename S,
			 template <typename> class layout_base,
			 int ... prp>
	bool SSendRecvP_opAsync(openfpm::vector<T> & send,
			           S & recv,
					   openfpm::vector<size_t> & prc_send,
					   op & op_param,
					   openfpm::vector<size_t> & prc_recv,
					   openfpm::vector<size_t> & recv_sz,
				 	   size_t opt = NONE)
	{
		prepare_send_buffer<op,T,S,layout_base>(send,recv,prc_send,prc_recv,recv_sz,opt);

		NBX_prc_scnt++;

		return true;
	}

	/*! \brief Synchronize with SSendRecv
	 *
	 * \note arguments are discussed in SSendRecvAsync
	 *
	 */
	template<typename T,
	         typename S,
			 template <typename> class layout_base = memory_traits_lin>
	bool SSendRecvWait(openfpm::vector<T> & send,
		       S & recv,
			   openfpm::vector<size_t> & prc_send,
			   openfpm::vector<size_t> & prc_recv,
			   openfpm::vector<size_t> & sz_recv,
			   size_t opt = NONE)
	{
		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_pcnt],sz_recv_byte[NBX_prc_pcnt]);

		mem[NBX_prc_pcnt]->decRef();
		delete mem[NBX_prc_pcnt];
		delete pmem[NBX_prc_pcnt];

		// we generate the list of the properties to pack
		typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

		op_ssend_recv_add<void> opa;

		index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,&sz_recv,NULL,opa,opt);

		NBX_prc_pcnt++;
		if (NBX_prc_scnt == NBX_prc_pcnt)
		{
			NBX_prc_scnt = 0;
			NBX_prc_pcnt = 0;
		}

		return true;
	}

	/*! \brief Synchronize with SSendRecvP
	 *
	 * \note arguments are discussed in SSendRecvPAsync
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvPWait(openfpm::vector<T> & send,
			                                                      S & recv,
																  openfpm::vector<size_t> & prc_send,
																  openfpm::vector<size_t> & prc_recv,
																  openfpm::vector<size_t> & sz_recv,
																  openfpm::vector<size_t> & sz_recv_byte_out,
																  size_t opt = NONE)
	{
		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_pcnt],sz_recv_byte[NBX_prc_pcnt]);

		mem[NBX_prc_pcnt]->decRef();
		delete mem[NBX_prc_pcnt];
		delete pmem[NBX_prc_pcnt];

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,&sz_recv_byte_out,opa,opt);

		NBX_prc_pcnt++;
		if (NBX_prc_scnt == NBX_prc_pcnt)
		{
			NBX_prc_scnt = 0;
			NBX_prc_pcnt = 0;
		}

		return true;
	}

	/*! \brief Synchronize with SSendRecvP
	 *
	 * \note arguments are discussed in SSendRecvPAsync
	 *
	 */
	template<typename T, typename S, template <typename> class layout_base, int ... prp>
	bool SSendRecvPWait(openfpm::vector<T> & send,
			        S & recv,
					openfpm::vector<size_t> & prc_send,
			    	openfpm::vector<size_t> & prc_recv,
					openfpm::vector<size_t> & sz_recv,
					size_t opt = NONE)
	{
		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_pcnt],sz_recv_byte[NBX_prc_pcnt]);

		mem[NBX_prc_pcnt]->decRef();
		delete mem[NBX_prc_pcnt];
		delete pmem[NBX_prc_pcnt];

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,NULL,opa,opt);

		NBX_prc_pcnt++;
		if (NBX_prc_scnt == NBX_prc_pcnt)
		{
			NBX_prc_scnt = 0;
			NBX_prc_pcnt = 0;
		}

		return true;
	}

	/*! \brief Synchronize with SSendRecvP_op
	 *
	 * \note arguments are discussed in SSendRecvP_opAsync
	 *
	 */
	template<typename op,
	         typename T,
			 typename S,
			 template <typename> class layout_base,
			 int ... prp>
	bool SSendRecvP_opWait(openfpm::vector<T> & send,
			           S & recv,
					   openfpm::vector<size_t> & prc_send,
					   op & op_param,
					   openfpm::vector<size_t> & prc_recv,
					   openfpm::vector<size_t> & recv_sz,
				 	   size_t opt = NONE)
	{
		self_base::sendrecvMultipleMessagesNBXWait();

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags[NBX_prc_pcnt],sz_recv_byte[NBX_prc_pcnt]);

		mem[NBX_prc_pcnt]->decRef();
		delete mem[NBX_prc_pcnt];
		delete pmem[NBX_prc_pcnt];

		// process the received information
		process_receive_buffer_with_prp<op,T,S,layout_base,prp...>(recv,NULL,NULL,op_param,opt);

		NBX_prc_pcnt++;
		if (NBX_prc_scnt == NBX_prc_pcnt)
		{
			NBX_prc_scnt = 0;
			NBX_prc_pcnt = 0;
		}

		return true;
	}

};



// Function to initialize the global VCluster //

extern Vcluster<> * global_v_cluster_private_heap;
extern Vcluster<CudaMemory> * global_v_cluster_private_cuda;

/*! \brief Initialize a global instance of Runtime Virtual Cluster Machine
 *
 * Initialize a global instance of Runtime Virtual Cluster Machine
 *
 */

static inline void init_global_v_cluster_private(int *argc, char ***argv)
{
	if (global_v_cluster_private_heap == NULL)
	{global_v_cluster_private_heap = new Vcluster<>(argc,argv);}

	if (global_v_cluster_private_cuda == NULL)
	{global_v_cluster_private_cuda = new Vcluster<CudaMemory>(argc,argv);}
}

static inline void delete_global_v_cluster_private()
{
	delete global_v_cluster_private_heap;
	delete global_v_cluster_private_cuda;
}

template<typename Memory>
struct get_vcl
{
	static Vcluster<Memory> & get()
	{
		return *global_v_cluster_private_heap;
	}
};

template<>
struct get_vcl<CudaMemory>
{
	static Vcluster<CudaMemory> & get()
	{
		return *global_v_cluster_private_cuda;
	}
};

template<typename Memory = HeapMemory>
static inline Vcluster<Memory> & create_vcluster()
{
	if (global_v_cluster_private_heap == NULL)
	{std::cerr << __FILE__ << ":" << __LINE__ << " Error you must call openfpm_init before using any distributed data structures";}

	return get_vcl<Memory>::get();
}



/*! \brief Check if the library has been initialized
 *
 * \return true if the library has been initialized
 *
 */
static inline bool is_openfpm_init()
{
	return ofp_initialized;
}


/*! \brief Initialize the library
 *
 * This function MUST be called before any other function
 *
 */
void openfpm_init(int *argc, char ***argv);


/*! \brief Finalize the library
 *
 * This function MUST be called at the end of the program
 *
 */
void openfpm_finalize();


#endif

