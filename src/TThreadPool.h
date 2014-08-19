#ifndef __TTHREADPOOL_HH
#define __TTHREADPOOL_HH
//
//  Project : ThreadPool
//  File    : TThreadPool.hh
//  Author  : Ronald Kriemann
//  Purpose : class for managing a pool of threads
//
// arch-tag: 7b363e2f-5623-471f-b0dd-55d7ac55b93e
//

typedef unsigned int uint;

#include "sll.h"
#include "TThread.h"

// no specific processor
#define NO_PROC ((uint) -1)

// forward decl. for internal class
class TPoolThr;

//
// takes jobs and executes them in threads
//
class TThreadPool
{
    friend class TPoolThr;
    
public:
    ///////////////////////////////////////////
    //
    // class for a job in the pool
    //

    class TJob
    {
    protected:
        // number of processor this job was assigned to
        const uint  _job_no;

        // mutex for synchronisation
        TMutex     _sync_mutex;
        
    public:
        // constructor
        TJob ( const uint n = NO_PROC ) : _job_no(n) {}

        virtual ~TJob ()
        {
            if ( _sync_mutex.is_locked() )
                std::cerr << "(TJob) destructor : job is still running!" << std::endl;
        }
        
        // running method
        virtual void run ( void * ptr ) = 0;
        
        // access processor-number
        uint job_no () const { return _job_no; }

        // and mutex
        void lock   () { _sync_mutex.lock(); }
        void unlock () { _sync_mutex.unlock(); }

        // compare if given proc-no is local one
        bool on_proc ( const uint p ) const
        { return ((p == NO_PROC) || (_job_no == NO_PROC) || (p == _job_no)); }
    };
    
protected:
    // maximum degree of parallelism
    uint               _max_parallel;

    // array of threads, handled by pool
    TPoolThr **        _threads;
    
    // list of idle threads
    sll< TPoolThr * >  _idle_threads;

    // condition for synchronisation of idle list
    TCondition         _idle_cond;
    
public:
    ///////////////////////////////////////////////
    //
    // constructor and destructor
    //

    TThreadPool ( const uint max_p );

    ~TThreadPool ();

    ///////////////////////////////////////////////
    //
    // access local variables
    //

    uint max_parallel () const { return _max_parallel; }
    
    ///////////////////////////////////////////////
    //
    // run, stop and synch with job
    //

    void  run  ( TJob * job, void * ptr = NULL, const bool del = false );
    void  sync ( TJob * job );
    
    void  sync_all ();

protected:
    ///////////////////////////////////////////////
    //
    // manage pool threads
    //

    // return idle thread form pool
    TPoolThr * get_idle ();

    // insert idle thread into pool
    void append_idle ( TPoolThr * t );
};

///////////////////////////////////////////////////
//
// to access the global thread-pool
//
///////////////////////////////////////////////////

// init global thread_pool
void tp_init ( const uint p );

// run job
void tp_run ( TThreadPool::TJob * job, void * ptr = NULL, const bool del = false );

// synchronise with specific job
void tp_sync ( TThreadPool::TJob * job );

// synchronise with all jobs
void tp_sync_all ();

// finish thread pool
void tp_done ();

#endif  // __TTHREADPOOL_HH
