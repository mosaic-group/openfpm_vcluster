/*
 * VCluster_semantic.hpp
 *
 * Implementation of semantic communications
 *
 *  Created on: Feb 8, 2016
 *      Author: Pietro Incardona
 */

/*! \brief Reset the receive buffer
 * 
 * 
 */
void reset_recv_buf()
{
	for (size_t i = 0 ; i < recv_buf.size() ; i++)
		recv_buf.get(i).resize(0);
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
static void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, void * ptr)
{
	openfpm::vector<HeapMemory> * recv_buf = (openfpm::vector<HeapMemory> *)ptr;

	if (recv_buf == NULL)
		std::cerr << __FILE__ << ":" << __LINE__ << " Internal error this processor is not suppose to receive\n";

	recv_buf->resize(ri+1);

	recv_buf->get(ri).resize(msg_i);

	// return the pointer
	return recv_buf->last().getPointer();
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
 * \param Object to send
 * \param Object to receive
 * \param root witch node should collect the information
 *
 * \return true if the function completed succefully
 *
 */
template<typename T, typename S> bool SGather(T & send, S & recv,size_t root)
{
	// Reset the receive buffer
	reset_recv_buf();
	
	// If we are on master collect the information
	if (getProcessUnitID() == root)
	{
		// send buffer (master does not send anything) so send req and send_buf
		// remain buffer with size 0
		openfpm::vector<size_t> send_req;

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(send_req.size(),NULL,NULL,NULL,msg_alloc,&recv_buf);

		for (size_t i = 0 ; i < recv_buf.size() ; i++)
		{
			// for each received buffer create a memory reppresentation
			// calculate the number of received elements
			size_t n_ele = recv_buf.get(i).size() / sizeof(typename T::value_type);
	
			// add the received particles to the vector
			PtrMemory * ptr1 = new PtrMemory(recv_buf.get(i).getPointer(),recv_buf.get(i).size());
			
			// create vector representation to a piece of memory already allocated
			openfpm::vector<typename T::value_type,PtrMemory,openfpm::grow_policy_identity> v2;
	
			v2.setMemory(*ptr1);
	
			// resize with the number of elements
			v2.resize(n_ele);
	
			// Merge the information
			recv.add(v2);
		}
		
		recv.add(send);
	}
	else
	{
		// send buffer (master does not send anything) so send req and send_buf
		// remain buffer with size 0
		openfpm::vector<size_t> send_prc;
		send_prc.add(root);
		openfpm::vector<void *> send_buf;
		send_buf.add(send.getPointer());
		openfpm::vector<size_t> sz;
		sz.add(send.size()*sizeof(typename T::value_type));

		// Send and recv multiple messages
		sendrecvMultipleMessagesNBX(send_prc.size(),(size_t *)sz.getPointer(),(size_t *)send_prc.getPointer(),(void **)send_buf.getPointer(),msg_alloc,NULL);
	}
	
	return true;
}
