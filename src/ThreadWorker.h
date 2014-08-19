/**
 *
 * This is a modified version of GPUWorker from
 *
 *
 * Highly Optimized Object-Oriented Molecular Dynamics (HOOMD) Open
 * Source Software License
 * Copyright (c) 2008 Ames Laboratory Iowa State University
 * All rights reserved.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND
CONTRIBUTORS ``AS IS''  AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

// $Id$
// $URL$

/*! \file GPUWorker.h
	\brief Defines the GPUWorker class
*/

// only compile if USE_CUDA is enabled
//#ifdef USE_CUDA

#ifndef __GPUWORKER_H__
#define __GPUWORKER_H__

#define HAVE_CUDA

#include <deque>
#include <stdexcept>

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/scoped_ptr.hpp>
#include <cuda.h>


//! Implements a worker thread controlling a single GPU
/*! CUDA requires one thread per GPU in multiple GPU code. It is not always
	convenient to write multiple-threaded code where all threads are peers.
	Sometimes, a master/slave approach can be the simplest and quickest to write.
	
	GPUWorker provides the underlying worker threads that a master/slave
	approach needs to execute on multiple GPUs. It is designed so that 
	a \b single thread can own multiple GPUWorkers, each of whom execute on 
	their own GPU. The master thread can call any CUDA function on that GPU
	by passing a bound boost::function into call() or callAsync(). Internally, these
	calls are executed inside the worker thread so that they all share the same
	CUDA context.
	
	On construction, a GPUWorker is automatically associated with a device. You
	pass in an integer device number which is used to call cudaSetDevice() 
	in the worker thread. 
	
	After the GPUWorker is constructed, you can make calls on the GPU
	by submitting them with call(). To queue calls, use callAsync(), but
	please read carefully and understand the race condition warnings before 
	using callAsync(). sync() can be used to synchronize the master thread
	with the worker thread. If any called GPU function returns an error,
	call() (or the sync() after a callAsync()) will throw a std::runtime_error.
	
	To share a single GPUWorker with multiple objects, use boost::shared_ptr.
\code
boost::shared_ptr<GPUWorker> gpu(new GPUWorker(dev));
gpu->call(whatever...)
SomeClass cls(gpu);
// now cls can use gpu to execute in the same worker thread as everybody else
\endcode	
	
	\warning A single GPUWorker is intended to be used by a \b single master thread
	(though master threads can use multiple GPUWorkers). If a single GPUWorker is
	shared amoung multiple threads then ther \e should not be any horrible consequences.
	All tasks will still be exected in the order in which they
	are recieved, but sync() becomes ill-defined (how can one synchronize with a worker that
	may be receiving commands from another master thread?) and consequently all synchronous
	calls via call() \b may not actually be synchronous leading to weird race conditions for the
	caller. Then againm calls via call() \b might work due to the inclusion of a mutex lock:
	still, multiple threads calling a single GPUWorker is an untested configuration.
	Use at your own risk.

	\note GPUWorker works in both Linux and Windows (tested with VS2005). However,
	in Windows, you need to define BOOST_BIND_ENABLE_STDCALL in your project options
	in order to be able to call CUDA runtime API functions with boost::bind.
*/

#define COMMAND_ENQUEUE		1
#define COMMAND_EXECUTING	2
#define COMMAND_COMPLETED	3
#define COMMAND_FAILED		-4

#include "TThread.h"

struct error_table
{
	const char * err_msg;
	int err_n;
};

const struct error_table cudaerr[] = {{"CUDA_SUCCESS",0},
{"CUDA_ERROR_INVALID_VALUE",1},
{"CUDA_ERROR_OUT_OF_MEMORY",2},
{"CUDA_ERROR_NOT_INITIALIZED",3},
{"CUDA_ERROR_DEINITIALIZED",4},
{"CUDA_ERROR_NO_DEVICE",100},
{"CUDA_ERROR_INVALID_DEVICE",101},
{"CUDA_ERROR_INVALID_IMAGE",200},
{"CUDA_ERROR_INVALID_CONTEXT",201},
{"CUDA_ERROR_CONTEXT_ALREADY_CURRENT",202},
{"CUDA_ERROR_MAP_FAILED",205},
{"CUDA_ERROR_UNMAP_FAILED",206},
{"CUDA_ERROR_ARRAY_IS_MAPPED",207},
{"CUDA_ERROR_ALREADY_MAPPED",208},
{"CUDA_ERROR_NO_BINARY_FOR_GPU",209},
{"CUDA_ERROR_ALREADY_ACQUIRED",210},
{"CUDA_ERROR_NOT_MAPPED",211},
{"CUDA_ERROR_NOT_MAPPED_AS_ARRAY",212},
{"CUDA_ERROR_NOT_MAPPED_AS_POINTER",213},
{"CUDA_ERROR_ECC_UNCORRECTABLE",214},
{"CUDA_ERROR_UNSUPPORTED_LIMIT",215},
{"CUDA_ERROR_INVALID_SOURCE",300},
{"CUDA_ERROR_FILE_NOT_FOUND",301},
{"CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND",302},
{"CUDA_ERROR_SHARED_OBJECT_INIT_FAILED",303},
{"CUDA_ERROR_OPERATING_SYSTEM",304},
{"CUDA_ERROR_INVALID_HANDLE",400},
{"CUDA_ERROR_NOT_FOUND",500},
{"CUDA_ERROR_NOT_READY",600},
{"CUDA_ERROR_LAUNCH_FAILED",700},
{"CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES",701},
{"CUDA_ERROR_LAUNCH_TIMEOUT",702},
{"CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING",703},
{"CUDA_ERROR_UNKNOWN",999}};

struct event_sign
{
	unsigned int ID;
	TCondition * cond;
	int status;
};

struct function_callback
{
	struct event_sign * event_ptr;
	struct event_sign * event_wait_ptr;
	bool IsWaitEvent;
	boost::function< CUresult (void) > func;
};

#ifndef POOL_SIZE
#define POOL_SIZE 1024
#endif

class ThreadWorker
{
	public:
		//! Creates a worker thread and ties it to a particular gpu \a dev
		ThreadWorker();
		
		//! Destructor
		~ThreadWorker();
		
		//! Makes a synchronous function call executed by the worker thread
		CUresult call(const boost::function< CUresult (void) > &func);
		
		//! Queues an asynchronous function call to be executed by the worker thread
		CUresult callAsync(const boost::function< CUresult (void) > &func);
		
		//! Queues an asynchronous function call to be executed by the worker thread with event
		CUresult callAsync(const boost::function< CUresult (void) > &func, std::list<event_sign>::iterator * event);

		//! Queues an asynchronous event wait
		CUresult callAsyncWait(std::list<event_sign>::iterator & event_w);
		
		//! Blocks the calling thread until all queued calls have been executed
		CUresult sync();
	
		//! get event info
		static CUresult GetEventInfo(std::list<event_sign>::iterator & event, int & eventstat);
		
		//! ReleaseEvent();
		void ReleaseEvent(std::list<event_sign>::iterator & event);
		
		//! Error message;
		const char * error_msg(unsigned int id);
		
		//! Is empety the events list 
		bool IsEventListEmpety();
		
		//! Is queue flushed
		bool IsFlushed();
		
	private:

		unsigned int pool_pointer;
		TCondition ConditionPool[POOL_SIZE];
		unsigned int IDevent;
	  
		//! Flag to indicate the worker thread is to exit
		bool m_exit;
		
		//! Flag to indicate there is work to do
		bool m_work_to_do;
		
		//! Error from last cuda call
		CUresult m_last_error;
		
		//! The queue of function calls to make
		std::deque< struct function_callback > m_work_queue;
		
		//! Event status info
		std::list<event_sign> eventlist;
		
		//! Mutex for accessing m_exit, m_work_queue, m_work_to_do, and m_last_error
		boost::mutex m_mutex;
		
		//! Mutex for syncing after every operation
		boost::mutex m_call_mutex;
		
		//! Condition variable to signal m_work_to_do = true
		boost::condition m_cond_work_to_do;
		
		//! Condition variable to signal m_work_to_do = false (work is complete)
		boost::condition m_cond_work_done;
		
		//! Thread
		boost::scoped_ptr<boost::thread> m_thread;
		
		static CUresult wait_cond(TCondition * cond);
		
		//! Worker thread loop
		void performWorkLoop();
	};
		
		
#endif
//#endif
