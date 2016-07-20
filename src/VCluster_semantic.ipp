/*
 * VCluster_semantic.hpp
 *
 * Implementation of semantic communications
 *
 *  Created on: Feb 8, 2016
 *      Author: Pietro Incardona
 */

private:

/*! \brief Reset the receive buffer
 * 
 * 
 */
void reset_recv_buf()
{
	for (size_t i = 0 ; i < recv_buf.size() ; i++)
		recv_buf.get(i).resize(0);

	recv_buf.resize(0);
}

/*! \brief Base info
 *
 * \param recv_buf receive buffers
 * \param prc processors involved
 * \param size of the received data
 *
 */
struct base_info
{
	openfpm::vector<BHeapMemory> * recv_buf;
	openfpm::vector<size_t> & prc;
	openfpm::vector<size_t> & sz;

	// constructor
	base_info(openfpm::vector<BHeapMemory> * recv_buf, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz)
	:recv_buf(recv_buf),prc(prc),sz(sz)
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
static void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, void * ptr)
{
	base_info & rinfo = *(base_info *)ptr;

	if (rinfo.recv_buf == NULL)
		std::cerr << __FILE__ << ":" << __LINE__ << " Internal error this processor is not suppose to receive\n";

	rinfo.recv_buf->resize(ri+1);

	rinfo.recv_buf->get(ri).resize(msg_i);

	// Receive info
	rinfo.prc.add(i);
	rinfo.sz.add(msg_i);

	// return the pointer
	return rinfo.recv_buf->last().getPointer();
}

/*! \brief Process the receive buffer
 *
 * \tparam T type of sending object
 * \tparam S type of receiving object
 *
 * \param recv receive object
 *
 */
template<typename T, typename S> void process_receive_buffer(S & recv, openfpm::vector<size_t> * sz = NULL)
{
	if (sz != NULL)
		sz->resize(recv_buf.size());

	for (size_t i = 0 ; i < recv_buf.size() ; i++)
	{
		// for each received buffer create a memory reppresentation
		// calculate the number of received elements
		size_t n_ele = recv_buf.get(i).size() / sizeof(typename T::value_type);

		// add the received particles to the vector
		PtrMemory * ptr1 = new PtrMemory(recv_buf.get(i).getPointer(),recv_buf.get(i).size());

		// create vector representation to a piece of memory already allocated
		openfpm::vector<typename T::value_type,PtrMemory,typename memory_traits_lin<typename T::value_type>::type, memory_traits_lin,openfpm::grow_policy_identity> v2;

		v2.setMemory(*ptr1);

		// resize with the number of elements
		v2.resize(n_ele);

		// Merge the information
		recv.add(v2);

		if (sz != NULL)
			sz->get(i) = v2.size();
	}
}

public:

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
 * \param Object to send
 * \param Object to receive
 * \param root witch node should collect the information
 *
 * \return true if the function completed succefully
 *
 */
template<typename T, typename S> bool SGather(T & send, S & recv,size_t root)
{
	openfpm::vector<size_t> prc;
	openfpm::vector<size_t> sz;

	return SGather(send,recv,prc,sz,root);
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
 * \param Object to send
 * \param Object to receive
 * \param root witch node should collect the information
 * \param prc processors from witch we received the information
 * \param sz size of the received information for each processor
 *
 * \return true if the function completed succefully
 *
 */
template<typename T, typename S> bool SGather(T & send, S & recv, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz,size_t root)
{
	// Reset the receive buffer
	reset_recv_buf();
	
	// If we are on master collect the information
	if (getProcessUnitID() == root)
	{
		// send buffer (master does not send anything) so send req and send_buf
		// remain buffer with size 0
		openfpm::vector<size_t> send_req;

		// receive information
		base_info bi(&recv_buf,prc,sz);

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&bi);

		// Convert the received byte into number of elements
		for (size_t i = 0 ; i < sz.size() ; i++)
			sz.get(i) /= sizeof(typename T::value_type);

		// process the received information
		process_receive_buffer<T,S>(recv,&sz);

		recv.add(send);
		prc.add(root);
		sz.add(send.size());
	}
	else
	{
		// send buffer (master does not send anything) so send req and send_buf
		// remain buffer with size 0
		openfpm::vector<size_t> send_prc;
		send_prc.add(root);
		openfpm::vector<const void *> send_buf;
		send_buf.add(send.getPointer());
		openfpm::vector<size_t> sz;
		sz.add(send.size()*sizeof(typename T::value_type));

		// receive information
		base_info bi(NULL,prc,sz);

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(send_prc.size(),(size_t *)sz.getPointer(),(size_t *)send_prc.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi);
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
 * \param Object to send
 * \param Object to receive
 * \param prc processor involved in the scatter
 * \param sz size of each chunks
 * \param root which processor should scatter the information
 *
 * \return true if the function completed succefully
 *
 */
template<typename T, typename S> bool SScatter(T & send, S & recv, openfpm::vector<size_t> & prc, openfpm::vector<size_t> & sz, size_t root)
{
	// Reset the receive buffer
	reset_recv_buf();

	// If we are on master scatter the information
	if (getProcessUnitID() == root)
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

		// receive information
		base_info bi(&recv_buf,prc,sz);

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(prc.size(),(size_t *)sz_byte.getPointer(),(size_t *)prc.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi);

		// process the received information
		process_receive_buffer<T,S>(recv,NULL);
	}
	else
	{
		// The non-root receive
		openfpm::vector<size_t> send_req;

		// receive information
		base_info bi(&recv_buf,prc,sz);

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&bi);

		process_receive_buffer<T,S>(recv,NULL);
	}

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
 * In order to work S must implement the interface S.add(T).
 *
 * ### Example scatter a vector of structures, to other processors
 * \snippet VCluster_semantic_unit_tests.hpp Scatter the data from master
 *
 * \tparam T type of sending object
 * \tparam S type of receiving object
 *
 * \param Object to send
 * \param Object to receive
 * \param prc processor involved in the scatter
 * \param sz size of each chunks
 * \param root which processor should scatter the information
 *
 * \return true if the function completed succefully
 *
 */
template<typename T, typename S> bool SSendRecv(openfpm::vector<T> & send, S & recv, openfpm::vector<size_t> & prc_send, openfpm::vector<size_t> & prc_recv, openfpm::vector<size_t> & sz_recv)
{
	// Reset the receive buffer
	reset_recv_buf();

#ifdef SE_CLASS1

	if (send.size() != prc_send.size())
		std::cerr << __FILE__ << ":" << __LINE__ << " Error, the number of processor involved \"prc.size()\" must match the number of sending buffers \"send.size()\" " << std::endl;

#endif

	// Prepare the sending buffer
	openfpm::vector<const void *> send_buf;

	openfpm::vector<size_t> sz_byte;
	sz_byte.resize(send.size());

	for (size_t i = 0; i < send.size() ; i++)
	{
		send_buf.add((char *)send.get(i).getPointer());
		sz_byte.get(i) = send.get(i).size() * sizeof(typename T::value_type);
	}

	// receive information
	base_info bi(&recv_buf,prc_recv,sz_recv);

	// Send and recv multiple messages
	sendrecvMultipleMessagesNBX(prc_send.size(),(size_t *)sz_byte.getPointer(),(size_t *)prc_send.getPointer(),(void **)send_buf.getPointer(),msg_alloc,(void *)&bi);

	// process the received information
	process_receive_buffer<T,S>(recv,&sz_recv);

	return true;
}
