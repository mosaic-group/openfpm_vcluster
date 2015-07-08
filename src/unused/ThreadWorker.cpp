/**
 *
 * This is a modified version of GPUWorker from
 *
 *
 * Highly Optimized Object-Oriented Molecular Dynamics (HOOMD) Open
 * Source Software License
 * Copyright (c) 2008 Ames Laboratory Iowa State University
 * All rights reserved.

// $Id$
// $URL$

/*! \file GPUWorker.cc
	\brief Code the GPUWorker class
*/

//#ifdef USE_CUDA

#include <cuda.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>
#include <sstream>
#include <iostream>

#include "ThreadWorker.h"

using namespace boost;
using namespace std;

/*! \param dev GPU device number to be passed to cudaSetDevice()
	
	Constructing a GPUWorker creates the worker thread
*/

ThreadWorker::ThreadWorker() : m_exit(false), m_work_to_do(false), m_last_error(CUDA_SUCCESS)
{
	IDevent = 1;
	pool_pointer = 0;
	m_thread.reset(new thread(bind(&ThreadWorker::performWorkLoop, this)));
}
	
/*! Shuts down the worker thread
*/
ThreadWorker::~ThreadWorker()
{
	// set the exit condition
	{
		mutex::scoped_lock lock(m_mutex);
		m_work_to_do = true;
		m_exit = true;
	}
	
	// notify the thread there is work to do
	m_cond_work_to_do.notify_one();
	
	// join with the thread
	m_thread->join();
	}
}

/*! \brief Check if the event list is empty
 *
 * \return true if the event list is empty
 *
 */

bool ThreadWorker::IsEventListEmpty()
{
	if (eventlist.size() == 0)
		return true;
	return false;
}

/*! \brief Release an event
 *
 * \param event to release
 *
 * Release an event, associated to a command carefully an event associated to a command
 * not completed cannot be released for safety reason
 *
 */

void ThreadWorker::ReleaseEvent(std::list<event_sign>::iterator & event)
{
	if ((*event).status != COMMAND_COMPLETED)
	{std::cerr << "ERROR CRITICAL: Releasing an uncompleted event is not allowed ... terminating \n";exit(-1);}

	eventlist.erase(event);
}

/*! \param func Function call to execute in the worker thread

	call() executes a CUDA call to in a worker thread. Any function
	with any arguments can be passed in to be queued using boost::bind.
	Examples:
\code
gpu.call(bind(function, arg1, arg2, arg3, ...));
gpu.call(bind(cudaMemcpy, &h_float, d_float, sizeof(float), cudaMemcpyDeviceToHost));
gpu.call(bind(cudaThreadSynchronize));
\endcode
	The only requirement is that the function returns a CUResult. Since every
	single CUDA Driver API function does so, you can call any Driver API function.
	You can call any custom functions too, as long as you return a CUResult representing
	the error of any CUDA functions called within. This is typical in kernel
	driver functions. For example, a .cu file might contain:
\code
__global__ void kernel() { ... }
cudaError_t kernel_driver()
	{
	kernel<<<blocks, threads>>>();
	#ifdef NDEBUG
	return cudaSuccess;
	#else
	cudaThreadSynchronize();
	return cudaGetLastError();
	#endif
	}
\endcode
	It is recommended to just return cudaSuccess in release builds to keep the asynchronous
	call stream going with no cudaThreadSynchronize() overheads.
	
	call() ensures that \a func has been executed before it returns. This is
	desired behavior, most of the time. For calling kernels or other asynchronous
	CUDA functions, use callAsync(), but read the warnings in it's documentation
	carefully and understand what you are doing. Why have callAsync() at all?
	The original purpose for designing GPUWorker is to allow execution on 
	multiple GPUs simultaneously which can only be done with asynchronous calls.
	
	An exception will be thrown if the CUDA call returns anything other than
	cudaSuccess.
*/

CUresult ThreadWorker::call(const boost::function< CUresult (void) > &func)
{
	// this mutex lock is to prevent multiple threads from making
	// simultaneous calls. Thus, they can depend on the exception
	// thrown to exactly be the error from their call and not some
	// race condition from another thread
	// making GPUWorker calls to a single GPUWorker from multiple threads 
	// still isn't supported
	
	mutex::scoped_lock lock(m_call_mutex);

	// call and then sync
	callAsync(func);
	return sync();

}

/*! \brief Get info on the event
 *
 * \param event to querry
 * \param eventstat the status of the event is returned
 *
 */

CUresult ThreadWorker::GetEventInfo(std::list<event_sign>::iterator & event, int & eventstat)
{
	eventstat = (*event).status;

	return CUDA_SUCCESS;
}

/*! \param func Function to execute inside the worker thread
	
	callAsync is like call(), but  returns immeadiately after entering \a func into the queue. 
	The worker thread will eventually get around to running it. Multiple contiguous
	calls to callAsync() will result in potentially many function calls 
	being queued before any run.
*/

CUresult ThreadWorker::callAsync(const boost::function< CUresult (void) > &func)
{

	// add the function object to the queue
	{
		struct function_callback fun_c;
		mutex::scoped_lock lock(m_mutex);
		fun_c.event_ptr = NULL;
		fun_c.func = func;
		fun_c.IsWaitEvent = false;
		m_work_queue.push_back(fun_c);
		m_work_to_do = true;
	}
	
	// notify the threads there is work to do
	m_cond_work_to_do.notify_one();
	
	return CUDA_SUCCESS;
}

/*! \param func Function to execute inside the worker thread

	callAsync is like call(), but  returns immeadiately after entering \a func into the queue.
	The worker thread will eventually get around to running it. Multiple contiguous
	calls to callAsync() will result in potentially many function calls
	being queued before any run.
*/

CUresult ThreadWorker::callAsync(const boost::function< CUresult (void) > &func, std::list<event_sign>::iterator * event)
{
  
	// add the function object to the queue
	{
		struct function_callback fun_c;
		struct event_sign evt;
		mutex::scoped_lock lock(m_mutex);
		evt.cond = &ConditionPool[pool_pointer];
		pool_pointer++;
		if (pool_pointer >= POOL_SIZE)	{pool_pointer = 0;}
		evt.status = COMMAND_ENQUEUE;
		eventlist.push_back(evt);
		*event = eventlist.end();
		(*event)--;
		fun_c.event_ptr = &eventlist.back();
		fun_c.func = func;
		fun_c.IsWaitEvent = false;
		m_work_queue.push_back(fun_c);
		m_work_to_do = true;
	}
	
	// notify the threads there is work to do
	m_cond_work_to_do.notify_one();
	
	return CUDA_SUCCESS;
}

CUresult ThreadWorker::callAsyncWait(std::list<event_sign>::iterator & event_w)
{
  
	// add the function object to the queue
	{
		struct function_callback fun_c;
		mutex::scoped_lock lock(m_mutex);
		fun_c.event_ptr = NULL;
		fun_c.event_wait_ptr = &*event_w;
		fun_c.func = boost::bind(ThreadWorker::wait_cond,(*event_w).cond);
		fun_c.IsWaitEvent = true;
		m_work_queue.push_back(fun_c);
		m_work_to_do = true;
	}
	
	// notify the threads there is work to do
	m_cond_work_to_do.notify_one();
	
	
	return CUDA_SUCCESS;
}

CUresult ThreadWorker::wait_cond(TCondition * cond)
{
	pthread_cond_wait( & cond->_cond, & cond->_mutex );
	return CUDA_SUCCESS;
}

const char * ThreadWorker::error_msg(unsigned int id)
{
	switch (id)
	{
		case CUDA_SUCCESS:
		return cudaerr[0].err_msg;
		case CUDA_ERROR_INVALID_VALUE:
		return cudaerr[1].err_msg;
		case CUDA_ERROR_OUT_OF_MEMORY:
		return cudaerr[2].err_msg;
		case CUDA_ERROR_NOT_INITIALIZED:
		return cudaerr[3].err_msg;
		case CUDA_ERROR_DEINITIALIZED:
		return cudaerr[4].err_msg;
		case CUDA_ERROR_NO_DEVICE:
		return cudaerr[5].err_msg;
		case CUDA_ERROR_INVALID_DEVICE:
		return cudaerr[6].err_msg;
		case CUDA_ERROR_INVALID_IMAGE:
		return cudaerr[7].err_msg;
		case CUDA_ERROR_INVALID_CONTEXT:
		return cudaerr[8].err_msg;
		case CUDA_ERROR_CONTEXT_ALREADY_CURRENT:
		return cudaerr[9].err_msg;
		case CUDA_ERROR_MAP_FAILED:
		return cudaerr[10].err_msg;
		case CUDA_ERROR_UNMAP_FAILED:
		return cudaerr[11].err_msg;
		case CUDA_ERROR_ARRAY_IS_MAPPED:
		return cudaerr[12].err_msg;
		case CUDA_ERROR_ALREADY_MAPPED:
		return cudaerr[13].err_msg;
		case CUDA_ERROR_NO_BINARY_FOR_GPU:
		return cudaerr[14].err_msg;
		case CUDA_ERROR_ALREADY_ACQUIRED:
		return cudaerr[15].err_msg;
		case CUDA_ERROR_NOT_MAPPED:
		return cudaerr[16].err_msg;
		case CUDA_ERROR_NOT_MAPPED_AS_ARRAY:
		return cudaerr[17].err_msg;
		case CUDA_ERROR_NOT_MAPPED_AS_POINTER:
		return cudaerr[18].err_msg;
		case CUDA_ERROR_ECC_UNCORRECTABLE:
		return cudaerr[19].err_msg;
		case CUDA_ERROR_UNSUPPORTED_LIMIT:
		return cudaerr[20].err_msg;
		case CUDA_ERROR_INVALID_SOURCE:
		return cudaerr[21].err_msg;
		case CUDA_ERROR_FILE_NOT_FOUND:
		return cudaerr[22].err_msg;
		case CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND:
		return cudaerr[23].err_msg;
		case CUDA_ERROR_SHARED_OBJECT_INIT_FAILED:
		return cudaerr[24].err_msg;
#if CUDART_VERION >= 3020
		case CUDA_ERROR_OPERATING_SYSTEM:
		return cudaerr[25].err_msg;
#endif
		case CUDA_ERROR_INVALID_HANDLE:
		return cudaerr[26].err_msg;
		case CUDA_ERROR_NOT_FOUND:
		return cudaerr[27].err_msg;
		case CUDA_ERROR_NOT_READY:
		return cudaerr[28].err_msg;
		case CUDA_ERROR_LAUNCH_FAILED:
		return cudaerr[29].err_msg;
		case CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES:
		return cudaerr[30].err_msg;
		case CUDA_ERROR_LAUNCH_TIMEOUT:
		return cudaerr[31].err_msg;
		case CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING:
		return cudaerr[32].err_msg;
		case CUDA_ERROR_UNKNOWN:
		return cudaerr[33].err_msg;
		default:
		return cudaerr[33].err_msg;
	}
}

bool ThreadWorker::IsFlushed()
{
	return !(m_work_to_do);
}

/*! Call sync() to synchronize the master thread with the worker thread.
	After a call to sync() returns, it is guarunteed that all previous
	queued calls (via callAsync()) have been called in the worker thread. 
	
	\note Since many CUDA calls are asynchronous, a call to sync() does not
	necessarily mean that all calls have completed on the GPU. To ensure this,
	one must call() cudaThreadSynchronize():
	\code
gpu.call(bind(cudaThreadSynchronize));
	\endcode

	sync() will throw an exception if any of the queued calls resulted in
	a return value not equal to cudaSuccess.
*/

CUresult ThreadWorker::sync()
{

	// wait on the work done signal
	mutex::scoped_lock lock(m_mutex);
	while (m_work_to_do)
		m_cond_work_done.wait(lock);
		
	// if there was an error
	if (m_last_error != CUDA_SUCCESS)
	{
		std::stringstream Num;
		Num << m_last_error;
     
		// build the exception
		
		runtime_error error("CUDA Error: " + string(error_msg(m_last_error)) + string(" ") + string(Num.str()));

		// throw
		throw(error);

		// reset the error value so that it doesn't propagate to continued calls
		m_last_error = CUDA_SUCCESS;

	}
	
	return m_last_error;
}


/*! \internal
	The worker thread spawns a loop that continusously checks the condition variable
	m_cond_work_to_do. As soon as it is signaled that there is work to do with
	m_work_to_do, it processes all queued calls. After all calls are made,
	m_work_to_do is set to false and m_cond_work_done is notified for anyone 
	interested (namely, sync()). During the work, m_exit is also checked. If m_exit
	is true, then the worker thread exits.
*/
void ThreadWorker::performWorkLoop()
{
	bool working = true;
	
	// temporary queue to ping-pong with the m_work_queue
	// this is done so that jobs can be added to m_work_queue while
	// the worker thread is emptying pong_queue
	deque< struct function_callback > pong_queue;
	
	while (working)
	{
		// aquire the lock and wait until there is work to do
		{
			mutex::scoped_lock lock(m_mutex);
			while (!m_work_to_do)
				m_cond_work_to_do.wait(lock);
			
			// check for the exit condition
			if (m_exit)
				working = false;
				
			// ping-pong the queues
			pong_queue.swap(m_work_queue);
		}
			
		// track any error that occurs in this queue
		CUresult error = CUDA_SUCCESS;
			
		// execute any functions in the queue
		while (!pong_queue.empty())
		{
			if (pong_queue.front().event_ptr != NULL)
				pong_queue.front().event_ptr->status = COMMAND_EXECUTING;
			  

			CUresult tmp_error;
			if (pong_queue.front().IsWaitEvent == true)
			{
				struct event_sign * event_wait_ptr = pong_queue.front().event_wait_ptr;
				event_wait_ptr->cond->lock();
				if (event_wait_ptr->status != COMMAND_COMPLETED && event_wait_ptr->status != COMMAND_FAILED )
				{
					tmp_error = pong_queue.front().func();
				}
				event_wait_ptr->cond->unlock();
			}
			else
			{
				tmp_error = pong_queue.front().func();
			}
				
			// Signal event fail or success
		
			if (pong_queue.front().event_ptr != NULL)
			{
				if (tmp_error != CUDA_SUCCESS)
				{
					pong_queue.front().event_ptr->cond->lock();
					pong_queue.front().event_ptr->status = COMMAND_FAILED;
					pong_queue.front().event_ptr->cond->signal();
					pong_queue.front().event_ptr->cond->unlock();
				}
				else
				{
					pong_queue.front().event_ptr->cond->lock();
					pong_queue.front().event_ptr->status = COMMAND_COMPLETED;
					pong_queue.front().event_ptr->cond->signal();
					pong_queue.front().event_ptr->cond->unlock();
				}
			}
		
			// update error only if it is cudaSuccess
			// this is done so that any error that occurs will propagate through
			// to the next sync()
			if (error == CUDA_SUCCESS)
				error = tmp_error;

			pong_queue.pop_front();
		}
		
		// reaquire the lock so we can update m_last_error and 
		// notify that we are done
		{
			mutex::scoped_lock lock(m_mutex);
			
			// update m_last_error only if it is cudaSuccess
			// this is done so that any error that occurs will propagate through
			// to the next sync()
			if (m_last_error == CUDA_SUCCESS)
				m_last_error = error;
			
			// notify that we have emptied the queue, but only if the queue is actually empty
			// (call_async() may have added something to the queue while we were executing above)
			if (m_work_queue.empty())
			{
				m_work_to_do = false;
				m_cond_work_done.notify_all();
			}
		}
	}
}

//#endif

