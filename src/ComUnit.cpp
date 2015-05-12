#include "ComUnit.hpp"

#define SERVICE_TAG	0xFFFFFFF

/*! \brief Send some data globally to one processor when the other side
 *         do not know about the other side
 *
 * Send some data globally to one processor when the other side
 * do not know about the other side
 *
 * \Warning if you already call this function with p, will overwrite the request
 *
 * \param p is the processor number
 * \param buf is the buffer pointer
 * \param sz is the size of the communication
 *
 */

bool SentToU(size_t p, void * buf,size_t sz)
{
	// before complete the communication we have to notify to the other
	// processor that we have some data to send.

	if (p >= comReq.size())
	{
		std::cerr << "Error: file: " << __FILE__ << " line: " << __LINE__ << " processor " << p << " does not exist";
		return false;
	}

	return true;
}

/*! \brief Send some data locally (to its neighborhood) to one processor
 *
 * Send some data locally to one processor
 *
 */

bool SendToNU(void * buf, size_t sz)
{
	return true;
}

/*! \brief Send some data globally to one processor when the other side
 *         know about the other side
 *
 * Send some data globally to one processor when the other side
 * know about the other side
 *
 * \Warning if you already call this function with p, will overwrite the request
 *
 * \param p is the processor number
 * \param buf is the buffer pointer
 * \param sz is the size of the communication
 *
 */

bool SendTo(size_t p, void * buf, size_t sz)
{
	MPI_ISend(p,buf,sz);
}

/*! \brief Wait for all communication to complete
 *
 * Wait for all communication to complete
 *
 * \return true if no error occur
 *
 */

bool wait()
{
	// Here we have to type of communication to handle

	// Type 1 One side does not know that the other side want to communcate
	// Type 2 The other side know that want to communicate

	// The reqs structure handle the communication of type 2
	// comReq and comSize store request of type 2

	// For the type 1 we have to reduce scatter the comReq this
	// for each processor K return the number of processors that need
	// to communicate with K

	MPI_ireduce_scatter();

	// For the type 2 we already have recv coupled to send so just wait to complete

	//! wait for all request to complete
	MPI_Waitall(reqs.size(),&reqs[0],&status[0]);

	//! For the number of incoming request queue an MPI_IRecv with MPI_ANY_SOURCE
	//! It is going to receive the length of the message that each processor need
	//! communicate

	for (int i = 0 ; i < 5; i++)
	{

	}

	//! For the number of outcomming request queue MPI_ISend sending the length of
	//! the message the processor need to comunicate

	for (int i = 0 ; i < 5; i++)
	{

	}

	//! wait for all request to complete
	MPI_Waitall(reqs.size(),&reqs[0],&status[0]);

	//! finally send and receive the data

	for (int i = 0 ; i < 5; i++)
	{

	}

	for (int i = 0 ; i < 5; i++)
	{

	}

	//! wait for all request to complete
	MPI_Waitall(reqs.size(),&reqs[0],&status[0]);

	return true;
}
