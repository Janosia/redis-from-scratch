#include "thread_pool.h"
#include <cassert>
#include <iostream>

using namespace std;


/*@brief Producer function*/
void thread_pool_queue(ThreadPool *tp, void (*f)(void *), void *arg){
    pthread_mutex_lock(&tp->mu); // mutex is locked restricting access to queue
    tp->queue.push_back(Work {f, arg}); // adding work to queue
    pthread_cond_signal(&tp->notempty); // signal wake up to any sleeping threads(consumer)
    // unlock mutex allowing access to queue
    pthread_mutex_unlock(&tp->mu);
}