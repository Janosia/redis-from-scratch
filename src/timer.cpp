// C++ imports
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <time.h>
// Project Imports

#include "timer.h"
#include "dl_list.h"
#include "server_helper.h"

using namespace std;

uint64_t get_monotonic_msec(){
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000;
}

uint32_t next_timer_ms(){
    uint64_t now_ms = get_monotonic_msec(), next_ms = (size_t)-1;
    Conn *conn = NULL;
    // idle timers are handled using Doubly Linked List
    if(g_data.idle_list.next != NULL){ 
        conn = container_of(g_data.idle_list.next, Conn, idle_node);
        uint64_t next_ms = conn->last_active_ms +k_idle_timeout_ms;
        if(next_ms <=now_ms) return 0;
        return (int32_t)(next_ms-now_ms);
    }
    // handling ttl using heap
    if(!g_data.heap.empty() && g_data.heap[0].val < next_ms){
        next_ms = g_data.heap[0].val;
    }
    if(next_ms == (uint64_t)-1){
        // expired 
        return -1;
    }else if (next_ms <= now_ms){
        // missed
        return 0;
    }

    return  (uint32_t)(now_ms - next_ms);
}
