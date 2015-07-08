#ifndef COM_UNIT_HPP
#define COM_UNIT_HPP

#include <mpi.h>
#include <vector>

/*! \brief This is the abstraction of the communication
 *          unit for the virtual cluster
 *
 * This with the abstraction of the communication
 *          unit of the virtual cluster
 *
 * When this unit is returned back, you must ensure that no other thread
 * is using MPI call
 *
 */

class ComUnit
{
	// if this processor need to communicate with the processor K
	// it put 1 at the position K
	std::vector<unsigned int> comReq;

	// if this processor need to communicate with the processor K
	// a message to length m put m at position K
	std::vector<size_t> sizeReq;

	// List of all status request
    std::vector<MPI_Request> reqs;

    // List of the status of all the request
    std::vector<MPI_Status> stat;

	//! Send data
	bool SentTo();

	//! Send data to the neighborhood
	bool SendToN();

	//! wait for communication to complete
	bool wait();
};

#endif
