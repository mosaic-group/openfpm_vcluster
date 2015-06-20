/*
 * VCluster_unit_test_util.hpp
 *
 *  Created on: May 30, 2015
 *      Author: i-bird
 */

#ifndef VCLUSTER_UNIT_TEST_UTIL_HPP_
#define VCLUSTER_UNIT_TEST_UTIL_HPP_

#define VERBOSE_TEST

#include "VCluster.hpp"

#define NBX 1
#define PCX 2

#define N_TRY 2
#define N_LOOP 67108864
#define BUFF_STEP 524288

bool totp_check;
size_t global_step = 0;
size_t global_rank;

// Alloc the buffer to receive the messages

void * msg_alloc(size_t msg_i ,size_t total_msg, size_t total_p, size_t i,size_t ri, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	if (global_v_cluster->getProcessingUnits() <= 8)
	{if (totp_check) BOOST_REQUIRE_EQUAL(total_p,global_v_cluster->getProcessingUnits()-1);}
	else
	{if (totp_check) BOOST_REQUIRE_EQUAL(total_p,8);}

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

void * msg_alloc3(size_t msg_i ,size_t total_msg, size_t total_p, size_t i, size_t ri, void * ptr)
{
	openfpm::vector<openfpm::vector<unsigned char>> * v = static_cast<openfpm::vector<openfpm::vector<unsigned char>> *>(ptr);

	v->add();
	prc_recv.add();

	BOOST_REQUIRE_EQUAL(msg_i, global_step);

	v->last().resize(msg_i);
	prc_recv.last() = i;
	return &(v->last().get(0));
}

template<unsigned int ip, typename T> void commFunc(Vcluster & vcl,openfpm::vector< size_t > & prc, openfpm::vector< T > & data, void * (* msg_alloc)(size_t,size_t,size_t,size_t,size_t,void *), void * ptr_arg)
{
	if (ip == PCX)
		vcl.sendrecvMultipleMessagesPCX(prc,data,msg_alloc,ptr_arg);
	else if (ip == NBX)
		vcl.sendrecvMultipleMessagesNBX(prc,data,msg_alloc,ptr_arg);
}

template <unsigned int ip> std::string method()
{
	if (ip == PCX)
		return std::string("PCX");
	else if (ip == NBX)
		return std::string("NBX");
}

template<unsigned int ip> void test()
{
	Vcluster & vcl = *global_v_cluster;

	// send/recv messages

	global_rank = vcl.getProcessUnitID();
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
				}
			}

			recv_message.resize(n_proc);
			// The pattern is not really random preallocate the receive buffer
			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				long int p_id = vcl.getProcessUnitID() - i - 1;
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != (long int)vcl.getProcessUnitID())
					recv_message.get(p_id).resize(j);
			}

#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			commFunc<ip>(vcl,prc,message,msg_alloc,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();
			double clk_max = clk;

			size_t size_send_recv = 2 * j * (prc.size());
			vcl.reduce(size_send_recv);
			vcl.max(clk_max);
			vcl.execute();

			if (vcl.getProcessUnitID() == 0)
				std::cout << "(Short pattern: " << method<ip>() << ")Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 << " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max <<"\n";
#endif

			// Check the message
			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				long int p_id = vcl.getProcessUnitID() - i - 1;
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != (long int)vcl.getProcessUnitID())
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

			commFunc<ip>(vcl,prc,message,msg_alloc3,&recv_message);

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
				std::cout << "(Random Pattern: " << method<ip>() << ") Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 <<  " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max << "\n";
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

			commFunc<ip>(vcl,prc,message,msg_alloc3,&recv_message);

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
			long int ps = n_proc / (8 + 1);

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
				}
			}

			recv_message.resize(n_proc);
			// The pattern is not really random preallocate the receive buffer
			for (size_t i = 0 ; i < 8  && i < n_proc ; i++)
			{
				long int p_id = (- (i+1) * ps + (long int)vcl.getProcessUnitID());
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != (long int)vcl.getProcessUnitID())
					recv_message.get(p_id).resize(j);
			}

#ifdef VERBOSE_TEST
			timer t;
			t.start();
#endif

			commFunc<ip>(vcl,prc,message,msg_alloc,&recv_message);

#ifdef VERBOSE_TEST
			t.stop();
			double clk = t.getwct();
			double clk_max = clk;

			size_t size_send_recv = 2 * j * (prc.size());
			vcl.reduce(size_send_recv);
			vcl.max(clk_max);
			vcl.execute();

			if (vcl.getProcessUnitID() == 0)
				std::cout << "(Long pattern: " << method<ip>() << ")Buffer size: " << j << "    Bandwidth (Average): " << size_send_recv / vcl.getProcessingUnits() / clk / 1e6 << " MB/s  " << "    Bandwidth (Total): " << size_send_recv / clk / 1e6 << " MB/s    Clock: " << clk << "   Clock MAX: " << clk_max <<"\n";
#endif

			// Check the message
			for (long int i = 0 ; i < 8  && i < (long int)n_proc ; i++)
			{
				long int p_id = (- (i+1) * ps + (long int)vcl.getProcessUnitID());
				if (p_id < 0)
					p_id += n_proc;
				else
					p_id = p_id % n_proc;

				if (p_id != (long int)vcl.getProcessUnitID())
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
}



#endif /* VCLUSTER_UNIT_TEST_UTIL_HPP_ */
