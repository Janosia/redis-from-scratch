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

