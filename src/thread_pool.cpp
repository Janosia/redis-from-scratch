// Project Import
#include "thread_pool.h"
// C++ Import
#include <cassert>
#include <iostream>

using namespace std;


/*@brief Producer Function*/
void thread_pool_queue(ThreadPool *tp, void (*f)(void *), void *arg){
    pthread_mutex_lock(&tp->mu); // mutex is locked restricting access to queue
    tp->queue.push_back(Work {f, arg}); // adding work to queue
    pthread_cond_signal(&tp->notempty); // signal wake up to any sleeping threads(consumer)
    // unlock mutex allowing access to queue
    pthread_mutex_unlock(&tp->mu);
}

/*@brief Actual Consumer
@param arg anything and everything BUT void*/
void *worker(void *arg){
    ThreadPool *tp = (ThreadPool *)arg;
    // wait for queue to become non-empty
    while(true){
        pthread_mutex_lock(&tp->mu);
        // wait for condition 
        while(tp->queue.empty()){
            pthread_cond_wait(&tp->notempty, &tp->mu);
        }
        // get the job 
        Work w = tp->queue.front();
        tp->queue.pop_front();
        pthread_mutex_unlock(&tp->mu); // Unlock mutex
        // do the job
        w.f(w.arg);
    }
    return NULL;
}

/*@brief Consumer Function*/
void thread_pool_init(ThreadPool *tp, size_t num_threads){
    pthread_mutex_init(&tp->mu, NULL);
    pthread_cond_init(&tp->notempty, NULL);
    tp->threads.resize(num_threads);

    for(size_t i =0; i<num_threads; ++i){
        int rv = pthread_create(&tp->threads[i], NULL, &worker,tp);
        assert (rv ==0);
    }
}