/*
 * py_runner.hpp
 *
 *  Created on: Sep 8, 2020
 *      Author: i-bird
 */

#include "config.h"
#include <mutex>
#include <thread>
#include "Vector/map_vector.hpp"
#include "py_runner.hpp"
#include "VCluster/VCluster.hpp"

std::thread t_py;
openfpm::vector<execute_request> py_er;
openfpm::vector<structs_list> py_slist;
std::mutex py_er_mx;
std::mutex py_sl_mx;

#if defined(HAVE_ZEROMQ) and defined(HAVE_PYTHON_SERVER)
#include <zmq.hpp>
#include <Python.h>

PyObject *pModule;

int queue_run_python_code(char * code);

void answer_bad_request(zmq::socket_t & socket)
{
    //  Send the result of the code running
	std::string br("Bad request");
    zmq::message_t reply(br.size());
    memcpy(reply.data (),br.c_str(),br.size());
    socket.send (reply);
}

void answer_incomplete_request(zmq::socket_t & socket)
{
    //  Send the result of the code running
	std::string in("incomp:");
    zmq::message_t reply(in.size());
    memcpy(reply.data (),in.c_str(),in.size());
    socket.send(reply);
}

void answer_executed_request(zmq::socket_t & socket, int r)
{
    //  Send the result of the code running
	// Calculate the size of the answer
	size_t size_pack = sizeof(size_t) + py_er.get(r).out.size()
			         + sizeof(size_t) + py_er.get(r).err.size()
			         + sizeof(size_t) + py_er.get(r).data.size();


	zmq::message_t reply(size_pack + 7);
	char * ptr = (char *)reply.data();
	memcpy(ptr,"comple:",7);
	ptr += 7;

	auto & er = py_er.get(r);

	// output message
	*(size_t *)ptr = er.out.size();
	ptr += sizeof(size_t);
	memcpy(ptr,er.out.c_str(),er.out.size());
	ptr += er.out.size();

	// error message
	*(size_t *)ptr = er.err.size();
	ptr += sizeof(size_t);
	memcpy(ptr,er.err.c_str(),er.err.size());
	ptr += er.err.size();

	// payload
	*(size_t *)ptr = er.data.size();
	ptr += sizeof(size_t);
	memcpy(ptr,er.data.getPointer(),er.data.size());
	ptr += er.data.size();

    socket.send(reply);
}

void answer_list_of_structs(zmq::socket_t & socket)
{
    //  number of structs
	size_t size_pack = sizeof(int);

	for (int i = 0 ; i < py_slist.size() ; i++)
	{
		// name number of chars
		size_pack += sizeof(int);
		size_pack += py_slist.get(i).name.size();
		// type number of chars
		size_pack += sizeof(int);
		size_pack += py_slist.get(i).type.size();
		// dimensions
		// dim of the struct
		size_pack += sizeof(int);
		// number of dims total
		size_pack += sizeof(int);
		size_pack += sizeof(int) * py_slist.get(i).sizes.size();
	}

	zmq::message_t reply(size_pack + 7);
	char * ptr = (char *)reply.data();
	memcpy(ptr,"lstruc:",7);
	ptr += 7;

	*(int *)ptr = py_slist.size();
	ptr += sizeof(int);
	for (int i = 0 ; i < py_slist.size() ; i++)
	{
		// name
		*(int *)ptr = py_slist.get(i).name.size();
		ptr += sizeof(int);
		memcpy(ptr,py_slist.get(i).name.c_str(),py_slist.get(i).name.size());
		ptr += py_slist.get(i).name.size();

		// type
		*(int *)ptr = py_slist.get(i).type.size();
		ptr += sizeof(int);
		memcpy(ptr,py_slist.get(i).type.c_str(),py_slist.get(i).type.size());
		ptr += py_slist.get(i).type.size();


		// dimensions
		// dim of the struct
		*(int *)ptr = py_slist.get(i).dim;
		ptr += sizeof(int);
		// number of dims total
		*(int *)ptr = py_slist.get(i).sizes.size();
		ptr += sizeof(int);
		for (int j = 0 ; j < py_slist.get(i).sizes.size() ; j++)
		{
			*(int *)ptr = py_slist.get(i).sizes.get(j);
			ptr += sizeof(int);
		}
	}

    socket.send(reply);
}

void init_zeromq_server_thr(int argc, char **argv)
{
	std::cout << "Starting ZeroMQ server" << std::endl;

#ifdef HAVE_ZEROMQ

	pModule = PyImport_AddModule("__main__");

    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);

        // queue code run
        int cmd_h = 7;
        if (request.size() >= cmd_h)
        {
        	std::string command(reinterpret_cast<const char *>(request.data()), cmd_h);

        	if (command == "runcmd:")
        	{
        		int r = queue_run_python_code((char *)request.data() + 7);

                //  Send reply back to client
            	std::string ans("queued:");
                zmq::message_t reply(ans.size() + sizeof(int));
                memcpy(reply.data (),ans.c_str(),ans.size());
                *(int *)((char *)reply.data()+cmd_h) = r;
                socket.send (reply);
        	}
        	else if (command == "lstruc:")
        	{
                //  Send reply back to client
        		answer_list_of_structs(socket);
        	}
        	else if (command == "nranks:")
        	{
                //  Send reply back to client
            	std::string ans("nranks:");
                zmq::message_t reply(ans.size() + sizeof(int));
                memcpy(reply.data (),ans.c_str(),ans.size());
                int nr = create_vcluster().size();
                *(int *)((char *)reply.data()+cmd_h) = nr;
                socket.send (reply);
        	}
        	else if (command == "queryr:")
        	{
        		int r = *(int *)((char *)request.data() + 7);
        		if (r < py_er.size())
        		{
        			if (py_er.get(r).executed == true)
        			{answer_executed_request(socket,r);}
        			else
        			{answer_incomplete_request(socket);}

        		}
        		else
        		{answer_bad_request(socket);}
        	}
        }
        else
        {answer_bad_request(socket);}
    }

#endif
}

void finalize_py_server()
{
//	PyEval_RestoreThread( pMainThreadState );
	Py_Finalize();
}

/*! \brief Python call this function to queue request to execute python code on every node
 *
 * \param code to run on each node
 *
 */
int queue_run_python_code(char * code)
{
	// lock the mutex
	py_er_mx.lock();

	py_er.add();

	py_er.last().code = std::string(code);

	// unlock the mutex
	py_er_mx.unlock();

	return py_er.size() - 1;
}

/*! \brief Python call this function to check if there are completed requests
 *
 * \retur the index of the request completed
 *
 */
int get_completed_request()
{
	for (int i = 0 ; i < py_er.size() ; i++)
	{
		if (py_er.get(i).executed == true)
		{return i;}
	}
}

const char * get_completed_request_output(int request)
{
	return py_er.get(request).out.c_str();
}

const char * get_completed_request_error(int request)
{
	return py_er.get(request).err.c_str();
}

void * get_completed_request_payload(int request)
{
	return py_er.get(request).data.getPointer();
}

void eliminate_request(int request)
{
	// lock the mutex
	py_er_mx.lock();

	py_er.remove(request);

	// unlock the mutex
	py_er_mx.unlock();
}

void add_structure(const structs_list & sa)
{
	// lock the mutex
	py_sl_mx.lock();

	py_slist.add(sa);

	// unlock the mutex
	py_sl_mx.unlock();
}

std::string bcast_code(int i)
{
	auto & v_cl = create_vcluster();

	openfpm::vector<int> size;
	if (i != -1)
	{size.add(py_er.get(i).code.size());}
	else
	{size.add();}

	v_cl.Bcast(size,0);
	v_cl.execute();

	std::string code;
	openfpm::vector<char> code_;
	code_.resize(size.get(0)+1);
	code_.get(size.get(0)) = 0;
	const char * ptr = py_er.get(i).code.c_str();
	std::copy((const char *)ptr,(const char *)ptr + py_er.get(i).code.size(),(char *)code_.getPointer());

	v_cl.Bcast(code_,0);
	v_cl.execute();

	code = std::string((char *)code_.getPointer());

	return code;
}

void run_py(const char * py_cmd,
		    std::string & out,
		    std::string & err,
		    openfpm::vector<unsigned char> & payload,
		    PyObject * pModule)
{
	std::string stdOutErr =
"import sys\n\
class CatchOut:\n\
    def __init__(self):\n\
      self.value = ''\n\
    def write(self, txt):\n\
      self.value += txt\n\
catchOut = CatchOut()\n\
catchErr = CatchOut()\n\
sys.stdout = catchOut\n\
sys.stderr = catchErr\n\
";

	PyRun_SimpleString(stdOutErr.c_str());
	PyRun_SimpleString(py_cmd);

	PyObject *catcherO = PyObject_GetAttrString(pModule,"catchOut");
	if (catcherO == NULL)
	{std::cout << "No catcher out" << std::endl;return;}
	PyObject *catcherE = PyObject_GetAttrString(pModule,"catchErr");
	if (catcherE == NULL)
	{std::cout << "No catcher err" << std::endl;return;}

	PyObject *output = PyObject_GetAttrString(catcherO,"value");
	PyObject *error = PyObject_GetAttrString(catcherE,"value");

	out = std::string(PyUnicode_AsUTF8(output));
	err = std::string(PyUnicode_AsUTF8(error));
}

enum exe_command
{
	CONTINUE,
	STOP,
	NO_COMMAND
};

exe_command flush_python_commands()
{
	exe_command ec = exe_command::NO_COMMAND;
	auto & v_cl = create_vcluster();
	int rank = v_cl.rank();
	std::string code;


	if (rank == 0)
	{
		// lock the mutex
		py_er_mx.lock();

		// Bcast number of commands
		openfpm::vector<int> size;
		size.add(py_er.size());

		v_cl.Bcast(size,0);
		v_cl.execute();

		for (int i = 0 ; i < py_er.size() ; i++)
		{
			if (py_er.get(i).executed == false)
			{
				std::string code_t = bcast_code(i);
				if (code_t.substr(0,7) == "continue")
				{ec = exe_command::CONTINUE;}
				else if (code_t.substr(0,4) == "stop")
				{ec = exe_command::STOP;}
				else
				{
					run_py(code_t.c_str(),py_er.get(i).out,py_er.get(i).err,py_er.get(i).data,pModule);
				}
				py_er.get(i).executed = true;

			}
		}

		// unlock the mutex
		py_er_mx.unlock();
	}
	else
	{
		openfpm::vector<int> size;
		size.add();

		v_cl.Bcast(size,0);
		v_cl.execute();

		for (int i = 0 ; i < size.get(0) ; i++)
		{
			std::string code_t = bcast_code(-1);
			if (code_t.substr(0,7) == "continue")
			{ec = exe_command::CONTINUE;}
			else if (code_t.substr(0,4) == "stop")
			{ec = exe_command::STOP;}
			else
			{PyRun_SimpleString(code_t.c_str());}
		}
	}

	return ec;
}

int tot_step = 0;
int target_step = -1;

/*! \brief suspend the execution until continue
 *
 *
 */
void do_breakpoint()
{
	while(1)
	{
		exe_command ec = flush_python_commands();
		if (ec == exe_command::CONTINUE)	{break;}
		usleep(100000);
	}
}

void step_done()
{
	flush_python_commands();

	if (target_step > tot_step)
	{return;}

	do_breakpoint();

	tot_step++;
}

#endif

/*! \brief Initialize python server
 *
 *
 */
void init_python_server(int argc, char **argv, int rank)
{
#if defined(HAVE_ZEROMQ) && defined(HAVE_PYTHON_SERVER)
	wchar_t *program = Py_DecodeLocale(argv[0], NULL);
	if (program == NULL)
	{
		fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
		exit(1);
	}
	Py_SetProgramName(program);
	Py_Initialize();

	if (rank == 0)
	{
		t_py = std::thread(init_zeromq_server_thr,argc,argv);
	}
#else
	std::cout << __FILE__ << ":" << __LINE__ << " ZeroMQ is required to start the python server" << std::endl;
#endif
}

