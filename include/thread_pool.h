#pragma once

#include <stddef.h>
#include <pthread.h>
#include <vector>
#include <deque>
#include <iostream>

using namespace std;

class Work{
public:
    void (*f) (void *) = NULL;
    void *arg = NULL;
};

class ThreadPool{
public:
    vector<pthread_t> threads;
    deque<Work> queue;
    pthread_mutex_t mu; // mutual exclusion ; lock
    pthread_cond_t notempty;
};

void thread_pool_init(ThreadPool *tp, size_t num_threads);
void thread_pool_queue(ThreadPool *tp, void (*f)(void *), void *arg);
