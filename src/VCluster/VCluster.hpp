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
#include "InVis.hpp"

#ifdef CUDA_GPU
extern CudaMemory mem_tmp;
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
	template<typename op, typename T, typename S, template <typename> class layout_base> void prepare_send_buffer(openfpm::vector<T> & send,
			                                                               S & recv,
																		   openfpm::vector<size_t> & prc_send,
																		   openfpm::vector<size_t> & prc_recv,
																		   openfpm::vector<size_t> & sz_recv,
																		   size_t opt)
	{
		openfpm::vector<size_t> sz_recv_byte(sz_recv.size());

		// Reset the receive buffer
		reset_recv_buf();

	#ifdef SE_CLASS1

		if (send.size() != prc_send.size())
			std::cerr << __FILE__ << ":" << __LINE__ << " Error, the number of processor involved \"prc.size()\" must match the number of sending buffers \"send.size()\" " << std::endl;

	#endif

		// Prepare the sending buffer
		openfpm::vector<const void *> send_buf;
		openfpm::vector<size_t> send_sz_byte;
		openfpm::vector<size_t> prc_send_;

		size_t tot_size = 0;

		for (size_t i = 0; i < send.size() ; i++)
		{
			size_t req = 0;

			//Pack requesting
			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op, T, S, layout_base>::packingRequest(send.get(i), req, send_sz_byte);
			tot_size += req;
		}

		pack_unpack_cond_with_prp_inte_lin<T>::construct_prc(prc_send,prc_send_);

		HeapMemory pmem;

		ExtPreAlloc<HeapMemory> & mem = *(new ExtPreAlloc<HeapMemory>(tot_size,pmem));
		mem.incRef();

		for (size_t i = 0; i < send.size() ; i++)
		{
			//Packing

			Pack_stat sts;

			pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value, op, T, S, layout_base>::packing(mem, send.get(i), sts, send_buf,opt);
		}

		// receive information
		base_info<InternalMemory> bi(&this->recv_buf,prc_recv,sz_recv_byte,this->tags,opt);

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
						sz_recv_byte.get(i) = sz_recv.get(i) * sizeof(typename T::value_type);
				}
				else
				{std::cout << __FILE__ << ":" << __LINE__ << " Error " << demangle(typeid(T).name()) << " the type does not work with the option or NO_CHANGE_ELEMENTS" << std::endl;}

				self_base::sendrecvMultipleMessagesNBX(prc_send.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send.getPointer(),(void **)send_buf.getPointer(),
											prc_recv.size(),(size_t *)prc_recv.getPointer(),(size_t *)sz_recv_byte.getPointer(),msg_alloc_known,(void *)&bi);
			}
			else
			{
				self_base::sendrecvMultipleMessagesNBX(prc_send.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send.getPointer(),(void **)send_buf.getPointer(),
											prc_recv.size(),(size_t *)prc_recv.getPointer(),msg_alloc_known,(void *)&bi);
				sz_recv_byte = self_base::sz_recv_tmp;
			}
		}
		else
		{
			self_base::tags.clear();
			prc_recv.clear();
			self_base::sendrecvMultipleMessagesNBX(prc_send_.size(),(size_t *)send_sz_byte.getPointer(),(size_t *)prc_send_.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi);
		}

		// Reorder the buffer
		reorder_buffer(prc_recv,self_base::tags,sz_recv_byte);

		mem.decRef();
		delete &mem;
	}


	/*! \brief Reset the receive buffer
	 *
	 *
	 */
	void reset_recv_buf()
	{
		for (size_t i = 0 ; i < self_base::recv_buf.size() ; i++)
		{self_base::recv_buf.get(i).resize(0);}

		self_base::recv_buf.resize(0);
	}

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
		openfpm::vector<size_t> & prc;
		//! size of each message
		openfpm::vector<size_t> & sz;
		//! tags
		openfpm::vector<size_t> &tags;

		//! options
		size_t opt;

		//! constructor
		base_info(openfpm::vector_fr<BMemory<Memory>> * recv_buf, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, openfpm::vector<size_t> & tags,size_t opt)
		:recv_buf(recv_buf),prc(prc),sz(sz),tags(tags),opt(opt)
		{}
	};

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
		rinfo.prc.add(i);
		rinfo.sz.add(msg_i);
		rinfo.tags.add(tag);

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
		{sz->resize(self_base::recv_buf.size());}

		pack_unpack_cond_with_prp<has_max_prop<T, has_value_type<T>::value>::value,op, T, S, layout_base, prp... >::unpacking(recv, self_base::recv_buf, sz, sz_byte, op_param,opt);
	}

	public:

	/*! \brief Constructor
	 *
	 * \param argc main number of arguments
	 * \param argv main set of arguments
	 *
	 */
	Vcluster(int *argc, char ***argv,MPI_Comm ext_comm = MPI_COMM_WORLD)
	:Vcluster_base<InternalMemory>(argc,argv,ext_comm)
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

			self_base::tags.clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf,prc,sz,this->tags,0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&bi);

			// we generate the list of the properties to unpack
			typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

			// operation object
			op_ssend_recv_add<void> opa;

			// Reorder the buffer
			reorder_buffer(prc,self_base::tags,sz);

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

			self_base::tags.clear();

			// receive information
			base_info<InternalMemory> bi(NULL,prc,sz,self_base::tags,0);

			// Send and recv multiple messages
			self_base::sendrecvMultipleMessagesNBX(send_prc_.size(),(size_t *)sz.getPointer(),(size_t *)send_prc_.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi,NONE);

			mem.decRef();
			delete &mem;
		}
		
		return true;
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
	template<typename T, typename S, template <typename> class layout_base=memory_traits_lin> bool SScatter(T & send, S & recv, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, size_t root)
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

			self_base::tags.clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf,prc,sz,this->tags,0);

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

			self_base::tags.clear();

			// receive information
			base_info<InternalMemory> bi(&this->recv_buf,prc,sz,this->tags,0);

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

		rcv.resize(self_base::recv_buf.size());

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
			recv_ord.get(i).swap(self_base::recv_buf.get(rcv.get(i).pos));
			prc_ord.get(i) = rcv.get(i).proc;
			sz_recv_ord.get(i) = sz_recv.get(rcv.get(i).pos);
		}

		// move rcv into recv
		// Now we swap back to recv_buf in an ordered way
		for (size_t i = 0 ; i < rcv.size() ; i++)
		{
			self_base::recv_buf.get(i).swap(recv_ord.get(i));
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

		// we generate the list of the properties to pack
		typedef typename ::generate_indexes<int, has_max_prop<T, has_value_type<T>::value>::number, MetaFuncOrd>::result ind_prop_to_pack;

		op_ssend_recv_add<void> opa;

		index_gen<ind_prop_to_pack>::template process_recv<op_ssend_recv_add<void>,T,S,layout_base>(*this,recv,&sz_recv,NULL,opa,opt);

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
	template<typename T, typename S, template <typename> class layout_base, int ... prp> bool SSendRecvP(openfpm::vector<T> & send,
			                                                      S & recv,
																  openfpm::vector<size_t> & prc_send,
																  openfpm::vector<size_t> & prc_recv,
																  openfpm::vector<size_t> & sz_recv,
																  openfpm::vector<size_t> & sz_recv_byte,
																  size_t opt = NONE)
	{
		prepare_send_buffer<op_ssend_recv_add<void>,T,S,layout_base>(send,recv,prc_send,prc_recv,sz_recv,opt);

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,&sz_recv_byte,opa,opt);

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

		// operation object
		op_ssend_recv_add<void> opa;

		// process the received information
		process_receive_buffer_with_prp<op_ssend_recv_add<void>,T,S,layout_base,prp...>(recv,&sz_recv,NULL,opa,opt);

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

		// process the received information
		process_receive_buffer_with_prp<op,T,S,layout_base,prp...>(recv,NULL,NULL,op_param,opt);

		return true;
	}

};

enum init_options
{
	none = 0x0,
	in_situ_visualization = 0x1,
};

extern init_options global_option;

// Function to initialize the global VCluster //

extern Vcluster<> * global_v_cluster_private_heap;
extern Vcluster<CudaMemory> * global_v_cluster_private_cuda;

static inline void delete_global_v_cluster_private()
{
        delete global_v_cluster_private_heap;
        delete global_v_cluster_private_cuda;
}


/*! \brief Finalize the library
 *
 * This function MUST be called at the end of the program
 *
 */
static inline void openfpm_finalize()
{
        if (global_option == init_options::in_situ_visualization)
        {
                MPI_Request bar_req;
                MPI_Ibarrier(MPI_COMM_WORLD,&bar_req);
        }

#ifdef HAVE_PETSC

        PetscFinalize();

#endif

        delete_global_v_cluster_private();
        ofp_initialized = false;

#ifdef CUDA_GPU

        // Release memory
        mem_tmp.destroy();
        mem_tmp.decRef();

#endif
}

static void get_comm_ranks(MPI_Comm comm, openfpm::vector<unsigned int> & world_ranks)
{
	MPI_Group grp, world_grp;

	MPI_Comm_group(MPI_COMM_WORLD, &world_grp);
	MPI_Comm_group(comm, &grp);

	int grp_size;

	MPI_Group_size(grp, &grp_size);

	openfpm::vector<unsigned int> local_ranks;

	local_ranks.resize(grp_size);
	world_ranks.resize(grp_size);

	for (int i = 0; i < grp_size; i++)
	{local_ranks.get(i) = i;}

	MPI_Group_translate_ranks(grp, grp_size, (int *)local_ranks.getPointer(), world_grp, (int *)world_ranks.getPointer());

	MPI_Group_free(&grp);
	MPI_Group_free(&world_grp);
}


/*! \brief Initialize a global instance of Runtime Virtual Cluster Machine
 *
 * Initialize a global instance of Runtime Virtual Cluster Machine
 *
 */

static inline void init_global_v_cluster_private(int *argc, char ***argv, init_options option)
{
	global_option = option;
	if (option == init_options::in_situ_visualization)
	{
		int flag;
		MPI_Initialized(&flag);

		if (flag == false)
		{
            int threadLevel;
            MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &threadLevel);
            std::cout << "MPI initialized with thread level " << threadLevel << ". The desired level was " << MPI_THREAD_MULTIPLE << std::endl;
		}

		MPI_Comm comm_compute;
		MPI_Comm comm_steer;
		MPI_Comm comm_vis;

		int rank;
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);

		MPI_Comm nodeComm;
		MPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank,
							 MPI_INFO_NULL, &nodeComm );

		openfpm::vector<unsigned int> world_ranks;
		get_comm_ranks(nodeComm,world_ranks);

		int nodeRank;
		int len;
		MPI_Comm_rank(nodeComm,&nodeRank);

		bool is_vis_process = false;

		if (rank != 0)
		{
			if (nodeRank == 0)
			{
			    // The lowest ranked process on a given node (except head node); the rendering process of that node
				char name[MPI_MAX_PROCESSOR_NAME];

				// Vis process
				MPI_Get_processor_name(name, &len);

				std::cout << "Node: " << name << " vis process: " << rank << std::endl;

				is_vis_process = true;
			}
			else
			{
				for (int i = 0 ; i < world_ranks.size() ; i++)
				{
					if (world_ranks.get(i) == 0 && (nodeRank == 1 || nodeRank == 2))
					{
						char name[MPI_MAX_PROCESSOR_NAME];

						// Vis process
						MPI_Get_processor_name(name, &len);

						std::cout << "Vis process on node 0 " << name << "   " << nodeRank << "  " << rank << std::endl;

						is_vis_process = true;
					}
				}
			}
		}

		int colorVis;
		int colorSteer;
		if (is_vis_process == true)
		{
		    //All visualization processes are part of the vis communicator, but not part of the steering communicator
		    colorVis = 0;
		    colorSteer = MPI_UNDEFINED;
        }
		else
		{
            //All non-visualization processes are part of the steering communicator, but not part of the vis communicator
            colorVis = MPI_UNDEFINED;
            colorSteer = 0;
        }

        MPI_Comm_split(MPI_COMM_WORLD, colorSteer, rank, &comm_steer);
        MPI_Comm_split(MPI_COMM_WORLD, colorVis, rank, &comm_vis);


		if (rank == 0 || is_vis_process == true)
		{MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED,rank, &comm_compute);}
		else
		{MPI_Comm_split(MPI_COMM_WORLD,0,rank, &comm_compute);}


		if (rank != 0 && is_vis_process == false)
		{
			if (global_v_cluster_private_heap == NULL)
			{global_v_cluster_private_heap = new Vcluster<>(argc,argv,comm_compute);}

                	if (global_v_cluster_private_cuda == NULL)
                	{global_v_cluster_private_cuda = new Vcluster<CudaMemory>(argc,argv,comm_compute);}
		}
		else if (is_vis_process == true)
		{
            int flag = false;
			MPI_Request bar_req;
			MPI_Ibarrier(MPI_COMM_WORLD,&bar_req);
			//! barrier status
			MPI_Status bar_stat;

			//How many simulation processes are running on this node?
			int numSimProcesses;
			int nodeCommSize;
			MPI_Comm_size(nodeComm, &nodeCommSize);

			if(nodeRank != 0)
            {
			    //This process' rank on its node is not 0. So it is on the head node
			    numSimProcesses = nodeCommSize - 3; //OpenFPM Head + Vis Head + Vis Renderer
            }
			else
            {
			    numSimProcesses = nodeCommSize - 1; //All process apart from Vis Renderer are simulation processes
            }
            char name[MPI_MAX_PROCESSOR_NAME];
            MPI_Get_processor_name(name, &len);

            std::cout<<"Node size is " << nodeCommSize << " and no. of sim processes on node " << name << " are: " << numSimProcesses << std::endl;

            while(flag == false)
			{

				int visRank;
                MPI_Comm_rank(comm_vis, &visRank);

                sleep(1);

                if(visRank == 0)
                {
                    // The head process of the visualization system
//                    sleep(10);
                    InVis *visSystem = new InVis(700, numSimProcesses, comm_vis, true);
                    visSystem->manageVisHead();
                }
                else
                {
                    // A rendering process of the the visualization system
//                    sleep(10);
                    InVis *visSystem = new InVis(700, numSimProcesses, comm_vis, false);
                    visSystem->manageVisRenderer();
                }

				MPI_SAFE_CALL(MPI_Test(&bar_req,&flag,&bar_stat));
			}

			openfpm_finalize();
			exit(0);
		}
		else
		{
			int flag = false;
			MPI_Request bar_req;
			MPI_Ibarrier(MPI_COMM_WORLD,&bar_req);
			//! barrier status
			MPI_Status bar_stat;

			while(flag == false)
			{
				std::cout << "I am node " << rank << std::endl;
				sleep(1);
				MPI_SAFE_CALL(MPI_Test(&bar_req,&flag,&bar_stat));
			}

			openfpm_finalize();
			exit(0);
		}
	}
	else
	{
        	if (global_v_cluster_private_heap == NULL)
        	{global_v_cluster_private_heap = new Vcluster<>(argc,argv);}

        	if (global_v_cluster_private_cuda == NULL)
        	{global_v_cluster_private_cuda = new Vcluster<CudaMemory>(argc,argv);}
	}
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
static inline void openfpm_init(int *argc, char ***argv, init_options option = init_options::none )
{
#ifdef HAVE_PETSC

	PetscInitialize(argc,argv,NULL,NULL);

#endif

	init_global_v_cluster_private(argc,argv,option);

#ifdef SE_CLASS1
	std::cout << "OpenFPM is compiled with debug mode LEVEL:1. Remember to remove SE_CLASS1 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS2
	std::cout << "OpenFPM is compiled with debug mode LEVEL:2. Remember to remove SE_CLASS2 when you go in production" << std::endl;
#endif

#ifdef SE_CLASS3
	std::cout << "OpenFPM is compiled with debug mode LEVEL:3. Remember to remove SE_CLASS3 when you go in production" << std::endl;
#endif

	// install segmentation fault signal handler

	struct sigaction sa;

	sa.sa_sigaction = bt_sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGSEGV, &sa, NULL);

	if (*argc != 0)
		program_name = std::string(*argv[0]);

	// Initialize math pre-computation tables
	openfpm::math::init_getFactorization();

	ofp_initialized = true;

#ifdef CUDA_GPU

	// Initialize temporal memory
	mem_tmp.incRef();

#endif
}




#endif

