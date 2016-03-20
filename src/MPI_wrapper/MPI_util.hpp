/*
 * MPI_util.hpp
 *
 *  Created on: Jul 7, 2015
 *      Author: Pietro Incardona
 */

#ifndef MPI_UTIL_HPP_
#define MPI_UTIL_HPP_

#include <iostream>

/*! \brief From an MPI error it print an human readable message
 *
 * \param error_code
 *
 */
static void error_handler(int error_code)
{
   int rank;
   char error_string[BUFSIZ];
   int length_of_error_string, error_class;

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   MPI_Error_class(error_code, &error_class);
   MPI_Error_string(error_class, error_string, &length_of_error_string);
   std::cerr << rank << ": " << error_string;
   MPI_Error_string(error_code, error_string, &length_of_error_string);
   std::cerr << rank << ": " << error_string;
}

#define MPI_SAFE_CALL(call) {\
	int err = call;\
	if (MPI_SUCCESS != err) {\
		std::cerr << "MPI error: "<< __FILE__ << " " << __LINE__ << "\n";\
		error_handler(err);\
		MPI_Abort(MPI_COMM_WORLD,-1);\
	}\
}


#endif /* MPI_UTIL_HPP_ */
