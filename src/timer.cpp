// C++ imports
#include <stddef.h>
#include <stdint.h>
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
    if (dlist_empty(&g_data.idle_list)) return -1;
    uint64_t now_ms = get_monotonic_msec();
    Conn *conn = NULL;
    // idle timers are handled using Doubly Linked List
    if(g_data.idle_list.next != NULL){ 
        conn = container_of(g_data.idle_list.next, Conn, idle_node);
        uint64_t next_ms = conn->last_active_ms +k_idle_timeout_ms;
        if(next_ms <=now_ms) return 0;
        return (int32_t)(next_ms-now_ms);
    }
    return -1;
}
