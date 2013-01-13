/**
This implementation refers to some of the 
implementations of multi-threading in YAEL: https://gforge.inria.fr/projects/yael/ from INRIA.
@file MultiThd.h
@brief provides utilities used for multi-threading (pthread based).
@author wzhang
@date May 7th, 2012
*/

#ifndef MUTITHD_MUTEX_H_INCLUDED
#define MUTITHD_MUTEX_H_INCLUDED

#include <cassert>
#include <pthread.h>

/// keeps the context for running multi-threading
struct context_t
{
	/// the mutex
    pthread_mutex_t mutex;
    /// number of task, total number of task, thread id
    int i, n, tid;
    /// function that does one time computation
    void (*task_fun) (void *arg, int tid, int i, pthread_mutex_t& m);
    /// arguments used while computing
    void *task_arg;
};


/// provides utility for multi-threading programming
class MultiThd
{
public:
	/**
	@brief interface to multi-threading computation
	*/
    static void compute_tasks (int n, int nthread, void (*task_fun) (void *arg, int tid, int i, pthread_mutex_t& m), void *task_arg)
    {
        // init context
        context_t context;
        context.i = 0; // i-th task
        context.n = n; // number of task
        context.tid = 0; // boosted thread number
        context.task_fun = task_fun;
        context.task_arg = task_arg;
        pthread_mutex_init (&context.mutex, NULL);

        if(nthread==1)
            start_routine(&context);
        else
        {
            pthread_t *threads = new pthread_t[nthread];

            for (int i = 0; i < nthread; i++)
                pthread_create (&threads[i], NULL, &start_routine, &context);

            /* all computing */

            for (int i = 0; i < nthread; i++)
                pthread_join (threads[i], NULL);

            delete[] threads;
        }
    }

private:

	/**
	scheduler of the whole computation. 
	distribute tasks to different threads one by one.
	*/
    static void *start_routine (void *cp)
    {
        // get context
        context_t * context = (struct context_t*)cp;
        int tid;
        pthread_mutex_lock (&context->mutex);
        tid = context->tid++;
        pthread_mutex_unlock (&context->mutex);

        while(1) // keep computing till no more item in list
        {
            int item;
            pthread_mutex_lock (&context->mutex);
            item = context->i++;
            pthread_mutex_unlock (&context->mutex);
            if (item >= context->n) // no more item to do
                break;
            else
                printf("\rtid:%d assign task %d of %d total task.",tid, item, context->n);
                //fflush(stdout);
                context->task_fun (context->task_arg, tid, item, context->mutex);
        }

        return NULL;
    }

};

#endif // MUTITHD_MUTEX_H_INCLUDED
