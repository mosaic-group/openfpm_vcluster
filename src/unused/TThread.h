#ifndef __TTHREAD_HH
#define __TTHREAD_HH
//
//  Project   : ThreadPool
//  File      : TThread.hh
//  Author    : Ronald Kriemann
//  Purpose   : baseclass for a thread-able class
//
// arch-tag: d09c570a-520a-48ce-b612-a813b50e87b4
//

#include <stdio.h>
#include <pthread.h>
#ifdef HAVE_CUDA
#include <cuda.h>
#endif

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//
// baseclass for all threaded classes
// - defines basic interface
//
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

class TThread
{
protected:
    // thread-specific things
    pthread_t       _thread_id;
    pthread_attr_t  _thread_attr;

    // is the thread running or not
    bool            _running;

    // no of thread
    int             _thread_no;
    
public:
    ////////////////////////////////////////////
    //
    // constructor and destructor
    //
    
    TThread ( const int thread_no = -1 );

    virtual ~TThread ();

    ////////////////////////////////////////////
    //
    // access local data
    //

    int thread_no () const { return _thread_no; }
    int proc_no   () const { return _thread_no; }

    void set_thread_no ( const int n );

    // compare if given proc-no is local one
    bool on_proc ( const int p ) const { return ((p == -1) || (_thread_no == -1) || (p == _thread_no)); }

    // resets running-status (used in _run_proc, see TThread.cc)
    void reset_running () { _running = false; }
    
    ////////////////////////////////////////////
    //
    // user-interface
    //
    
    // actually method the thread executes
    virtual void run () = 0;

    ////////////////////////////////////////////
    //
    // thread management
    //

    // create thread (actually start it)
    void create ( const bool detached = false, const bool sscope = false );

    // detach thread
    void detach ();
    
    // synchronise with thread (wait until finished)
    void join   ();

    // request cancellation of thread
    void cancel ();

protected:
    ////////////////////////////////////////////
    //
    // functions to be called by a thread itself
    //
    
    // terminate thread
    void exit   ();

    // put thread to sleep for <sec> seconds
    void sleep  ( const double sec );
};



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//
// wrapper for pthread_mutex
//
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

class TMutex
{
protected:
    // the mutex itself and the mutex-attr
    pthread_mutex_t      _mutex;
    pthread_mutexattr_t  _mutex_attr;

public:
    /////////////////////////////////////////////////
    //
    // constructor and destructor
    //

    TMutex ()
    {
        pthread_mutexattr_init( & _mutex_attr );
        pthread_mutex_init( & _mutex, & _mutex_attr );
    }

    ~TMutex ()
    {
        pthread_mutex_destroy( & _mutex );
        pthread_mutexattr_destroy( & _mutex_attr );
    }

    /////////////////////////////////////////////////
    //
    // usual behavior of a mutex
    //

    // lock and unlock mutex (return 0 on success)
    int lock    () { return pthread_mutex_lock(   & _mutex ); }
    int unlock  () { return pthread_mutex_unlock( & _mutex ); }

    // return true if mutex is locked, otherwise false
    bool is_locked ()
    {
        if ( pthread_mutex_trylock( & _mutex ) != 0 )
            return true;
        else
        {
            unlock();
            return false;
        }// else
    }
};



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//
// class for a condition variable
// - derived from mutex to allow locking of condition
//   to inspect or modify the predicate
//
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

class GPUWorker;

class TCondition : public TMutex
{
protected:
    // our condition variable
    pthread_cond_t  _cond;

public:
    /////////////////////////////////////////////////
    //
    // constructor and destructor
    //

    #ifdef HAVE_CUDA
    friend class GPUWorker;
    #endif
    
    TCondition  () { pthread_cond_init(    & _cond, NULL ); }
    ~TCondition () { pthread_cond_destroy( & _cond ); }

    /////////////////////////////////////////////////
    //
    // condition variable related methods
    //

    void wait      () { pthread_cond_wait( & _cond, & _mutex ); }
    void signal    () { pthread_cond_signal( & _cond ); }
    void broadcast () { pthread_cond_broadcast( & _cond ); }
};

#endif  // __TTHREAD_HH
