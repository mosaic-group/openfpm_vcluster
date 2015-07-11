/*
 * Vcluster_log.hpp
 *
 *  Created on: Jul 11, 2015
 *      Author: Pietro Incardona
 */

#ifndef VCLUSTER_LOG_HPP_
#define VCLUSTER_LOG_HPP_

#include <fstream>
#include "timer.hpp"

#ifdef VERBOSE_TEST

/*! \brief Vcluster log
 *
 * It basically produce a report of the communication status
 *
 */
class Vcluster_log
{
	timer t;

	// delay of the log
	size_t log_delay;

	size_t rank;

	// Receive status vector
	openfpm::vector<MPI_Status> r_log;

	// Send processors vector
	openfpm::vector<size_t> s_log;

	// log file
	std::ofstream f;

public:

	/*! \brief Start to count the seconds
	 *
	 * The log report is generated only after "log_delay" seconds
	 *
	 * \param log_delay
	 *
	 */
	void start(size_t log_delay)
	{
		this->log_delay = log_delay;
		t.start();
	}

	/*! \brief Create the log file
	 *
	 * \param rank processor id
	 *
	 */
	void openLog(size_t rank)
	{
		int result;
		char p_name[MPI_MAX_PROCESSOR_NAME];
		MPI_Get_processor_name(p_name, &result );

		std::stringstream str;
		str << "vcluster_log_" << p_name << "_" << rank;
		f.open(str.str());
	}

	/*! \brief Allocate and MPI_Status
	 *
	 * \return a valid MPI_Status
	 *
	 */
	void logRecv(MPI_Status & stat)
	{
		r_log.add(stat);
	}

	/*! \brief
	 *
	 * \param prc processor
	 *
	 */
	void logSend(size_t prc)
	{
		s_log.add(prc);
	}

	/*! \brief This function write a report for the NBX communication strategy
	 *
	 * \param nbx
	 * \param send request
	 */
	void NBXreport(size_t nbx, openfpm::vector<MPI_Request> & req, bool reach_b, MPI_Status bar_stat)
	{
		// req and s_log must match

		if (req.size() != s_log.size())
			std::cerr << "Error: " << __FILE__ << ":" << __LINE__ << " req.size() != s_log.size() " << req.size() << "!=" << s_log.size() << "\n" ;

		// if it is waiting more than 20 seconds
		// Write a deadlock status report
		if (t.getwct() >= log_delay)
		{
			f << "=============================== NBX ==================================\n";

			int flag;

			f << "NBX counter: " << nbx << "\n";
			f << "\n";

			// Print the send requests and their status
			for (size_t i = 0 ; i < req.size() ; i++)
			{
				MPI_Status stat;
				MPI_SAFE_CALL(MPI_Request_get_status(req.get(i),&flag,&stat));
				if (flag == true)
					f << "Send to: " << s_log.get(i) << " with tag " << stat.MPI_TAG << " completed" << "\n";
				else
					f << "Send to: " << s_log.get(i) << " with tag " << stat.MPI_TAG << " pending" << "\n";
			}

			f << "\n";

			// Print the receive request and their status

			for (size_t j = 0 ; j < r_log.size() ; j++)
			{
				f << "Received from: " << r_log.get(j).MPI_SOURCE << " with tag " << r_log.get(j).MPI_TAG << "\n";
			}

			// Barrier status

			if (reach_b == true)
				f << "Barrier status: active\n";
			else
				f << "Barrier status: inactive\n";



			f << "======================================================================\n";
			f.flush();

			t.reset();
		}
	}

	/*! \brief Clear all the logged status
	 *
	 *
	 */
	void clear()
	{
		r_log.clear();
		s_log.clear();
	}
};

#else

/*! \brief Vcluster log
 *
 * Stub object, it does nothing
 *
 */
class Vcluster_log
{
public:
	inline void start(size_t log_delay)	{}
	inline void openLog(size_t rank)	{}
	inline void logRecv(MPI_Status & stat)	{}
	inline void logSend(size_t prc)	{}
	inline void NBXreport(size_t nbx, openfpm::vector<MPI_Request> & req, bool reach_b, MPI_Status bar_stat)	{}
	inline void clear() {};
};

#endif



#endif /* VCLUSTER_LOG_HPP_ */
