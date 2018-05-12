//
// Created by Alcan on 2018/5/12.
//

/***************************************************************************
 *
 * Author: "Sjors H.W. Scheres"
 * MRC Laboratory of Molecular Biology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This complete copyright notice must be included in any revised version of the
 * source code. Additional authorship citations may be added, but existing
 * author citations must be preserved.
 ***************************************************************************/
/***************************************************************************
 *
 * Authors:     J.M. De la Rosa Trevin (jmdelarosa@cnb.csic.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

#ifndef PARALLEL_PARALLEL_H
#define PARALLEL_PARALLEL_H

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include "error.h"

class ThreadManager;

class ThreadArgument;

/* Prototype of functions for threads works. */
typedef void (*ThreadFunction)(ThreadArgument &arg);

/** Class wrapping around the pthreads mutex.
 * This class will provide a more object oriented implementation
 * of a mutex, to ensure the unique access to critical regions of
 * code and other syncronization problems.
 */
class Mutex {
private:
    pthread_mutex_t mutex; //our pthread mutex

public:
    /** Default constructor.
     * This constructor just initialize the pthread_mutex_t structure
     * with its defaults values, just like static initialization with PTHREAD_MUTEX_INITIALIZER
     */
    Mutex();

    /** Destructor. */
    virtual ~Mutex();

    /** Function to get the access to the mutex.
     * If some thread has the mutex and an other one
     * asks to lock it, it will be waiting until the first one
     * releases the mutex
     */
    virtual void lock();

    /** Function to release the mutex.
     * This allows access to the mutex to other
     * threads that are waiting for it.
     */
    virtual void unlock();
};//end of class Mutex

/** Class to synchronize several threads in some point of execution.
 * Threads is a way of distributing workload across
 * different workers to solve a problem faster. Nevertheless, sometimes
 * we need synchronization between threads to avoid undesired race
 * conditions and other problems. Here we are an implementation of a barrier
 * that allows putting all threads to wait at a given point until all of them
 * have reached such point and can continue working. Barriers are usually
 * available through pthreads system library. Nonetheless, sometimes it is not
 * so we have to implement it here.
 * @code
 * Mutex mutex;
 *
 * //Then in each thread to access the critical section:
 *  mutex.lock();
 *  //...Do critical section stuff
 *  mutex.unlock();
 *
   @endcode
 */
class Barrier {

private:
    int needed; ///< How many threads should arrive to meet point
    int called; ///< How many threads already arrived
    pthread_mutex_t mutex; ///< Mutex to update structure
    pthread_cond_t cond; ///< Condition on which the threads are waiting

public:
    /** Constructor of the barrier to initialize the object.
     * You should pass the number of working threads that
     * you want to wait on the barrier. The internal counter
     * of the barrier will be initialized with numberOfThreads + 1
     * taking into account the main thread, so it need to wait
     * also in the barrier with the worker threads to all
     * can move on.
     * @code
     *  //For syncronize 10 threads created by a main thread
     *  //you can create the barrier from the main thread
     *  Barrier * barrier = new Barrier(10);
     *  //...
     *  //In the syncronization point
     *  barrier->wait();
     * @endcode
     * */
    Barrier(int numberOfThreads);

    /** Destructor to free all memory used */
    ~Barrier();

    /** Request to wait in this meet point.
     * For each thread calling this function the execution will
     * be paused untill all threads arrive this point.
     */
    void wait();

};//end of class Barrier

/** Class to pass arguments to threads functions.
 * The argument passed can be obtained casting
 * the void * data received in the function.
 * @see ThreadManager
 */
class ThreadArgument {
private:
    ThreadManager *manager;
public:
    int thread_id; ///< The thread id
    void *data;

    ThreadArgument();

    ThreadArgument(int id, ThreadManager *manager = NULL, void *data = NULL);

    friend class ThreadManager;

    friend void *_threadMain(void *data);
};

void *_threadMain(void *data);

/** Class for manage a group of threads performing one or several tasks.
 * This class is very useful when we have some function that can be executed
 * in parrallel by threads. The threads are created in the contructor of the object
 * and released in destructor. This way threads can execute different
 * functions at diffent moments and exit at the end of manager life. Also, the
 * wait() function allow in the main thread to wait until all threads have
 * finish working on a task and maybe then execute another one.
 * This class is supposed to be used only in the main thread.
 */
class ThreadManager {
private:
    int threads; ///< number of working threads.
    pthread_t *ids; ///< pthreads identifiers
    ThreadArgument *arguments; ///< Arguments passed to threads
    Barrier *barrier; ///< barrier for syncronized between tasks.
    /// Pointer to the function to work on,
    /// if null threads should exit
    ThreadFunction workFunction;
    bool started;

    void startThreads();

public:
    /** Constructor, number of working threads should be supplied */
    ThreadManager(int numberOfThreads);

    /** Destructor, free memory and exit threads */
    ~ThreadManager();

    /** Function to start working in a task.
     * The function that want to be executed in parallel
     * by the working threads should be passed as argument.
     * Functions that can be executed by thread should by of the
     * type ThreadFunction, i.e., return void * and only
     * one argument of type ThreadArgument.
     * The call of this function will block the main thread
     * until all workers finish their job, if you dont want to block
     * use runAsync instead, and later can call wait for waiting
     * until threads are done.
     * @code
     *
     *  //Global variables, so it are visible in 'processSeveralImages()'
     *  ParallelTaskDistributor * td;
     *  //function to perform some operation
     *  //to N images executed in parellel
     *  void * processImages(ThreadArgument & data)
     *  {
     *      int thread_id = arg.thread_id;
     *
     *      size_t firstImage, lastImage;
     *      while (td->getTasks(firstImage, lastImage))
     *          for (int image = firstImage; image <= lastImage; ++image)
     *          {
     *              //...
     *              processOneImage(image);
     *              //...
     *          }
     *  }
     *
     *  int main()
     *  {
     *  //...
     *  //Distribute 1000 tasks in blocks of 100.
     *  td = new ThreadTaskDistributor(1000, 100);
     *  //Start 2 threads to work on it
     *  ThreadManager * tm = new ThreadManager(2);
     *  tm.run(processImages);
     *  //...
     *  //Same threads can work in other function
     *  tm.run(processVolumes);
     *  }
     * @endcode
     */
    void run(ThreadFunction function);

    /** Same as run but without blocking. */
    void runAsync(ThreadFunction function);

    /** Function that should be called to wait until all threads finished work */
    void wait();

    /** function to start running the threads.
     * Should be external and declared as friend */
    friend void *_threadMain(void *data);


};//end of class ThreadManager

class TaskDistributor {
public:
    // domain is [first, last], not [first, last)
    static int getDistributedSize(const int &i, const int &N, const int &first, const int &last) {
        int n = last - first + 1;
        return (n / N) + ((n % N > i) ? 1 : 0);
    }

    // domain is [first, last], not [first, last)
    static int getDistributedOffset(const int &i, const int &N, const int &first, const int &last) {
        int n = last - first + 1;
        int offset = (n / N) * i + ((n % N > i) ? i : n % N);
        return offset + first;
    }
};

/// @name Miscellaneous functions
//@{

// Class which use local scope locks on higher-scope mutexes,
// resulting in zero risk of leaving locks on. Effectively,
// a mutex is treated like an container capable of holding a
// single Lock. We create a Lock and put it inside the mutex,
// and as soon as the Lock goes out out scope, the mutex is
// emptied (unlocked).
class Lock {
public:
    explicit Lock(pthread_mutex_t *pm)
            : mutexPtr(pm) { pthread_mutex_lock(mutexPtr); }

    ~Lock() { pthread_mutex_unlock(mutexPtr); }

private:
    pthread_mutex_t *mutexPtr;
};

#endif //PARALLEL_PARALLEL_H
