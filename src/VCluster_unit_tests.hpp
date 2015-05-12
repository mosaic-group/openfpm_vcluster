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

#define VERBOSE_TEST

#define N_TRY 4
#define N_LOOP 128
#define BUFF_STEP 524288

BOOST_AUTO_TEST_SUITE( VCluster_test )

size_t global_step = 0;

// Alloc the buffer to receive the messages

void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	BOOST_REQUIRE_EQUAL(total_p,global_v_cluster->getProcessingUnits()-1);
	v->resize(total_p + 1);

	BOOST_REQUIRE_EQUAL(msg_i, (global_step + 1)*BUFF_STEP);
	v->get(i).resize(msg_i);

	return &(v->get(i).get(0));
}

// Alloc the buffer to receive the messages

size_t id = 0;
openfpm::vector<size_t> prc_recv;

void * msg_alloc2(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	v->resize(total_p);
	prc_recv.resize(total_p);

	BOOST_REQUIRE_EQUAL(msg_i, (global_step + 1)*BUFF_STEP);

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
	vcl.execute();

	if ( vcl.getProcessingUnits() < 128 )
	{BOOST_REQUIRE_EQUAL(c,vcl.getProcessingUnits());}
	if ( vcl.getProcessingUnits() < 256 )
	{BOOST_REQUIRE_EQUAL(uc,vcl.getProcessingUnits());}
	if ( vcl.getProcessingUnits() < 32768 )
	{BOOST_REQUIRE_EQUAL(s,vcl.getProcessingUnits());}
	if ( vcl.getProcessingUnits() < 65536 )
	{BOOST_REQUIRE_EQUAL(us,vcl.getProcessingUnits());}
	if ( vcl.getProcessingUnits() < 2147483648 )
	{BOOST_REQUIRE_EQUAL(i,vcl.getProcessingUnits());}
	if ( vcl.getProcessingUnits() < 4294967296 )
	{BOOST_REQUIRE_EQUAL(ui,vcl.getProcessingUnits());}

	BOOST_REQUIRE_EQUAL(li,vcl.getProcessingUnits());
	BOOST_REQUIRE_EQUAL(uli,vcl.getProcessingUnits());
	BOOST_REQUIRE_EQUAL(f,vcl.getProcessingUnits());
	BOOST_REQUIRE_EQUAL(d,vcl.getProcessingUnits());
}


BOOST_AUTO_TEST_CASE( VCluster_use_sendrecv)
{
	std::cout << "VCluster unit test start" << "\n";
	init_global_v_cluster(&boost::unit_test::framework::master_test_suite().argc,&boost::unit_test::framework::master_test_suite().argv);

	Vcluster & vcl = *global_v_cluster;

	// send/recv messages

	size_t n_proc = vcl.getProcessingUnits();

	// Checking All to All pattern

	for (size_t s = 0 ; s < N_TRY ; s++)
	{
		for (size_t j = 0 ; j < N_LOOP ; j++)
		{
			global_step = j;
			// send message
			openfpm::vector<openfpm::vector<unsigned char>> message;
			// recv message
			openfpm::vector<openfpm::vector<unsigned char>> recv_message(n_proc);

			openfpm::vector<size_t> prc;

			for (size_t i = 0 ; i < n_proc ; i++)
			{
				if (i != vcl.getProcessUnitID())
				{
					prc.add(i);
					message.add();
					std::ostringstream msg;
					msg << "Hello from " << vcl.getProcessUnitID() << " to " << i;
					std::string str(msg.str());
					message.last().resize((j+1)*BUFF_STEP);
					memset(message.last().getPointer(),0,(j+1)*BUFF_STEP);
					std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
					// resize also recv_message
					recv_message.get(i).resize((j+1)*BUFF_STEP);
					memset(recv_message.get(i).getPointer(),0,(j+1)*BUFF_STEP);
				}
			}

#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			vcl.sendrecvMultipleMessages(prc,message,msg_alloc,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();

			size_t size_send_recv = 2 * (j+1)*BUFF_STEP * (vcl.getProcessingUnits()-1);
			vcl.reduce(size_send_recv);
			vcl.execute();

			if (vcl.getProcessUnitID() == 0 && s == N_TRY - 1)
				std::cout << "(All to All: )Buffer size: " << (j+1)*BUFF_STEP << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 << "\n";
#endif

			// Check the message

			for (size_t i = 0 ; i < recv_message.size() ; i++)
			{
				if (i != vcl.getProcessUnitID())
				{
					std::ostringstream msg;
					msg << "Hello from " << i << " to " << vcl.getProcessUnitID();
					std::string str(msg.str());
					BOOST_REQUIRE_EQUAL(std::equal(str.c_str(),str.c_str() + str.size() ,&(recv_message.get(i).get(0))),true);
				}
				else
				{
					BOOST_REQUIRE_EQUAL(0,recv_message.get(i).size());
				}
			}
		}

		std::srand(global_v_cluster->getProcessUnitID());
		std::default_random_engine eg;
		std::uniform_int_distribution<int> d(0,2);

		// Check random pattern 1%3

		for (size_t j = 0 ; j < N_LOOP ; j++)
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
				if (d(eg) == vcl.getProcessUnitID())
				{
					prc.add(i);
					o_send.add(i);
					message.add();
					std::ostringstream msg;
					msg << "Hello from " << vcl.getProcessUnitID() << " to " << i;
					std::string str(msg.str());
					message.last().resize(str.size());
					std::copy(str.c_str(),&(str.c_str())[msg.str().size()],&(message.last().get(0)));
					// Resize the message to j+1 BUFF_STEP
					message.last().resize((j+1)*BUFF_STEP);
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

			size_t size_send_recv = (prc.size() + recv_message.size()) * (j+1)*BUFF_STEP * (vcl.getProcessingUnits()-1);
			vcl.reduce(size_send_recv);
			vcl.reduce(clk);
			vcl.execute();
			clk /= vcl.getProcessingUnits();

			if (vcl.getProcessUnitID() == 0 && s == N_TRY - 1)
				std::cout << "(Random Pattern: ) Buffer size: " << (j+1)*BUFF_STEP << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 <<  " MB/s    Clock: " << clk <<  "\n";
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
				// Resize the message to j+1 BUFF_STEP
				message.last().resize((j+1)*BUFF_STEP);
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
	}

	std::cout << "VCluster unit test stop" << "\n";
}

BOOST_AUTO_TEST_SUITE_END()


#endif /* VCLUSTER_UNIT_TESTS_HPP_ */
