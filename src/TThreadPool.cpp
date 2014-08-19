//
//  Project : ThreadPool
//  File    : TThreadPool.cc
//  Author  : Ronald Kriemann
//  Purpose : class for managing a pool of threads
//
// arch-tag: d4739323-4e22-42d9-9fcb-f98f5890d77b
//

#include <pthread.h>

#include "TThreadPool.h"

//
// set to one to enable sequential execution
//
#define THR_SEQUENTIAL  0

//
// global thread-pool
//

TThreadPool * thread_pool = NULL;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// thread handled by threadpool
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// thread handled by threadpool
//

class TPoolThr : public TThread
{
protected:
    // pool we are in
    TThreadPool       * _pool;
    
    // job to run and data for it
    TThreadPool::TJob * _job;
    void              * _data_ptr;

    // should the job be deleted upon completion
    bool                _del_job;
    
    // condition for job-waiting
    TCondition          _work_cond;
    
    // indicates end-of-thread
    bool                _end;

    // mutex for preventing premature deletion
    TMutex              _del_mutex;
    
public:
    //
    // constructor
    //
    TPoolThr ( const int n, TThreadPool * p )
            : TThread(n), _pool(p), _job(NULL), _data_ptr(NULL), _del_job(false), _end(false)
    {}
    
    ~TPoolThr () {}
    
    //
    // parallel running method
    //
    void run ()
    {
        _del_mutex.lock();

        while ( ! _end )
        {
            //
            // append thread to idle-list and wait for work
            //

            _pool->append_idle( this );

            _work_cond.lock();
            
            while (( _job == NULL ) && ! _end )
                _work_cond.wait();

            _work_cond.unlock();
        
            //
            // look if we really have a job to do
            // and handle it
            //

            if ( _job != NULL )
            {
                // execute job
                _job->run( _data_ptr );
                _job->unlock();
            
                if ( _del_job )
                    delete _job;

                // reset data
                _work_cond.lock();
                _job      = NULL;
                _data_ptr = NULL;
                _work_cond.unlock();
            }// if
        }// while

        _del_mutex.unlock();
    }

    //
    // set and run job with optional data
    //
    void run_job  ( TThreadPool::TJob * j, void * p, const bool del = false )
    {
        _work_cond.lock();
        
        _job      = j;
        _data_ptr = p;
        _del_job  = del;
        
        _work_cond.signal();
        _work_cond.unlock();
    }

    //
    // give access to delete mutex
    //
    
    TMutex & del_mutex  ()
    {
        return _del_mutex;
    }

    //
    // quit thread (reset data and wake up)
    //
    
    void quit ()
    {
        _work_cond.lock();
        
        _end      = true;
        _job      = NULL;
        _data_ptr = NULL;
        
        _work_cond.signal();
        _work_cond.unlock();
    }
};
    
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// ThreadPool - implementation
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//
// constructor and destructor
//

TThreadPool::TThreadPool ( const uint max_p )
{
    //
    // create max_p threads for pool
    //

    _max_parallel = max_p;

    _threads = new TPoolThr*[ _max_parallel ];

    if ( _threads == NULL )
    {
        _max_parallel = 0;
        std::cerr << "(TThreadPool) TThreadPool : could not allocate thread array" << std::endl;
    }// if
    
    for ( uint i = 0; i < _max_parallel; i++ )
    {
        _threads[i] = new TPoolThr( i, this );

        if ( _threads == NULL )
            std::cerr << "(TThreadPool) TThreadPool : could not allocate thread" << std::endl;
        else
            _threads[i]->create( true, true );
    }// for

    // tell the scheduling system, how many threads to expect
    // (commented out since not needed on most systems)
//     if ( pthread_setconcurrency( _max_parallel + pthread_getconcurrency() ) != 0 )
//         std::cerr << "(TThreadPool) TThreadPool : pthread_setconcurrency ("
//                   << strerror( status ) << ")" << std::endl;
}

TThreadPool::~TThreadPool ()
{
    // wait till all threads have finished
    sync_all();
    
    // finish all thread
    for ( uint i = 0; i < _max_parallel; i++ )
        _threads[i]->quit();
    
    // cancel and delete all threads (not really safe !)
    for ( uint i = 0; i < _max_parallel; i++ )
    {
        _threads[i]->del_mutex().lock();
        delete _threads[i];
    }// for

    delete[] _threads;
}

///////////////////////////////////////////////
//
// run, stop and synch with job
//

void
TThreadPool::run ( TThreadPool::TJob * job, void * ptr, const bool del )
{
    if ( job == NULL )
        return;

#if THR_SEQUENTIAL == 1
    //
    // run in calling thread
    //
    
    job->run( ptr );

    if ( del )
        delete job;
    
#else
    //
    // run in parallel thread
    //

    TPoolThr * thr = get_idle();
    
    // lock job for synchronisation
    job->lock();

    // attach job to thread
    thr->run_job( job, ptr, del );
#endif
}

//
// wait until <job> was executed
//
void
TThreadPool::sync ( TJob * job )
{
    if ( job == NULL )
        return;
    
    job->lock();
    job->unlock();
}

//
// wait until all jobs have been executed
//
void
TThreadPool::sync_all ()
{
    while ( true )
    {
        _idle_cond.lock();
        
        // wait until next thread becomes idle
        if ( _idle_threads.size() < _max_parallel )
            _idle_cond.wait();
        else
        {
            _idle_cond.unlock();
            break;
        }// else

        _idle_cond.unlock();
    }// while
}

///////////////////////////////////////////////
//
// manage pool threads
//

//
// return idle thread form pool
//
TPoolThr *
TThreadPool::get_idle ()
{
    while ( true )
    {
        //
        // wait for an idle thread
        //

        _idle_cond.lock();
        
        while ( _idle_threads.size() == 0 )
            _idle_cond.wait();

        //
        // get first idle thread
        //
        
        if ( _idle_threads.size() > 0 )
        {
            TPoolThr * t = _idle_threads.behead();

            _idle_cond.unlock();
            
            return t;
        }// if

        _idle_cond.unlock();
    }// while
}

//
// append recently finished thread to idle list
//
void
TThreadPool::append_idle ( TPoolThr * t )
{
    //
    // CONSISTENCY CHECK: if given thread is already in list
    //
    
    _idle_cond.lock();

    for ( sll< TPoolThr * >::iterator iter = _idle_threads.first(); ! iter.eol(); ++iter )
    {
        if ( iter() == t )
        {
            _idle_cond.unlock();
            return;
        }// if
    }// while
    
    _idle_threads.push_back( t );

    // wake a blocked thread for job execution
    _idle_cond.signal();

    _idle_cond.unlock();
}

///////////////////////////////////////////////////
//
// to access global thread-pool
//
///////////////////////////////////////////////////

//
// init global thread_pool
//
void
tp_init ( const uint p )
{
    if ( thread_pool != NULL )
        delete thread_pool;
    
    if ((thread_pool = new TThreadPool( p )) == NULL)
        std::cerr << "(init_thread_pool) could not allocate thread pool" << std::endl;
}

//
// run job
//
void
tp_run ( TThreadPool::TJob * job, void * ptr, const bool del )
{
    if ( job == NULL )
        return;
    
    thread_pool->run( job, ptr, del );
}

//
// synchronise with specific job
//
void
tp_sync ( TThreadPool::TJob * job )
{
    thread_pool->sync( job );
}

//
// synchronise with all jobs
//
void
tp_sync_all ()
{
    thread_pool->sync_all();
}

//
// finish thread pool
//
void
tp_done ()
{
    delete thread_pool;
}

