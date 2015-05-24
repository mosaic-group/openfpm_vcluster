/*
 * VCluster_unit_tests.hpp
 *
 *  Created on: May 9, 2015
 *      Author: Pietro incardona
 */

#ifndef VCLUSTER_UNIT_TESTS_HPP_
#define VCLUSTER_UNIT_TESTS_HPP_

#include "VCluster.hpp"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
#include "timer.hpp"
#include <random>

#define VERBOSE_TEST

#define N_TRY 2
#define N_LOOP 67108864
#define BUFF_STEP 524288

BOOST_AUTO_TEST_SUITE( VCluster_test )

size_t global_step = 0;

// Alloc the buffer to receive the messages

void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i,size_t ri, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	if (global_v_cluster->getProcessingUnits() <= 8)
		BOOST_REQUIRE_EQUAL(total_p,global_v_cluster->getProcessingUnits()-1);
	else
		BOOST_REQUIRE_EQUAL(total_p,8);

	BOOST_REQUIRE_EQUAL(msg_i, global_step);
	v->get(i).resize(msg_i);

	return &(v->get(i).get(0));
}

// Alloc the buffer to receive the messages

size_t id = 0;
openfpm::vector<size_t> prc_recv;

void * msg_alloc2(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	v->resize(total_p);
	prc_recv.resize(total_p);

	BOOST_REQUIRE_EQUAL(msg_i, global_step);

	id++;
	v->get(id-1).resize(msg_i);
	prc_recv.get(id-1) = i;
	return &(v->get(id-1).get(0));
}

BOOST_AUTO_TEST_CASE( VCluster_use_reductions)
{
	init_global_v_cluster(&boost::unit_test::framework::master_test_suite().argc,&boost::unit_test::framework::master_test_suite().argv);
	Vcluster & vcl = *global_v_cluster;

	unsigned char uc = 1;
	char c = 1;
	short s = 1;
	unsigned short us = 1;
	int i = 1;
	unsigned int ui = 1;
	long int li = 1;
	unsigned long int uli = 1;
	float f = 1;
	double d = 1;

	unsigned char uc_max = vcl.getProcessUnitID();
	char c_max = vcl.getProcessUnitID();
	short s_max = vcl.getProcessUnitID();
	unsigned short us_max = vcl.getProcessUnitID();
	int i_max = vcl.getProcessUnitID();
	unsigned int ui_max = vcl.getProcessUnitID();
	long int li_max = vcl.getProcessUnitID();
	unsigned long int uli_max = vcl.getProcessUnitID();
	float f_max = vcl.getProcessUnitID();
	double d_max = vcl.getProcessUnitID();

	// Sum reductions
	if ( vcl.getProcessingUnits() < 128 )
		vcl.reduce(c);
	if ( vcl.getProcessingUnits() < 256 )
		vcl.reduce(uc);
	if ( vcl.getProcessingUnits() < 32768 )
		vcl.reduce(s);
	if ( vcl.getProcessingUnits() < 65536 )
		vcl.reduce(us);
	if ( vcl.getProcessingUnits() < 2147483648 )
		vcl.reduce(i);
	if ( vcl.getProcessingUnits() < 4294967296 )
		vcl.reduce(ui);
	vcl.reduce(li);
	vcl.reduce(uli);
	vcl.reduce(f);
	vcl.reduce(d);

	// Max reduction
	if ( vcl.getProcessingUnits() < 128 )
		vcl.max(c_max);
	if ( vcl.getProcessingUnits() < 256 )
		vcl.max(uc_max);
	if ( vcl.getProcessingUnits() < 32768 )
		vcl.max(s_max);
	if ( vcl.getProcessingUnits() < 65536 )
		vcl.max(us_max);
	if ( vcl.getProcessingUnits() < 2147483648 )
		vcl.max(i_max);
	if ( vcl.getProcessingUnits() < 4294967296 )
		vcl.max(ui_max);
	vcl.max(li_max);
	vcl.max(uli_max);
	vcl.max(f_max);
	vcl.max(d_max);
	vcl.execute();

	if ( vcl.getProcessingUnits() < 128 )
	{BOOST_REQUIRE_EQUAL(c_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 256 )
	{BOOST_REQUIRE_EQUAL(uc_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 32768 )
	{BOOST_REQUIRE_EQUAL(s_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 65536 )
	{BOOST_REQUIRE_EQUAL(us_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 2147483648 )
	{BOOST_REQUIRE_EQUAL(i_max,vcl.getProcessingUnits()-1);}
	if ( vcl.getProcessingUnits() < 4294967296 )
	{BOOST_REQUIRE_EQUAL(ui_max,vcl.getProcessingUnits()-1);}

	BOOST_REQUIRE_EQUAL(li_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(uli_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(f_max,vcl.getProcessingUnits()-1);
	BOOST_REQUIRE_EQUAL(d_max,vcl.getProcessingUnits()-1);
}


BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv)
{
	std::cout << "VCluster unit test start" << "\n";
	init_global_v_cluster(&boost::unit_test::framework::master_test_suite().argc,&boost::unit_test::framework::master_test_suite().argv);

	Vcluster & vcl = *global_v_cluster;

	// send/recv messages

	size_t n_proc = vcl.getProcessingUnits();

	// Checking short communication pattern

	for (size_t s = 0 ; s < N_TRY ; s++)
	{
		for (size_t j = 32 ; j < N_LOOP ; j*=2)
		{
			global_step = j;
			// send message
			openfpm::vector<openfpm::vector<unsigned char>> message;
			// recv message
			openfpm::vector<openfpm::vector<unsigned char>> recv_message(n_proc);

			openfpm::vector<size_t> prc;

			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				size_t p_id = (i + 1 + vcl.getProcessUnitID()) % n_proc;
				if (p_id != vcl.getProcessUnitID())
				{
					prc.add(p_id);
					message.add();
					std::ostringstream msg;
					msg << "Hello from " << vcl.getProcessUnitID() << " to " << p_id;
					std::string str(msg.str());
					message.last().resize(j);
					memset(message.last().getPointer(),0,j);
					std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
					// resize also recv_message
					recv_message.get(p_id).resize(j);
					memset(recv_message.get(p_id).getPointer(),0,j);
				}
			}

#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			recv_message.resize(n_proc);
			vcl.sendrecvMultipleMessages(prc,message,msg_alloc,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();
			double clk_max = clk;

			size_t size_send_recv = 2 * j * (prc.size());
			vcl.reduce(size_send_recv);
			vcl.max(clk_max);
			vcl.execute();

			if (vcl.getProcessUnitID() == 0)
				std::cout << "(Short pattern: )Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 << " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max <<"\n";
#endif

			// Check the message
			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				long int p_id = vcl.getProcessUnitID() - i - 1;
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != vcl.getProcessUnitID())
				{
					std::ostringstream msg;
					msg << "Hello from " << p_id << " to " << vcl.getProcessUnitID();
					std::string str(msg.str());
					BOOST_REQUIRE_EQUAL(std::equal(str.c_str(),str.c_str() + str.size() ,&(recv_message.get(p_id).get(0))),true);
				}
				else
				{
					BOOST_REQUIRE_EQUAL(0,recv_message.get(p_id).size());
				}
			}
		}

		std::srand(global_v_cluster->getProcessUnitID());
		std::default_random_engine eg;
		std::uniform_int_distribution<int> d(0,n_proc/8);

		// Check random pattern (maximum 16 processors)

		for (size_t j = 32 ; j < N_LOOP && n_proc < 16 ; j*=2)
		{
			global_step = j;
			// original send
			openfpm::vector<size_t> o_send;
			// send message
			openfpm::vector<openfpm::vector<unsigned char>> message;
			// recv message
			openfpm::vector<openfpm::vector<unsigned char>> recv_message;

			openfpm::vector<size_t> prc;

			for (size_t i = 0 ; i < n_proc ; i++)
			{
				// randomly with witch processor communicate
				if (d(eg) == 0)
				{
					prc.add(i);
					o_send.add(i);
					message.add();
					std::ostringstream msg;
					msg << "Hello from " << vcl.getProcessUnitID() << " to " << i;
					std::string str(msg.str());
					message.last().resize(str.size());
					std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
					message.last().resize(j);
				}
			}

			id = 0;
			prc_recv.clear();


#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			vcl.sendrecvMultipleMessages(prc,message,msg_alloc2,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();
			double clk_max = clk;

			size_t size_send_recv = (prc.size() + recv_message.size()) * j;
			vcl.reduce(size_send_recv);
			vcl.reduce(clk);
			vcl.max(clk_max);
			vcl.execute();
			clk /= vcl.getProcessingUnits();

			if (vcl.getProcessUnitID() == 0)
				std::cout << "(Random Pattern: ) Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 <<  " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max << "\n";
#endif

			// Check the message

			for (size_t i = 0 ; i < recv_message.size() ; i++)
			{
				std::ostringstream msg;
				msg << "Hello from " << prc_recv.get(i) << " to " << vcl.getProcessUnitID();
				std::string str(msg.str());
				BOOST_REQUIRE_EQUAL(std::equal(str.c_str(),str.c_str() + str.size() ,&(recv_message.get(i).get(0))),true);
			}

			// Reply back

			// Create the message

			prc.clear();
			message.clear();
			for (size_t i = 0 ; i < prc_recv.size() ; i++)
			{
				prc.add(prc_recv.get(i));
				message.add();
				std::ostringstream msg;
				msg << "Hey from " << vcl.getProcessUnitID() << " to " << prc_recv.get(i);
				std::string str(msg.str());
				message.last().resize(str.size());
				std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
				message.last().resize(j);
			}

			id = 0;
			prc_recv.clear();
			recv_message.clear();
			vcl.sendrecvMultipleMessages(prc,message,msg_alloc2,&recv_message);

			// Check if the received hey message match the original send

			BOOST_REQUIRE_EQUAL(o_send.size(),prc_recv.size());

			for (size_t i = 0 ; i < o_send.size() ; i++)
			{
				size_t j = 0;
				for ( ; j < prc_recv.size() ; j++)
				{
					if (o_send.get(i) == prc_recv.get(j))
					{
						// found the message check it

						std::ostringstream msg;
						msg << "Hey from " << prc_recv.get(i) << " to " << vcl.getProcessUnitID();
						std::string str(msg.str());
						BOOST_REQUIRE_EQUAL(std::equal(str.c_str(),str.c_str() + str.size() ,&(recv_message.get(i).get(0))),true);
						break;
					}
				}
				// Check that we find always a match
				BOOST_REQUIRE_EQUAL(j != prc_recv.size(),true);
			}
		}

		// Check long communication pattern

		for (size_t j = 32 ; j < N_LOOP ; j*=2)
		{
			global_step = j;
			// Processor step
			size_t ps = n_proc / 8 + 1;

			// send message
			openfpm::vector<openfpm::vector<unsigned char>> message;
			// recv message
			openfpm::vector<openfpm::vector<unsigned char>> recv_message(n_proc);

			openfpm::vector<size_t> prc;

			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				size_t p_id = ((i+1) * ps + vcl.getProcessUnitID()) % n_proc;
				if (p_id != vcl.getProcessUnitID())
				{
					prc.add(p_id);
					message.add();
					std::ostringstream msg;
					msg << "Hello from " << vcl.getProcessUnitID() << " to " << p_id;
					std::string str(msg.str());
					message.last().resize(j);
					memset(message.last().getPointer(),0,j);
					std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
					// resize also recv_message
					recv_message.get(p_id).resize(j);
					memset(recv_message.get(p_id).getPointer(),0,j);
				}
			}

#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			recv_message.resize(n_proc);
			vcl.sendrecvMultipleMessages(prc,message,msg_alloc,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();
			double clk_max = clk;

			size_t size_send_recv = 2 * j * (prc.size());
			vcl.reduce(size_send_recv);
			vcl.max(clk_max);
			vcl.execute();

			if (vcl.getProcessUnitID() == 0)
				std::cout << "(Long pattern: )Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 << " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max <<"\n";
#endif

			// Check the message
			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				long int p_id = (- (i+1) * ps + vcl.getProcessUnitID()) % n_proc;
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != vcl.getProcessUnitID())
				{
					std::ostringstream msg;
					msg << "Hello from " << p_id << " to " << vcl.getProcessUnitID();
					std::string str(msg.str());
					BOOST_REQUIRE_EQUAL(std::equal(str.c_str(),str.c_str() + str.size() ,&(recv_message.get(p_id).get(0))),true);
				}
				else
				{
					BOOST_REQUIRE_EQUAL(0,recv_message.get(p_id).size());
				}
			}
		}
	}

	std::cout << "VCluster unit test stop" << "\n";
}

BOOST_AUTO_TEST_SUITE_END()


#endif /* VCLUSTER_UNIT_TESTS_HPP_ */
